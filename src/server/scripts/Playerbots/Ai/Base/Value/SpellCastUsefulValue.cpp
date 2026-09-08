/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "SpellCastUsefulValue.h"

#include "LastSpellCastValue.h"
#include "Playerbots.h"

bool SpellCastUsefulValue::Calculate()
{
    uint32 spellid = AI_VALUE2(uint32, "spell id", qualifier);
    if (!spellid)
        return true;  // there can be known alternatives

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellid);
    if (!spellInfo)
        return true;  // there can be known alternatives

    if ((spellInfo->Attributes & SPELL_ATTR0_ON_NEXT_SWING_NO_DAMAGE) != 0 ||
        (spellInfo->Attributes & SPELL_ATTR0_ON_NEXT_SWING) != 0)
    {
        if (Spell* spell = bot->GetCurrentSpell(CURRENT_MELEE_SPELL))
            if (spell->m_spellInfo->Id == spellid && spell->IsNextMeleeSwingSpell() &&
                bot->HasUnitState(UNIT_STATE_MELEE_ATTACKING))
                return false;
    }
    else
    {
        // uint32 lastSpellId = AI_VALUE(LastSpellCast&, "last spell cast").id;
        // if (spellid == lastSpellId)
        //     if (Spell* const pSpell = bot->FindCurrentSpellBySpellId(lastSpellId))
        //         return false;
    }

    if (spellInfo->IsAutoRepeatRangedSpell() && bot->GetCurrentSpell(CURRENT_AUTOREPEAT_SPELL) &&
        bot->GetCurrentSpell(CURRENT_AUTOREPEAT_SPELL)->m_spellInfo->Id == spellid)
    {
        return false;
    }

    if (qualifier == "windfury weapon" || qualifier == "flametongue weapon" ||
        qualifier == "frostbrand weapon" ||  qualifier == "rockbiter weapon" ||
        qualifier == "earthliving weapon" || qualifier == "spellstone")
    {
        if (Item* item = AI_VALUE2(Item*, "item for spell", spellid);
            item && item->IsInWorld() && item->GetEnchantmentId(TEMP_ENCHANTMENT_SLOT))
            return false;
    }

    std::set<uint32>& skipSpells = AI_VALUE(std::set<uint32>&, "skip spells list");
    if (skipSpells.find(spellid) != skipSpells.end())
        return false;

    //By leewheel 2026-07-14: 使用带spellnameeng缓存的辅助函数获取spell名称
    if (!spellInfo->SpellName)
        return true;
    std::string const spellName = GetSpellNameBestLocaleWithCache(spellid, spellInfo->SpellName);
    //End By leewheel
    for (uint32 skipSpellId : skipSpells)
    {
        SpellInfo const* skipSpellInfo = sSpellMgr->GetSpellInfo(skipSpellId);
        if (!skipSpellInfo)
            continue;

        //By leewheel 2026-07-14: 使用带spellnameeng缓存的匹配
        if (SpellNameMatchesWithCache(skipSpellId, skipSpellInfo->SpellName, spellName))
            return false;
        //End By leewheel
    }

    return true;
}
