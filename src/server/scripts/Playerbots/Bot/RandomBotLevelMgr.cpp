/* 随机机器人等级管理——等级分档 + 满级重置 */
/*By leewheel 2026-08-15: 从 the-lab(brighton-chi) mod-playerbots 移植到 TC 3.4.3。
TC适配：AC的LOG_*→TC_LOG_*；WorldScript/PlayerScript 构造去掉 AC 的 hook 枚举列表，
改 TC 虚函数 override；其余逻辑与上游一致。*/

#include "RandomBotLevelMgr.h"
//By leewheel 2026-09-05: 上游1fea462f——引入ArenaTeamMgr用于按战队ID校验战队成员
#include "ArenaTeamMgr.h"
//End By leewheel
#include "DatabaseEnv.h"
#include "LFGMgr.h"
#include "Log.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "PlayerbotFactory.h"
#include "Playerbots.h"
#include "QueryResult.h"
#include "Random.h"
#include "RandomPlayerbotMgr.h"
#include "ScriptMgr.h"
#include "World.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <string>
#include <vector>

// bot的名字是否在排除列表中
static bool IsNameInExcludeList(Player* bot, std::vector<std::string> const& excludeList)
{
    if (!bot)
        return false;

    return std::find(excludeList.begin(), excludeList.end(), bot->GetName()) != excludeList.end();
}

// 指定bot是否出现在任何真实玩家的好友列表中
static bool BotInFriendList(Player* bot, std::vector<uint32> const& socialFriendsList)
{
    if (!bot || !bot->IsInWorld() || !bot->GetSession() || bot->GetSession()->isLogingOut() ||
        bot->IsDuringRemoveFromWorld())
        return false;

    return std::find(socialFriendsList.begin(), socialFriendsList.end(), bot->GetGUID().GetCounter()) !=
        socialFriendsList.end();
}

//By leewheel 2026-09-05: 上游1fea462f——指定分档是否被禁用(pct==0)
static bool IsDisabledBracket(std::vector<LevelBracketConfig> const& configured, uint8 index)
{
    return index < configured.size() && configured[index].pct == 0;
}
//End By leewheel

// 指定bot是否属于任何竞技场战队
static bool BotInArenaTeam(Player* bot)
{
    if (!bot)
        return false;
    //By leewheel 2026-09-05: 上游1fea462f——只检查2v2/3v3/5v5三个实际槽位,且校验战队确实存在
    for (uint8 slot = ARENA_SLOT_2v2; slot <= ARENA_SLOT_5v5; ++slot)
    {
        if (sArenaTeamMgr->GetArenaTeamById(bot->GetArenaTeamId(slot)))
            return true;
    }
    //End By leewheel
    return false;
}

// bot当前是否处于可安全执行等级重置的状态(存活、非战斗、非战场/竞技场/随机本/飞行、
// 且仅与其它bot组队)
static bool IsBotSafeForLevelReset(Player* bot)
{
    if (!bot || !bot->GetSession() || bot->GetSession()->isLogingOut() || bot->IsDuringRemoveFromWorld())
        return false;

    if (!bot->IsInWorld())
        return false;

    if (!bot->IsAlive())
        return false;

    if (bot->IsInCombat())
        return false;

    if (bot->InBattleground() || bot->InArena() || bot->inRandomLfgDungeon() || bot->InBattlegroundQueue())
        return false;

    if (sLFGMgr->GetState(bot->GetGUID()) != lfg::LFG_STATE_NONE)
        return false;

    if (Group* group = bot->GetGroup())
    {
        if (sLFGMgr->GetState(group->GetGUID()) != lfg::LFG_STATE_NONE)
            return false;
    }

    if (bot->IsInFlight())
        return false;

    if (Group* group = bot->GetGroup())
    {
        for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
        {
            Player* member = ref->GetSource();
            if (member && member->IsInWorld() && !GET_PLAYERBOT_AI(member))
                return false;
        }
    }
    return true;
}

// =============================================================================
// LEVEL BRACKETS FEATURE
// =============================================================================

std::vector<LevelBracketConfig>& RandomBotLevelMgr::GetFactionRanges(TeamId team)
{
    return (team == TEAM_ALLIANCE) ? _allianceRanges : _hordeRanges;
}

// 把分档定义从PlayerbotAIConfig复制进工作状态并重置工作边界/百分比。
// 动态分布与钳位/再平衡会在这些工作副本上做运行期修改，因此此点之后绝不触碰
// PlayerbotAIConfig自己的向量。
void RandomBotLevelMgr::LoadConfig()
{
    _allianceRanges = sPlayerbotAIConfig.levelBracketsAlliance;
    _hordeRanges = sPlayerbotAIConfig.levelBracketsHorde;
    _numRanges = sPlayerbotAIConfig.levelBracketsNumRanges;
    _randomBotMinLevel = static_cast<uint8>(sPlayerbotAIConfig.randomBotMinLevel);
    _randomBotMaxLevel = static_cast<uint8>(sPlayerbotAIConfig.randomBotMaxLevel);

    ClampAndBalanceBrackets();
}

