//By leewheel 2026-08-01: 日志清理——注释掉公会创建/徽章/分配过程DEBUG与INFO，仅保留错误、警告与加载统计
//End By leewheel

#include "PlayerbotGuildMgr.h"
//By leewheel 2026-09-05: 上游c1fed461——按会长账号判定随机机器人公会需要查角色缓存
#include "CharacterCache.h"
//End By leewheel
#include "Player.h"
#include "PlayerbotAIConfig.h"
#include "DatabaseEnv.h"
#include "Guild.h"
#include "GuildMgr.h"
#include "ScriptMgr.h"
#include <thread>
#include <chrono>

//By leewheel 2026-07-13: 修复guild数据完整性问题(最终修复v3)
//根因分析:
//1. Guild::Disband()使用CommitTransaction(异步),事务可能未提交或失败
//2. Guild::Validate()在LoadGuilds()期间可能调用Disband()(会长不存在时)
//   当m_members为空时,Disband()不会为残留的guild_member条目追加DELETE
//3. Disband()调用sGuildMgr->RemoveGuild()立即从内存移除公会
//4. 结果: 内存中公会已不存在,但DB中guild和guild_member记录可能还在
//   (异步事务未提交/失败,或Disband未覆盖所有guild_member条目)
//5. LEFT JOIN清理只能删除guild表中已不存在的guild_member条目
//   如果guild表行还在(事务失败),LEFT JOIN找不到孤儿条目!
//
//修复方案(核弹级v3):
//A. DeleteRandomBotGuilds()后等待异步队列清空
//B. 查询guild表所有guildid,逐个检查是否在sGuildMgr内存中
//   不在内存中的 → 直接用DirectExecute强制删除guild表和所有关联表记录
//   这处理异步事务失败导致guild表行残留的情况
//C. 再用LEFT JOIN清理guild_member孤儿条目(额外保险)
//D. InitGuild中AddMember前同步删除旧guild_member条目
//E. CharacterHandler中检测到not existing guild时同步删除DB条目
void PlayerbotGuildMgr::Init()
{
    _guildCache.clear();
    if (sPlayerbotAIConfig.deleteRandomBotGuilds)
        DeleteRandomBotGuilds();

    // 等待Disband的异步事务全部提交完成
    uint32 waitCount = 0;
    while (CharacterDatabase.QueueSize() > 0)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        if (++waitCount > 300) // 最多等待30秒
        {
            TC_LOG_WARN("playerbots", "PlayerbotGuildMgr::Init: 等待异步DB队列超时(30s),队列中仍有{}个任务",
                CharacterDatabase.QueueSize());
            break;
        }
    }
    if (waitCount > 0)
    {
        TC_LOG_INFO("playerbots", "PlayerbotGuildMgr::Init: 等待异步DB队列清空完成({}ms)", waitCount * 100);
    }

    // 核心修复: 查询guild表中所有公会,检查哪些不在sGuildMgr内存中
    // 不在内存中的公会说明已被Disband/Validate移除,但DB记录可能还在
    // (异步事务未提交或失败,导致guild表行残留)
    // 直接用DirectExecute强制删除这些孤儿guild记录
    QueryResult guildResult = CharacterDatabase.Query("SELECT guildid FROM guild");
    if (guildResult)
    {
        std::vector<uint32> orphanGuildIds;
        do
        {
            Field* fields = guildResult->Fetch();
            uint32 guildId = (uint32)fields[0].Get<uint64>();
            // 检查这个公会是否在sGuildMgr内存中
            if (!sGuildMgr->GetGuildById(guildId))
            {
                orphanGuildIds.push_back(guildId);
            }
        } while (guildResult->NextRow());

        if (!orphanGuildIds.empty())
        {
            TC_LOG_WARN("playerbots", "PlayerbotGuildMgr::Init: 发现{}个孤儿guild记录(不在内存中),强制删除",
                orphanGuildIds.size());

            for (uint32 guildId : orphanGuildIds)
            {
                TC_LOG_INFO("playerbots", "PlayerbotGuildMgr::Init: 强制删除孤儿公会 guildid={}", guildId);
                // 按顺序删除所有关联表记录
                CharacterDatabase.DirectPExecute("DELETE FROM guild_member WHERE guildid = {}", guildId);
                CharacterDatabase.DirectPExecute("DELETE FROM guild_rank WHERE guildid = {}", guildId);
                CharacterDatabase.DirectPExecute("DELETE FROM guild_bank_tab WHERE guildid = {}", guildId);
                CharacterDatabase.DirectPExecute("DELETE FROM guild_bank_item WHERE guildid = {}", guildId);
                CharacterDatabase.DirectPExecute("DELETE FROM guild_bank_eventlog WHERE guildid = {}", guildId);
                CharacterDatabase.DirectPExecute("DELETE FROM guild_bank_right WHERE guildid = {}", guildId);
                CharacterDatabase.DirectPExecute("DELETE FROM guild_eventlog WHERE guildid = {}", guildId);
                CharacterDatabase.DirectPExecute("DELETE FROM guild_newslog WHERE guildid = {}", guildId);
                CharacterDatabase.DirectPExecute("DELETE FROM guild WHERE guildid = {}", guildId);
            }
        }
    }

    // 额外保险: 用LEFT JOIN清理任何残留的孤儿guild_member条目
    CharacterDatabase.DirectExecute(
        "DELETE gm FROM guild_member gm LEFT JOIN guild g ON gm.guildid = g.guildid WHERE g.guildid IS NULL");

    // 统计清理后的状态
    QueryResult orphanCheck = CharacterDatabase.Query(
        "SELECT COUNT(*) FROM guild_member gm LEFT JOIN guild g ON gm.guildid = g.guildid WHERE g.guildid IS NULL");
    if (orphanCheck)
    {
        uint64 remaining = (*orphanCheck)[0].Get<uint64>();
        if (remaining > 0)
        {
            TC_LOG_ERROR("playerbots", "PlayerbotGuildMgr::Init: 清理后仍有{}条孤儿guild_member记录!", remaining);
        }
        else
        {
            TC_LOG_INFO("playerbots", "PlayerbotGuildMgr::Init: guild_member孤儿记录清理完成");
        }
    }

    LoadGuildNames();
    ValidateGuildCache();
}
//End By leewheel

