#ifndef PLAYERBOTS_ACSTRATEGY_H
#define PLAYERBOTS_ACSTRATEGY_H

#include "AiObjectContext.h"
#include "Strategy.h"
//By leewheel 2026-08-24: 合并 the-lab——补充 <string>/<vector> (保留 HEAD 的 Multiplier.h)
#include "Multiplier.h"
#include <string>
#include <vector>
//End By leewheel

class TbcDungeonAuchenaiCryptsStrategy : public Strategy
{
public:
    TbcDungeonAuchenaiCryptsStrategy(PlayerbotAI* botAI) : Strategy(botAI) {}
    virtual std::string const getName() override { return "tbc-ac"; }

    virtual void InitTriggers(std::vector<TriggerNode*> &triggers) override;
    virtual void InitMultipliers(std::vector<Multiplier*> &multipliers) override;
};

#endif
