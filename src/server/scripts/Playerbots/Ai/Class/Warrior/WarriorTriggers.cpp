/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "WarriorTriggers.h"
#include "Playerbots.h"

namespace
{
constexpr uint32 SPELL_VIGILANCE = 50720;
constexpr uint32 SPELL_SHATTERING_THROW = 64382;
constexpr uint32 SPELL_DIVINE_SHIELD = 642;
constexpr uint32 SPELL_ICE_BLOCK = 45438;
constexpr uint32 SPELL_BLESSING_OF_PROTECTION = 41450;
constexpr uint32 SPELL_COMMANDING_PRESENCE_RANKS[] = { 12318, 12857, 12858, 12860, 12861 };

// By leewheel 2026-08-07: Battle Shout + Blessing of Might spell ID 数组 (依据 classic_db2.spelllevels 玩家可学版本)
// 用于 locale 匹配失败时的 spell ID fallback
constexpr uint32 BATTLE_SHOUT_SPELL_IDS[] = {
    6673, 5242, 6192, 11549, 11550, 11551, 25101, 25289, 27578, 46763, 47436
};
constexpr uint32 BLESSING_OF_MIGHT_SPELL_IDS[] = {
    19740, 19834, 19835, 19836, 19837, 19838, 25291, 27140, 48931, 48932, 56520,
    25782, 25916, 27141, 29381, 33564, 43940, 48933, 48934
};

bool HasAuraFromList(Unit* unit, const uint32* spellIds, size_t count)
{
    if (!unit) return false;
    for (size_t i = 0; i < count; ++i)
        if (unit->HasAura(spellIds[i])) return true;
    return false;
}

// 通过 spell ID 查找 Blessing of Might aura (locale 匹配失败时的 fallback)
Aura* FindBlessingOfMightAuraBySpellId(Unit* unit)
{
    if (!unit) return nullptr;
    for (uint32 spellId : BLESSING_OF_MIGHT_SPELL_IDS)
    {
        if (Aura* aura = unit->GetAura(spellId))
            return aura;
    }
    return nullptr;
}
}

bool BloodrageBuffTrigger::IsActive()
{
    return AI_VALUE2(uint8, "health", "self target") >= sPlayerbotAIConfig.mediumHealth &&
           AI_VALUE2(uint8, "rage", "self target") < 20;
}

bool VigilanceTrigger::IsActive()
{
    if (!bot->HasSpell(SPELL_VIGILANCE))
        return false;

    Group* group = bot->GetGroup();
    if (!group)
        return false;

    Player* currentVigilanceTarget = nullptr;
    Player* mainTank = nullptr;
    Player* assistTank1 = nullptr;
    Player* assistTank2 = nullptr;
    Player* highestGearScorePlayer = nullptr;
    uint32 highestGearScore = 0;

    for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
    {
        Player* member = ref->GetSource();
        if (!member || member == bot || !member->IsAlive())
            continue;

        if (!currentVigilanceTarget && (botAI->HasAura("vigilance", member, false, true) || member->HasAura(SPELL_VIGILANCE)))
            currentVigilanceTarget = member;

        if (!mainTank && botAI->IsMainTank(member))
            mainTank = member;
        else if (!assistTank1 && botAI->IsAssistTankOfIndex(member, 0))
            assistTank1 = member;
        else if (!assistTank2 && botAI->IsAssistTankOfIndex(member, 1))
            assistTank2 = member;

        uint32 gearScore = botAI->GetEquipGearScore(member);
        if (gearScore > highestGearScore)
        {
            highestGearScore = gearScore;
            highestGearScorePlayer = member;
        }
    }

    Player* highestPriorityTarget = mainTank ? mainTank :
                                      (assistTank1 ? assistTank1 :
                                      (assistTank2 ? assistTank2 : highestGearScorePlayer));

    if (!currentVigilanceTarget || currentVigilanceTarget != highestPriorityTarget)
        return true;

    return false;
}

