/* 副本机器人策略 */
#ifndef PLAYERBOTS_PVPCOMBATSTRATEGY_H
#define PLAYERBOTS_PVPCOMBATSTRATEGY_H

#include "Strategy.h"

class PlayerbotAI;

// PVP 战斗策略: 管理玩家对玩家战斗中的控制/打断/爆发/走位/保命/逃跑
// 原理参考 Trickerer NPCBots——每个战斗时刻根据局势选择最优动作
class PvpCombatStrategy : public Strategy
{
public:
    PvpCombatStrategy(PlayerbotAI* botAI);

    std::string const getName() override { return "pvp combat"; }
    void InitTriggers(std::vector<TriggerNode*>& triggers) override;
    void InitMultipliers(std::vector<Multiplier*>& multipliers) override;
};

#endif