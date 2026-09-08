/* 毒蛇神殿 机器人策略 */
#ifndef PLAYERBOTS_SSCSTRATEGY_H
#define PLAYERBOTS_SSCSTRATEGY_H

#include "Strategy.h"

//By leewheel 2026-09-04: 上游——RaidSSCStrategy 改名 RaidSscStrategy
class RaidSscStrategy : public Strategy
{
public:
    RaidSscStrategy(PlayerbotAI* botAI) : Strategy(botAI) {}
//End By leewheel

    std::string const getName() override { return "ssc"; }

    void InitTriggers(std::vector<TriggerNode*>& triggers) override;
    void InitMultipliers(std::vector<Multiplier*>& multipliers) override;
};

#endif