void RandomBotLevelMgr::LogStartupSummary() const
{
    if (!sPlayerbotAIConfig.levelBracketsEnabled)
        TC_LOG_INFO("playerbots", "[RandomBotLevelMgr] Level brackets sub-feature disabled via configuration.");
    else
    {
        TC_LOG_DEBUG("playerbots",
            "[RandomBotLevelMgr] Level brackets loaded. Check frequency: {} seconds, flagged check frequency: {} "
            "seconds.",
            sPlayerbotAIConfig.levelBracketsCheckFrequency, sPlayerbotAIConfig.levelBracketsFlaggedCheckFrequency);
        for (uint8 i = 0; i < _numRanges; ++i)
            TC_LOG_DEBUG("playerbots", "[RandomBotLevelMgr] Alliance Range {}: {}-{}, Desired Percentage: {}%", i + 1,
                _allianceRanges[i].lower, _allianceRanges[i].upper, _allianceRanges[i].pct);
        for (uint8 i = 0; i < _numRanges; ++i)
            TC_LOG_DEBUG("playerbots", "[RandomBotLevelMgr] Horde Range {}: {}-{}, Desired Percentage: {}%", i + 1,
                _hordeRanges[i].lower, _hordeRanges[i].upper, _hordeRanges[i].pct);
    }

    if (!sPlayerbotAIConfig.resetBotLevelEnabled)
        TC_LOG_INFO("playerbots", "[RandomBotLevelMgr] Level reset sub-feature disabled via configuration.");
    else
    {
        TC_LOG_INFO("playerbots",
            "[RandomBotLevelMgr] Level reset loaded. MaxLevel = {} ({}), ResetToLevel = {}, SkipFromLevel = {} ({}), "
            "SkipToLevel = {}, ResetChance = {}%, ScaledChance = {}, RestrictTimePlayed = {}, "
            "IgnoreGuildBotsWithRealPlayers = {}, ExcludedNames = {}.",
            static_cast<int>(sPlayerbotAIConfig.resetBotLevelMaxLevel),
            sPlayerbotAIConfig.resetBotLevelMaxLevel > 0 ? "Enabled" : "Disabled",
            static_cast<int>(sPlayerbotAIConfig.resetBotLevelResetTo),
            static_cast<int>(sPlayerbotAIConfig.resetBotLevelSkipFrom),
            sPlayerbotAIConfig.resetBotLevelSkipFrom > 0 ? "Enabled" : "Disabled",
            static_cast<int>(sPlayerbotAIConfig.resetBotLevelSkipTo),
            static_cast<int>(sPlayerbotAIConfig.resetBotLevelChance),
            sPlayerbotAIConfig.resetBotLevelScaledChance ? "Enabled" : "Disabled",
            sPlayerbotAIConfig.resetBotLevelRestrictTimePlayed ? "Enabled" : "Disabled",
            sPlayerbotAIConfig.resetBotLevelIgnoreGuildWithRealPlayers ? "Enabled" : "Disabled",
            sPlayerbotAIConfig.resetBotLevelExcludeNames.empty()
                ? "None"
                : std::to_string(sPlayerbotAIConfig.resetBotLevelExcludeNames.size()) + " names");
    }
}

// 把分档边界钳制到[_randomBotMinLevel, _randomBotMaxLevel]，并按阵营把期望百分比
// 再平衡回100(若尚未如此)。
void RandomBotLevelMgr::ClampAndBalanceBrackets()
{
    for (uint8 i = 0; i < _numRanges; ++i)
    {
        if (_allianceRanges[i].lower < _randomBotMinLevel)
            _allianceRanges[i].lower = _randomBotMinLevel;
        if (_allianceRanges[i].upper > _randomBotMaxLevel)
            _allianceRanges[i].upper = _randomBotMaxLevel;
        if (_allianceRanges[i].lower > _allianceRanges[i].upper)
            _allianceRanges[i].pct = 0;
    }
    for (uint8 i = 0; i < _numRanges; ++i)
    {
        if (_hordeRanges[i].lower < _randomBotMinLevel)
            _hordeRanges[i].lower = _randomBotMinLevel;
        if (_hordeRanges[i].upper > _randomBotMaxLevel)
            _hordeRanges[i].upper = _randomBotMaxLevel;
        if (_hordeRanges[i].lower > _hordeRanges[i].upper)
            _hordeRanges[i].pct = 0;
    }

    uint32 totalAlliance = 0;
    uint32 totalHorde = 0;
    for (uint8 i = 0; i < _numRanges; ++i)
    {
        totalAlliance += _allianceRanges[i].pct;
        totalHorde += _hordeRanges[i].pct;
    }

    if (totalAlliance != 100 && totalAlliance > 0)
    {
        TC_LOG_TRACE("playerbots",
            "[RandomBotLevelMgr] Alliance: Sum of percentages is {} (expected 100). Auto adjusting.", totalAlliance);
        int missing = 100 - totalAlliance;
        while (missing > 0)
        {
            for (uint8 i = 0; i < _numRanges && missing > 0; ++i)
            {
                if (_allianceRanges[i].lower <= _allianceRanges[i].upper && _allianceRanges[i].pct > 0)
                {
                    _allianceRanges[i].pct++;
                    missing--;
                }
            }
        }
    }
    if (totalHorde != 100 && totalHorde > 0)
    {
        TC_LOG_TRACE("playerbots", "[RandomBotLevelMgr] Horde: Sum of percentages is {} (expected 100). Auto adjusting.",
            totalHorde);
        int missing = 100 - totalHorde;
        while (missing > 0)
        {
            for (uint8 i = 0; i < _numRanges && missing > 0; ++i)
            {
                if (_hordeRanges[i].lower <= _hordeRanges[i].upper && _hordeRanges[i].pct > 0)
                {
                    _hordeRanges[i].pct++;
                    missing--;
                }
            }
        }
    }
}

// 把真实玩家普查权重归一化为一个阵营工作分档向量的、总和为100的desiredPercent值。
// 同步加权与按阵营加权两条分支共用。
void RandomBotLevelMgr::ApplyBracketWeights(std::vector<LevelBracketConfig>& ranges, std::vector<float> const& weights)
{
    float total = 0.0f;
    for (uint8 i = 0; i < _numRanges; ++i)
        total += weights[i];

    int pctSum = 0;
    for (uint8 i = 0; i < _numRanges; ++i)
    {
        uint8 pct = (total > 0.0f) ? static_cast<uint8>(std::round((weights[i] / total) * 100)) : 0;
        ranges[i].pct = pct;
        pctSum += pct;
    }
    // 修整舍入漂移使总和=100
    int missing = 100 - pctSum;
    for (uint8 i = 0; i < _numRanges && missing > 0; ++i)
    {
        if (ranges[i].lower <= ranges[i].upper && ranges[i].pct > 0)
        {
            ranges[i].pct++;
            missing--;
        }
    }
}

// 返回给定等级所在的分档索引(按阵营)。范围外返回-1
int RandomBotLevelMgr::GetLevelRangeIndex(uint8 level, TeamId team)
{
    if (level < _randomBotMinLevel || level > _randomBotMaxLevel)
        return -1;

    if (team != TEAM_ALLIANCE && team != TEAM_HORDE)
        return -1;

    std::vector<LevelBracketConfig> const& ranges = GetFactionRanges(team);
    for (uint8 i = 0; i < _numRanges; ++i)
    {
        if (level >= ranges[i].lower && level <= ranges[i].upper)
            return i;
    }

    return -1;
}

