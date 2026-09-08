
#ifndef PLAYERBOTS_ASSASSINATIONROGUESTRATEGY_H
#define PLAYERBOTS_ASSASSINATIONROGUESTRATEGY_H

//By leewheel 2026-09-04: 对齐上游188a7f9b，刺杀贼策略基类由 MeleeCombatStrategy 改为 GenericRogueStrategy(战斗补毒)
#include "GenericRogueStrategy.h"

class AssassinationRogueStrategy : public GenericRogueStrategy
{
public:
    AssassinationRogueStrategy(PlayerbotAI* ai);

public:
    virtual void InitTriggers(std::vector<TriggerNode*>& triggers) override;
    virtual std::string const getName() override { return "melee"; }
    virtual std::vector<NextAction> getDefaultActions() override;
};
//End By leewheel

#endif
