/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "DKTriggers.h"

#include <string>

#include "GenericTriggers.h"
#include "Playerbots.h"
#include "SharedDefines.h"

//By leewheel 2026-08-06: DK Presence改用spell ID检查,防止locale匹配失败导致重复切姿态
//By leewheel 2026-08-07: 通过 classic_db2.spelllevels 复核, 修正注释(原 Blood/Frost 写反), 补全天赋变体ID
//Blood Presence=48266(主)+50475(变体), Frost Presence=48263(主)+61261(变体), Unholy Presence=48265(主)+49772(变体)
bool DKPresenceTrigger::IsActive()
{
    Unit* target = GetTarget();
    return !target->HasAura(48263) && !target->HasAura(48265) && !target->HasAura(48266) &&
           !target->HasAura(50475) && !target->HasAura(61261) && !target->HasAura(49772);
}

//By leewheel 2026-07-30: 三个独立姿态缺失 trigger
//判定: 角色没有该 buff 且同时也没有其他姿态 (避免在其他姿态中也重复激活)
//By leewheel 2026-08-06: 改用spell ID检查,防止locale匹配失败
//By leewheel 2026-08-07: 补全天赋变体ID
bool NoBloodPresenceTrigger::IsActive()
{
    return BuffTrigger::IsActive() && !bot->HasAura(48266) && !bot->HasAura(50475) &&
           !bot->HasAura(48265) && !bot->HasAura(49772);
}

bool NoFrostPresenceTrigger::IsActive()
{
    return BuffTrigger::IsActive() && !bot->HasAura(48263) && !bot->HasAura(61261) &&
           !bot->HasAura(48265) && !bot->HasAura(49772);
}

bool NoUnholyPresenceTrigger::IsActive()
{
    return BuffTrigger::IsActive() && !bot->HasAura(48263) && !bot->HasAura(61261) &&
           !bot->HasAura(48266) && !bot->HasAura(50475);
}
//End By leewheel

bool PestilenceGlyphTrigger::IsActive()
{
    if (!SpellTrigger::IsActive())
    {
        return false;
    }
    if (!bot->HasAura(63334))
    {
        return false;
    }
    Aura* blood_plague = botAI->GetAura("blood plague", GetTarget(), true, true);
    Aura* frost_fever = botAI->GetAura("frost fever", GetTarget(), true, true);
    if ((blood_plague && blood_plague->GetDuration() <= 3000) || (frost_fever && frost_fever->GetDuration() <= 3000))
    {
        return true;
    }
    return false;
}

// Based on runeSlotTypes
bool HighBloodRuneTrigger::IsActive()
{
    return bot->GetRuneCooldown(0) <= 2000 && bot->GetRuneCooldown(1) <= 2000;
}

bool HighFrostRuneTrigger::IsActive()
{
    return bot->GetRuneCooldown(4) <= 2000 && bot->GetRuneCooldown(5) <= 2000;
}

bool HighUnholyRuneTrigger::IsActive()
{
    return bot->GetRuneCooldown(2) <= 2000 && bot->GetRuneCooldown(3) <= 2000;
}

bool NoRuneTrigger::IsActive()
{
    for (uint32 i = 0; i < MAX_RUNES; ++i)
    {
        if (!bot->GetRuneCooldown(i))
            return false;
    }
    return true;
}

bool DesolationTrigger::IsActive()
{
    return bot->HasAura(66817) && BuffTrigger::IsActive();
}

bool DeathAndDecayCooldownTrigger::IsActive()
{
    uint32 spellId = AI_VALUE2(uint32, "spell id", name);
    if (!spellId)
        return true;

    //By leewheel 2026-07-10: TC使用GetSpellHistory()->GetRemainingCooldown替代GetSpellCooldownDelay
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);
    if (!spellInfo)
        return true;

    return bot->GetSpellHistory()->GetRemainingCooldown(spellInfo).count() >= 2000;
    //End By leewheel
}