// 把bot等级调整到其阵营给定分档内。死亡骑士从不被分配到CONFIG_START_HEROIC_PLAYER_LEVEL以下。
// 阵营分档在调用时经GetFactionRanges()解析(而非缓存引用)，因为配置重载会resize这些向量。
void RandomBotLevelMgr::AdjustBotToRange(Player* bot, int targetRangeIndex, TeamId team)
{
    if (!bot || !bot->IsInWorld() || !bot->GetSession() || bot->GetSession()->isLogingOut() ||
        bot->IsDuringRemoveFromWorld())
        return;

    if (targetRangeIndex < 0 || targetRangeIndex >= _numRanges)
        return;

    std::vector<LevelBracketConfig> const& factionRanges = GetFactionRanges(team);
    if (static_cast<size_t>(targetRangeIndex) >= factionRanges.size())
        return;

    if (bot->IsMounted())
        bot->Dismount();

    uint8 botOriginalLevel = bot->GetLevel();
    uint8 newLevel = 0;

    uint8 dkMinLevel = static_cast<uint8>(sWorld->getIntConfig(CONFIG_START_HEROIC_PLAYER_LEVEL));

    if (bot->getClass() == CLASS_DEATH_KNIGHT)
    {
        uint8 lowerBound = factionRanges[targetRangeIndex].lower;
        uint8 upperBound = factionRanges[targetRangeIndex].upper;
        if (upperBound < dkMinLevel)
        {
            TC_LOG_TRACE("playerbots",
                "[RandomBotLevelMgr] AdjustBotToRange: Cannot assign {} Death Knight '{}' ({}) to range {}-{} "
                "(below level {}).",
                (team == TEAM_ALLIANCE) ? "Alliance" : "Horde", bot->GetName(), botOriginalLevel, lowerBound,
                upperBound, dkMinLevel);
            return;
        }
        if (lowerBound < dkMinLevel)
            lowerBound = dkMinLevel;
        if (lowerBound > upperBound)
            return;
        newLevel = urand(lowerBound, upperBound);
    }
    else
    {
        LevelBracketConfig const& range = factionRanges[targetRangeIndex];
        if (range.lower > range.upper)
        {
            TC_LOG_TRACE("playerbots", "[RandomBotLevelMgr] AdjustBotToRange: Invalid range {}-{} for {} bot '{}'.",
                range.lower, range.upper, (team == TEAM_ALLIANCE) ? "Alliance" : "Horde", bot->GetName());
            return;
        }
        newLevel = urand(range.lower, range.upper);
    }

    PlayerbotFactory newFactory(bot, newLevel);
    newFactory.Randomize(false);

    // 装备与专精持久化开启且bot滚到满级时，强制重置天赋——规避随机化与满级bot
    // 装备/专精持久化的交互问题
    if (newLevel == _randomBotMaxLevel && sPlayerbotAIConfig.equipAndSpecPersistence)
    {
        PlayerbotFactory tempFactory(bot, newLevel);
        tempFactory.InitTalentsTree(false, true, true);
    }

    PlayerbotAI* botAI = GET_PLAYERBOT_AI(bot);
    TC_LOG_TRACE("playerbots",
        "[RandomBotLevelMgr] AdjustBotToRange: {} Bot '{}' - {} ({}) adjusted to level {} (target range {}-{}).",
        (team == TEAM_ALLIANCE) ? "Alliance" : "Horde", bot->GetName(),
        botAI ? botAI->GetChatHelper()->FormatClass(bot->getClass()) : "Unknown", botOriginalLevel, newLevel,
        factionRanges[targetRangeIndex].lower, factionRanges[targetRangeIndex].upper);
}

// 把社交好友低GUID列表(character_social, flags = 1)载入_socialFriendsList
void RandomBotLevelMgr::LoadSocialFriendList()
{
    _socialFriendsList.clear();
    QueryResult result = CharacterDatabase.Query("SELECT friend FROM character_social WHERE flags = 1");

    if (!result || result->GetRowCount() == 0)
        return;

    do
    {
        _socialFriendsList.push_back(result->Fetch()->Get<uint32>());
    } while (result->NextRow());
}

// 返回玩家所在分档索引，若其当前落在其阵营所有定义范围之外则标记为待重置(到最近分档)。
//
// 只有随机bot会进入_pendingLevelResets。真实玩家在动态分布真实玩家普查中也会调用本函数；
// 范围外的真实玩家直接返回-1(其普查贡献被跳过)而不会被排队等级重置——给真实玩家排队
// 最终会重置其等级，这是原模块继承的bug，本移植已修复。
int RandomBotLevelMgr::GetOrFlagPlayerBracket(Player* player)
{
    bool isRandomBot = sRandomPlayerbotMgr.IsRandomBot(player);

    if (isRandomBot && IsNameInExcludeList(player, sPlayerbotAIConfig.levelBracketsExcludeNames))
        return -1;

    PlayerbotAI* botAI = GET_PLAYERBOT_AI(player);
    if (isRandomBot && sPlayerbotAIConfig.levelBracketsIgnoreGuildWithRealPlayers && botAI && botAI->IsInRealGuild())
        return -1;

    if (isRandomBot && sPlayerbotAIConfig.levelBracketsIgnoreArenaTeamBots && BotInArenaTeam(player))
        return -1;

    // 排除与真实玩家组队的bot
    if (isRandomBot)
    {
        if (Group* group = player->GetGroup())
        {
            for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
            {
                Player* member = ref->GetSource();
                if (member && member->IsInWorld() && !GET_PLAYERBOT_AI(member))
                    return -1;
            }
        }
    }

    TeamId team = player->GetTeamId();
    int rangeIndex = GetLevelRangeIndex(player->GetLevel(), team);
    if (rangeIndex >= 0)
        return rangeIndex;

    if (team != TEAM_ALLIANCE && team != TEAM_HORDE)
        return -1;

    // 只有随机bot可能在此排队等级重置。范围外的真实玩家(及非随机bot)直接落空返回-1
    if (!isRandomBot)
        return -1;

    std::vector<LevelBracketConfig> const& factionRanges = GetFactionRanges(team);

    int targetRange = -1;
    int smallestDiff = std::numeric_limits<int>::max();
    uint8 dkMinLevel = static_cast<uint8>(sWorld->getIntConfig(CONFIG_START_HEROIC_PLAYER_LEVEL));
    for (int i = 0; i < _numRanges; ++i)
    {
        if (factionRanges[i].lower > factionRanges[i].upper)
            continue;

        //By leewheel 2026-09-05: 上游1fea462f——跳过被禁用(pct==0)的分档
        if (factionRanges[i].pct == 0)
            continue;
        //End By leewheel

        // 跳过死亡骑士不可被分配到的分档
        if (player->getClass() == CLASS_DEATH_KNIGHT && factionRanges[i].upper < dkMinLevel)
            continue;

        int diff = 0;
        if (player->GetLevel() < factionRanges[i].lower)
            diff = factionRanges[i].lower - player->GetLevel();
        else if (player->GetLevel() > factionRanges[i].upper)
            diff = player->GetLevel() - factionRanges[i].upper;
        if (diff < smallestDiff)
        {
            smallestDiff = diff;
            targetRange = i;
        }
    }

    if (targetRange >= 0)
    {
        bool alreadyFlagged = false;
        ObjectGuid guid = player->GetGUID();
        for (auto const& entry : _pendingLevelResets)
        {
            if (entry.botGuid == guid)
            {
                alreadyFlagged = true;
                break;
            }
        }
        if (!alreadyFlagged)
            _pendingLevelResets.push_back({guid, targetRange, team});
    }

    return -1;
}