bool ShatteringThrowTrigger::IsActive()
{
    if (!bot->HasSpell(SPELL_SHATTERING_THROW) || bot->HasSpellCooldown(SPELL_SHATTERING_THROW))
        return false;

    GuidVector enemies = AI_VALUE(GuidVector, "possible targets");

    for (ObjectGuid const& guid : enemies)
    {
        Unit* enemy = botAI->GetUnit(guid);
        if (!enemy || !enemy->IsAlive() || enemy->IsFriendlyTo(bot))
            continue;

        if (bot->IsWithinDistInMap(enemy, 25.0f) &&
            (enemy->HasAura(SPELL_DIVINE_SHIELD) ||
             enemy->HasAura(SPELL_ICE_BLOCK) ||
             enemy->HasAura(SPELL_BLESSING_OF_PROTECTION)))
        {
            return true;
        }
    }

    return false;
}

bool BattleShoutTrigger::IsActive()
{
    // By leewheel 2026-08-07: 先用 spell ID fallback 检查是否已有 Battle Shout
    // BuffTrigger::IsActive() 通过名称检查, locale 匹配失败时会误判为"没有 buff"导致重复施放
    if (HasAuraFromList(bot, BATTLE_SHOUT_SPELL_IDS, std::size(BATTLE_SHOUT_SPELL_IDS)))
        return false;

    if (!BuffTrigger::IsActive())
        return false;

    uint32 battleShoutSpellId = AI_VALUE2(uint32, "spell id", "battle shout");
    if (!battleShoutSpellId)
        return false;

    SpellInfo const* bsInfo = sSpellMgr->GetSpellInfo(battleShoutSpellId);
    if (!bsInfo)
        return false;

    int32 bsApValue = 0;
    for (uint8 eff = 0; eff < MAX_SPELL_EFFECTS; ++eff)
    {
        if (bsInfo->GetEffects()[eff].ApplyAuraName == SPELL_AURA_MOD_ATTACK_POWER)
        {
            bsApValue = bsInfo->GetEffects()[eff].BasePoints + 1;
            break;
        }
    }
    if (!bsApValue)
        return false;

    static const float commandingPresenceBonus[]   = {
        0.05f, 0.10f, 0.15f, 0.20f, 0.25f };

    float cpBonus = 0.0f;
    for (int rank = 4; rank >= 0; --rank)
    {
        if (bot->HasAura(SPELL_COMMANDING_PRESENCE_RANKS[rank]))
        {
            cpBonus = commandingPresenceBonus[rank];
            break;
        }
    }
    int32 effectiveBsAp = int32(bsApValue * (1.0f + cpBonus));

    static const char* blessingNames[] = {
        "blessing of might", "greater blessing of might", nullptr
    };
    bool foundBomByName = false;
    for (int i = 0; blessingNames[i] != nullptr; ++i)
    {
        Aura* bom = botAI->GetAura(blessingNames[i], bot);
        if (!bom)
            continue;
        foundBomByName = true;

        SpellInfo const* bomInfo = bom->GetSpellInfo();
        if (!bomInfo)
            continue;

        for (uint8 eff = 0; eff < MAX_SPELL_EFFECTS; ++eff)
        {
            if (bomInfo->GetEffects()[eff].ApplyAuraName == SPELL_AURA_MOD_ATTACK_POWER)
            {
                int32 bomApValue = bomInfo->GetEffects()[eff].BasePoints + 1;
                if (bomApValue >= effectiveBsAp)
                    return false;
                break;
            }
        }
    }

    // By leewheel 2026-08-07: 名称匹配失败时, 用 spell ID fallback 查找 Blessing of Might aura
    if (!foundBomByName)
    {
        if (Aura* bom = FindBlessingOfMightAuraBySpellId(bot))
        {
            SpellInfo const* bomInfo = bom->GetSpellInfo();
            if (bomInfo)
            {
                for (uint8 eff = 0; eff < MAX_SPELL_EFFECTS; ++eff)
                {
                    if (bomInfo->GetEffects()[eff].ApplyAuraName == SPELL_AURA_MOD_ATTACK_POWER)
                    {
                        int32 bomApValue = bomInfo->GetEffects()[eff].BasePoints + 1;
                        if (bomApValue >= effectiveBsAp)
                            return false;
                        break;
                    }
                }
            }
        }
    }

    return true;
}
