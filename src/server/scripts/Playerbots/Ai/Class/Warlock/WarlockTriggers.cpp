/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "WarlockTriggers.h"
#include "GenericTriggers.h"
#include "Playerbots.h"
#include "PlayerbotAI.h"
#include "Player.h"

//By leewheel 2026-08-06: Warlock buff spell ID数组,防止locale匹配失败导致重复加buff
//By leewheel 2026-08-07: 通过 classic_db2.spellname 复核修正, 原数组含大量错误ID(13298-13302/54085/19029-19030/25211-25215/47193-47194 都不是对应法术)
namespace {
// Demon Skin + Demon Armor + Fel Armor 全等级 (依据 classic_db2.spellname 实际查得)
constexpr uint32 DEMON_ARMOR_SPELL_IDS[] = {
    // Demon Skin rank 1-2 + TBC 联盟专属版本
    687, 696, 20798,
    // Demon Armor 全等级(含 NPC/物品触发版本, 加上无害)
    706, 1086, 11733, 11734, 11735, 12956, 13787, 27260, 34881, 47793, 47889,
    // Fel Armor 全等级
    28176, 28189, 44520, 44977, 47892, 47893
};
// Soul Link 全等级 (3.3.5a 仅 rank1=19028, rank2=25228; 原数组其余ID全错)
constexpr uint32 SOUL_LINK_SPELL_IDS[] = { 19028, 25228 };

// By leewheel 2026-08-07: Seed of Corruption 全等级 (用于 CorruptionTrigger 检查是否已有腐蚀之种, 防止重复施放腐蚀术)
constexpr uint32 SEED_OF_CORRUPTION_SPELL_IDS[] = {
    27243, 27285, 32863, 32865, 36123, 38252, 39367, 43991, 44141, 47831, 47832, 47833, 47834, 47835, 47836
};

bool HasAnyAuraFromList(Unit* unit, const uint32* spellIds, size_t count)
{
    if (!unit) return false;
    for (size_t i = 0; i < count; ++i)
        if (unit->HasAura(spellIds[i])) return true;
    return false;
}
} // namespace
//End By leewheel

static const uint32 SOUL_SHARD_ITEM_ID = 6265;

uint32 GetSoulShardCount(Player* bot)
{
    return bot->GetItemCount(SOUL_SHARD_ITEM_ID, false);  // false = only bags
}

// List of all Soulstone item IDs
static const std::vector<uint32> soulstoneItemIds = {
    5232,   // Minor Soulstone
    16892,  // Lesser Soulstone
    16893,  // Soulstone
    16895,  // Greater Soulstone
    16896,  // Major Soulstone
    22116,  // Master Soulstone
    36895   // Demonic Soulstone
};

uint32 GetSoulstoneCount(Player* bot)
{
    uint32 count = 0;
    for (uint32 id : soulstoneItemIds)
        count += bot->GetItemCount(id, false);  // false = only bags
    return count;
}

bool SpellstoneTrigger::IsActive() { return BuffTrigger::IsActive() && AI_VALUE2(uint32, "item count", getName()) > 0; }

bool FirestoneTrigger::IsActive() { return BuffTrigger::IsActive() && AI_VALUE2(uint32, "item count", getName()) > 0; }

bool WarlockConjuredItemTrigger::IsActive()
{
    return ItemCountTrigger::IsActive() && AI_VALUE2(uint32, "item count", "soul shard") > 0;
}

bool OutOfSoulShardsTrigger::IsActive() { return GetSoulShardCount(botAI->GetBot()) == 0; }

bool TooManySoulShardsTrigger::IsActive() { return GetSoulShardCount(botAI->GetBot()) >= 26; }

bool OutOfSoulstoneTrigger::IsActive() { return GetSoulstoneCount(botAI->GetBot()) == 0; }

//By leewheel 2026-08-06: 添加spell ID fallback
bool DemonArmorTrigger::IsActive()
{
    Unit* target = GetTarget();
    // 名称检查
    if (botAI->HasAura("demon skin", target) || botAI->HasAura("demon armor", target) ||
        botAI->HasAura("fel armor", target))
        return false;
    // spell ID fallback
    return !HasAnyAuraFromList(target, DEMON_ARMOR_SPELL_IDS, std::size(DEMON_ARMOR_SPELL_IDS));
}

bool SoulLinkTrigger::IsActive()
{
    Unit* target = GetTarget();
    // 名称检查
    if (botAI->HasAura("soul link", target))
        return false;
    // spell ID fallback
    return !HasAnyAuraFromList(target, SOUL_LINK_SPELL_IDS, std::size(SOUL_LINK_SPELL_IDS));
}
//End By leewheel

