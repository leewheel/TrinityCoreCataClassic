/* 太阳之井高地 机器人策略 */
#ifndef PLAYERBOTS_SWPSTRATEGY_H
#define PLAYERBOTS_SWPSTRATEGY_H

#include "Strategy.h"
#include <string>
#include <vector>

//By leewheel 2026-09-04: 上游——RaidSunwellStrategy 改名 RaidSwpStrategy
class RaidSwpStrategy : public Strategy
{
public:
    RaidSwpStrategy(PlayerbotAI* botAI) : Strategy(botAI) {}
//End By leewheel
    std::string const getName() override { return "sunwell"; }
    bool HasTargetExclusions() const override { return true; }
    void AppendTargetExclusions(GuidSet& exclusions, TargetValueExclusionType type) override;
    void InitTriggers(std::vector<TriggerNode*>& triggers) override;
    void InitMultipliers(std::vector<Multiplier*>& multipliers) override;
};

#endif
