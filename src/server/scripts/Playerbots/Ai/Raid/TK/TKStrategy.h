/* 风暴要塞 机器人策略 */
#ifndef PLAYERBOTS_TKSTRATEGY_H
#define PLAYERBOTS_TKSTRATEGY_H

#include "Strategy.h"
#include <string>
#include <vector>

class RaidTempestKeepStrategy : public Strategy
{
public:
    RaidTempestKeepStrategy(PlayerbotAI* botAI) : Strategy(botAI) {}
    std::string const getName() override { return "tempestkeep"; }
    bool HasTargetExclusions() const override { return true; }
    void AppendTargetExclusions(GuidSet& exclusions, TargetValueExclusionType type) override;
    void InitTriggers(std::vector<TriggerNode*>& triggers) override;
    void InitMultipliers(std::vector<Multiplier*>& multipliers) override;
};

#endif