bool PlayerbotGuildMgr::CreateGuild(Player* player, std::string guildName)
{
    Guild* guild = new Guild();
    if (!guild->Create(player, guildName))
    {
        TC_LOG_ERROR("playerbots", "Error creating guild [ {} ] with leader [ {} ]", guildName,
            player->GetName());
        delete guild;
        return false;
    }
    sGuildMgr->AddGuild(guild);

    // TC_LOG_DEBUG("playerbots", "Guild created: id={} name='{}'", guild->GetId(), guildName);
    SetGuildEmblem(guild->GetId());

    GuildCache entry;
    entry.name = guildName;
    entry.memberCount = 1;
    entry.status = 1;
    entry.maxMembers = sPlayerbotAIConfig.randomBotGuildSizeMax;
    entry.faction = player->GetTeamId();

    _guildCache[guild->GetId()] = entry;
    return true;
}

bool PlayerbotGuildMgr::SetGuildEmblem(uint32 guildId)
{
    Guild* guild = sGuildMgr->GetGuildById(guildId);
    if (!guild)
        return false;

    // create random emblem
    uint32 st, cl, br, bc, bg;
    bg = urand(0, 51);
    bc = urand(0, 17);
    cl = urand(0, 17);
    br = urand(0, 7);
    st = urand(0, 180);

    // TC_LOG_DEBUG("playerbots",
    //     "[TABARD] new guild id={} random -> style={}, color={}, borderStyle={}, borderColor={}, bgColor={}",
    //     guild->GetId(), st, cl, br, bc, bg);

    // populate guild table with a random tabard design
    //By leewheel 2026-07-10: TC的Execute不支持格式化参数，使用PExecute
    CharacterDatabase.PExecute(
        "UPDATE guild SET EmblemStyle={}, EmblemColor={}, BorderStyle={}, BorderColor={}, BackgroundColor={} "
        "WHERE guildid={}",
        st, cl, br, bc, bg, guild->GetId());
    //End By leewheel
    // TC_LOG_DEBUG("playerbots", "[TABARD] UPDATE done for guild id={}", guild->GetId());

    // Immediate reading for log
    //By leewheel 2026-07-10: TC的Query不支持格式化参数，使用PQuery
    if (QueryResult qr = CharacterDatabase.PQuery(
            "SELECT EmblemStyle,EmblemColor,BorderStyle,BorderColor,BackgroundColor FROM guild WHERE guildid={}",
            guild->GetId()))
    //End By leewheel
    {
        //By leewheel 2026-09-04: 修 C4189——字段读取的调试日志已注释, QueryResult 仍需消费校验行存在
        [[maybe_unused]] Field* f = qr->Fetch();
        // TC_LOG_DEBUG("playerbots",
        //     "[TABARD] DB check guild id={} => style={}, color={}, borderStyle={}, borderColor={}, bgColor={}",
        //     guild->GetId(), f[0].Get<uint8>(), f[1].Get<uint8>(), f[2].Get<uint8>(), f[3].Get<uint8>(), f[4].Get<uint8>());
    }
    return true;
}

