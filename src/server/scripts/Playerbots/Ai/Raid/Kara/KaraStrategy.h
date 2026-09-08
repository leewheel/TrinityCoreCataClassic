/* 卡拉赞 机器人策略 */
#ifndef PLAYERBOTS_KARASTRATEGY_H
#define PLAYERBOTS_KARASTRATEGY_H

#include "Strategy.h"
#include <string>
#include <vector>

class RaidKarazhanStrategy : public Strategy
{
public:
    RaidKarazhanStrategy(PlayerbotAI* botAI) : Strategy(botAI) {}
    std::string const getName() override { return "karazhan"; }
    bool HasTargetExclusions() const override { return true; }
    void AppendTargetExclusions(GuidSet& exclusions, TargetValueExclusionType type) override;
    void InitTriggers(std::vector<TriggerNode*>& triggers) override;
    void InitMultipliers(std::vector<Multiplier*>& multipliers) override;
};

#endif
