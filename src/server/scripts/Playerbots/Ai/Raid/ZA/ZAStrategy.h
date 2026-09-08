/* 副本机器人策略 */
#ifndef PLAYERBOTS_ZASTRATEGY_H
#define PLAYERBOTS_ZASTRATEGY_H

#include "Strategy.h"
#include <string>
#include <vector>

class RaidZulAmanStrategy : public Strategy
{
public:
    RaidZulAmanStrategy(PlayerbotAI* botAI) : Strategy(botAI) {}
    std::string const getName() override { return "zulaman"; }

    void InitTriggers(std::vector<TriggerNode*>& triggers) override;
    void InitMultipliers(std::vector<Multiplier*>& multipliers) override;
};

#endif