// 把bot从过饱和分档移到仍缺bot的分档。ProcessFactionDistribution 对 safeBots 与
// flaggedBots 各调用一次。
void RandomBotLevelMgr::RedistributeSurplusBots(std::vector<Player*>& sourceBots, int fromRange, TeamId team,
    std::vector<int>& actualCounts, std::vector<int> const& desiredCounts, std::vector<int> const& targetRanges)
{
    size_t targetIdx = 0;
    while (actualCounts[fromRange] > desiredCounts[fromRange] && !sourceBots.empty() && targetIdx < targetRanges.size())
    {
        Player* bot = sourceBots.back();
        sourceBots.pop_back();

        int targetRange = targetRanges[targetIdx];
        if (actualCounts[targetRange] >= desiredCounts[targetRange])
        {
            ++targetIdx;
            continue;
        }

        ObjectGuid botGuid = bot->GetGUID();
        bool alreadyFlagged = false;
        for (auto const& entry : _pendingLevelResets)
        {
            if (entry.botGuid == botGuid)
            {
                alreadyFlagged = true;
                break;
            }
        }
        if (!alreadyFlagged)
            _pendingLevelResets.push_back({botGuid, targetRange, team});

        actualCounts[fromRange]--;
        actualCounts[targetRange]++;
        if (actualCounts[targetRange] >= desiredCounts[targetRange])
            ++targetIdx;
    }
}

// 为一个阵营计算期望vs实际数量，并把盈余bot(先安全bot再已标记bot)排队重置进欠饱和分档。
// 两阵营共用，避免复制原先约100行的联盟/部落块。
void RandomBotLevelMgr::ProcessFactionDistribution(TeamId team, uint32 totalBots, std::vector<int>& actualCounts,
    std::vector<std::vector<Player*>>& botsByRange)
{
    if (totalBots == 0)
        return;

    std::vector<LevelBracketConfig> const& ranges = GetFactionRanges(team);
    char const* factionName = (team == TEAM_ALLIANCE) ? "Alliance" : "Horde";

    std::vector<int> desiredCounts(_numRanges, 0);
    for (uint8 i = 0; i < _numRanges; ++i)
    {
        desiredCounts[i] = static_cast<int>(std::round((ranges[i].pct / 100.0) * totalBots));
        TC_LOG_DEBUG("playerbots", "[RandomBotLevelMgr] {} Range {} ({}-{}): Desired = {}, Actual = {}.",
            factionName, i + 1, ranges[i].lower, ranges[i].upper, desiredCounts[i], actualCounts[i]);
    }

    for (uint8 i = 0; i < _numRanges; ++i)
    {
        std::vector<Player*> safeBots;
        std::vector<Player*> flaggedBots;
        for (Player* bot : botsByRange[i])
        {
            if (IsBotSafeForLevelReset(bot))
                safeBots.push_back(bot);
            else
                flaggedBots.push_back(bot);
        }

        std::vector<int> targetRanges;
        for (uint8 j = 0; j < _numRanges; ++j)
        {
            if (actualCounts[j] < desiredCounts[j])
                targetRanges.push_back(j);
        }

        RedistributeSurplusBots(safeBots, i, team, actualCounts, desiredCounts, targetRanges);
        RedistributeSurplusBots(flaggedBots, i, team, actualCounts, desiredCounts, targetRanges);
    }
}

