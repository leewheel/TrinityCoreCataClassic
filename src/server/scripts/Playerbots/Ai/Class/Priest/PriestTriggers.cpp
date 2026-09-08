/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "PriestTriggers.h"
#include "PlayerbotAI.h"
#include "Player.h"
#include "Playerbots.h"

//By leewheel 2026-08-06: Priest buff spell ID数组,防止locale匹配失败导致重复加buff
//By leewheel 2026-08-07: 通过 classic_db2.spellname 复核修正, 原数组漏了多个高等级rank, 且 25333/33000/25331 都是错误ID
namespace {
// Power Word: Fortitude 全等级 + Prayer of Fortitude 群体版 (依据 classic_db2.spellname 实际查得)
constexpr uint32 FORTITUDE_SPELL_IDS[] = {
    // Power Word: Fortitude rank 1-13
    1243, 1244, 1245, 2791, 10937, 10938, 13864, 23947, 23948, 25389, 36004, 48161, 58921,
    // Prayer of Fortitude rank 1-6
    21562, 21564, 25392, 39231, 43939, 48162
};
// Divine Spirit 全等级 + Prayer of Spirit 群体版
constexpr uint32 DIVINE_SPIRIT_SPELL_IDS[] = {
    // Divine Spirit rank 1-8
    14752, 14818, 14819, 16875, 25312, 27841, 39234, 48073,
    // Prayer of Spirit rank 1-3
    27681, 32999, 48074
};
// Shadow Protection 全等级 + Prayer of Shadow Protection 群体版
// 注意: 原 25333 是错误ID, 正确为 25433; 原 33000 不是 Prayer of Shadow Protection
constexpr uint32 SHADOW_PROTECTION_SPELL_IDS[] = {
    // Shadow Protection rank 1-5 + NPC 版本
    976, 7235, 7241, 7242, 7243, 7244, 10957, 10958, 16874, 16891, 17548, 25433, 28537, 48169, 53915,
    // Prayer of Shadow Protection rank 1-4
    27683, 39236, 39374, 48170
};
// Inner Fire 全等级
// 注意: 原 25331 是错误ID, 正确为 25431; 漏了 48040
constexpr uint32 INNER_FIRE_SPELL_IDS[] = {
    588, 602, 1006, 7128, 10951, 10952, 25431, 48040, 48168
};
// Shadowform (单等级, 含 NPC 版本)
constexpr uint32 SHADOWFORM_SPELL_ID = 15473;

bool HasAnyAuraFromList(Unit* unit, const uint32* spellIds, size_t count)
{
    if (!unit) return false;
    for (size_t i = 0; i < count; ++i)
        if (unit->HasAura(spellIds[i])) return true;
    return false;
}
} // namespace
//End By leewheel

//By leewheel 2026-08-06: 所有buff trigger添加spell ID fallback
bool ShadowProtectionTrigger::IsActive()
{
    if (!BuffTrigger::IsActive())
        return false;
    Unit* target = GetTarget();
    // 名称检查群体版
    if (botAI->HasAura("prayer of shadow protection", target))
        return false;
    // spell ID fallback: 检查所有等级+群体版
    if (HasAnyAuraFromList(target, SHADOW_PROTECTION_SPELL_IDS, std::size(SHADOW_PROTECTION_SPELL_IDS)))
        return false;
    return true;
}

bool PowerWordFortitudeTrigger::IsActive()
{
    if (!BuffTrigger::IsActive())
        return false;
    Unit* target = GetTarget();
    // 名称检查单体+群体版
    if (botAI->HasAura("power word: fortitude", target) || botAI->HasAura("prayer of fortitude", target))
        return false;
    // spell ID fallback
    if (HasAnyAuraFromList(target, FORTITUDE_SPELL_IDS, std::size(FORTITUDE_SPELL_IDS)))
        return false;
    return true;
}

bool DivineSpiritTrigger::IsActive()
{
    if (!BuffTrigger::IsActive())
        return false;
    Unit* target = GetTarget();
    // 名称检查单体+群体版
    if (botAI->HasAura("divine spirit", target) || botAI->HasAura("prayer of spirit", target))
        return false;
    // spell ID fallback
    if (HasAnyAuraFromList(target, DIVINE_SPIRIT_SPELL_IDS, std::size(DIVINE_SPIRIT_SPELL_IDS)))
        return false;
    return true;
}

bool InnerFireTrigger::IsActive()
{
    if (!SpellTrigger::IsActive())
        return false;
    Unit* target = GetTarget();
    // 名称检查
    if (botAI->HasAura(spell, target))
        return false;
    // spell ID fallback
    if (HasAnyAuraFromList(target, INNER_FIRE_SPELL_IDS, std::size(INNER_FIRE_SPELL_IDS)))
        return false;
    return true;
}

bool ShadowformTrigger::IsActive()
{
    // 名称检查 + spell ID fallback
    return !botAI->HasAura("shadowform", bot) && !bot->HasAura(SHADOWFORM_SPELL_ID);
}
//End By leewheel

//By leewheel 2026-09-04: 对齐上游50e780e9——fear ward 对主坦触发：先做CD判定，再走基类(检查主坦无buff)
bool FearWardOnMainTankTrigger::IsActive()
{
    uint32 const spellId = AI_VALUE2(uint32, "spell id", spell);
    if (!spellId || bot->HasSpellCooldown(spellId))
        return false;

    return BuffOnMainTankTrigger::IsActive();
}
//End By leewheel

bool ShadowfiendTrigger::IsActive() { return BoostTrigger::IsActive() && !bot->HasSpellCooldown(34433); }

BindingHealTrigger::BindingHealTrigger(PlayerbotAI* botAI)
    : PartyMemberLowHealthTrigger(botAI, "binding heal", sPlayerbotAIConfig.lowHealth, 0)
{
}

bool BindingHealTrigger::IsActive()
{
    return PartyMemberLowHealthTrigger::IsActive() &&
           AI_VALUE2(uint8, "health", "self target") < sPlayerbotAIConfig.mediumHealth;
}

const std::set<uint32> MindSearChannelCheckTrigger::MIND_SEAR_SPELL_IDS = {
    48045,  // Mind Sear Rank 1
    53023   // Mind Sear Rank 2
};

bool MindSearChannelCheckTrigger::IsActive()
{
    Player* bot = botAI->GetBot();

    // Check if the bot is channeling a spell
    if (Spell* spell = bot->GetCurrentSpell(CURRENT_CHANNELED_SPELL))
    {
        // Only trigger if the spell being channeled is Mind Sear
        if (MIND_SEAR_SPELL_IDS.count(spell->m_spellInfo->Id))
        {
            uint8 attackerCount = AI_VALUE(uint8, "attacker count");
            return attackerCount < minEnemies;
        }
    }

    // Not channeling Mind Sear
    return false;
}