std::string PlayerbotGuildMgr::AssignToGuild(Player* player)
{
    if (!player)
        return "";

    uint8_t playerFaction = player->GetTeamId();
    std::vector<GuildCache*> partiallyfilledguilds;
    partiallyfilledguilds.reserve(_guildCache.size());

    for (auto& keyValue : _guildCache)
    {
        GuildCache& cached = keyValue.second;
        if (!cached.hasRealPlayer && cached.status == 1 && cached.faction == playerFaction)
            partiallyfilledguilds.push_back(&cached);
    }

    if (!partiallyfilledguilds.empty())
    {
        size_t idx = static_cast<size_t>(urand(0, static_cast<int>(partiallyfilledguilds.size()) - 1));
        return (partiallyfilledguilds[idx]->name);
    }

    size_t count = std::count_if(
        _guildCache.begin(), _guildCache.end(),
        [](const std::pair<const uint32, GuildCache>& pair)
        {
            return !pair.second.hasRealPlayer;
        }
        );

    if (count < sPlayerbotAIConfig.randomBotGuildCount)
    {
        for (auto& key : _shuffled_guild_keys)
        {
            if (_guildNames[key])
            {
                // TC_LOG_INFO("playerbots","Assigning player [{}] to guild [{}]", player->GetName(), key);
                return key;
            }
        }
        TC_LOG_ERROR("playerbots","No available guild names left.");
    }
    return "";
}

void PlayerbotGuildMgr::OnGuildUpdate(Guild* guild)
{
    auto it = _guildCache.find(guild->GetId());
    if (it == _guildCache.end())
        return;

    GuildCache& entry = it->second;
    //By leewheel 2025-07-10
    // TC的Guild使用GetMembersCount()方法
    entry.memberCount = guild->GetMembersCount();
    //End By leewheel
    if (entry.memberCount < entry.maxMembers)
        entry.status = 1;
    else if (entry.memberCount >= entry.maxMembers)
        entry.status = 2; // Full
    std::string guildName = guild->GetName();
    for (auto& it : _guildNames)
    {
        if (it.first == guildName)
        {
            it.second = false;
            break;
        }
    }
}

void PlayerbotGuildMgr::ResetGuildCache()
{
    _guildCache.clear();

    for (auto& nameEntry : _guildNames)
        nameEntry.second = true;
}

void PlayerbotGuildMgr::LoadGuildNames()
{
    TC_LOG_INFO("playerbots", "Loading guild names from playerbots_guild_names...");

    QueryResult result = CharacterDatabase.Query("SELECT name_id, name FROM playerbots_guild_names");

    if (!result)
    {
        TC_LOG_ERROR("playerbots", "No entries found in playerbots_guild_names. List is empty.");
        return;
    }

    do
    {
        Field* fields = result->Fetch();
        _guildNames[fields[1].Get<std::string>()] = true;
    } while (result->NextRow());

    for (auto& pair : _guildNames)
        _shuffled_guild_keys.push_back(pair.first);

    std::random_device rd;
    std::mt19937 g(rd());

    std::shuffle(_shuffled_guild_keys.begin(), _shuffled_guild_keys.end(), g);
    TC_LOG_INFO("playerbots", "Loaded {} guild entries from playerbots_guild_names table.", _guildNames.size());
}

