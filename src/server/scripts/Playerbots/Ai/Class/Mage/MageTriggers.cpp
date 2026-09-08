/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "MageTriggers.h"
#include "Playerbots.h"
#include "Player.h"
#include "Spell.h"
#include "DynamicObject.h"
#include "Value.h"
#include "SpellAuraEffects.h"
#include "ServerFacade.h"

//By leewheel 2026-08-06: Mage buff spell ID数组,防止locale匹配失败导致重复加buff
//By leewheel 2026-08-07: 通过 classic_db2.spellname + spelllevels 复核修正, 原数组 27127/43008/43009 都是错误ID, 且多个数组漏 rank
namespace {
// Arcane Intellect 全等级 + Arcane Brilliance 群体版 (依据 classic_db2 实际查得)
constexpr uint32 ARCANE_INTELLECT_SPELL_IDS[] = {
    // Arcane Intellect 玩家可学全等级
    1459, 1460, 1461, 10156, 10157, 13326, 16876, 27126, 36880, 39235, 42995, 45525,
    // Arcane Brilliance 群体版
    23028, 27127, 43002
};
// Mage Armor 全等级 (原 43008 是 Ice Armor 不是 Mage Armor)
constexpr uint32 MAGE_ARMOR_SPELL_IDS[] = { 6117, 22782, 22783, 27125, 43023, 43024 };
// Ice Armor 全等级 (原 43009 不是 Ice Armor; 漏 10219/36881/43008)
constexpr uint32 ICE_ARMOR_SPELL_IDS[] = { 7302, 7320, 10219, 10220, 10221, 27124, 36881, 43008 };
// Frost Armor 全等级 (低级版本, 玩家 1-20 级用)
constexpr uint32 FROST_ARMOR_SPELL_IDS[] = { 168, 7300, 7301, 12544, 12556, 15784, 18100, 31256 };
// Molten Armor 全等级
constexpr uint32 MOLTEN_ARMOR_SPELL_IDS[] = { 30482, 34913, 35915, 35916, 43043, 43044, 43045, 43046 };

bool HasAnyAuraFromList(Unit* unit, const uint32* spellIds, size_t count)
{
    if (!unit) return false;
    for (size_t i = 0; i < count; ++i)
        if (unit->HasAura(spellIds[i])) return true;
    return false;
}
} // namespace
//End By leewheel

bool NoManaGemTrigger::IsActive()
{
    static const std::vector<uint32> gemIds = {
        33312,  // Mana Sapphire
        22044,  // Mana Emerald
        8008,   // Mana Ruby
        8007,   // Mana Citrine
        5513,   // Mana Jade
        5514    // Mana Agate
    };

    for (uint32 gemId : gemIds)
    {
        if (bot->GetItemCount(gemId, false) > 0)  // false = only in bags
            return false;
    }
    return true;
}

//By leewheel 2026-08-06: 添加spell ID fallback
bool ArcaneIntellectTrigger::IsActive()
{
    if (!BuffTrigger::IsActive())
        return false;
    Unit* target = GetTarget();
    // 名称检查群体版
    if (botAI->HasAura("arcane brilliance", target))
        return false;
    // spell ID fallback
    if (HasAnyAuraFromList(target, ARCANE_INTELLECT_SPELL_IDS, std::size(ARCANE_INTELLECT_SPELL_IDS)))
        return false;
    return true;
}

bool MageArmorTrigger::IsActive()
{
    Unit* target = GetTarget();
    if (!botAI->HasSpell("mage armor"))
        return false;
    // 名称检查
    if (botAI->HasAura("mage armor", target) || botAI->HasAura("ice armor", target) ||
        botAI->HasAura("frost armor", target) || botAI->HasAura("molten armor", target))
        return false;
    // spell ID fallback
    if (HasAnyAuraFromList(target, MAGE_ARMOR_SPELL_IDS, std::size(MAGE_ARMOR_SPELL_IDS)) ||
        HasAnyAuraFromList(target, ICE_ARMOR_SPELL_IDS, std::size(ICE_ARMOR_SPELL_IDS)) ||
        HasAnyAuraFromList(target, FROST_ARMOR_SPELL_IDS, std::size(FROST_ARMOR_SPELL_IDS)) ||
        HasAnyAuraFromList(target, MOLTEN_ARMOR_SPELL_IDS, std::size(MOLTEN_ARMOR_SPELL_IDS)))
        return false;
    return true;
}

