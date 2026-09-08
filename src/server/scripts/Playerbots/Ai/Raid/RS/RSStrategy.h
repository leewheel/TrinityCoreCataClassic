/* 红玉圣殿 机器人策略 */
#ifndef PLAYERBOTS_RSS_H
#define PLAYERBOTS_RSS_H

#include "Strategy.h"

class RaidRsStrategy : public Strategy
{
public:
    RaidRsStrategy(PlayerbotAI* ai) : Strategy(ai) {}
    virtual std::string const getName() override { return "rs"; }
    virtual void InitTriggers(std::vector<TriggerNode*>& triggers) override;
    virtual void InitMultipliers(std::vector<Multiplier*> &multipliers) override;
};

#endif
