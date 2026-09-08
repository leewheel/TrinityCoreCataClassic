/* 随机机器人等级管理——等级分档 + 满级重置 */
/*By leewheel 2026-08-15: 从 the-lab(brighton-chi) mod-playerbots 移植到 TC 3.4.3。
基于 mod-player-bot-level-brackets 与 mod-player-bot-reset 模块，贡献者 NoxMax(等级重置)、
jimm0thy(好友列表排除)、Jered Little(竞技场战队排除)。*/

#ifndef PLAYERBOTS_RANDOMBOTLEVELMGR_H
#define PLAYERBOTS_RANDOMBOTLEVELMGR_H

#include "ObjectGuid.h"
#include "PlayerbotAIConfig.h"
#include "SharedDefines.h"
#include <vector>

class Player;

// 拥有两个移植子功能：按阵营等级分档周期性重分配随机机器人、重置达到满级的随机机器人。
// 配置存于 PlayerbotAIConfig；本类持有运行期工作状态(分档工作副本、待重置队列、计时器)。
class RandomBotLevelMgr
{
public:
    static RandomBotLevelMgr& instance()
    {
        static RandomBotLevelMgr instance;

        return instance;
    }

    void LoadConfig();
    void LogStartupSummary() const;
    void Update(uint32 diff);
    void OnBotLogin(Player* player);
    void OnBotLevelChanged(Player* player, uint8 oldLevel);
    void OnPlayerLogout(Player* player);

private:
    RandomBotLevelMgr() = default;
    ~RandomBotLevelMgr() = default;

    RandomBotLevelMgr(RandomBotLevelMgr const&) = delete;
    RandomBotLevelMgr& operator=(RandomBotLevelMgr const&) = delete;

    RandomBotLevelMgr(RandomBotLevelMgr&&) = delete;
    RandomBotLevelMgr& operator=(RandomBotLevelMgr&&) = delete;

    // 处理时解析回存活的 _allianceRanges/_hordeRanges 向量，因为配置重载可能resize这些向量
    struct PendingResetEntry
    {
        ObjectGuid botGuid;
        int targetRange;
        TeamId team;
    };

    // ---- Level brackets sub-feature ----
    std::vector<LevelBracketConfig>& GetFactionRanges(TeamId team);
    void ClampAndBalanceBrackets();
    void ApplyBracketWeights(std::vector<LevelBracketConfig>& ranges, std::vector<float> const& weights);
    int GetLevelRangeIndex(uint8 level, TeamId team);
    void AdjustBotToRange(Player* bot, int targetRangeIndex, TeamId team);
    void LoadSocialFriendList();
    int GetOrFlagPlayerBracket(Player* player);
    void RunLevelBracketsDistribution();
    void ProcessFactionDistribution(TeamId team, uint32 totalBots, std::vector<int>& actualCounts,
        std::vector<std::vector<Player*>>& botsByRange);
    void RedistributeSurplusBots(std::vector<Player*>& sourceBots, int fromRange, TeamId team,
        std::vector<int>& actualCounts, std::vector<int> const& desiredCounts, std::vector<int> const& targetRanges);
    void ProcessPendingLevelResets();

    // ---- Level reset sub-feature ----
    uint8 ComputeResetChance(uint8 level) const;
    void ResetBot(Player* player, uint8 currentLevel);
    void SkipBotLevel(Player* player, uint8 currentLevel);
    void RunResetPlayedTimeCheck();

    // Level brackets: 工作副本——动态分布与钳位/再平衡会运行期改百分比，绝不写回PlayerbotAIConfig
    std::vector<LevelBracketConfig> _allianceRanges;
    std::vector<LevelBracketConfig> _hordeRanges;
    uint8 _numRanges = 9;
    uint8 _randomBotMinLevel = 1;
    uint8 _randomBotMaxLevel = 80;
    std::vector<PendingResetEntry> _pendingLevelResets;
    std::vector<uint32> _socialFriendsList;

    uint32 _bracketsTimer = 0; // Level brackets: 分布调整
    uint32 _flaggedTimer = 0;  // Level brackets: 待重置检查
    uint32 _resetTimer = 0;    // Level reset: 在线时长重置检查
};

// 注册随机机器人等级分档+等级重置的 world/player 脚本
void AddSC_randombot_level_mgr();

#endif // PLAYERBOTS_RANDOMBOTLEVELMGR_H