// By leewheel 2026-08-07: 检查 Seed of Corruption aura 的 spell ID fallback
// 用于 CorruptionTrigger/CorruptionOnAttackerTrigger, 防止 locale 匹配失败导致重复施放腐蚀术
static bool HasSeedOfCorruption(PlayerbotAI* ai, Unit* target)
{
    if (ai->HasAura("seed of corruption", target, false, true))
        return true;
    return HasAnyAuraFromList(target, SEED_OF_CORRUPTION_SPELL_IDS, std::size(SEED_OF_CORRUPTION_SPELL_IDS));
}

// By leewheel 2026-08-07: CorruptionTrigger IsActive 实现, 使用 spell ID fallback 检查 Seed of Corruption
bool CorruptionTrigger::IsActive()
{
    return BuffTrigger::IsActive() && !HasSeedOfCorruption(botAI, GetTarget());
}

// By leewheel 2026-08-07: CorruptionOnAttackerTrigger IsActive 实现, 使用 spell ID fallback 检查 Seed of Corruption
bool CorruptionOnAttackerTrigger::IsActive()
{
    return BuffTrigger::IsActive() && !HasSeedOfCorruption(botAI, GetTarget());
}

bool UnendingBreathTrigger::IsActive()
{
    return BuffTrigger::IsActive() && AI_VALUE2(bool, "swimming", "self target");
}

bool UnendingBreathOnPartyTrigger::IsActive()
{
    return BuffOnPartyTrigger::IsActive() && AI_VALUE2(bool, "swimming", "self target");
}

bool DemonicEmpowermentTrigger::IsActive()
{
    Pet* pet = bot->GetPet();
    if (!pet)
        return false;
    return !botAI->HasAura("demonic empowerment", pet);
}

// By leewheel 2026-08-07: 添加 spell ID fallback, 防止 locale 匹配失败导致 Decimation proc 永远不触发
bool DecimationTrigger::IsActive()
{
    Aura* aura = botAI->GetAura(getName(), GetTarget(), false, true);
    if (!aura)
    {
        // 名称匹配失败时, 用 spell ID fallback 查找
        for (uint32 spellId : GetFallbackSpellIds())
        {
            aura = GetTarget() ? GetTarget()->GetAura(spellId) : nullptr;
            if (aura) break;
        }
    }
    return aura && aura->GetDuration() > 3000;
}

// Checks if the bot's mana is below 85% and health is above a low health threshold
bool LifeTapTrigger::IsActive()
{
    if (AI_VALUE2(uint8, "health", "self target") <= sPlayerbotAIConfig.lowHealth)
        return false;

    if (!AI_VALUE2(bool, "has mana", "self target"))
        return false;
    if (AI_VALUE2(uint8, "mana", "self target") >= 85)
        return false;

    return true;
}

// Checks if the Life Tap Glyph buff is active
bool LifeTapGlyphBuffTrigger::IsActive()
{
    constexpr uint32 SPELL_LIFE_TAP_GLYPH = 63320;
    return bot->HasAura(SPELL_LIFE_TAP_GLYPH) && BuffTrigger::IsActive();
}

// Checks if the target has a conflicting debuff that is equal to Curse of the Elements
bool CurseOfTheElementsTrigger::IsActive()
{
    Unit* target = GetTarget();
    if (!target || !target->IsAlive() || !target->IsInWorld())
        return false;

    // List of all spell IDs for Ebon Plague, Earth and Moon, and Curse of the Elements
    static const uint32 CurseOfTheElementsExclusiveDebuffs[] = {// Ebon Plague
                                                                51735, 51734, 51726,
                                                                // Earth and Moon
                                                                48511, 48513, 48514,
                                                                // Curse of the Elements
                                                                1490, 11721, 11722, 27228, 47865};

    // Check if target has any of the exclusive debuffs
    for (uint32 spellId : CurseOfTheElementsExclusiveDebuffs)
    {
        if (target->HasAura(spellId))
            return false;
    }

    // Use default BuffTrigger logic for the rest (only trigger if debuff is missing or expiring)
    return BuffTrigger::IsActive();
}

