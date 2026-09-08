/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "PartyMemberToResurrect.h"

#include "Playerbots.h"

class IsTargetOfResurrectSpell : public SpellEntryPredicate
{
public:
    bool Check(SpellInfo const* spellInfo) override
    {
        for (uint8 i = 0; i < 3; ++i)
        {
            if (spellInfo->GetEffects()[i].Effect == SPELL_EFFECT_RESURRECT ||
                spellInfo->GetEffects()[i].Effect == SPELL_EFFECT_RESURRECT_NEW ||
                spellInfo->GetEffects()[i].Effect == SPELL_EFFECT_SELF_RESURRECT)
                return true;
        }

        return false;
    }
};

class FindDeadPlayer : public FindPlayerPredicate
{
public:
    FindDeadPlayer(PartyMemberValue* value) : value(value) {}

    bool Check(Unit* unit) override
    {
        Player* player = unit->ToPlayer();
        //By leewheel 2026-07-10: TC的DeathState是非作用域枚举，值是大写CORPSE
        return player && !player->isResurrectRequested() && player->getDeathState() == CORPSE &&
               !value->IsTargetOfSpellCast(player, predicate);
        //End By leewheel
    }

private:
    PartyMemberValue* value;
    IsTargetOfResurrectSpell predicate;
};

Unit* PartyMemberToResurrect::Calculate()
{
    FindDeadPlayer finder(this);
    return FindPartyMember(finder);
}