// 运行周期性的bot等级分布pass：可选地按真实玩家普查重算动态分档百分比，然后把过饱和
// 分档的盈余bot排队重置进欠饱和分档(按阵营，经ProcessFactionDistribution)。
void RandomBotLevelMgr::RunLevelBracketsDistribution()
{
    auto const& allPlayers = ObjectAccessor::GetPlayers();

    LoadSocialFriendList();

    if (sPlayerbotAIConfig.levelBracketsDynamicDistribution)
    {
        // 计算真实玩家分档数量
        std::vector<int> allianceRealCounts(_numRanges, 0);
        std::vector<int> hordeRealCounts(_numRanges, 0);
        uint32 totalAllianceReal = 0;
        uint32 totalHordeReal = 0;

        for (auto const& itr : allPlayers)
        {
            Player* player = itr.second;
            if (!player || !player->IsInWorld())
                continue;
            if (GET_PLAYERBOT_AI(player))
                continue; // 只统计真实玩家
            int rangeIndex = GetOrFlagPlayerBracket(player);
            if (rangeIndex < 0)
                continue;
            if (player->GetTeamId() == TEAM_ALLIANCE)
            {
                allianceRealCounts[rangeIndex]++;
                totalAllianceReal++;
            }
            else if (player->GetTeamId() == TEAM_HORDE)
            {
                hordeRealCounts[rangeIndex]++;
                totalHordeReal++;
            }
        }

        float const baseline = 1.0f;
        std::vector<float> allianceWeights(_numRanges, 0.0f);
        std::vector<float> hordeWeights(_numRanges, 0.0f);

        // SYNCED MODE: 真实玩家权重两阵营合并计算，应用于两套分档表
        if (sPlayerbotAIConfig.levelBracketsSyncFactions)
        {
            uint32 totalCombinedReal = totalAllianceReal + totalHordeReal;
            for (uint8 i = 0; i < _numRanges; ++i)
            {
                int combinedReal = allianceRealCounts[i] + hordeRealCounts[i];
                float weight = baseline + sPlayerbotAIConfig.levelBracketsRealPlayerWeight *
                    (totalCombinedReal > 0 ? (1.0f / float(totalCombinedReal)) : 1.0f) * std::log(1 + combinedReal);

                //By leewheel 2026-09-05: 上游1fea462f——被禁用的分档权重为0
                allianceWeights[i] = IsDisabledBracket(sPlayerbotAIConfig.levelBracketsAlliance, i) ? 0.0f : weight;
                hordeWeights[i] = IsDisabledBracket(sPlayerbotAIConfig.levelBracketsHorde, i) ? 0.0f : weight;
                //End By leewheel
            }
        }
        else
        {
            // 每阵营独立动态加权
            for (uint8 i = 0; i < _numRanges; ++i)
            {
                if (_allianceRanges[i].lower > _allianceRanges[i].upper ||
                    IsDisabledBracket(sPlayerbotAIConfig.levelBracketsAlliance, i))
                    allianceWeights[i] = 0.0f;
                else
                    allianceWeights[i] = baseline + sPlayerbotAIConfig.levelBracketsRealPlayerWeight *
                        (totalAllianceReal > 0 ? (1.0f / totalAllianceReal) : 1.0f) *
                        std::log(1 + allianceRealCounts[i]);

                if (_hordeRanges[i].lower > _hordeRanges[i].upper ||
                    IsDisabledBracket(sPlayerbotAIConfig.levelBracketsHorde, i))
                    hordeWeights[i] = 0.0f;
                else
                    hordeWeights[i] = baseline + sPlayerbotAIConfig.levelBracketsRealPlayerWeight *
                        (totalHordeReal > 0 ? (1.0f / totalHordeReal) : 1.0f) * std::log(1 + hordeRealCounts[i]);
            }
        }

        ApplyBracketWeights(_allianceRanges, allianceWeights);
        ApplyBracketWeights(_hordeRanges, hordeWeights);

        // 确保分档尊重全局最小/最大等级且百分比和为100
        ClampAndBalanceBrackets();

        for (uint8 i = 0; i < _numRanges; ++i)
            TC_LOG_DEBUG("playerbots",
                "[RandomBotLevelMgr] Final Range {}: {}-{}, Alliance Desired: {}%, Horde Desired: {}%", i + 1,
                _allianceRanges[i].lower, _allianceRanges[i].upper, _allianceRanges[i].pct, _hordeRanges[i].pct);
    }

    uint32 totalAllianceBots = 0;
    std::vector<int> allianceActualCounts(_numRanges, 0);
    std::vector<std::vector<Player*>> allianceBotsByRange(_numRanges);

    uint32 totalHordeBots = 0;
    std::vector<int> hordeActualCounts(_numRanges, 0);
    std::vector<std::vector<Player*>> hordeBotsByRange(_numRanges);

    for (auto const& itr : allPlayers)
    {
        Player* player = itr.second;
        if (!player || !player->IsInWorld())
            continue;
        if (!sRandomPlayerbotMgr.IsRandomBot(player))
            continue;
        if (IsNameInExcludeList(player, sPlayerbotAIConfig.levelBracketsExcludeNames))
            continue;

        PlayerbotAI* botAI = GET_PLAYERBOT_AI(player);
        if (sPlayerbotAIConfig.levelBracketsIgnoreGuildWithRealPlayers && botAI && botAI->IsInRealGuild())
            continue;
        if (sPlayerbotAIConfig.levelBracketsIgnoreFriendListed && BotInFriendList(player, _socialFriendsList))
            continue;
        if (sPlayerbotAIConfig.levelBracketsIgnoreArenaTeamBots && BotInArenaTeam(player))
            continue;

        if (player->GetTeamId() == TEAM_ALLIANCE)
        {
            totalAllianceBots++;
            int rangeIndex = GetOrFlagPlayerBracket(player);
            if (rangeIndex >= 0)
            {
                allianceActualCounts[rangeIndex]++;
                allianceBotsByRange[rangeIndex].push_back(player);
            }
        }
        else if (player->GetTeamId() == TEAM_HORDE)
        {
            totalHordeBots++;
            int rangeIndex = GetOrFlagPlayerBracket(player);
            if (rangeIndex >= 0)
            {
                hordeActualCounts[rangeIndex]++;
                hordeBotsByRange[rangeIndex].push_back(player);
            }
        }
    }

    TC_LOG_DEBUG("playerbots", "[RandomBotLevelMgr] Total Alliance Bots: {}. Total Horde Bots: {}.",
        totalAllianceBots, totalHordeBots);

    ProcessFactionDistribution(TEAM_ALLIANCE, totalAllianceBots, allianceActualCounts, allianceBotsByRange);
    ProcessFactionDistribution(TEAM_HORDE, totalHordeBots, hordeActualCounts, hordeBotsByRange);

    TC_LOG_DEBUG("playerbots",
        "[RandomBotLevelMgr] Distribution adjustment complete. Alliance bots: {}, Horde bots: {}.", totalAllianceBots,
        totalHordeBots);
}