bool MoltenArmorTrigger::IsActive()
{
    Unit* target = GetTarget();
    if (!botAI->HasSpell("molten armor"))
        return false;
    // 名称检查
    if (botAI->HasAura("molten armor", target) || botAI->HasAura("ice armor", target) ||
        botAI->HasAura("frost armor", target) || botAI->HasAura("mage armor", target))
        return false;
    // spell ID fallback
    if (HasAnyAuraFromList(target, MOLTEN_ARMOR_SPELL_IDS, std::size(MOLTEN_ARMOR_SPELL_IDS)) ||
        HasAnyAuraFromList(target, ICE_ARMOR_SPELL_IDS, std::size(ICE_ARMOR_SPELL_IDS)) ||
        HasAnyAuraFromList(target, FROST_ARMOR_SPELL_IDS, std::size(FROST_ARMOR_SPELL_IDS)) ||
        HasAnyAuraFromList(target, MAGE_ARMOR_SPELL_IDS, std::size(MAGE_ARMOR_SPELL_IDS)))
        return false;
    return true;
}
//End By leewheel

bool FrostNovaOnTargetTrigger::IsActive()
{
    Unit* target = GetTarget();
    if (!target || !target->IsAlive() || !target->IsInWorld())
        return false;

    return botAI->HasAura(spell, target);
}

bool FrostbiteOnTargetTrigger::IsActive()
{
    Unit* target = GetTarget();
    if (!target || !target->IsAlive() || !target->IsInWorld())
        return false;

    return botAI->HasAura(spell, target);
}

bool NoFocusMagicTrigger::IsActive()
{
    constexpr uint32 SPELL_FOCUS_MAGIC = 54646;
    if (!bot->HasSpell(SPELL_FOCUS_MAGIC))
        return false;

    Group* group = bot->GetGroup();
    if (!group)
        return false;

    for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
    {
        Player* member = ref->GetSource();
        if (!member || member == bot || !member->IsAlive())
            continue;

        if (member->HasAura(SPELL_FOCUS_MAGIC, bot->GetGUID()))
            return false;
    }
    return true;
}

bool DeepFreezeCooldownTrigger::IsActive()
{
    constexpr uint32 SPELL_DEEP_FREEZE = 44572;
    return !bot->HasSpell(SPELL_DEEP_FREEZE) ||
           SpellCooldownTrigger::IsActive();
}

const std::unordered_set<uint32> FlamestrikeNearbyTrigger::FLAMESTRIKE_SPELL_IDS = {
    2120, 2121, 8422, 8423, 10215, 10216, 27086, 42925, 42926
};

bool FlamestrikeNearbyTrigger::IsActive()
{
    for (uint32 spellId : FLAMESTRIKE_SPELL_IDS)
    {
        Aura* aura = bot->GetAura(spellId, bot->GetGUID());
        if (!aura)
            continue;

        DynamicObject* dynObj = aura->GetDynobjOwner();
        if (!dynObj)
            continue;

        float dist = bot->GetDistance2d(dynObj->GetPositionX(), dynObj->GetPositionY());
        if (dist <= radius)
            return true;
    }
    return false;
}

bool ImprovedScorchTrigger::IsActive()
{
    Unit* target = GetTarget();
    if (!target || !target->IsAlive() || !target->IsInWorld())
        return false;

    static const uint32 ImprovedScorchExclusiveDebuffs[] = {// Shadow Mastery
                                                            17794, 17797, 17798, 17799, 17800,
                                                            // Winter's Chill
                                                            12579,
                                                            // Improved Scorch
                                                            22959};

    for (uint32 spellId : ImprovedScorchExclusiveDebuffs)
    {
        if (target->HasAura(spellId))
            return false;
    }

    return DebuffTrigger::IsActive();
}

const std::unordered_set<uint32> BlizzardChannelCheckTrigger::BLIZZARD_SPELL_IDS = {
    10,     // Blizzard Rank 1
    6141,   // Blizzard Rank 2
    8427,   // Blizzard Rank 3
    10185,  // Blizzard Rank 4
    10186,  // Blizzard Rank 5
    10187,  // Blizzard Rank 6
    27085,  // Blizzard Rank 7
    42938,  // Blizzard Rank 8
    42939   // Blizzard Rank 9
};

bool BlizzardChannelCheckTrigger::IsActive()
{
    if (Spell* spell = bot->GetCurrentSpell(CURRENT_CHANNELED_SPELL);
        spell && BLIZZARD_SPELL_IDS.count(spell->m_spellInfo->Id))
    {
        uint8 attackerCount = AI_VALUE(uint8, "attacker count");
        return attackerCount < minEnemies;
    }

    return false;
}
