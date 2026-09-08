/* 玛瑟里顿的巢穴 机器人策略 */
#ifndef PLAYERBOTS_MAGSTRATEGY_H
#define PLAYERBOTS_MAGSTRATEGY_H

#include "Strategy.h"
#include <string>
#include <vector>

class RaidMagtheridonStrategy : public Strategy
{
public:
    RaidMagtheridonStrategy(PlayerbotAI* botAI) : Strategy(botAI) {}

    std::string const getName() override { return "magtheridon"; }

    void InitTriggers(std::vector<TriggerNode*>& triggers) override;
    void InitMultipliers(std::vector<Multiplier*>& multipliers) override;
};

#endif
