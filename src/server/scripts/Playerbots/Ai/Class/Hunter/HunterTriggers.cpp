/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "HunterTriggers.h"
#include "GenericSpellActions.h"
#include "GenericTriggers.h"
#include "HunterActions.h"
#include "PlayerbotAI.h"
#include "PlayerbotAIConfig.h"
#include "Playerbots.h"
#include "ServerFacade.h"
#include "SharedDefines.h"
#include "Player.h"

// By leewheel 2026-08-07: Hunter Sting 系列 spell ID (依据 classic_db2.spelllevels 玩家可学版本)
namespace {
constexpr uint32 ASPECT_OF_HAWK_SPELL_IDS[] = { 13165, 14318, 14319, 14320, 14321, 14322, 25296, 27044 };
constexpr uint32 ASPECT_OF_DRAGONHAWK_SPELL_IDS[] = { 61846, 61847, 61848 };
constexpr uint32 ASPECT_OF_VIPER_SPELL_IDS[] = { 34074, 34075 };
constexpr uint32 ASPECT_OF_CHEETAH_SPELL_ID = 5118;

// Serpent Sting 全等级
constexpr uint32 SERPENT_STING_SPELL_IDS[] = {
    1978, 13549, 13550, 13551, 13552, 13553, 13554, 13555, 25295, 27016, 36984, 49000, 49001
};
// Scorpid Sting 全等级
constexpr uint32 SCORPID_STING_SPELL_IDS[] = { 3043, 14325, 52604 };
// Viper Sting 全等级
constexpr uint32 VIPER_STING_SPELL_IDS[] = { 3034, 31407, 37551, 39413 };

bool HasAuraFromList(Unit* unit, const uint32* spellIds, size_t count)
{
    if (!unit) return false;
    for (size_t i = 0; i < count; ++i)
        if (unit->HasAura(spellIds[i])) return true;
    return false;
}
} // namespace

bool KillCommandTrigger::IsActive()
{
    return !botAI->HasAura("kill command", GetTarget());
}

bool BlackArrowTrigger::IsActive()
{
    if (botAI->HasStrategy("trap weave", BOT_STATE_COMBAT))
        return false;

    return DebuffTrigger::IsActive();
}

bool HunterAspectOfTheDragonhawkTrigger::IsActive()
{
    Unit* target = GetTarget();
    if (!target)
        return false;

    if (!SpellTrigger::IsActive())
        return false;

    // By leewheel 2026-08-07: 名称检查 + spell ID fallback, 防止 locale 匹配失败导致重复切守护
    bool hasHawkOrDragonhawk = botAI->HasAura("aspect of the hawk", target) ||
                               botAI->HasAura("aspect of the dragonhawk", target);
    if (!hasHawkOrDragonhawk)
    {
        // 名称匹配失败, 用 spell ID 确认
        hasHawkOrDragonhawk = HasAuraFromList(target, ASPECT_OF_HAWK_SPELL_IDS, std::size(ASPECT_OF_HAWK_SPELL_IDS)) ||
                              HasAuraFromList(target, ASPECT_OF_DRAGONHAWK_SPELL_IDS, std::size(ASPECT_OF_DRAGONHAWK_SPELL_IDS));
    }
    if (hasHawkOrDragonhawk)
        return false;

    bool hasViper = botAI->HasAura("aspect of the viper", target);
    if (!hasViper)
        hasViper = HasAuraFromList(target, ASPECT_OF_VIPER_SPELL_IDS, std::size(ASPECT_OF_VIPER_SPELL_IDS));
    if (hasViper)
        return AI_VALUE2(uint8, "mana", "self target") >= 60;

    return true;
}

bool HunterNoStingsActiveTrigger::IsActive()
{
    Unit* target = AI_VALUE(Unit*, "current target");
    if (!target)
        return false;
    // By leewheel 2026-08-07: 名称检查 + spell ID fallback, 防止 locale 匹配失败导致重复施放钉刺
    bool hasSerpent = botAI->HasAura("serpent sting", target, false, true);
    if (!hasSerpent)
        hasSerpent = HasAuraFromList(target, SERPENT_STING_SPELL_IDS, std::size(SERPENT_STING_SPELL_IDS));
    bool hasScorpid = botAI->HasAura("scorpid sting", target, false, true);
    if (!hasScorpid)
        hasScorpid = HasAuraFromList(target, SCORPID_STING_SPELL_IDS, std::size(SCORPID_STING_SPELL_IDS));
    bool hasViper = botAI->HasAura("viper sting", target, false, true);
    if (!hasViper)
        hasViper = HasAuraFromList(target, VIPER_STING_SPELL_IDS, std::size(VIPER_STING_SPELL_IDS));
    return DebuffTrigger::IsActive() && target && !hasSerpent && !hasScorpid && !hasViper;
}

bool HuntersPetDeadTrigger::IsActive()
{
    return AI_VALUE(bool, "pet dead") && !AI_VALUE2(bool, "mounted", "self target");
}

