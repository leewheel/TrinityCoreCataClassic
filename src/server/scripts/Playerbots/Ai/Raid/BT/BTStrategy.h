/* 黑暗神殿 机器人策略 */
#ifndef PLAYERBOTS_BTSTRATEGY_H
#define PLAYERBOTS_BTSTRATEGY_H

#include "Strategy.h"

class RaidBlackTempleStrategy : public Strategy
{
public:
    RaidBlackTempleStrategy(PlayerbotAI* botAI) : Strategy(botAI) {}

    std::string const getName() override { return "blacktemple"; }

    void InitTriggers(std::vector<TriggerNode*>& triggers) override;
    void InitMultipliers(std::vector<Multiplier*>& multipliers) override;
};

#endif