void PlayerbotGuildMgr::ValidateGuildCache()
{
    QueryResult result = CharacterDatabase.Query("SELECT guildid, name FROM guild");
    if (!result)
    {
        TC_LOG_ERROR("playerbots", "No guilds found in database, resetting guild cache");
        ResetGuildCache();
        return;
    }

    std::unordered_map<uint32, std::string> dbGuilds;
    do
    {
        Field* fields = result->Fetch();
        uint32 guildId = (uint32)fields[0].Get<uint64>(); //By leewheel 2026-07-12: TC的guild.guildid是bigint unsigned
        std::string guildName = fields[1].Get<std::string>();
        dbGuilds[guildId] = guildName;
    } while (result->NextRow());

    for (auto it = dbGuilds.begin(); it != dbGuilds.end(); it++)
    {
        uint32 guildId = it->first;
        GuildCache cache;
        cache.name = it->second;
        cache.maxMembers = sPlayerbotAIConfig.randomBotGuildSizeMax;

        Guild* guild = sGuildMgr ->GetGuildById(guildId);
        if (!guild)
            continue;

        //By leewheel 2025-07-10
        // TC使用GetMembersCount()方法
        cache.memberCount = guild->GetMembersCount();
        ObjectGuid leaderGuid = guild->GetLeaderGUID();
        // CharacterCacheEntry const* leaderEntry = sCharacterCache->GetCharacterCacheByGuid(leaderGuid);
        // TC中使用CharacterCacheEntry，成员访问方式不同，暂注释
        // uint32 leaderAccount = leaderEntry->AccountId;
        cache.hasRealPlayer = true; // TC中无法直接判断，暂时设为true
        cache.faction = TEAM_ALLIANCE; // 默认值
        //End By leewheel
        if (cache.memberCount == 0)
            cache.status = 0; // empty
        else if (cache.memberCount < cache.maxMembers)
            cache.status = 1; // partially filled
        else
            cache.status = 2; // full

        _guildCache.insert_or_assign(guildId, cache);
        for (auto& it : _guildNames)
        {
            if (it.first == cache.name)
            {
                it.second = false;
                break;
            }
        }
    }
}

void PlayerbotGuildMgr::DeleteRandomBotGuilds()
{
    TC_LOG_INFO("playerbots", "Deleting randombot guilds...");
    std::vector<uint32> guildsToDisband;

    //By leewheel 2026-09-05: 上游c1fed461——改为直接扫guild表并按会长账号判定,
    //不依赖玩家在线状态/'add'事件,addclass bot与randombot的公会(会长账号在随机账号名单内)都会被删除
    if (QueryResult result = CharacterDatabase.Query("SELECT guildid, leaderguid FROM guild"))
    {
        do
        {
            Field* fields = result->Fetch();
            uint32 guildId = fields[0].Get<uint32>();
            ObjectGuid leader = ObjectGuid::Create<HighGuid::Player>(fields[1].Get<uint32>());

            if (sPlayerbotAIConfig.IsInRandomAccountList(sCharacterCache->GetCharacterAccountIdByGuid(leader)))
                guildsToDisband.push_back(guildId);
        } while (result->NextRow());
    }
    //End By leewheel

    for (uint32 guildId : guildsToDisband)
    {
        if (Guild* guild = sGuildMgr->GetGuildById(guildId))
            guild->Disband();
    }
    TC_LOG_INFO("playerbots", "Randombot guilds deleted");
}

bool PlayerbotGuildMgr::IsRealGuild(Player* bot)
{
    if (!bot)
        return false;
    uint32 guildId = bot->GetGuildId();
    if (!guildId)
        return false;

    return IsRealGuild(guildId);
}

bool PlayerbotGuildMgr::IsRealGuild(uint32 guildId)
{
    if (!guildId)
        return false;

    auto it = _guildCache.find(guildId);
    if (it == _guildCache.end())
        return false;

    return it->second.hasRealPlayer;
}

class BotGuildCacheWorldScript : public WorldScript
{
    public:

        BotGuildCacheWorldScript() : WorldScript("BotGuildCacheWorldScript"), _validateTimer(0){}

        void OnUpdate(uint32 diff) override
        {
            _validateTimer += diff;

            if (_validateTimer >= _validateInterval) // Validate every hour
            {
                _validateTimer = 0;
                PlayerbotGuildMgr::instance().ValidateGuildCache();
                TC_LOG_INFO("playerbots", "Scheduled guild cache validation");
            }
        }

    private:
        uint32 _validateInterval = HOUR*IN_MILLISECONDS;
        uint32 _validateTimer;
};

void PlayerBotsGuildValidationScript()
{
    new BotGuildCacheWorldScript();
}