bool HuntersPetLowHealthTrigger::IsActive()
{
    Unit* pet = AI_VALUE(Unit*, "pet target");
    return pet && AI_VALUE2(uint8, "health", "pet target") < 40 &&
           !AI_VALUE2(bool, "dead", "pet target") &&
           !AI_VALUE2(bool, "mounted", "self target");
}

bool HuntersPetMediumHealthTrigger::IsActive()
{
    Unit* pet = AI_VALUE(Unit*, "pet target");
    return pet && AI_VALUE2(uint8, "health", "pet target") < sPlayerbotAIConfig.mediumHealth &&
           !AI_VALUE2(bool, "dead", "pet target") && !AI_VALUE2(bool, "mounted", "self target");
}

bool HunterPetNotHappy::IsActive()
{
    return !AI_VALUE(bool, "pet happy") && !AI_VALUE2(bool, "mounted", "self target");
}

bool HunterAspectOfTheViperTrigger::IsActive()
{
    if (botAI->HasStrategy("rnature", BotState::BOT_STATE_COMBAT) ||
        botAI->HasStrategy("rnature", BotState::BOT_STATE_NON_COMBAT) ||
        botAI->HasStrategy("bspeed", BotState::BOT_STATE_COMBAT) ||
        botAI->HasStrategy("bspeed", BotState::BOT_STATE_NON_COMBAT))
        return false;

    return BuffTrigger::IsActive() &&
           AI_VALUE2(uint8, "mana", "self target") < (sPlayerbotAIConfig.lowMana / 2);
}

bool HunterAspectOfThePackTrigger::IsActive()
{
    // By leewheel 2026-08-07: 添加 spell ID fallback, 防止 locale 匹配失败导致重复切豹群
    Unit* target = GetTarget();
    if (!target)
        return false;
    bool hasCheetah = botAI->HasAura("aspect of the cheetah", target);
    if (!hasCheetah)
        hasCheetah = target->HasAura(ASPECT_OF_CHEETAH_SPELL_ID);
    return BuffTrigger::IsActive() && !hasCheetah;
};

bool HunterLowAmmoTrigger::IsActive()
{
    uint32 ammoCount = AI_VALUE2(uint32, "item count", "ammo");
    return bot->GetGroup() && ammoCount > 0 && ammoCount < 100;
}

bool HunterHasAmmoTrigger::IsActive()
{
    return !AmmoCountTrigger::IsActive();
}

bool SwitchToRangedTrigger::IsActive()
{
    Unit* target = AI_VALUE(Unit*, "current target");
    return botAI->HasStrategy("close", BOT_STATE_COMBAT) && target &&
           (target->GetVictim() != bot &&
            ServerFacade::instance().IsDistanceGreaterThan(AI_VALUE2(float, "distance", "current target"), 8.0f));
}

bool SwitchToMeleeTrigger::IsActive()
{
    Unit* target = AI_VALUE(Unit*, "current target");
    return botAI->HasStrategy("ranged", BOT_STATE_COMBAT) && target &&
           (target->GetVictim() == bot &&
            ServerFacade::instance().IsDistanceLessOrEqualThan(AI_VALUE2(float, "distance", "current target"), 8.0f));
}

// Valid targets for "Improved Tracking".
// Optional/Utility targets (uncomment for selfbot).
bool NoTrackTrigger::IsActive()
{
    std::vector<std::string> track_list = {
        "track beasts",
        "track demons",
        "track dragonkin",
        "track elementals",
        "track giants",
        "track humanoids",
        "track undead",
        // "track hidden",
        // "find herbs",
        // "find minerals",
        // "find fish",
        // "find treasure",
    };

    for (auto &track: track_list)
    {
        if (botAI->HasAura(track, bot))
            return false;
    }

    return true;
}

bool SerpentStingOnAttackerTrigger::IsActive()
{
    if (!DebuffOnAttackerTrigger::IsActive())
        return false;

    Unit* target = GetTarget();
    if (!target)
        return false;

    return !botAI->HasAura("scorpid sting", target, false, true) &&
           !botAI->HasAura("viper sting", target, false, true);
}

const std::set<uint32> VolleyChannelCheckTrigger::VOLLEY_SPELL_IDS =
{
    1510,   // Volley Rank 1
    14294,  // Volley Rank 2
    14295,  // Volley Rank 3
    27022,  // Volley Rank 4
    58431,  // Volley Rank 5
    58434   // Volley Rank 6
};

bool VolleyChannelCheckTrigger::IsActive()
{
    if (Spell* spell = bot->GetCurrentSpell(CURRENT_CHANNELED_SPELL);
        spell && VOLLEY_SPELL_IDS.count(spell->m_spellInfo->Id))
    {
        uint8 attackerCount = AI_VALUE(uint8, "attacker count");
        return attackerCount < minEnemies;
    }

    return false;
}