// 处理待重置队列，每周期最多应用 FlaggedProcessLimit 次重置(0=不限)。
// bot离线/被排除/加入应保护其的公会/好友/竞技场/队伍/不再是随机bot/不再合格时从队列删除；
// 确认安全后重置并删除。
void RandomBotLevelMgr::ProcessPendingLevelResets()
{
    if (_pendingLevelResets.empty())
        return;

    uint32 processed = 0;
    for (auto it = _pendingLevelResets.begin(); it != _pendingLevelResets.end();)
    {
        if (sPlayerbotAIConfig.levelBracketsFlaggedProcessLimit > 0 &&
            processed >= sPlayerbotAIConfig.levelBracketsFlaggedProcessLimit)
            break;

        Player* bot = ObjectAccessor::FindPlayer(it->botGuid);

        if (!bot || !bot->IsInWorld() || !bot->GetSession() || bot->GetSession()->isLogingOut() ||
            bot->IsDuringRemoveFromWorld())
        {
            it = _pendingLevelResets.erase(it);
            continue;
        }

        // 防御：队列条目绝不能解析为随机bot之外的对象。真实玩家绝不应出现在此队列
        // (见GetOrFlagPlayerBracket)，但入队与处理之间玩家bot状态可能变化，这里也做防护
        if (!sRandomPlayerbotMgr.IsRandomBot(bot))
        {
            it = _pendingLevelResets.erase(it);
            continue;
        }

        if (IsNameInExcludeList(bot, sPlayerbotAIConfig.levelBracketsExcludeNames))
        {
            it = _pendingLevelResets.erase(it);
            continue;
        }

        int targetRange = it->targetRange;

        PlayerbotAI* botAI = GET_PLAYERBOT_AI(bot);
        if (sPlayerbotAIConfig.levelBracketsIgnoreGuildWithRealPlayers && botAI && botAI->IsInRealGuild())
        {
            it = _pendingLevelResets.erase(it);
            continue;
        }

        if (sPlayerbotAIConfig.levelBracketsIgnoreFriendListed && BotInFriendList(bot, _socialFriendsList))
        {
            it = _pendingLevelResets.erase(it);
            continue;
        }

        if (sPlayerbotAIConfig.levelBracketsIgnoreArenaTeamBots && BotInArenaTeam(bot))
        {
            it = _pendingLevelResets.erase(it);
            continue;
        }

        // 检查bot是否与真实玩家组队
        if (Group* group = bot->GetGroup())
        {
            bool hasRealPlayer = false;
            for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
            {
                Player* member = ref->GetSource();
                if (member && member->IsInWorld() && !GET_PLAYERBOT_AI(member))
                {
                    hasRealPlayer = true;
                    break;
                }
            }
            if (hasRealPlayer)
            {
                it = _pendingLevelResets.erase(it);
                continue;
            }
        }

        if (IsBotSafeForLevelReset(bot))
        {
            AdjustBotToRange(bot, targetRange, it->team);
            it = _pendingLevelResets.erase(it);
            ++processed;
        }
        else
            ++it;
    }
}

// =============================================================================
// LEVEL RESET FEATURE
// =============================================================================

// 计算给定等级bot应被重置的百分比概率。启用AiPlayerbot.ResetBotLevel.ScaledChance时，
// 概率从1级0%线性升到AiPlayerbot.ResetBotLevel.MaxLevel处的ResetChance
uint8 RandomBotLevelMgr::ComputeResetChance(uint8 level) const
{
    uint8 chance = sPlayerbotAIConfig.resetBotLevelChance;
    if (sPlayerbotAIConfig.resetBotLevelScaledChance)
    {
        chance = static_cast<uint8>((static_cast<float>(level) / sPlayerbotAIConfig.resetBotLevelMaxLevel) *
            sPlayerbotAIConfig.resetBotLevelChance);
        TC_LOG_DEBUG("playerbots",
            "[RandomBotLevelMgr] ComputeResetChance: For level {} / {} with scaling, computed chance = {}%", level,
            sPlayerbotAIConfig.resetBotLevelMaxLevel, chance);
    }
    else
        TC_LOG_DEBUG("playerbots",
            "[RandomBotLevelMgr] ComputeResetChance: For level {} / {} without scaling, chance = {}%", level,
            sPlayerbotAIConfig.resetBotLevelMaxLevel, chance);
    return chance;
}

// 通过完整PlayerbotFactory随机化把bot重置到AiPlayerbot.ResetBotLevel.ResetToLevel
// (或死亡骑士起始等级，取较高者)
void RandomBotLevelMgr::ResetBot(Player* player, uint8 currentLevel)
{
    uint8 levelToResetTo = sPlayerbotAIConfig.resetBotLevelResetTo;

    uint8 dkMinLevel = static_cast<uint8>(sWorld->getIntConfig(CONFIG_START_HEROIC_PLAYER_LEVEL));
    if (player->getClass() == CLASS_DEATH_KNIGHT && levelToResetTo < dkMinLevel)
        levelToResetTo = dkMinLevel;

    // 随机化前先下马，防止新等级出现错误坐骑
    if (player->IsMounted())
        player->Dismount();

    PlayerbotFactory newFactory(player, levelToResetTo);
    newFactory.Randomize(false);

    PlayerbotAI* botAI = GET_PLAYERBOT_AI(player);
    TC_LOG_DEBUG("playerbots", "[RandomBotLevelMgr] ResetBot: Bot '{}' - {} at level {} was reset to level {}.",
        player->GetName(), botAI ? botAI->GetChatHelper()->FormatClass(player->getClass()) : "Unknown", currentLevel,
        levelToResetTo);
}

// 通过完整PlayerbotFactory随机化把bot直接送到AiPlayerbot.ResetBotLevel.SkipToLevel
// (或死亡骑士起始等级，取较高者)
void RandomBotLevelMgr::SkipBotLevel(Player* player, uint8 currentLevel)
{
    uint8 levelToSkipTo = sPlayerbotAIConfig.resetBotLevelSkipTo;

    uint8 dkMinLevel = static_cast<uint8>(sWorld->getIntConfig(CONFIG_START_HEROIC_PLAYER_LEVEL));
    if (player->getClass() == CLASS_DEATH_KNIGHT && levelToSkipTo < dkMinLevel)
        levelToSkipTo = dkMinLevel;

    // 随机化前先下马，防止新等级出现错误坐骑
    if (player->IsMounted())
        player->Dismount();

    PlayerbotFactory newFactory(player, levelToSkipTo);
    newFactory.Randomize(false);

    PlayerbotAI* botAI = GET_PLAYERBOT_AI(player);
    TC_LOG_DEBUG("playerbots", "[RandomBotLevelMgr] SkipBotLevel: Bot '{}' - {} at level {} was skipped to level {}.",
        player->GetName(), botAI ? botAI->GetChatHelper()->FormatClass(player->getClass()) : "Unknown", currentLevel,
        levelToSkipTo);
}

