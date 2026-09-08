/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#ifndef PLAYERBOTS_ATTACKACTION_H
#define PLAYERBOTS_ATTACKACTION_H

#include "MovementActions.h"

class PlayerbotAI;

class AttackAction : public MovementAction
{
public:
    AttackAction(PlayerbotAI* botAI, std::string const name) : MovementAction(botAI, name) {}

    bool Execute(Event event) override;

protected:
    bool Attack(Unit* target, bool with_pet = true);
};

class AttackMyTargetAction : public AttackAction
{
public:
    AttackMyTargetAction(PlayerbotAI* botAI, std::string const name = "attack my target") : AttackAction(botAI, name) {}

    bool Execute(Event event) override;
};

class AttackDuelOpponentAction : public AttackAction
{
public:
    AttackDuelOpponentAction(PlayerbotAI* botAI, std::string const name = "attack duel opponent")
        : AttackAction(botAI, name)
    {
    }

public:
    bool Execute(Event event) override;
    bool isUseful() override;
};

//By leewheel 2026-08-21: 移植 brighton-chi b094029a——MeleeAction从GenericActions迁移至此
class MeleeAction : public AttackAction
{
public:
    MeleeAction(PlayerbotAI* botAI) : AttackAction(botAI, "melee") {}

    std::string const GetTargetName() override { return "current target"; }
    bool isUseful() override;
};
//End By leewheel

#endif
