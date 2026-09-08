/* 海加尔山 机器人策略 */
#ifndef PLAYERBOTS_HYJALSTRATEGY_H
#define PLAYERBOTS_HYJALSTRATEGY_H

#include "Strategy.h"
#include <string>
#include <vector>

//By leewheel 2026-09-04: 上游——RaidHyjalSummitStrategy 改名 RaidHyjalStrategy
class RaidHyjalStrategy : public Strategy
{
public:
    RaidHyjalStrategy(PlayerbotAI* botAI) : Strategy(botAI) {}
//End By leewheel

    std::string const getName() override { return "hyjal"; }

    void InitTriggers(std::vector<TriggerNode*>& triggers) override;
    void InitMultipliers(std::vector<Multiplier*>& multipliers) override;
};

#endif