// 对处于或高于MaxLevel的bot运行周期性在线时长重置检查。仅当Enabled、RestrictTimePlayed
// 且MaxLevel>0时才会走到(见Update())
void RandomBotLevelMgr::RunResetPlayedTimeCheck()
{
    TC_LOG_DEBUG("playerbots", "[RandomBotLevelMgr] OnUpdate: Starting time-based reset check...");

    auto const& allPlayers = ObjectAccessor::GetPlayers();
    for (auto const& itr : allPlayers)
    {
        Player* candidate = itr.second;
        if (!candidate || !candidate->IsInWorld())
            continue;
        if (!sRandomPlayerbotMgr.IsRandomBot(candidate))
            continue;

        if (IsNameInExcludeList(candidate, sPlayerbotAIConfig.resetBotLevelExcludeNames))
            continue;

        PlayerbotAI* botAI = GET_PLAYERBOT_AI(candidate);
        if (sPlayerbotAIConfig.resetBotLevelIgnoreGuildWithRealPlayers && botAI && botAI->IsInRealGuild())
            continue;

        uint8 currentLevel = candidate->GetLevel();
        if (currentLevel < sPlayerbotAIConfig.resetBotLevelMaxLevel)
            continue;

        // 只有在该等级至少游玩MinTimePlayed秒才重置
        if (candidate->GetLevelPlayedTime() < sPlayerbotAIConfig.resetBotLevelMinTimePlayed)
        {
            TC_LOG_DEBUG("playerbots",
                "[RandomBotLevelMgr] OnUpdate: Bot '{}' at level {} has insufficient played time ({} < {} "
                "seconds).",
                candidate->GetName(), currentLevel, candidate->GetLevelPlayedTime(),
                sPlayerbotAIConfig.resetBotLevelMinTimePlayed);
            continue;
        }

        uint8 resetChance = ComputeResetChance(currentLevel);
        TC_LOG_DEBUG("playerbots",
            "[RandomBotLevelMgr] OnUpdate: Bot '{}' qualifies for time-based reset. Level: {}, "
            "LevelPlayedTime: {} seconds, computed reset chance: {}%.",
            candidate->GetName(), currentLevel, candidate->GetLevelPlayedTime(), resetChance);
        if (urand(0, 99) < resetChance)
        {
            TC_LOG_DEBUG("playerbots",
                "[RandomBotLevelMgr] OnUpdate: Reset chance check passed for bot '{}'. Resetting bot.",
                candidate->GetName());
            ResetBot(candidate, currentLevel);
        }
    }
}

// =============================================================================
// SHARED UPDATE / HOOKS
// =============================================================================

void RandomBotLevelMgr::Update(uint32 diff)
{
    if (sPlayerbotAIConfig.levelBracketsEnabled)
    {
        _bracketsTimer += diff;
        _flaggedTimer += diff;

        if (_flaggedTimer >= sPlayerbotAIConfig.levelBracketsFlaggedCheckFrequency * 1000)
        {
            ProcessPendingLevelResets();
            _flaggedTimer = 0;
        }

        if (_bracketsTimer >= sPlayerbotAIConfig.levelBracketsCheckFrequency * 1000)
        {
            _bracketsTimer = 0;
            RunLevelBracketsDistribution();
        }
    }

    if (sPlayerbotAIConfig.resetBotLevelEnabled && sPlayerbotAIConfig.resetBotLevelRestrictTimePlayed &&
        sPlayerbotAIConfig.resetBotLevelMaxLevel > 0)
    {
        _resetTimer += diff;
        if (_resetTimer >= sPlayerbotAIConfig.resetBotLevelPlayedTimeCheckFrequency * 1000)
        {
            _resetTimer = 0;
            RunResetPlayedTimeCheck();
        }
    }
}

void RandomBotLevelMgr::OnBotLogin(Player* player)
{
    if (!sRandomPlayerbotMgr.IsRandomBot(player))
        return;

    if (IsNameInExcludeList(player, sPlayerbotAIConfig.resetBotLevelExcludeNames))
        return;

    PlayerbotAI* botAI = GET_PLAYERBOT_AI(player);
    if (sPlayerbotAIConfig.resetBotLevelIgnoreGuildWithRealPlayers && botAI && botAI->IsInRealGuild())
        return;

    uint8 currentLevel = player->GetLevel();

    if (sPlayerbotAIConfig.resetBotLevelMaxLevel > 0)
    {
        // bot高于MaxLevel：立即重置
        if (currentLevel > sPlayerbotAIConfig.resetBotLevelMaxLevel)
        {
            TC_LOG_DEBUG("playerbots",
                "[RandomBotLevelMgr] OnPlayerLogin: Bot '{}' above max level {}. Resetting immediately.",
                player->GetName(), sPlayerbotAIConfig.resetBotLevelMaxLevel);
            ResetBot(player, currentLevel);
            return;
        }

        // bot恰在MaxLevel：应用在线时长限制(若有)与概率
        if (currentLevel == sPlayerbotAIConfig.resetBotLevelMaxLevel)
        {
            if (!sPlayerbotAIConfig.resetBotLevelRestrictTimePlayed ||
                player->GetLevelPlayedTime() >= sPlayerbotAIConfig.resetBotLevelMinTimePlayed)
            {
                uint8 resetChance = ComputeResetChance(currentLevel);
                if (urand(0, 99) < resetChance)
                {
                    TC_LOG_DEBUG("playerbots",
                        "[RandomBotLevelMgr] OnPlayerLogin: Bot '{}' meets reset criteria. Resetting.",
                        player->GetName());
                    ResetBot(player, currentLevel);
                }
            }
        }
    }

    if (sPlayerbotAIConfig.resetBotLevelSkipFrom > 0 && currentLevel == sPlayerbotAIConfig.resetBotLevelSkipFrom)
    {
        TC_LOG_DEBUG("playerbots",
            "[RandomBotLevelMgr] OnPlayerLogin: Bot '{}' at skip level {}. Applying skip.", player->GetName(),
            currentLevel);
        SkipBotLevel(player, currentLevel);
    }
}

