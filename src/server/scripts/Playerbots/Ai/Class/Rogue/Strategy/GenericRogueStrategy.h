/*
 * 移植来源: AC azerothcore-wotlk mod-playerbots the-lab 提交 188a7f9b (implement generic rogue strategy)
 * 移植适配 TC 框架
 * 业务对标: AC azerothcore-wotlk mod-playerbots
 * 作者: leewheel
 */

#ifndef PLAYERBOTS_GENERICROGUESTRATEGY_H
#define PLAYERBOTS_GENERICROGUESTRATEGY_H

#include "CombatStrategy.h"

class PlayerbotAI;

// 盗贼通用战斗策略基类：战斗状态下给主/副手武器上毒。
// 上游新增该中间类后，战斗(combat)与刺杀(assassination)策略均改由此派生，
// 使两类专精在战斗中也能补毒(此前只有非战斗 GenericRogueNonCombatStrategy 会上毒)。
class GenericRogueStrategy : public CombatStrategy
{
public:
    GenericRogueStrategy(PlayerbotAI* botAI);

    std::string const getName() override { return "rogue"; }
    void InitTriggers(std::vector<TriggerNode*>& triggers) override;
    uint32 GetType() const override
    {
        return CombatStrategy::GetType() | STRATEGY_TYPE_DPS | STRATEGY_TYPE_MELEE;
    }
};

#endif
