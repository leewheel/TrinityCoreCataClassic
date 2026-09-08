/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#ifndef PLAYERBOTS_DPSROGUESTRATEGY_H
#define PLAYERBOTS_DPSROGUESTRATEGY_H

//By leewheel 2026-09-04: 对齐上游188a7f9b，战斗贼策略基类由 MeleeCombatStrategy 改为 GenericRogueStrategy(战斗补毒)
#include "GenericRogueStrategy.h"

class PlayerbotAI;

class DpsRogueStrategy : public GenericRogueStrategy
{
public:
    DpsRogueStrategy(PlayerbotAI* botAI);

    void InitTriggers(std::vector<TriggerNode*>& triggers) override;
    std::string const getName() override { return "dps"; }
    std::vector<NextAction> getDefaultActions() override;
};
//End By leewheel

class StealthedRogueStrategy : public Strategy
{
public:
    StealthedRogueStrategy(PlayerbotAI* botAI);

    void InitTriggers(std::vector<TriggerNode*>& triggers) override;
    std::string const getName() override { return "stealthed"; }
    std::vector<NextAction> getDefaultActions() override;
};

class StealthStrategy : public Strategy
{
public:
    StealthStrategy(PlayerbotAI* botAI) : Strategy(botAI){};

    // virtual int GetType() { return STRATEGY_TYPE_NONCOMBAT; }
    void InitTriggers(std::vector<TriggerNode*>& triggers) override;
    std::string const getName() override { return "stealth"; }
};

class RogueAoeStrategy : public Strategy
{
public:
    RogueAoeStrategy(PlayerbotAI* botAI) : Strategy(botAI) {}

    void InitTriggers(std::vector<TriggerNode*>& triggers) override;
    std::string const getName() override { return "aoe"; }
};

class RogueBoostStrategy : public Strategy
{
public:
    RogueBoostStrategy(PlayerbotAI* botAI) : Strategy(botAI) {}

    void InitTriggers(std::vector<TriggerNode*>& triggers) override;
    std::string const getName() override { return "boost"; }
};

class RogueCcStrategy : public Strategy
{
public:
    RogueCcStrategy(PlayerbotAI* botAI) : Strategy(botAI) {}

    void InitTriggers(std::vector<TriggerNode*>& triggers) override;
    std::string const getName() override { return "cc"; }
};

#endif