void RandomBotLevelMgr::OnBotLevelChanged(Player* player, uint8 oldLevel)
{
    if (!sRandomPlayerbotMgr.IsRandomBot(player))
        return;

    // 只对自然升级做出反应
    if (player->GetLevel() != oldLevel + 1)
        return;

    if (IsNameInExcludeList(player, sPlayerbotAIConfig.resetBotLevelExcludeNames))
        return;

    PlayerbotAI* botAI = GET_PLAYERBOT_AI(player);
    if (sPlayerbotAIConfig.resetBotLevelIgnoreGuildWithRealPlayers && botAI && botAI->IsInRealGuild())
        return;

    uint8 newLevel = player->GetLevel();

    //By leewheel 2026-09-05: 上游1fea462f——新建bot(1级)与DK起始等级不触发重置,避免误重置
    if (newLevel == 1)
        return;

    if (player->getClass() == CLASS_DEATH_KNIGHT &&
        newLevel == static_cast<uint8>(sWorld->getIntConfig(CONFIG_START_HEROIC_PLAYER_LEVEL)))
        return;
    //End By leewheel

    // SkipFromLevel优先且不受ScaledChance或RestrictTimePlayed影响
    if (sPlayerbotAIConfig.resetBotLevelSkipFrom > 0 && newLevel == sPlayerbotAIConfig.resetBotLevelSkipFrom)
    {
        TC_LOG_DEBUG("playerbots",
            "[RandomBotLevelMgr] OnPlayerLevelChanged: Bot '{}' reached skip level {}. Skipping to level {}.",
            player->GetName(), newLevel, sPlayerbotAIConfig.resetBotLevelSkipTo);
        SkipBotLevel(player, newLevel);
        return;
    }

    if (sPlayerbotAIConfig.resetBotLevelMaxLevel == 0)
        return;

    // 严格高于MaxLevel：无视在线时长立即重置
    if (newLevel > sPlayerbotAIConfig.resetBotLevelMaxLevel)
    {
        TC_LOG_DEBUG("playerbots",
            "[RandomBotLevelMgr] OnPlayerLevelChanged: Bot '{}' exceeded max level {}. Resetting immediately.",
            player->GetName(), sPlayerbotAIConfig.resetBotLevelMaxLevel);
        ResetBot(player, newLevel);
        return;
    }

    // 恰在MaxLevel且带在线时长限制：交由OnUpdate计时器处理
    if (sPlayerbotAIConfig.resetBotLevelRestrictTimePlayed && newLevel == sPlayerbotAIConfig.resetBotLevelMaxLevel)
    {
        TC_LOG_DEBUG("playerbots",
            "[RandomBotLevelMgr] OnPlayerLevelChanged: Bot '{}' at level {} deferred to OnUpdate due to "
            "time-played restriction.",
            player->GetName(), newLevel);
        return;
    }

    uint8 resetChance = ComputeResetChance(newLevel);
    if (sPlayerbotAIConfig.resetBotLevelScaledChance || newLevel >= sPlayerbotAIConfig.resetBotLevelMaxLevel)
    {
        TC_LOG_DEBUG("playerbots",
            "[RandomBotLevelMgr] OnPlayerLevelChanged: Bot '{}' at level {} has reset chance {}%.",
            player->GetName(), newLevel, resetChance);
        if (urand(0, 99) < resetChance)
            ResetBot(player, newLevel);
    }
}

void RandomBotLevelMgr::OnPlayerLogout(Player* player)
{
    // Level brackets: 把bot从待重置队列删除。即使分档子功能禁用也可安全运行，
    // 因为那时队列恒为空
    ObjectGuid guid = player->GetGUID();
    _pendingLevelResets.erase(
        std::remove_if(_pendingLevelResets.begin(), _pendingLevelResets.end(),
            [guid](PendingResetEntry const& entry) { return entry.botGuid == guid; }),
        _pendingLevelResets.end());
}

// =============================================================================
// WORLD SCRIPT
// =============================================================================
class RandomBotLevelWorldScript : public WorldScript
{
public:
    RandomBotLevelWorldScript() : WorldScript("RandomBotLevelWorldScript") {}

    // TC适配：AC的OnAfterConfigLoad在TC对应OnConfigLoad(reload)。reload=true时sConfigMgr
    // 已重载全部配置文件(含playerbots.conf)。初始(reload==false)调用发生在
    // PlayerbotAIConfig::Initialize()运行前，故只对真实reload生效——OnStartup覆盖初始加载
    void OnConfigLoad(bool reload) override
    {
        if (!reload)
            return;

        sPlayerbotAIConfig.LoadRandomBotLevelConfig();
        RandomBotLevelMgr::instance().LoadConfig();
        TC_LOG_INFO("playerbots", "[RandomBotLevelMgr] Level management config reloaded.");
    }

    void OnStartup() override
    {
        RandomBotLevelMgr::instance().LoadConfig();
        RandomBotLevelMgr::instance().LogStartupSummary();
    }

    void OnUpdate(uint32 diff) override
    {
        RandomBotLevelMgr::instance().Update(diff);
    }
};

// =============================================================================
// PLAYER SCRIPT
// =============================================================================
class RandomBotLevelPlayerScript : public PlayerScript
{
public:
    RandomBotLevelPlayerScript() : PlayerScript("RandomBotLevelPlayerScript") {}

    void OnLogin(Player* player, bool /*firstLogin*/) override
    {
        if (!sPlayerbotAIConfig.resetBotLevelEnabled)
            return;
        RandomBotLevelMgr::instance().OnBotLogin(player);
    }

    void OnLevelChanged(Player* player, uint8 oldLevel) override
    {
        if (!sPlayerbotAIConfig.resetBotLevelEnabled)
            return;
        RandomBotLevelMgr::instance().OnBotLevelChanged(player, oldLevel);
    }

    void OnLogout(Player* player) override
    {
        RandomBotLevelMgr::instance().OnPlayerLogout(player);
    }
};

// -----------------------------------------------------------------------------
// ENTRY POINT
// -----------------------------------------------------------------------------
void AddSC_randombot_level_mgr()
{
    new RandomBotLevelWorldScript();
    new RandomBotLevelPlayerScript();
}
