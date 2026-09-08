/*
 * 纳克萨玛斯团本策略声明
 *
 * 作者: leewheel
 */

#ifndef PLAYERBOTS_NAXXSTRATEGY_H
#define PLAYERBOTS_NAXXSTRATEGY_H

#include "Strategy.h"

class RaidNaxxStrategy : public Strategy
{
public:
    RaidNaxxStrategy(PlayerbotAI* ai) : Strategy(ai) {}
    virtual std::string const getName() override { return "naxx"; }
    virtual void InitTriggers(std::vector<TriggerNode*>& triggers) override;
    virtual void InitMultipliers(std::vector<Multiplier*>& multipliers) override;
};

#endif