// Checks if the target has a conflicting debuff that is equal to Curse of Weakness
bool CurseOfWeaknessTrigger::IsActive()
{
    Unit* target = GetTarget();
    if (!target || !target->IsAlive() || !target->IsInWorld())
        return false;
    // List of all spell IDs for Curse of Weakness, Demoralizing Roar, Demoralizing Shout, and Vindication
    static const uint32 CurseOfWeaknessExclusiveDebuffs[] = {// Curse of Weakness
                                                             702, 1108, 6205, 7646, 11707, 11708, 27224, 30909, 50511,
                                                             // Demoralizing Roar
                                                             99, 1735, 9490, 9747, 9898, 26998, 48559, 48560,
                                                             // Demoralizing Shout
                                                             1160, 6190, 11554, 11555, 11556, 25202, 25203, 47437,
                                                             // Vindication
                                                             67, 26017};

    // Check if target has any of the exclusive debuffs
    for (uint32 spellId : CurseOfWeaknessExclusiveDebuffs)
    {
        if (target->HasAura(spellId))
            return false;
    }
    // Use default BuffTrigger logic for the rest (only trigger if debuff is missing or expiring)
    return BuffTrigger::IsActive();
}

struct WarlockPetDef
{
    const char* strategy;  // The strategy string as recognized by the AI (e.g., "imp", "voidwalker", etc.)
    uint32 spellId;        // The spell ID required to summon this pet
    uint32 npcEntry;       // The NPC entry ID for the summoned pet creature
};

// Static array with all relevant Warlock pets and their data
static const WarlockPetDef pets[] = {{"imp", 688, 416},
                                     {"voidwalker", 697, 1860},
                                     {"succubus", 712, 1863},
                                     {"felhunter", 691, 417},
                                     {"felguard", 30146, 17252}};

bool WrongPetTrigger::IsActive()
{
    // Retrieve the bot player and its current pet (if any)
    Player* bot = botAI->GetBot();
    Pet* pet = bot->GetPet();

    // Step 1: Count how many pet strategies are currently enabled for this bot.
    //         While doing so, also remember which pet strategy is the only enabled one (if that's the case).
    int enabledCount = 0;
    const WarlockPetDef* enabledPet =
        nullptr;  // Pointer to the pet definition of the enabled strategy, if only one is enabled
    for (const WarlockPetDef& pd : pets)
    {
        if (botAI->HasStrategy(pd.strategy, BOT_STATE_NON_COMBAT))
        {
            enabledCount++;
            enabledPet = &pd;  // Save the pointer to last enabled pet
        }
    }

    // Step 2: If not exactly one pet strategy is enabled, we should not trigger.
    //         This prevents ambiguous or conflicting situations.
    if (enabledCount != 1)
        return false;

    // Step 3: If there is no pet, do not trigger.
    if (!pet)
        return false;

    // Step 4: At this point, we know only one pet strategy is enabled.
    //         We check if the currently summoned pet matches the enabled strategy.
    bool correctPet = false;
    if (pet)
    {
        CreatureTemplate const* ct = pet->GetCreatureTemplate();
        // Check if the pet's NPC entry matches the expected one for the enabled strategy
        if (ct && ct->Entry == enabledPet->npcEntry)
            correctPet = true;
    }

    // Step 5: If the correct pet is already summoned, the trigger should not activate.
    if (correctPet)
        return false;

    // Step 6: Finally, check if the bot actually knows the spell to summon the desired pet.
    //         If so, the trigger is active (bot should summon the correct pet).
    if (bot->HasSpell(enabledPet->spellId))
        return true;

    // Step 7: If we get here, the bot doesn't know the spell required to support the active pet strategy
    return false;
}

const std::set<uint32> RainOfFireChannelCheckTrigger::RAIN_OF_FIRE_SPELL_IDS = {
    5740,   // Rain of Fire Rank 1
    6219,   // Rain of Fire Rank 2
    11677,  // Rain of Fire Rank 3
    11678,  // Rain of Fire Rank 4
    27212,  // Rain of Fire Rank 5
    47819,  // Rain of Fire Rank 6
    47820   // Rain of Fire Rank 7
};

bool RainOfFireChannelCheckTrigger::IsActive()
{
    Player* bot = botAI->GetBot();

    // Check if the bot is channeling a spell
    if (Spell* spell = bot->GetCurrentSpell(CURRENT_CHANNELED_SPELL))
    {
        // Only trigger if the spell being channeled is Rain of Fire
        if (RAIN_OF_FIRE_SPELL_IDS.count(spell->m_spellInfo->Id))
        {
            uint8 attackerCount = AI_VALUE(uint8, "attacker count");
            return attackerCount < minEnemies;
        }
    }

    // Not channeling Rain of Fire
    return false;
}
