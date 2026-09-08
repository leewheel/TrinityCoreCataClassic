/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#ifndef PLAYERBOTS_PALADINHELPER_H
#define PLAYERBOTS_PALADINHELPER_H

#include <initializer_list>

#include "Unit.h"

class Player;

namespace ai::paladin
{
static constexpr uint32 SPELL_HAND_OF_PROTECTION = 1022;
static constexpr uint32 SPELL_HAND_OF_SALVATION = 1038;
static constexpr uint32 SPELL_HAND_OF_FREEDOM = 1044;
static constexpr uint32 SPELL_HAND_OF_SACRIFICE = 6940;
static constexpr uint32 SPELL_BLESSING_OF_SANCTUARY = 20911;
static constexpr uint32 SPELL_GREATER_BLESSING_OF_SANCTUARY = 25899;

//By leewheel 2026-08-06: 基于spell ID的祝福检测,不依赖locale名称匹配
//By leewheel 2026-08-07: 通过 classic_db2.spellname 复核修正, 原数组 19839/19856/25890 都不是祝福法术
// 所有祝福法术ID(全等级+强效版本, 依据 classic_db2.spellname 实际查得)
static constexpr uint32 BLESSING_SPELL_IDS[] = {
    // Blessing of Might 全等级
    19740, 19834, 19835, 19836, 19837, 19838, 25291, 27140, 48931, 48932, 56520,
    // Greater Blessing of Might 全等级
    25782, 25916, 27141, 29381, 33564, 43940, 48933, 48934,
    // Blessing of Wisdom 全等级
    19742, 19850, 19852, 19853, 19854, 19855, 25290, 27142, 48935, 48936, 56521,
    // Greater Blessing of Wisdom 全等级
    25894, 25918, 27143, 48937, 48938,
    // Blessing of Kings 全等级
    20217, 56525, 58054,
    // Greater Blessing of Kings 全等级
    25898, 43223,
    // Blessing of Sanctuary 全等级
    20911, 57319, 57320, 57321, 67480,
    // Greater Blessing of Sanctuary
    25899
};

// 基于spell ID检测单位是否已有任何骑士祝福(不依赖名称locale匹配)
inline bool HasAnyBlessing(Unit* unit)
{
    if (!unit)
        return false;
    for (uint32 spellId : BLESSING_SPELL_IDS)
    {
        if (unit->HasAura(spellId))
            return true;
    }
    return false;
}
//End By leewheel

//By leewheel 2026-08-06: 基于spell ID的圣印检测,不依赖locale名称匹配
//SealTrigger::IsActive用名称检查7个Seal,locale匹配失败时永远返回true导致重复切Seal
//By leewheel 2026-08-07: 通过 classic_db2.spellname 复核修正, 原数组含大量错误ID(21082-21084/27155/20269-20272/27156/20347-20349/27157/20356-20357/27158/20915/20918-20920/27159 都不是 Seal)
// 所有Seal法术ID(3.3.5a 中 Seal 已改为单等级, 但保留旧 spell ID 兼容; 依据 classic_db2.spellname 实际查得)
static constexpr uint32 SEAL_SPELL_IDS[] = {
    // Seal of Righteousness (正义圣印, 含旧 rank)
    20154, 21084, 25742,
    // Seal of Justice (公正圣印)
    20164,
    // Seal of Light (光明圣印, 含触发版)
    20165, 20167,
    // Seal of Wisdom (智慧圣印, 含触发版)
    20166, 20168,
    // Seal of Command (命令圣印, 含 NPC/物品版本)
    20375, 20424, 29385, 33127, 41469, 42058, 57769, 57770, 66004, 69403,
    // Seal of Vengeance (复仇圣印, 联盟专属)
    31801, 42463,
    // Seal of Corruption (腐蚀圣印, 部落专属)
    53736, 53739,
    // Seal of Blood (鲜血圣印, TBC 联盟专属, WOTLK 已移除但保留兼容)
    31892, 31893, 32221, 38008,
    // Seal of the Martyr (殉难圣印, TBC 联盟版鲜血圣印)
    53718, 53719, 53720
};

// 基于spell ID检测单位是否已有任何圣印
inline bool HasAnySeal(Unit* unit)
{
    if (!unit)
        return false;
    for (uint32 spellId : SEAL_SPELL_IDS)
    {
        if (unit->HasAura(spellId))
            return true;
    }
    return false;
}

// 基于spell ID检测单位是否已有智慧圣印(Seal of Wisdom)
// SealTrigger对智慧圣印有特殊逻辑:蓝量>70%时即使有智慧圣印也要触发切换
// By leewheel 2026-08-07: 通过 classic_db2.spellname 复核修正, 20356/20357/27158 都不是智慧圣印
static constexpr uint32 SEAL_OF_WISDOM_SPELL_IDS[] = {
    20166, 20168
};

inline bool HasSealOfWisdom(Unit* unit)
{
    if (!unit)
        return false;
    for (uint32 spellId : SEAL_OF_WISDOM_SPELL_IDS)
    {
        if (unit->HasAura(spellId))
            return true;
    }
    return false;
}
//End By leewheel

inline bool HasHandFromCaster(Unit* target, Player* caster, std::initializer_list<uint32> spellIds)
{
    if (!target || !caster)
        return false;

    for (uint32 spellId : spellIds)
    {
        if (target->HasAura(spellId, caster->GetGUID()))
            return true;
    }

    return false;
}

inline bool HasAnyPaladinHandFromCaster(Unit* target, Player* caster)
{
    return HasHandFromCaster(target, caster,
        { SPELL_HAND_OF_PROTECTION, SPELL_HAND_OF_SALVATION, SPELL_HAND_OF_FREEDOM, SPELL_HAND_OF_SACRIFICE });
}
}

#endif
