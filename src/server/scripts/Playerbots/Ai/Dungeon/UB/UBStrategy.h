/*
 * 幽暗沼泽 机器人策略
 */

#ifndef PLAYERBOTS_UBSTRATEGY_H
#define PLAYERBOTS_UBSTRATEGY_H

#include "Multiplier.h"
#include "Strategy.h"

class TbcDungeonUnderbogStrategy : public Strategy
{
public:
    TbcDungeonUnderbogStrategy(PlayerbotAI* botAI) : Strategy(botAI) {}

    std::string const getName() override { return "tbc-ub"; }

    void InitTriggers(std::vector<TriggerNode*>& triggers) override;
    void InitMultipliers(std::vector<Multiplier*>& multipliers) override;

    bool HasTargetExclusions() const override { return true; }
    void AppendTargetExclusions(GuidSet& exclusions, TargetValueExclusionType type) override;
};

#endif
