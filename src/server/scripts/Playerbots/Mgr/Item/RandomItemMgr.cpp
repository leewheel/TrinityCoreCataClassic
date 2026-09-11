/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

//By leewheel 2026-07-11: 添加Containers.h以获取Trinity::Containers::SelectRandomContainerElement
//By leewheel 2026-08-01: 日志清理——注释掉装备评估高频INFO与缓存构建DEBUG，仅保留错误与一次性加载统计
//End By leewheel

#include "RandomItemMgr.h"

#include "Containers.h"
#include "DBCStores.h"
#include "DB2Stores.h"
#include "ItemTemplate.h"
#include "Playerbots.h"
//End By leewheel 2026-07-11

std::unordered_set<uint32> RandomItemMgr::itemCache;

class RandomItemGuildTaskPredicate : public RandomItemPredicate
{
public:
    bool Apply(ItemTemplate const* proto) override
    {
        if (!proto)
            return false;

        if (proto->GetBonding() == BIND_WHEN_PICKED_UP || proto->GetBonding() == BIND_WHEN_USE ||
            proto->GetBonding() == BIND_QUEST_ITEM)
            return false;

        if (proto->GetQuality() < ITEM_QUALITY_NORMAL)
            return false;

        if ((proto->GetClass() == ITEM_CLASS_WEAPON || proto->GetClass() == ITEM_CLASS_ARMOR) &&
            proto->GetQuality() >= ITEM_QUALITY_RARE)
            return true;

        if (proto->GetClass() == ITEM_CLASS_CONSUMABLE || proto->GetClass() == ITEM_CLASS_TRADE_GOODS)
            return true;

        return false;
    }
};

class RandomItemGuildTaskRewardPredicate : public RandomItemPredicate
{
public:
    RandomItemGuildTaskRewardPredicate(bool equip, bool rare) : equip(equip), rare(rare) {}

    bool Apply(ItemTemplate const* proto) override
    {
        if (!proto)
            return false;

        if (proto->GetBonding() == BIND_WHEN_PICKED_UP || proto->GetBonding() == BIND_WHEN_USE ||
            proto->GetBonding() == BIND_QUEST_ITEM)
            return false;

        if (proto->GetClass() == ITEM_CLASS_QUEST)
            return false;

        if (equip)
        {
            uint32 desiredQuality = rare ? ITEM_QUALITY_RARE : ITEM_QUALITY_UNCOMMON;
            if (proto->GetQuality() < desiredQuality || proto->GetQuality() >= ITEM_QUALITY_EPIC)
                return false;

            if (proto->GetClass() == ITEM_CLASS_WEAPON || proto->GetClass() == ITEM_CLASS_ARMOR)
                return true;
        }
        else
        {
            uint32 desiredQuality = rare ? ITEM_QUALITY_UNCOMMON : ITEM_QUALITY_NORMAL;
            if (proto->GetQuality() < desiredQuality || proto->GetQuality() >= ITEM_QUALITY_RARE)
                return false;

            if (proto->GetClass() == ITEM_CLASS_CONSUMABLE || proto->GetClass() == ITEM_CLASS_TRADE_GOODS)
                return true;
        }

        return false;
    }

private:
    bool equip;
    bool rare;
};

RandomItemMgr::RandomItemMgr()
{
    InitWeaponProficiency();
    InitPredicates();
    InitViableSlots();
    InitWeightLinks();
}

RandomItemMgr::~RandomItemMgr()
{
    for (auto& pair : predicates)
        delete pair.second;

    predicates.clear();
}

void RandomItemMgr::Init()
{
    // Prevent double initialization when the reload command is executed.
    // Use exchange so only the first caller continues; others stop.
    // NOTE: If someone implements reload/rebuild hash, update all getters from
    //       returning by reference to returning by value. Also, replace all
    //       DEFAULT_MAX_LEVEL maxLevel with randomBotMaxLevel and clear caches
    //       before processing.
    if (m_initialized.exchange(true))
        return;

    BuildCacheItemInfo();
    // if (!LoadCacheEquip())
    //     BuildCacheEquip();
    BuildCacheEquipNew();
    BuildCacheAmmo();
    BuildCacheFood();
    BuildCachePotion();
    BuildCacheTrade();
    LoadEnchantmentPool();
}

void RandomItemMgr::InitAfterAhBot()
{
    if (!LoadCacheRandomItem())
        BuildCacheRandomItem();
    DebugCacheRandomItem();
    // if (!LoadCacheRarity())
    //     BuildCacheRarity();
}

bool RandomItemMgr::HandleConsoleCommand(ChatHandler* /*handler*/, char const* args)
{
    if (!args || !*args)
    {
        TC_LOG_ERROR("playerbots", "Usage: rnditem");
        return false;
    }

    return false;
}

RandomItemList RandomItemMgr::Query(uint32 level, RandomItemType type, RandomItemPredicate* predicate) const
{
    auto const levelItr = randomItemCache.find((level - 1) / 10);
    if (levelItr == randomItemCache.end())
        return {};

    auto const typeItr = levelItr->second.find(type);
    if (typeItr == levelItr->second.end())
        return {};

    RandomItemList result;
    for (uint32 const itemId : typeItr->second)
    {
        ItemTemplate const* proto = sObjectMgr->GetItemTemplate(itemId);
        if (!proto)
            continue;

        if (predicate && !predicate->Apply(proto))
            continue;

        result.push_back(itemId);
    }

    return result;
}

uint32 RandomItemMgr::GetUpgrade(Player* player, std::string spec, uint8 slot, uint32 quality, uint32 itemId)
{
    if (!player)
        return 0;

    // get old item statWeight
    uint32 oldStatWeight = 0;
    uint32 specId = 0;
    uint32 closestUpgrade = 0;
    uint32 closestUpgradeWeight = 0;
    std::vector<uint32> classspecs;

    for (uint32 specNum = 1; specNum < 5; ++specNum)
    {
        if (!m_weightScales[player->getClass()][specNum].info.id)
            continue;

        classspecs.push_back(m_weightScales[player->getClass()][specNum].info.id);

        if (m_weightScales[player->getClass()][specNum].info.name == spec)
            specId = m_weightScales[player->getClass()][specNum].info.id;
    }
    if (!specId)
        return 0;

    if (itemId && itemInfoCache.find(itemId) != itemInfoCache.end())
    {
        oldStatWeight = itemInfoCache[itemId].weights[specId];

        //By leewheel 2026-08-01: 修复历史日志清理残留——原本是if/else各一句日志，
        //注释后空分支导致C2181语法错误，整段移除
        // if (oldStatWeight)
        //     TC_LOG_INFO("playerbots", "Old Item: {}, weight: {}", itemId, oldStatWeight);
        // else
        //     TC_LOG_INFO("playerbots", "Old item has no stat weight");
        //End By leewheel
    }

    for (std::unordered_map<uint32, ItemInfoEntry>::iterator i = itemInfoCache.begin(); i != itemInfoCache.end(); ++i)
    {
        ItemInfoEntry& info = i->second;

        // skip useless items
        if (info.weights[specId] == 0)
            continue;

        // skip higher lvl
        if (info.minLevel > player->GetLevel())
            continue;

        // skip too low level
        if (static_cast<int32>(info.minLevel) < (static_cast<int32>(player->GetLevel()) - 10))
            continue;

        // skip wrong team
        //By leewheel 2026-09-03 修复C4389警告：GetTeamId()返回TeamId枚举，与uint32 team比较产生有符号/无符号不匹配，显式转换对齐
        if (info.team != TEAM_NEUTRAL && info.team != static_cast<uint32>(player->GetTeamId()))
            continue;
        //End By leewheel

        // skip wrong slot
        if ((EquipmentSlots)info.slot != (EquipmentSlots)slot)
            continue;

        // skip higher quality
        if (quality && info.quality != quality)
            continue;

        // skip worse items
        if (info.weights[specId] <= oldStatWeight)
            continue;

        // skip items that only fit in slot, but not stats
        if (!itemId && info.weights[specId] == 1 && player->GetLevel() > 40)
            continue;

        // skip quest items
        if (info.source == ITEM_SOURCE_QUEST)
        {
            if (player->GetQuestRewardStatus(info.sourceId) != QUEST_STATUS_COMPLETE)
                continue;
        }

        // skip no stats trinkets
        if (info.weights[specId] == 1 && (info.slot == EQUIPMENT_SLOT_NECK || info.slot == EQUIPMENT_SLOT_TRINKET1 ||
            info.slot == EQUIPMENT_SLOT_TRINKET2 || info.slot == EQUIPMENT_SLOT_FINGER1 ||
            info.slot == EQUIPMENT_SLOT_FINGER2))
            continue;

        // check if item stat score is the best among class specs
        uint32 bestSpecId = 0;
        uint32 bestSpecScore = 0;
        for (std::vector<uint32>::iterator i = classspecs.begin(); i != classspecs.end(); ++i)
        {
            if (info.weights[*i] > bestSpecScore)
            {
                bestSpecId = *i;
                bestSpecScore = info.weights[specId];
            }
        }

        if (bestSpecId && bestSpecId != specId && player->GetLevel() > 40)
            return 0;

        if (!closestUpgrade)
        {
            closestUpgrade = info.itemId;
            closestUpgradeWeight = info.weights[specId];
        }

        // pick closest upgrade
        if (info.weights[specId] < closestUpgradeWeight)
        {
            closestUpgrade = info.itemId;
            closestUpgradeWeight = info.weights[specId];
        }
    }

    //By leewheel 2026-09-03 修复C4715警告：原AC实现此处有升级日志，移植时被注释掉导致 if 空体+落穿，
    //MSVC 流分析报"不是所有控制路径都返回值"；按 AC 原版恢复日志（log 输出改为中文）
    if (closestUpgrade)
        TC_LOG_INFO("playerbots", "新装备: {}, 权重: {}", closestUpgrade, closestUpgradeWeight);
    //End By leewheel

    return closestUpgrade;
}

std::vector<uint32> RandomItemMgr::GetUpgradeList(Player* player, std::string spec, uint8 slot, uint32 quality,
                                                  uint32 itemId, uint32 /*amount*/)
{
    std::vector<uint32> listItems;
    if (!player)
        return listItems;

    // get old item statWeight
    uint32 oldStatWeight = 0;
    uint32 specId = 0;
    uint32 closestUpgradeWeight = 0;
    std::vector<uint32> classspecs;

    for (uint32 specNum = 1; specNum < 5; ++specNum)
    {
        if (!m_weightScales[player->getClass()][specNum].info.id)
            continue;

        classspecs.push_back(m_weightScales[player->getClass()][specNum].info.id);

        if (m_weightScales[player->getClass()][specNum].info.name == spec)
            specId = m_weightScales[player->getClass()][specNum].info.id;
    }

    if (!specId)
        return listItems;

    if (itemId && itemInfoCache.find(itemId) != itemInfoCache.end())
    {
        oldStatWeight = itemInfoCache[itemId].weights[specId];

        //By leewheel 2026-08-01: 修复历史日志清理残留——原本是if/else各一句日志，
        //注释后空分支导致C2181语法错误，整段移除
        // if (oldStatWeight)
        //     TC_LOG_INFO("playerbots", "Old Item: {}, weight: {}", itemId, oldStatWeight);
        // else
        //     TC_LOG_INFO("playerbots", "Old item has no stat weight");
        //End By leewheel
    }

    for (std::unordered_map<uint32, ItemInfoEntry>::iterator i = itemInfoCache.begin(); i != itemInfoCache.end(); ++i)
    {
        ItemInfoEntry& info = i->second;

        // skip useless items
        if (info.weights[specId] == 0)
            continue;

        // skip higher lvl
        if (info.minLevel > player->GetLevel())
            continue;

        // skip too low level
        if ((int32)info.minLevel < (int32)(player->GetLevel() - 20))
            continue;

        // skip wrong team
        //By leewheel 2026-09-03 修复C4389警告：GetTeamId()返回TeamId枚举，与uint32 team比较产生有符号/无符号不匹配，显式转换对齐
        if (info.team != TEAM_NEUTRAL && info.team != static_cast<uint32>(player->GetTeamId()))
            continue;
        //End By leewheel

        // skip wrong slot
        if ((EquipmentSlots)info.slot != (EquipmentSlots)slot)
            continue;

        // skip higher quality
        if (quality && info.quality != quality)
            continue;

        // skip worse items
        if (info.weights[specId] <= oldStatWeight)
            continue;

        // skip items that only fit in slot, but not stats
        if (!itemId && info.weights[specId] == 1 && player->GetLevel() > 40)
            continue;

        // skip quest items
        if (info.source == ITEM_SOURCE_QUEST)
        {
            if (player->GetQuestRewardStatus(info.sourceId) != QUEST_STATUS_COMPLETE)
                continue;
        }

        // skip no stats trinkets
        if (info.weights[specId] < 2 && (info.slot == EQUIPMENT_SLOT_NECK || info.slot == EQUIPMENT_SLOT_TRINKET1 ||
                                         info.slot == EQUIPMENT_SLOT_TRINKET2 || info.slot == EQUIPMENT_SLOT_FINGER1 ||
                                         info.slot == EQUIPMENT_SLOT_FINGER2))
            continue;

        // if (player->GetLevel() >= 40)
        //{
        //     // check if item stat score is the best among class specs
        //     uint32 bestSpecId = 0;
        //     uint32 bestSpecScore = 0;
        //     for (vector<uint32>::iterator i = classspecs.begin(); i != classspecs.end(); ++i)
        //     {
        //         if (info->weights[*i] > bestSpecScore)
        //         {
        //             bestSpecId = *i;
        //             bestSpecScore = info->weights[specId];
        //         }
        //     }

        //    if (bestSpecId && bestSpecId != specId)
        //        continue;
        //}

        listItems.push_back(info.itemId);
        // continue;

        // pick closest upgrade
        if (info.weights[specId] > closestUpgradeWeight)
        {
            closestUpgradeWeight = info.weights[specId];
        }
    }

    // if (listItems.size())
    //     TC_LOG_INFO("playerbots", "New Items: {}, Old item:{}, New items max: {}", listItems.size(), oldStatWeight,
    //              closestUpgradeWeight);

    return listItems;
}

bool RandomItemMgr::HasStatWeight(uint32 itemId)
{
    auto itr = itemInfoCache.find(itemId);
    return itr != itemInfoCache.end();
}

uint32 RandomItemMgr::CalculateStatWeight(ItemTemplate const* proto, uint8 playerclass, uint8 spec)
{
    if (!proto)
        return false;

    uint32 statWeight = 0;
    bool isCasterItem = false;
    bool isAttackItem = false;
    bool noCaster = (Classes)playerclass == CLASS_WARRIOR || (Classes)playerclass == CLASS_ROGUE ||
                    (Classes)playerclass == CLASS_DEATH_KNIGHT || (Classes)playerclass == CLASS_HUNTER;
    uint32 spellPower = 0;
    uint32 spellHeal = 0;
    uint32 attackPower = 0;
    bool hasInt = false;
    bool hasMana = !((Classes)playerclass == CLASS_WARRIOR || (Classes)playerclass == CLASS_ROGUE ||
                     (Classes)playerclass == CLASS_DEATH_KNIGHT);

    if (proto->GetSubClass() == ITEM_SUBCLASS_ARMOR_LIBRAM || proto->GetSubClass() == ITEM_SUBCLASS_ARMOR_IDOL ||
        proto->GetSubClass() == ITEM_SUBCLASS_ARMOR_TOTEM || proto->GetSubClass() == ITEM_SUBCLASS_ARMOR_SIGIL)
        return (uint32)(proto->GetQuality() + proto->GetItemLevel());

    // check basic item stats
    int32 basicStatsWeight = 0;
    for (uint8 j = 0; j < MAX_ITEM_PROTO_STATS; ++j)
    {
        uint32 statType;
        int32 val;
        std::string weightName;

        // if (j >= proto->StatsCount)
        //     continue;

        statType = proto->GetStatModifierBonusStat(j);
        val = proto->GetStatModifierBonusAmount(j);

        if (val == 0)
            continue;

        for (std::unordered_map<std::string, uint32>::iterator i = weightStatLink.begin(); i != weightStatLink.end(); ++i)
        {
            uint32 modd = i->second;
            if (modd == statType)
            {
                weightName = i->first;
                break;
            }
        }

        if (weightName.empty())
            continue;

        uint32 singleStat = CalculateSingleStatWeight(playerclass, spec, weightName, val);
        basicStatsWeight += singleStat;

        if (val)
        {
            if (weightName == "int" && !noCaster)
                isCasterItem = true;

            if (weightName == "int")
                hasInt = true;

            if (weightName == "splpwr")
                isCasterItem = true;

            if (weightName == "str")
                isAttackItem = true;

            if (weightName == "agi")
                isAttackItem = true;

            if (weightName == "atkpwr")
                isAttackItem = true;
        }
    }

    // check armor & block
    //By leewheel 2026-09-09: TC-Cata的ItemTemplate没有GetArmor/GetShieldBlockValue，使用兼容函数
    statWeight += CalculateSingleStatWeight(playerclass, spec, "armor", ItemTemplate_GetArmor(proto));
    statWeight += CalculateSingleStatWeight(playerclass, spec, "block", ItemTemplate_GetShieldBlockValue(proto, proto->GetItemLevel()));
    //End By leewheel

    // check weapon dps
    if (proto->IsWeapon())
    {
        //WeaponAttackType attType = BASE_ATTACK; //not used, line marked for removal.

        uint32 dps = 0;
        for (uint8 i = 0; i < MAX_ITEM_PROTO_DAMAGES; i++)
        {
            if (proto->GetMaxDamage(i) == 0)
                break;

            dps = (proto->GetMinDamage(i) + proto->GetMaxDamage(i)) / (float)(proto->GetDelay() / 1000.0f) / 2;
            if (dps)
            {
                if (proto->IsRangedWeapon())
                    statWeight += CalculateSingleStatWeight(playerclass, spec, "rgddps", dps);
                else
                    statWeight += CalculateSingleStatWeight(playerclass, spec, "mledps", dps);
            }
        }
    }

    // check item spells
    for (ItemEffectEntry const* effect : proto->Effects)
    {
        // 无法术
        if (!effect || !effect->SpellID)
            continue;

        // 仅应用装备时触发的法术
        if (effect->TriggerType != ITEM_SPELLTRIGGER_ON_EQUIP)
            continue;

        // 检查法术是否有效
        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(effect->SpellID);
        if (!spellInfo)
            continue;

        uint32 spellDamage = 0;
        uint32 spellHealing = 0;
        for (uint32 j = 0; j < MAX_SPELL_EFFECTS; j++)
        {
            if (spellInfo->GetEffects()[j].Effect == SPELL_EFFECT_APPLY_AURA && spellInfo->GetEffects()[j].BasePoints)
            {
                // spell damage
                // SPELL_AURA_MOD_DAMAGE_DONE
                if (spellInfo->GetEffects()[j].ApplyAuraName == SPELL_AURA_MOD_DAMAGE_DONE)
                {
                    spellDamage = spellInfo->GetEffects()[j].BasePoints + 1;
                }

                // spell healing
                // SPELL_AURA_MOD_HEALING_DONE
                if (spellInfo->GetEffects()[j].ApplyAuraName == SPELL_AURA_MOD_HEALING_DONE)
                {
                    spellHealing = spellInfo->GetEffects()[j].BasePoints + 1;
                }

                // check spell power
                if (spellDamage && spellDamage == spellHealing)
                {
                    isCasterItem = true;
                    spellPower += CalculateSingleStatWeight(playerclass, spec, "splpwr", spellDamage);
                }

                // spell hit rating (pre tbc)
                // SPELL_AURA_MOD_SPELL_HIT_CHANCE
                if (spellInfo->GetEffects()[j].ApplyAuraName == SPELL_AURA_MOD_SPELL_HIT_CHANCE)
                {
                    statWeight += CalculateSingleStatWeight(playerclass, spec, "spellhitrtng",
                                                            spellInfo->GetEffects()[j].BasePoints + 1);
                }

                // spell crit rating (pre tbc)
                // SPELL_AURA_MOD_SPELL_CRIT_CHANCE
                if (spellInfo->GetEffects()[j].ApplyAuraName == SPELL_AURA_MOD_SPELL_CRIT_CHANCE)
                {
                    statWeight += CalculateSingleStatWeight(playerclass, spec, "spellcritstrkrtng",
                                                            spellInfo->GetEffects()[j].BasePoints + 1);
                }

                // spell penetration
                // SPELL_AURA_MOD_TARGET_RESISTANCE
                if (spellInfo->GetEffects()[j].ApplyAuraName == SPELL_AURA_MOD_TARGET_RESISTANCE)
                {
                    // check if magic type
                    if (spellInfo->GetEffects()[j].MiscValue == SPELL_SCHOOL_MASK_SPELL)
                        statWeight += CalculateSingleStatWeight(playerclass, spec, "spellpenrtng",
                                                                abs(spellInfo->GetEffects()[j].BasePoints + 1));
                }

                // check attack power
                if (spellInfo->GetEffects()[j].ApplyAuraName == SPELL_AURA_MOD_ATTACK_POWER)
                {
                    isAttackItem = true;
                    attackPower +=
                        CalculateSingleStatWeight(playerclass, spec, "atkpwr", spellInfo->GetEffects()[j].BasePoints + 1);
                }

                // check ranged ap
                // SPELL_AURA_MOD_RANGED_ATTACK_POWER
                if (spellInfo->GetEffects()[j].ApplyAuraName == SPELL_AURA_MOD_RANGED_ATTACK_POWER)
                {
                    isAttackItem = true;
                    attackPower +=
                        CalculateSingleStatWeight(playerclass, spec, "atkpwr", spellInfo->GetEffects()[j].BasePoints + 1);
                }

                // check block
                if (spellInfo->GetEffects()[j].ApplyAuraName == SPELL_AURA_MOD_SHIELD_BLOCKVALUE)
                {
                    statWeight +=
                        CalculateSingleStatWeight(playerclass, spec, "block", spellInfo->GetEffects()[j].BasePoints + 1);
                }

                // block chance
                // SPELL_AURA_MOD_BLOCK_PERCENT
                if (spellInfo->GetEffects()[j].ApplyAuraName == SPELL_AURA_MOD_BLOCK_PERCENT)
                {
                    statWeight +=
                        CalculateSingleStatWeight(playerclass, spec, "blockrtng", spellInfo->GetEffects()[j].BasePoints + 1);
                }

                // armor penetration
                // SPELL_AURA_MOD_TARGET_RESISTANCE
                if (spellInfo->GetEffects()[j].ApplyAuraName == SPELL_AURA_MOD_TARGET_RESISTANCE)
                {
                    // check if physical type
                    if (spellInfo->GetEffects()[j].MiscValue == SPELL_SCHOOL_MASK_NORMAL)
                        statWeight += CalculateSingleStatWeight(playerclass, spec, "armorpenrtng",
                                                                abs(spellInfo->GetEffects()[j].BasePoints + 1));
                }

                // hit rating (pre tbc)
                // SPELL_AURA_MOD_HIT_CHANCE
                if (spellInfo->GetEffects()[j].ApplyAuraName == SPELL_AURA_MOD_HIT_CHANCE)
                {
                    statWeight +=
                        CalculateSingleStatWeight(playerclass, spec, "hitrtng", spellInfo->GetEffects()[j].BasePoints + 1);
                }

                // crit rating (pre tbc)
                // SPELL_AURA_MOD_HIT_CHANCE
                if (spellInfo->GetEffects()[j].ApplyAuraName == SPELL_AURA_MOD_CRIT_PCT)
                {
                    statWeight += CalculateSingleStatWeight(playerclass, spec, "critstrkrtng",
                                                            spellInfo->GetEffects()[j].BasePoints + 1);
                }

                // check defense SPELL_AURA_MOD_SKILL
                // check block
                // if (spellInfo->GetEffects()[j].ApplyAuraName == SPELL_AURA_MOD_SKILL)
                //{
                //    statWeight += CalculateSingleStatWeight(playerclass, spec, "block",
                //    spellInfo->GetEffects()[j].BasePoints + 1);
                //}

                // ratings
                // enum CombatRating
                if (spellInfo->GetEffects()[j].ApplyAuraName == SPELL_AURA_MOD_RATING)
                {
                    for (uint32 rating = 0; rating < MAX_COMBAT_RATING; ++rating)
                    {
                        if (spellInfo->GetEffects()[j].MiscValue & (1 << rating))
                        {
                            int32 val = spellInfo->GetEffects()[j].BasePoints + 1;
                            std::string weightName;

                            for (std::unordered_map<std::string, uint32>::iterator i = weightRatingLink.begin();
                                 i != weightRatingLink.end(); ++i)
                            {
                                uint32 modd = i->second;
                                if (modd == rating)
                                {
                                    weightName = i->first;
                                    break;
                                }
                            }

                            if (weightName.empty())
                                continue;

                            statWeight += CalculateSingleStatWeight(playerclass, spec, weightName, val);
                        }
                    }
                }

                // mana regen
                // SPELL_AURA_MOD_POWER_REGEN
                if (spellInfo->GetEffects()[j].ApplyAuraName == SPELL_AURA_MOD_POWER_REGEN)
                {
                    statWeight +=
                        CalculateSingleStatWeight(playerclass, spec, "manargn", spellInfo->GetEffects()[j].BasePoints + 1);
                }
            }
        }
    }

    // check for caster item
    if (isCasterItem || hasInt)
    {
        if ((!hasMana || noCaster) && spellPower)
            return 0;

        if (!hasMana && hasInt)
            return 0;

        if ((!hasMana && noCaster && playerclass != CLASS_PALADIN) && spellPower > attackPower)
            return 0;

        bool playerCaster = false;
        for (std::vector<WeightScaleStat>::iterator i = m_weightScales[playerclass][spec].stats.begin();
             i != m_weightScales[playerclass][spec].stats.end(); ++i)
        {
            if (i->stat == "splpwr" || i->stat == "int" || i->stat == "manargn")
            {
                playerCaster = true;
            }
        }

        if (!playerCaster)
            return 0;
    }

    // check for caster item
    if (isAttackItem)
    {
        if (hasMana && !noCaster && !(hasInt || spellPower))
            return 0;
        // if (!noCaster && attackPower)
        //     return 0;

        bool playerAttacker = false;
        for (std::vector<WeightScaleStat>::iterator i = m_weightScales[playerclass][spec].stats.begin();
             i != m_weightScales[playerclass][spec].stats.end(); ++i)
        {
            if (i->stat == "str" || i->stat == "agi" || i->stat == "atkpwr" || i->stat == "mledps" ||
                i->stat == "rgddps")
            {
                playerAttacker = true;
            }
        }

        if (!playerAttacker)
            return 0;
    }

    statWeight += spellPower;
    statWeight += spellHeal;
    statWeight += attackPower;

    // handle negative stats
    if (basicStatsWeight < 0)
    {
        uint32 const absVal = static_cast<uint32>(abs(basicStatsWeight));
        statWeight = (absVal >= statWeight) ? 0 : statWeight - absVal;
    }
    else
        statWeight += static_cast<uint32>(basicStatsWeight);

    return statWeight;
}

uint32 RandomItemMgr::CalculateSingleStatWeight(uint8 playerclass, uint8 spec, std::string stat, uint32 value)
{
    uint32 statWeight = 0;
    for (std::vector<WeightScaleStat>::iterator i = m_weightScales[playerclass][spec].stats.begin();
         i != m_weightScales[playerclass][spec].stats.end(); ++i)
    {
        if (stat == i->stat)
        {
            statWeight = i->weight * value;
            // if (statWeight)
            //     TC_LOG_INFO("playerbots", "stat: {}, val: {}, weight: {}, total: {}, class: {}, spec: {}",
            //         stat, value, i->weight, statWeight, playerclass, m_weightScales[playerclass][spec].info.name);
            return statWeight;
        }
    }

    return statWeight;
}

uint32 RandomItemMgr::GetStatWeight(Player* player, uint32 itemId)
{
    if (!player || !itemId)
        return 0;

    auto itr = itemInfoCache.find(itemId);
    if (itr == itemInfoCache.end())
        return 0;

    uint32 statWeight = 0;
    uint32 specId = 0;
    std::vector<uint32> classspecs;

    std::string const& specName = AiFactory::GetPlayerSpecName(player);
    if (specName.empty())
        return 0;

    for (uint32 specNum = 1; specNum < 5; ++specNum)
    {
        if (!m_weightScales[player->getClass()][specNum].info.id)
            continue;

        classspecs.push_back(m_weightScales[player->getClass()][specNum].info.id);

        if (m_weightScales[player->getClass()][specNum].info.name == specName)
            specId = m_weightScales[player->getClass()][specNum].info.id;

        if (m_weightScales[player->getClass()][specNum].info.name == specName)
        {
            specId = specNum;
            break;
        }
    }

    if (!specId)
        return 0;

    statWeight = itr->second.weights[specId];

    return statWeight;
}

uint32 RandomItemMgr::GetLiveStatWeight(Player* player, uint32 itemId)
{
    if (!player || !itemId)
        return 0;

    auto itr = itemInfoCache.find(itemId);
    if (itr == itemInfoCache.end())
        return 0;

    uint32 statWeight = 0;
    uint32 specId = 0;
    std::vector<uint32> classspecs;

    std::string const& specName = AiFactory::GetPlayerSpecName(player);
    if (specName.empty())
        return 0;

    for (uint32 specNum = 1; specNum < 5; ++specNum)
    {
        if (!m_weightScales[player->getClass()][specNum].info.id)
            continue;

        // for bestSpec check
        // classspecs.push_back(m_weightScales[player->getClass()][specNum].info.id);

        if (m_weightScales[player->getClass()][specNum].info.name == specName)
            specId = m_weightScales[player->getClass()][specNum].info.id;
    }
    if (!specId)
        return 0;

    statWeight = itr->second.weights[specId];

    // skip higher lvl
    if (itr->second.minLevel > player->GetLevel())
        return 0;

    // skip too low level
    // if ((int32)info->minLevel < (int32)(player->GetLevel() - 20))
    //    return 0;

    // skip wrong team
    //By leewheel 2026-09-03 修复C4389警告：同上，TeamId枚举与uint32 team比较显式转换
    if (itr->second.team != TEAM_NEUTRAL && itr->second.team != static_cast<uint32>(player->GetTeamId()))
        return 0;
    //End By leewheel

    // skip quest items
    if (itr->second.source == ITEM_SOURCE_QUEST && itr->second.sourceId)
    {
        if (player->GetQuestRewardStatus(itr->second.sourceId) != QUEST_STATUS_COMPLETE)
            return 0;
    }

    // skip pvp items
    if (itr->second.source == ITEM_SOURCE_PVP)
    {
        //By leewheel 2026-09-09: TC-Cata移除了竞技场点数，使用兼容函数
        if (!player->GetHonorPoints() && !Player_GetArenaPoints(player))
            return 0;
        //End By leewheel
    }

    // skip no stats trinkets
    if (itr->second.weights[specId] == 1 &&
        (itr->second.slot == EQUIPMENT_SLOT_NECK || itr->second.slot == EQUIPMENT_SLOT_TRINKET1 ||
         itr->second.slot == EQUIPMENT_SLOT_TRINKET2 || itr->second.slot == EQUIPMENT_SLOT_FINGER1 ||
         itr->second.slot == EQUIPMENT_SLOT_FINGER2))
        return 0;

    // skip items that only fit in slot, but not stats
    if (!itemId && itr->second.weights[specId] == 1 && player->GetLevel() > 20)
        return 0;

    // check if item stat score is the best among class specs
    /*uint32 bestSpecId = 0;
    uint32 bestSpecScore = 0;
    for (vector<uint32>::iterator i = classspecs.begin(); i != classspecs.end(); ++i)
    {
        if (itemCache[itemId]->weights[*i] > bestSpecScore && itemCache[itemId]->weights[*i] > 1)
        {
            bestSpecId = *i;
            bestSpecScore = itemCache[itemId]->weights[specId];
        }
    }

    if (bestSpecId && bestSpecId != specId && player->GetLevel() >= 60)
        return 0;*/

    return statWeight;
}

uint32 RandomItemMgr::GetMinLevelFromCache(uint32 itemId) const
{
    auto itr = itemInfoCache.find(itemId);
    if (itr == itemInfoCache.end())
        return 0;

    return itr->second.minLevel;
}

RandomItemList const& RandomItemMgr::GetEquipment(uint32 level, uint8 clazz, uint8 slot, uint32 quality) const
{
    static RandomItemList const empty;

    level = NormalizeLevel(level);

    BotEquipKey const key(level, clazz, slot, quality);
    auto const itr = equipCache.find(key);
    if (itr == equipCache.end())
        return empty;

    return itr->second;
}

RandomItemList const& RandomItemMgr::GetEquipmentNew(uint32 level, InventoryType invType) const
{
    static RandomItemList const empty;

    level = NormalizeLevel(level);

    auto const levelItr = equipCacheNew.find(level);
    if (levelItr == equipCacheNew.end())
        return empty;

    auto const typeItr = levelItr->second.find(invType);
    if (typeItr == levelItr->second.end())
        return empty;

    return typeItr->second;
}

uint32 RandomItemMgr::GetRandomItem(uint32 level, RandomItemType type, RandomItemPredicate* predicate) const
{
    level = NormalizeLevel(level);

    RandomItemList const list = Query(level, type, predicate);
    if (list.empty())
        return 0;

    //By leewheel 2026-07-11: 避免宏双重展开, 直接用SelectRandomContainerElement
    return SelectRandomContainerElement(list);
    //End By leewheel
}

uint32 RandomItemMgr::GetAmmo(uint32 level, uint32 subClass) const
{
    level = NormalizeLevel(level);

    auto const levelItr = ammoCache.find(level);
    if (levelItr == ammoCache.end())
        return 0;

    auto const subItr = levelItr->second.find(subClass);
    if (subItr == levelItr->second.end() || subItr->second.empty())
        return 0;

    std::vector<uint32> const& ammo = subItr->second;
    if (!sPlayerbotAIConfig.limitGearExpansion)
        return ammo.front();

    static constexpr uint32 EXPANSION_ITEM_ID_TBC   = 23728; // approx. first item in TBC content (patch 2.0)
    static constexpr uint32 EXPANSION_ITEM_ID_WOTLK = 35570; // approx. first item in WotLK content (patch 3.0)
    uint32 const maxEntryId = level <= 60 ? EXPANSION_ITEM_ID_TBC :
                              level <= 70 ? EXPANSION_ITEM_ID_WOTLK :
                              std::numeric_limits<uint32>::max();

    for (uint32 entry : ammo)
    {
        if (entry < maxEntryId)
            return entry;
    }

    return 0;
}

uint32 RandomItemMgr::GetRandomPotion(uint32 level, uint32 effect) const
{
    level = NormalizeLevel(level);

    auto const levelItr = potionCache.find(level);
    if (levelItr == potionCache.end())
        return 0;

    auto const effectItr = levelItr->second.find(effect);
    if (effectItr == levelItr->second.end() || effectItr->second.empty())
        return 0;

    //By leewheel 2026-07-11: 避免宏双重展开
    return SelectRandomContainerElement(effectItr->second);
    //End By leewheel
}

uint32 RandomItemMgr::GetRandomFood(uint32 level, uint32 category) const
{
    level = NormalizeLevel(level);

    uint32 const bucket = (level - 1) / 10;

    auto const levelItr = foodCache.find(bucket);
    if (levelItr == foodCache.end())
        return 0;

    auto const catItr = levelItr->second.find(category);
    if (catItr == levelItr->second.end() || catItr->second.empty())
        return 0;

    //By leewheel 2026-07-11: 避免宏双重展开
    return SelectRandomContainerElement(catItr->second);
    //End By leewheel
}

uint32 RandomItemMgr::GetRandomTrade(uint32 level) const
{
    level = NormalizeLevel(level);
    uint32 const bucket = (level - 1) / 10;

    auto const itr = tradeCache.find(bucket);
    if (itr == tradeCache.end() || itr->second.empty())
        return 0;

    //By leewheel 2026-07-11: 避免宏双重展开
    return SelectRandomContainerElement(itr->second);
    //End By leewheel
}

float RandomItemMgr::GetItemRarity(uint32 itemId) const
{
    auto const itr = rarityCache.find(itemId);
    return itr != rarityCache.end() ? itr->second : 0.0f;
}

std::vector<uint32> const& RandomItemMgr::GetEnchantmentPool(uint32 entry) const
{
    static std::vector<uint32> const empty;
    auto it = enchPoolCache.find(entry);
    if (it == enchPoolCache.end())
        return empty;
    return it->second;
}

bool RandomItemMgr::CanEquipArmor(ItemTemplate const* proto, uint8 clazz, uint32 level) const
{
    // skip null proto or invalid class
    if (!proto || clazz >= MAX_CLASSES)
        return false;

    // skip non-armor and out-of-range armor subclasses
    if (proto->GetClass() != ITEM_CLASS_ARMOR || proto->GetSubClass() >= MAX_ITEM_SUBCLASS_ARMOR)
        return false;

    // skip items with no viable equip slot
    if (!GetViableSlots(static_cast<InventoryType>(proto->GetInventoryType())))
        return false;

    // 披风和徽章总是可装备
    if (proto->GetInventoryType() == INVTYPE_TABARD)
        return true;

    if ((clazz == CLASS_WARRIOR || clazz == CLASS_PALADIN || clazz == CLASS_SHAMAN) &&
        proto->GetSubClass() == ITEM_SUBCLASS_ARMOR_SHIELD)
        return true;

    uint32 const requiredSubClass = [&]() -> uint32
    {
        if (clazz == CLASS_WARRIOR || clazz == CLASS_PALADIN)
            return level >= 40 ? ITEM_SUBCLASS_ARMOR_PLATE : ITEM_SUBCLASS_ARMOR_MAIL;
        if (clazz == CLASS_HUNTER || clazz == CLASS_SHAMAN)
            return level >= 40 ? ITEM_SUBCLASS_ARMOR_MAIL : ITEM_SUBCLASS_ARMOR_LEATHER;
        if (clazz == CLASS_DRUID || clazz == CLASS_ROGUE)
            return ITEM_SUBCLASS_ARMOR_LEATHER;
        return ITEM_SUBCLASS_ARMOR_CLOTH; // mage/warlock/priest
    }();

    // skip armor of wrong subclass (cloaks are exempt)
    if (proto->GetInventoryType() != INVTYPE_CLOAK && proto->GetSubClass() != requiredSubClass)
        return false;

    // skip stats calculation for poor/normal quality items (grey/white)
    if (proto->GetQuality() < ITEM_QUALITY_UNCOMMON)
        return true;

    uint8 sp = 0;
    uint8 ap = 0;
    uint8 tank = 0;

    for (uint8 j = 0; j < MAX_ITEM_PROTO_STATS; ++j)
    {
        // skip items with no stats value
        if (!proto->GetStatModifierBonusAmount(j))
            continue;

        AddItemStats(proto->GetStatModifierBonusStat(j), sp, ap, tank);
    }

    return CanUseItemStats(clazz, sp, ap, tank);
}

bool RandomItemMgr::CanEquipWeapon(ItemTemplate const* proto, uint8 clazz) const
{
    // skip null proto or invalid class
    if (!proto || clazz >= MAX_CLASSES)
        return false;

    // skip non-weapon and out-of-range weapon subclasses
    if (proto->GetClass() != ITEM_CLASS_WEAPON || proto->GetSubClass() >= MAX_ITEM_SUBCLASS_WEAPON)
        return false;

    // skip items with no viable equip slot
    if (!GetViableSlots(static_cast<InventoryType>(proto->GetInventoryType())))
        return false;

    // 跳过职业不精通的武器
    return (m_weaponProficiency[clazz] & (1u << proto->GetSubClass())) != 0;
}

bool RandomItemMgr::ShouldEquipArmorForSpec(ItemTemplate const* proto, uint8 clazz, uint8 spec) const
{
    if (!proto)
        return false;

    // skip additional checks for tabards - always equippable
    if (proto->GetInventoryType() == INVTYPE_TABARD)
        return true;

    std::unordered_map<uint8, WeightScale> const& specMap = m_weightScales[clazz];
    auto const itr = specMap.find(spec);
    if (itr == specMap.end() || !itr->second.info.id)
        return false;

    WeightScale const& scale = itr->second;
    std::string const& specName = scale.info.name;
    uint32 const subClass = proto->GetSubClass();
    bool const isHoldable = proto->GetInventoryType() == INVTYPE_HOLDABLE;

    switch (clazz)
    {
        case CLASS_DEATH_KNIGHT:
            if (isHoldable)
                return false;
            return subClass == ITEM_SUBCLASS_ARMOR_SIGIL || subClass == ITEM_SUBCLASS_ARMOR_PLATE;

        case CLASS_WARRIOR:
            if (isHoldable)
                return false;
            if (specName == "arms" || specName == "fury")
                return subClass == ITEM_SUBCLASS_ARMOR_LEATHER || subClass == ITEM_SUBCLASS_ARMOR_MAIL ||
                       subClass == ITEM_SUBCLASS_ARMOR_PLATE;
            return subClass == ITEM_SUBCLASS_ARMOR_MAIL || subClass == ITEM_SUBCLASS_ARMOR_PLATE;

        case CLASS_PALADIN:
            if (specName != "holy")
            {
                if (isHoldable)
                    return false;

                return subClass == ITEM_SUBCLASS_ARMOR_MAIL || subClass == ITEM_SUBCLASS_ARMOR_PLATE ||
                       subClass == ITEM_SUBCLASS_ARMOR_LIBRAM;
            }
            return subClass == ITEM_SUBCLASS_ARMOR_CLOTH || subClass == ITEM_SUBCLASS_ARMOR_LEATHER ||
                   subClass == ITEM_SUBCLASS_ARMOR_MAIL  || subClass == ITEM_SUBCLASS_ARMOR_PLATE   ||
                   subClass == ITEM_SUBCLASS_ARMOR_LIBRAM;

        case CLASS_HUNTER:
            if (isHoldable)
                return false;
            return subClass == ITEM_SUBCLASS_ARMOR_CLOTH || subClass == ITEM_SUBCLASS_ARMOR_LEATHER ||
                   subClass == ITEM_SUBCLASS_ARMOR_MAIL;

        case CLASS_ROGUE:
            if (isHoldable)
                return false;
            return subClass == ITEM_SUBCLASS_ARMOR_CLOTH || subClass == ITEM_SUBCLASS_ARMOR_LEATHER;

        case CLASS_PRIEST:
        case CLASS_MAGE:
        case CLASS_WARLOCK:
            return subClass == ITEM_SUBCLASS_ARMOR_CLOTH;

        case CLASS_SHAMAN:
            if (specName == "enhance" && isHoldable)
                return false;
            return subClass == ITEM_SUBCLASS_ARMOR_TOTEM   || subClass == ITEM_SUBCLASS_ARMOR_CLOTH ||
                   subClass == ITEM_SUBCLASS_ARMOR_LEATHER || subClass == ITEM_SUBCLASS_ARMOR_MAIL;

        case CLASS_DRUID:
            if ((specName == "feraltank" || specName == "feraldps") && isHoldable)
                return false;
            return subClass == ITEM_SUBCLASS_ARMOR_IDOL || subClass == ITEM_SUBCLASS_ARMOR_CLOTH ||
                   subClass == ITEM_SUBCLASS_ARMOR_LEATHER;

        default:
            return false;
    }
}

bool RandomItemMgr::ShouldEquipWeaponForSpec(ItemTemplate const* proto, uint8 clazz, uint8 spec) const
{
    if (!proto)
        return false;

    std::unordered_map<uint8, WeightScale> const& specMap = m_weightScales[clazz];
    auto const itr = specMap.find(spec);
    if (itr == specMap.end() || !itr->second.info.id)
        return false;

    bool hasMH = false, hasOH = false, hasRH = false;

    if (auto const* slots = GetViableSlots(static_cast<InventoryType>(proto->GetInventoryType())))
    {
        for (EquipmentSlots slot : *slots)
        {
            if (slot == EQUIPMENT_SLOT_MAINHAND) hasMH = true;
            if (slot == EQUIPMENT_SLOT_OFFHAND)  hasOH = true;
            if (slot == EQUIPMENT_SLOT_RANGED)   hasRH = true;
        }
    }

    if (!hasMH && !hasOH && !hasRH)
        return false;

    WeightScale const& scale = itr->second;
    std::string const& specName = scale.info.name;
    uint32 const subClass = proto->GetSubClass();

    auto inSlot = [&](bool hasSlot, std::initializer_list<uint32> list) {
        if (!hasSlot) return false;
        for (uint32 sc : list) if (sc == subClass) return true;
        return false;
    };

    switch (clazz)
    {
        case CLASS_DEATH_KNIGHT:
            return inSlot(hasMH, {ITEM_SUBCLASS_WEAPON_SWORD2, ITEM_SUBCLASS_WEAPON_AXE2, ITEM_SUBCLASS_WEAPON_MACE2,
                                  ITEM_SUBCLASS_WEAPON_POLEARM}) ||
                   inSlot(hasRH, {ITEM_SUBCLASS_ARMOR_SIGIL});

        case CLASS_WARRIOR:
            if (specName == "prot")
                return inSlot(hasMH, {ITEM_SUBCLASS_WEAPON_SWORD, ITEM_SUBCLASS_WEAPON_AXE, ITEM_SUBCLASS_WEAPON_MACE,
                                      ITEM_SUBCLASS_WEAPON_DAGGER, ITEM_SUBCLASS_WEAPON_FIST}) ||
                       inSlot(hasOH, {ITEM_SUBCLASS_ARMOR_SHIELD}) ||
                       inSlot(hasRH, {ITEM_SUBCLASS_WEAPON_BOW, ITEM_SUBCLASS_WEAPON_CROSSBOW, ITEM_SUBCLASS_WEAPON_GUN});
            return inSlot(hasMH, {ITEM_SUBCLASS_WEAPON_SWORD2, ITEM_SUBCLASS_WEAPON_AXE2, ITEM_SUBCLASS_WEAPON_MACE2,
                                  ITEM_SUBCLASS_WEAPON_STAFF, ITEM_SUBCLASS_WEAPON_POLEARM}) ||
                   inSlot(hasRH, {ITEM_SUBCLASS_WEAPON_BOW, ITEM_SUBCLASS_WEAPON_CROSSBOW, ITEM_SUBCLASS_WEAPON_GUN});

        case CLASS_PALADIN:
            if (specName == "prot")
                return inSlot(hasMH, {ITEM_SUBCLASS_WEAPON_SWORD, ITEM_SUBCLASS_WEAPON_AXE, ITEM_SUBCLASS_WEAPON_MACE}) ||
                       inSlot(hasOH, {ITEM_SUBCLASS_ARMOR_SHIELD}) ||
                       inSlot(hasRH, {ITEM_SUBCLASS_ARMOR_LIBRAM});
            if (specName == "holy")
                return inSlot(hasMH, {ITEM_SUBCLASS_WEAPON_SWORD, ITEM_SUBCLASS_WEAPON_AXE, ITEM_SUBCLASS_WEAPON_MACE}) ||
                       inSlot(hasOH, {ITEM_SUBCLASS_ARMOR_SHIELD, ITEM_SUBCLASS_ARMOR_MISC}) ||
                       inSlot(hasRH, {ITEM_SUBCLASS_ARMOR_LIBRAM});
            return inSlot(hasMH, {ITEM_SUBCLASS_WEAPON_SWORD2, ITEM_SUBCLASS_WEAPON_AXE2, ITEM_SUBCLASS_WEAPON_MACE2,
                                  ITEM_SUBCLASS_WEAPON_POLEARM}) ||
                   inSlot(hasRH, {ITEM_SUBCLASS_ARMOR_LIBRAM});

        case CLASS_HUNTER:
            return inSlot(hasMH, {ITEM_SUBCLASS_WEAPON_SWORD2, ITEM_SUBCLASS_WEAPON_AXE2, ITEM_SUBCLASS_WEAPON_STAFF,
                                  ITEM_SUBCLASS_WEAPON_POLEARM}) ||
                   inSlot(hasRH, {ITEM_SUBCLASS_WEAPON_BOW, ITEM_SUBCLASS_WEAPON_CROSSBOW, ITEM_SUBCLASS_WEAPON_GUN});

        case CLASS_ROGUE:
            return inSlot(hasMH, {ITEM_SUBCLASS_WEAPON_DAGGER}) ||
                   inSlot(hasOH, {ITEM_SUBCLASS_WEAPON_DAGGER}) ||
                   inSlot(hasRH, {ITEM_SUBCLASS_WEAPON_BOW, ITEM_SUBCLASS_WEAPON_CROSSBOW, ITEM_SUBCLASS_WEAPON_GUN});

        case CLASS_PRIEST:
            return inSlot(hasMH, {ITEM_SUBCLASS_WEAPON_STAFF, ITEM_SUBCLASS_WEAPON_DAGGER, ITEM_SUBCLASS_WEAPON_MACE}) ||
                   inSlot(hasOH, {ITEM_SUBCLASS_ARMOR_MISC}) ||
                   inSlot(hasRH, {ITEM_SUBCLASS_WEAPON_WAND});

        case CLASS_SHAMAN:
            if (specName == "resto")
                return inSlot(hasMH, {ITEM_SUBCLASS_WEAPON_DAGGER, ITEM_SUBCLASS_WEAPON_AXE, ITEM_SUBCLASS_WEAPON_MACE,
                                      ITEM_SUBCLASS_WEAPON_FIST}) ||
                       inSlot(hasOH, {ITEM_SUBCLASS_ARMOR_MISC, ITEM_SUBCLASS_ARMOR_SHIELD}) ||
                       inSlot(hasRH, {ITEM_SUBCLASS_ARMOR_TOTEM});
            if (specName == "enhance")
                return inSlot(hasMH, {ITEM_SUBCLASS_WEAPON_MACE2, ITEM_SUBCLASS_WEAPON_AXE2, ITEM_SUBCLASS_WEAPON_DAGGER,
                                      ITEM_SUBCLASS_WEAPON_AXE, ITEM_SUBCLASS_WEAPON_MACE, ITEM_SUBCLASS_WEAPON_FIST}) ||
                       inSlot(hasOH, {ITEM_SUBCLASS_ARMOR_SHIELD}) ||
                       inSlot(hasRH, {ITEM_SUBCLASS_ARMOR_TOTEM});
            return inSlot(hasMH, {ITEM_SUBCLASS_WEAPON_STAFF}) ||
                   inSlot(hasRH, {ITEM_SUBCLASS_ARMOR_TOTEM});

        case CLASS_MAGE:
        case CLASS_WARLOCK:
            return inSlot(hasMH, {ITEM_SUBCLASS_WEAPON_STAFF, ITEM_SUBCLASS_WEAPON_DAGGER, ITEM_SUBCLASS_WEAPON_SWORD}) ||
                   inSlot(hasOH, {ITEM_SUBCLASS_ARMOR_MISC}) ||
                   inSlot(hasRH, {ITEM_SUBCLASS_WEAPON_WAND});

        case CLASS_DRUID:
            if (specName == "feraltank" || specName == "feraldps")
                return inSlot(hasMH, {ITEM_SUBCLASS_WEAPON_STAFF, ITEM_SUBCLASS_WEAPON_MACE2}) ||
                       inSlot(hasRH, {ITEM_SUBCLASS_ARMOR_IDOL});
            if (specName == "resto")
                return inSlot(hasMH, {ITEM_SUBCLASS_WEAPON_STAFF, ITEM_SUBCLASS_WEAPON_DAGGER, ITEM_SUBCLASS_WEAPON_FIST,
                                      ITEM_SUBCLASS_WEAPON_MACE}) ||
                       inSlot(hasOH, {ITEM_SUBCLASS_ARMOR_MISC}) ||
                       inSlot(hasRH, {ITEM_SUBCLASS_ARMOR_IDOL});
            return inSlot(hasMH, {ITEM_SUBCLASS_WEAPON_STAFF}) ||
                   inSlot(hasRH, {ITEM_SUBCLASS_ARMOR_IDOL});

        default:
            return false;
    }
}

uint32 RandomItemMgr::GetQuestIdForItem(uint32 itemId) const
{
    for (auto const& [id, quest] : sObjectMgr->GetQuestTemplates())
    {
        for (uint32 i = 0; i < quest->GetRewItemsCount(); ++i)
        {
            if (quest->RewardItemId[i] == itemId)
                return quest->GetQuestId();
        }

        for (uint32 i = 0; i < quest->GetRewChoiceItemsCount(); ++i)
        {
            if (quest->RewardChoiceItemId[i] == itemId)
                return quest->GetQuestId();
        }
    }

    return 0;
}

std::vector<uint32> RandomItemMgr::GetQuestIdsForItem(uint32 itemId) const
{
    std::vector<uint32> questIds;

    for (auto const& [id, quest] : sObjectMgr->GetQuestTemplates())
    {
        for (uint32 i = 0; i < quest->GetRewItemsCount(); ++i)
        {
            if (quest->RewardItemId[i] == itemId)
            {
                questIds.push_back(quest->GetQuestId());
                break;
            }
        }

        for (uint32 i = 0; i < quest->GetRewChoiceItemsCount(); ++i)
        {
            //By leewheel 2026-07-11: TC中quest是值引用不是指针, 用.而不是->
            if (quest->RewardChoiceItemId[i] == itemId)
            {
                questIds.push_back(quest->GetQuestId());
                break;
            }
            //End By leewheel 2026-07-11
        }
    }

    return questIds;
}

bool RandomItemMgr::IsInternalItem(ItemTemplate const* proto)
{
    if (!proto)
        return false;

    if (proto->HasFlag(ITEM_FLAG_DEPRECATED))
        return true;

    //By leewheel 2026-07-11: TC的GetName()返回const char*, 不需要.c_str()
    char const* name = ItemTemplate_GetName(proto);
    //End By leewheel 2026-07-11
    return strstr(name, "NPC ")        ||  // 4859
           strstr(name, "怪物 ")        ||  // 574
           strstr(name, "测试")        ||  // 427
           strstr(name, "已弃用 ") ||  // 399
           strstr(name, "[占位]")        ||  // 138
           strstr(name, "测试")        ||  // 108
           strstr(name, "未使用 ")     ||  // 19
           strstr(name, "旧版")       ||  // 11
           strstr(name, "(测试)")      ||  // 8
           strstr(name, "(旧)")       ||  // 7
           strstr(name, "QR")          ||  // 5
           strstr(name, "2200 ");          // 4
}

bool RandomItemMgr::IsValidItem(ItemTemplate const* proto)
{
    if (!proto)
        return false;

    // 检查物品等级为0的物品
    if (proto->GetItemLevel() == 0)
        return false;

    // 检查临时/过期物品
    if (proto->GetDuration())
        return false;

    // 检查区域或地图限制物品
    if (proto->GetArea(0) || proto->GetMap())
        return false;

    // 检查职业限制物品
    if ((proto->GetAllowableClass() & CLASSMASK_ALL_PLAYABLE) == 0)
        return false;

    constexpr uint32 RACEMASK_ALL_PLAYABLE =
        (1 << (RACE_HUMAN - 1))         |
        (1 << (RACE_ORC - 1))           |
        (1 << (RACE_DWARF - 1))         |
        (1 << (RACE_NIGHTELF - 1))      |
        (1 << (RACE_UNDEAD_PLAYER - 1)) |
        (1 << (RACE_TAUREN - 1))        |
        (1 << (RACE_GNOME - 1))         |
        (1 << (RACE_TROLL - 1))         |
        (1 << (RACE_BLOODELF - 1))      |
        (1 << (RACE_DRAENEI - 1));

    // check race-restricted items
    //By leewheel 2026-09-09: TC-Cata使用GetAllowableRace()，返回RaceMask<int64>，通过RawValue访问原始值
    if ((proto->GetAllowableRace().RawValue & RACEMASK_ALL_PLAYABLE) == 0)
    //End By leewheel
        return false;

    // check test/internal items
    if (IsInternalItem(proto))
        return false;

    // skip items flagged as unobtainable
    if (sPlayerbotAIConfig.unobtainableItems.contains(proto->GetId()))
        return false;

    return true;
}

static bool IsCraftedBySpellInfo(uint32 itemId, SpellInfo const* spellInfo)
{
    for (uint8 x = 0; x < MAX_SPELL_REAGENTS; ++x)
    {
        if (spellInfo->Reagent[x] <= 0)
            continue;

        if (itemId == static_cast<uint32>(spellInfo->Reagent[x]))
            return true;
    }

    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
    {
        if (spellInfo->GetEffects()[i].Effect == SPELL_EFFECT_CREATE_ITEM &&
            spellInfo->GetEffects()[i].ItemType == itemId)
            return true;
    }

    return false;
}

static bool IsCraftedBySpell(uint32 itemId, uint32 spellId)
{
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);
    if (!spellInfo)
        return false;

    if (IsCraftedBySpellInfo(itemId, spellInfo))
        return true;

    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
    {
        SpellInfo const* triggerSpellInfo = sSpellMgr->GetSpellInfo(spellInfo->GetEffects()[i].TriggerSpell);
        if (!triggerSpellInfo)
            continue;

        if (IsCraftedBySpellInfo(itemId, triggerSpellInfo))
            return true;
    }

    return false;
}

static bool IsCraftedBySkill(uint32 itemId, uint32 skillId)
{
    //By leewheel 2026-07-11: TC使用sDB2Manager.GetSkillLineAbilitiesBySkill返回指针, 可能为nullptr
    if (auto skillLineAbilities = sDB2Manager.GetSkillLineAbilitiesBySkill(skillId))
    {
        for (SkillLineAbilityEntry const* skillLine : *skillLineAbilities)
        {
            if (IsCraftedBySpell(itemId, skillLine->Spell))
                return true;
        }
    }
    //End By leewheel 2026-07-11

    //By leewheel 2026-07-11: TC没有GetItemTemplateStoreFast, 使用GetItemTemplateStore()+取地址
    ItemTemplateContainer const* itemTemplates = &sObjectMgr->GetItemTemplateStore();
    for (auto const& recipeItr : *itemTemplates)
    {
        ItemTemplate const* recipe = &recipeItr.second;
    //End By leewheel 2026-07-11
        if (!recipe || recipe->GetClass() != ITEM_CLASS_RECIPE)
            continue;

        uint32 skillType = 0;
        switch (recipe->GetSubClass())
        {
            case ITEM_SUBCLASS_LEATHERWORKING_PATTERN: skillType = SKILL_LEATHERWORKING; break;
            case ITEM_SUBCLASS_TAILORING_PATTERN:      skillType = SKILL_TAILORING; break;
            case ITEM_SUBCLASS_ENGINEERING_SCHEMATIC:  skillType = SKILL_ENGINEERING; break;
            case ITEM_SUBCLASS_BLACKSMITHING:          skillType = SKILL_BLACKSMITHING; break;
            case ITEM_SUBCLASS_COOKING_RECIPE:         skillType = SKILL_COOKING; break;
            case ITEM_SUBCLASS_ALCHEMY_RECIPE:         skillType = SKILL_ALCHEMY; break;
            case ITEM_SUBCLASS_FIRST_AID_MANUAL:       skillType = SKILL_FIRST_AID; break;
            case ITEM_SUBCLASS_ENCHANTING_FORMULA:     skillType = SKILL_ENCHANTING; break;
            case ITEM_SUBCLASS_JEWELCRAFTING_RECIPE:   skillType = SKILL_JEWELCRAFTING; break;
            case ITEM_SUBCLASS_FISHING_MANUAL:         skillType = SKILL_FISHING; break;
            default: break;
        }

        if (!skillType)
            continue;

        //By leewheel 2026-08-30: 修复——TC 343 配方物品的教学法术在 Effects 中, Spells[] 恒为空
        for (ItemEffectEntry const* effect : recipe->Effects)
        {
            if (effect && IsCraftedBySpell(itemId, effect->SpellID))
                return true;
        }
        //End By leewheel
    }

    return false;
}

bool RandomItemMgr::IsUsedBySkill(ItemTemplate const* proto, uint32 skillId)
{
    if (!proto)
        return false;

    if (itemCache.contains(proto->GetId()))
        return true;

    switch (proto->GetClass())
    {
        case ITEM_CLASS_TRADE_GOODS:
        case ITEM_CLASS_MISC:
        case ITEM_CLASS_REAGENT:
        case ITEM_CLASS_GEM:
            if (IsCraftedBySkill(proto->GetId(), skillId))
            {
                itemCache.insert(proto->GetId());
                return true;
            }
            break;
        default:
            break;
    }

    return false;
}

static uint32 GetWeaponSkillId(uint32 subClass)
{
    switch (subClass)
    {
        case ITEM_SUBCLASS_WEAPON_AXE:      return SKILL_AXES;
        case ITEM_SUBCLASS_WEAPON_AXE2:     return SKILL_2H_AXES;
        case ITEM_SUBCLASS_WEAPON_BOW:      return SKILL_BOWS;
        case ITEM_SUBCLASS_WEAPON_GUN:      return SKILL_GUNS;
        case ITEM_SUBCLASS_WEAPON_MACE:     return SKILL_MACES;
        case ITEM_SUBCLASS_WEAPON_MACE2:    return SKILL_2H_MACES;
        case ITEM_SUBCLASS_WEAPON_POLEARM:  return SKILL_POLEARMS;
        case ITEM_SUBCLASS_WEAPON_SWORD:    return SKILL_SWORDS;
        case ITEM_SUBCLASS_WEAPON_SWORD2:   return SKILL_2H_SWORDS;
        case ITEM_SUBCLASS_WEAPON_STAFF:    return SKILL_STAVES;
        case ITEM_SUBCLASS_WEAPON_FIST:     return SKILL_FIST_WEAPONS;
        case ITEM_SUBCLASS_WEAPON_DAGGER:   return SKILL_DAGGERS;
        case ITEM_SUBCLASS_WEAPON_THROWN:   return SKILL_THROWN;
        case ITEM_SUBCLASS_WEAPON_CROSSBOW: return SKILL_CROSSBOWS;
        case ITEM_SUBCLASS_WEAPON_WAND:     return SKILL_WANDS;
        default:                            return SKILL_NONE;
    }
}

void RandomItemMgr::InitWeaponProficiency()
{
    m_weaponProficiency.fill(0);

    for (uint32 subClass = 0; subClass < MAX_ITEM_SUBCLASS_WEAPON; ++subClass)
    {
        uint32 const skillId = GetWeaponSkillId(subClass);
        if (!skillId)
            continue; // ITEM_SUBCLASS_WEAPON_SPEAR, FISHING_POLE, OBSOLETE, etc.

        for (uint8 cls = CLASS_WARRIOR; cls < MAX_CLASSES; ++cls)
        {
            if (!((1u << (cls - 1)) & CLASSMASK_ALL_PLAYABLE))
                continue;

            // Weapon proficiencies in WotLK are class-based, not race-based, so
            // using RACE_HUMAN here is acceptable as a placeholder. Otherwise,
            // method GetSkillRaceClassInfo should be splitted into two separate
            // methods for race and class.
            if (GetSkillRaceClassInfo(skillId, RACE_HUMAN, cls))
                m_weaponProficiency[cls] |= (1u << subClass);
        }
    }
}

void RandomItemMgr::InitPredicates()
{
    predicates[RANDOM_ITEM_GUILD_TASK] = new RandomItemGuildTaskPredicate();
    predicates[RANDOM_ITEM_GUILD_TASK_REWARD_EQUIP_GREEN] = new RandomItemGuildTaskRewardPredicate(true, false);
    predicates[RANDOM_ITEM_GUILD_TASK_REWARD_EQUIP_BLUE] = new RandomItemGuildTaskRewardPredicate(true, true);
    predicates[RANDOM_ITEM_GUILD_TASK_REWARD_TRADE] = new RandomItemGuildTaskRewardPredicate(false, false);
    predicates[RANDOM_ITEM_GUILD_TASK_REWARD_TRADE_RARE] = new RandomItemGuildTaskRewardPredicate(false, true);
}

void RandomItemMgr::InitViableSlots()
{
    auto addEquipmentSlot = [&](InventoryType invType, EquipmentSlots slot)
    {
        viableSlots[invType].push_back(slot);
    };

    addEquipmentSlot(INVTYPE_HEAD,           EQUIPMENT_SLOT_HEAD);
    addEquipmentSlot(INVTYPE_NECK,           EQUIPMENT_SLOT_NECK);
    addEquipmentSlot(INVTYPE_SHOULDERS,      EQUIPMENT_SLOT_SHOULDERS);
    addEquipmentSlot(INVTYPE_BODY,           EQUIPMENT_SLOT_BODY);
    addEquipmentSlot(INVTYPE_CHEST,          EQUIPMENT_SLOT_CHEST);
    addEquipmentSlot(INVTYPE_ROBE,           EQUIPMENT_SLOT_CHEST);
    addEquipmentSlot(INVTYPE_WAIST,          EQUIPMENT_SLOT_WAIST);
    addEquipmentSlot(INVTYPE_LEGS,           EQUIPMENT_SLOT_LEGS);
    addEquipmentSlot(INVTYPE_FEET,           EQUIPMENT_SLOT_FEET);
    addEquipmentSlot(INVTYPE_WRISTS,         EQUIPMENT_SLOT_WRISTS);
    addEquipmentSlot(INVTYPE_HANDS,          EQUIPMENT_SLOT_HANDS);
    addEquipmentSlot(INVTYPE_FINGER,         EQUIPMENT_SLOT_FINGER1);
    addEquipmentSlot(INVTYPE_FINGER,         EQUIPMENT_SLOT_FINGER2);
    addEquipmentSlot(INVTYPE_TRINKET,        EQUIPMENT_SLOT_TRINKET1);
    addEquipmentSlot(INVTYPE_TRINKET,        EQUIPMENT_SLOT_TRINKET2);

    addEquipmentSlot(INVTYPE_WEAPON,         EQUIPMENT_SLOT_MAINHAND);
    addEquipmentSlot(INVTYPE_2HWEAPON,       EQUIPMENT_SLOT_MAINHAND);
    addEquipmentSlot(INVTYPE_WEAPONMAINHAND, EQUIPMENT_SLOT_MAINHAND);

    addEquipmentSlot(INVTYPE_WEAPON,         EQUIPMENT_SLOT_OFFHAND);
    addEquipmentSlot(INVTYPE_WEAPONOFFHAND,  EQUIPMENT_SLOT_OFFHAND);
    addEquipmentSlot(INVTYPE_SHIELD,         EQUIPMENT_SLOT_OFFHAND);
    addEquipmentSlot(INVTYPE_HOLDABLE,       EQUIPMENT_SLOT_OFFHAND);

    addEquipmentSlot(INVTYPE_RANGED,         EQUIPMENT_SLOT_RANGED);
    addEquipmentSlot(INVTYPE_THROWN,         EQUIPMENT_SLOT_RANGED);
    addEquipmentSlot(INVTYPE_RANGEDRIGHT,    EQUIPMENT_SLOT_RANGED);
    addEquipmentSlot(INVTYPE_RELIC,          EQUIPMENT_SLOT_RANGED);

    addEquipmentSlot(INVTYPE_TABARD,         EQUIPMENT_SLOT_TABARD);
    addEquipmentSlot(INVTYPE_CLOAK,          EQUIPMENT_SLOT_BACK);
}

void RandomItemMgr::InitWeightLinks()
{
    // primary stats
    weightStatLink["sta"] = ITEM_MOD_STAMINA;
    weightStatLink["str"] = ITEM_MOD_STRENGTH;
    weightStatLink["agi"] = ITEM_MOD_AGILITY;
    weightStatLink["int"] = ITEM_MOD_INTELLECT;
    weightStatLink["spi"] = ITEM_MOD_SPIRIT;

    // offensive stats
    weightStatLink["splpwr"] = ITEM_MOD_SPELL_POWER;
    weightStatLink["atkpwr"] = ITEM_MOD_ATTACK_POWER;
    weightStatLink["exprtng"] = ITEM_MOD_EXPERTISE_RATING;
    weightStatLink["critstrkrtng"] = ITEM_MOD_CRIT_RATING;
    weightStatLink["hitrtng"] = ITEM_MOD_HIT_RATING;
    weightStatLink["hastertng"] = ITEM_MOD_HASTE_RATING;
    weightStatLink["armorpenrtng"] = ITEM_MOD_ARMOR_PENETRATION_RATING;

    // defensive stats
    weightStatLink["defrtng"] = ITEM_MOD_DEFENSE_SKILL_RATING;
    weightStatLink["dodgertng"] = ITEM_MOD_DODGE_RATING;
    weightStatLink["block"] = ITEM_MOD_BLOCK_VALUE;
    weightStatLink["blockrtng"] = ITEM_MOD_BLOCK_RATING;
    weightStatLink["parryrtng"] = ITEM_MOD_PARRY_RATING;

    // misc
    weightStatLink["manargn"] = ITEM_MOD_MANA_REGENERATION;

    // combat rating links
    weightRatingLink["exprtng"] = CR_EXPERTISE;
    weightRatingLink["critstrkrtng"] = CR_CRIT_MELEE;
    weightRatingLink["hitrtng"] = CR_HIT_MELEE;
    weightRatingLink["hastertng"] = CR_HASTE_MELEE;
    weightRatingLink["armorpenrtng"] = CR_ARMOR_PENETRATION;
    weightRatingLink["defrtng"] = CR_DEFENSE_SKILL;
    weightRatingLink["dodgertng"] = CR_DODGE;
    weightRatingLink["blockrtng"] = CR_BLOCK;
    weightRatingLink["parryrtng"] = CR_PARRY;
}

bool RandomItemMgr::LoadCacheEquip()
{
    uint32 const oldMSTime = getMSTime();

    TC_LOG_INFO("server.loading", "Loading equipment cache...");

    PreparedQueryResult result =
        PlayerbotsDatabase.Query(PlayerbotsDatabase.GetPreparedStatement(PLAYERBOTS_SEL_EQUIP_CACHE));

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 equipment items. DB table `playerbots_equip_cache` is empty!");
        TC_LOG_INFO("server.loading", " ");
        return false;
    }

    uint32 count = 0;
    do
    {
        Field* fields  = result->Fetch();
        uint8  clazz   = fields[0].Get<uint8>();
        uint32 level   = fields[1].Get<uint32>();
        uint8  slot    = fields[2].Get<uint8>();
        uint32 quality = fields[3].Get<uint32>();
        uint32 itemId  = fields[4].Get<uint32>();

        BotEquipKey key(level, clazz, slot, quality);
        equipCache[key].push_back(itemId);

        ++count;
    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded {} equipment records in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    TC_LOG_INFO("server.loading", " ");

    return true;
}

bool RandomItemMgr::LoadCacheRandomItem()
{
    uint32 const oldMSTime = getMSTime();

    TC_LOG_INFO("server.loading", "Loading random item cache...");

    PreparedQueryResult result =
        PlayerbotsDatabase.Query(PlayerbotsDatabase.GetPreparedStatement(PLAYERBOTS_SEL_RNDITEM_CACHE));

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 random items. DB table `playerbots_rnditem_cache` is empty!");
        TC_LOG_INFO("server.loading", " ");
        return false;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();
        uint32 bucket = fields[0].Get<uint32>();
        uint32 type   = fields[1].Get<uint32>();
        uint32 itemId = fields[2].Get<uint32>();

        RandomItemType const rit = static_cast<RandomItemType>(type);

        randomItemCache[bucket][rit].push_back(itemId);

        ++count;
    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded {} random item records in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    TC_LOG_INFO("server.loading", " ");

    return true;
}

bool RandomItemMgr::LoadCacheRarity()
{
    uint32 const oldMSTime = getMSTime();

    TC_LOG_INFO("server.loading", "Loading items rarity cache...");

    PreparedQueryResult result =
        PlayerbotsDatabase.Query(PlayerbotsDatabase.GetPreparedStatement(PLAYERBOTS_SEL_RARITY_CACHE));

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 rarity items. DB table `playerbots_rarity_cache` is empty!");
        TC_LOG_INFO("server.loading", " ");
        return false;
    }

    do
    {
        Field* fields = result->Fetch();
        uint32 itemId = fields[0].Get<uint32>();
        float rarity  = fields[1].Get<float>();

        rarityCache[itemId] = rarity;
    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded {} items rarity records in {} ms", rarityCache.size(),
             GetMSTimeDiffToNow(oldMSTime));
    TC_LOG_INFO("server.loading", " ");

    return true;
}

//By leewheel 2026-07-12: TC使用item_random_enchantment_template表(列Id,EnchantmentId),
//而非AC的item_enchantment_template(列entry,ench)
void RandomItemMgr::LoadEnchantmentPool()
{
    enchPoolCache.clear();

    QueryResult result = WorldDatabase.Query("SELECT Id, EnchantmentId FROM item_random_enchantment_template");
    if (!result)
    {
        LOG_WARN("playerbots", "item_random_enchantment_template empty; bot autogear cannot evaluate random suffixes");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();
        uint32 entry = fields[0].Get<uint32>();
        uint32 ench = fields[1].Get<uint32>();
        enchPoolCache[entry].push_back(ench);
        ++count;
    } while (result->NextRow());

    TC_LOG_INFO("playerbots", "Loaded {} item enchantment pool rows for bot autogear", count);
}
//End By leewheel 2026-07-12

void RandomItemMgr::BuildCacheRandomItem()
{
    uint32 const oldMSTime = getMSTime();

    //By leewheel 2026-07-11: TC的GetItemTemplateStore()返回引用, 需要取地址
    ItemTemplateContainer const* itemTemplates = &sObjectMgr->GetItemTemplateStore();
    //End By leewheel 2026-07-11
    TC_LOG_INFO("server.loading", "Building random item cache from {} items", itemTemplates->size());

    PlayerbotsDatabaseTransaction trans = PlayerbotsDatabase.BeginTransaction();

    uint32 count = 0;
    for (auto const& itr : *itemTemplates)
    {
        ItemTemplate const* proto = &itr.second;

        // skip items that fail basic validatoin (level, duration, flags, etc.)
        if (!IsValidItem(proto))
            continue;

        // skip items with no sell price
        if (proto->GetSellPrice() == 0)
            continue;

        uint32 const level = proto->GetItemLevel();
        uint32 const bucket = (level - 1) / 10;

        for (uint8 type = RANDOM_ITEM_GUILD_TASK; type <= RANDOM_ITEM_GUILD_TASK_REWARD_TRADE_RARE; ++type)
        {
            RandomItemType const rit = static_cast<RandomItemType>(type);
            if (predicates[rit] && !predicates[rit]->Apply(proto))
                continue;

            randomItemCache[bucket][rit].push_back(itr.first);
            ++count;

            PlayerbotsDatabasePreparedStatement* stmt =
                PlayerbotsDatabase.GetPreparedStatement(PLAYERBOTS_INS_RNDITEM_CACHE);
            stmt->SetData(0, bucket);
            stmt->SetData(1, type);
            stmt->SetData(2, itr.first);
            trans->Append(stmt);
        }
    }

    PlayerbotsDatabase.CommitTransaction(trans);

    TC_LOG_INFO("server.loading", ">> Cached total {} random items in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    TC_LOG_INFO("server.loading", " ");
}

void RandomItemMgr::BuildCacheEquip()
{
    uint32 const oldMSTime = getMSTime();

    TC_LOG_INFO("server.loading", "Building equipment cache for {} levels...", DEFAULT_MAX_LEVEL);

    //By leewheel 2026-07-11: TC的GetItemTemplateStore()返回引用, 需要取地址
    ItemTemplateContainer const* itemTemplates = &sObjectMgr->GetItemTemplateStore();
    //End By leewheel 2026-07-11

    PlayerbotsDatabaseTransaction trans = PlayerbotsDatabase.BeginTransaction();

    bool weaponOK = false;
    bool armorOKBelow40Lvl = false;
    bool armorOKAbove40Lvl = false;

    uint32 count = 0;
    for (auto const& itr : *itemTemplates)
    {
        ItemTemplate const* proto = &itr.second;

        // skip non-equipment classes
        if (proto->GetClass() != ITEM_CLASS_WEAPON && proto->GetClass() != ITEM_CLASS_ARMOR)
            continue;

        // skip legendary, artifact and heirloom quality
        if (proto->GetQuality() > ITEM_QUALITY_EPIC)
            continue;

        // skip items that fail basic validatoin (level, duration, flags, etc.)
        if (!IsValidItem(proto))
            continue;

        // skip items with no viable equip slot
        auto const* slots = GetViableSlots(static_cast<InventoryType>(proto->GetInventoryType()));
        if (!slots)
            continue;

        for (uint8 clazz = CLASS_WARRIOR; clazz < MAX_CLASSES; ++clazz)
        {
            // skip non-playable classes
            if (((1u << (clazz - 1)) & CLASSMASK_ALL_PLAYABLE) == 0)
                continue;

            // skip classes that cannot use this item
            if ((proto->GetAllowableClass() & (1u << (clazz - 1))) == 0)
                continue;

            // cache CanEquip results before processing level loop
weaponOK          = proto->GetClass() == ITEM_CLASS_WEAPON && CanEquipWeapon(proto, clazz);
armorOKBelow40Lvl = proto->GetClass() == ITEM_CLASS_ARMOR  && CanEquipArmor(proto, clazz, 1);
armorOKAbove40Lvl = proto->GetClass() == ITEM_CLASS_ARMOR  && CanEquipArmor(proto, clazz, 40);

            for (uint32 level = 1; level <= DEFAULT_MAX_LEVEL; ++level)
            {
                // skip if item cannot be equipped at this level
                if (!CanEquipItem(proto, level))
                    continue;

                // skip weapons the class cannot use
                if (proto->GetClass() == ITEM_CLASS_WEAPON && !weaponOK)
                    continue;

                // skip armor the class cannot use at this level bracket
                if (proto->GetClass() == ITEM_CLASS_ARMOR)
                {
                    if ((level < 40 && !armorOKBelow40Lvl) || (level >= 40 && !armorOKAbove40Lvl))
                        continue;
                }

                for (EquipmentSlots slot : *slots)
                {
                    BotEquipKey key(level, clazz, static_cast<uint8>(slot), proto->GetQuality());

                    equipCache[key].push_back(itr.first);
                    ++count;

                    PlayerbotsDatabasePreparedStatement* stmt =
                        PlayerbotsDatabase.GetPreparedStatement(PLAYERBOTS_INS_EQUIP_CACHE);
                    stmt->SetData(0, clazz);
                    stmt->SetData(1, level);
                    stmt->SetData(2, slot);
stmt->SetData(3, proto->GetQuality());
    stmt->SetData(4, proto->GetId());
                    trans->Append(stmt);
                }
            }
        }
    }

    PlayerbotsDatabase.CommitTransaction(trans);

    TC_LOG_INFO("server.loading", ">> Cached total {} equipment entries in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    TC_LOG_INFO("server.loading", " ");
}

void RandomItemMgr::BuildCacheEquipNew()
{
    uint32 const oldMSTime = getMSTime();

    TC_LOG_INFO("server.loading", "Building equipment cache (new) for {} levels...", DEFAULT_MAX_LEVEL);

    std::unordered_set<uint32> questItemIds;
    uint32 count = 0;

    auto processQuestItem = [&](uint32 itemId, int32 itemQuestLevel)
    {
        // skip empty or already-processed item ids
        if (!itemId || questItemIds.contains(itemId))
            return;

        ItemTemplate const* proto = sObjectMgr->GetItemTemplate(itemId);
        if (!proto)
            return;

        // skip non-equipment classes
        if (proto->GetClass() != ITEM_CLASS_WEAPON && proto->GetClass() != ITEM_CLASS_ARMOR)
            return;

        // skip legendary, artifact and heirloom quality
        if (proto->GetQuality() > ITEM_QUALITY_EPIC)
            return;

        // skip items that fail basic validatoin (level, duration, flags, etc.)
        if (!IsValidItem(proto))
            return;

        // skip items with no viable equip slot
        InventoryType invType = static_cast<InventoryType>(proto->GetInventoryType());
        if (!GetViableSlots(invType))
            return;

        uint32 const requiredLevel = static_cast<uint32>(
            std::max(0, std::max(static_cast<int32>(proto->GetBaseRequiredLevel()), itemQuestLevel)));

        equipCacheNew[requiredLevel][invType].push_back(itemId);
        questItemIds.insert(itemId);
        ++count;
    };

    for (auto const& [_, quest] : sObjectMgr->GetQuestTemplates())
    {
        // skip repeatable quests
        //By leewheel 2026-07-11: TC中quest是值引用不是指针
        if (quest->IsRepeatable())
            continue;

        // skip quests with invalid or out-of-range level
        int32 const questLevel = quest->GetQuestLevel();
        if (questLevel <= 0 || static_cast<uint32>(questLevel) > DEFAULT_MAX_LEVEL)
            continue;

        // skip class-restricted quests
        if (Quest_GetRequiredClasses(quest.get()))
            continue;
        //End By leewheel 2026-07-11

        //By leewheel 2026-07-11: TC中quest是值引用不是指针, 用.而不是->
        for (uint32 i = 0; i < quest->GetRewItemsCount(); ++i)
            processQuestItem(quest->RewardItemId[i], questLevel);

        for (uint32 i = 0; i < quest->GetRewChoiceItemsCount(); ++i)
            processQuestItem(quest->RewardChoiceItemId[i], questLevel);
        //End By leewheel 2026-07-11
    }

    //By leewheel 2026-07-11: TC的GetItemTemplateStore()返回引用, 需要取地址
    ItemTemplateContainer const* itemTemplates = &sObjectMgr->GetItemTemplateStore();
    //End By leewheel 2026-07-11
    for (auto const& itr : *itemTemplates)
    {
        ItemTemplate const* proto = &itr.second;

        // skip non-equipment classes
        if (proto->GetClass() != ITEM_CLASS_WEAPON && proto->GetClass() != ITEM_CLASS_ARMOR)
            continue;

        // skip legendary, artifact and heirloom quality
        if (proto->GetQuality() > ITEM_QUALITY_EPIC)
            continue;

        // skip items already added from quests
        if (questItemIds.contains(proto->GetId()))
            continue;

        // skip items that fail basic validatoin (level, duration, flags, etc.)
        if (!IsValidItem(proto))
            continue;

        // skip items with no viable equip slot
        InventoryType invType = static_cast<InventoryType>(proto->GetInventoryType());
        if (!GetViableSlots(invType))
            continue;

        equipCacheNew[proto->GetBaseRequiredLevel()][invType].push_back(proto->GetId());
        ++count;
    }

    TC_LOG_INFO("server.loading", ">> Cached total {} equipment entries in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    TC_LOG_INFO("server.loading", " ");
}

void RandomItemMgr::BuildCacheItemInfo()
{
    // load weightscales
    TC_LOG_INFO("playerbots", "Loading weightscales info");

    uint32 counter = 1;
    uint32 totalcount = 0;
    uint32 statcount = 0;
    uint32 curClass = CLASS_WARRIOR;

    PlayerbotsDatabasePreparedStatement* stmt = PlayerbotsDatabase.GetPreparedStatement(PLAYERBOTS_SEL_WEIGHTSCALES);
    if (PreparedQueryResult result = PlayerbotsDatabase.Query(stmt))
    {
        do
        {
            Field* fields = result->Fetch();
            uint32 id = fields[0].Get<uint32>();
            std::string name = fields[1].Get<std::string>();
            uint8 clazz = fields[2].Get<uint8>();

            if (clazz != curClass)
            {
                counter = 1;
                curClass = clazz;
            }

            WeightScale scale;
            scale.info.id = id;
            scale.info.name = name;
            m_weightScales[clazz][counter] = std::move(scale);
            counter++;
            totalcount++;

        } while (result->NextRow());

        TC_LOG_INFO("playerbots", "Loaded {} weightscale class specs", totalcount);
    }

    stmt = PlayerbotsDatabase.GetPreparedStatement(PLAYERBOTS_SEL_WEIGHTSCALE_DATA);
    if (PreparedQueryResult result = PlayerbotsDatabase.Query(stmt))
    {
        do
        {
            Field* fields = result->Fetch();
            uint32 id = fields[0].Get<uint32>();
            std::string field = fields[1].Get<std::string>();
            uint32 weight = fields[2].Get<uint32>();

            WeightScaleStat stat;
            stat.stat = field;
            stat.weight = weight;

            for (uint8 cls = CLASS_WARRIOR; cls < MAX_CLASSES; ++cls)
            {
                for (uint32 spec = 1; spec < 5; ++spec)
                {
                    if (m_weightScales[cls][spec].info.id == id)
                        m_weightScales[cls][spec].stats.push_back(std::move(stat));
                }
            }
            statcount++;

        } while (result->NextRow());

        TC_LOG_INFO("playerbots", "Loaded {} weightscale stat weights", statcount);
    }

    if (m_weightScales[1].empty())
    {
        TC_LOG_ERROR("playerbots", "Error loading item weight scales");
        return;
    }

    /**
     * DEADCODE: Remove or rewrite the code below in the next refactor. Also
     *           move weightscales loading operations into separate methods.
     */

    // // vendor items
    // TC_LOG_INFO("playerbots", "Loading vendor item list...");

    // std::set<uint32> vendorItems;
    // vendorItems.clear();
    // if (QueryResult result = WorldDatabase.Query("SELECT item FROM npc_vendor"))
    // {
    //     do
    //     {
    //         Field* fields = result->Fetch();
    //         int32 entry = fields[0].Get<int32>();
    //         if (entry <= 0)
    //             continue;

    //         vendorItems.insert(entry);
    //     } while (result->NextRow());
    // }

    // TC_LOG_INFO("playerbots", "Loaded {} vendor items...", vendorItems.size());

    // // calculate drop source
    // TC_LOG_INFO("playerbots", "Loading loot templates...");
    // DropMap* dropMap = new DropMap;

    // if (CreatureTemplateContainer const* creatures = sObjectMgr->GetCreatureTemplates())
    // {
    //     for (CreatureTemplateContainer::const_iterator itr = creatures->begin(); itr != creatures->end(); ++itr)
    //     {
    //         uint32 sEntry = itr->first;
    //         if (LootTemplateAccess const* lTemplateA =
    //                 DropMapValue::GetLootTemplate(ObjectGuid::Create<HighGuid::Unit>(sEntry, uint32(1)), LOOT_CORPSE))
    //             for (auto const& lItem : lTemplateA->Entries)
    //                 dropMap->insert(std::make_pair(lItem->itemid, sEntry));
    //     }
    // }

    // if (GameObjectTemplateContainer const* gameobjects = sObjectMgr->GetGameObjectTemplates())
    // {
    //     for (auto const& itr : *gameobjects)
    //     {
    //         uint32 sEntry = itr.first;
    //         if (LootTemplateAccess const* lTemplateA = DropMapValue::GetLootTemplate(
    //                 ObjectGuid::Create<HighGuid::GameObject>(sEntry, uint32(1)), LOOT_CORPSE))
    //             for (auto const& lItem : lTemplateA->Entries)
    //                 dropMap->insert(std::make_pair(lItem->itemid, sEntry));
    //     }
    // }

    // TC_LOG_INFO("playerbots", "Loaded {} loot templates...", dropMap->size());

    // ItemTemplateContainer const* itemTemplate = sObjectMgr->GetItemTemplateStore();
    // TC_LOG_INFO("playerbots", "Calculating stat weights for {} items...", itemTemplate->size());

    /*PlayerbotsDatabaseTransaction trans = PlayerbotsDatabase.BeginTransaction();

    for (auto const& itr : *itemTemplate)
    {
        ItemTemplate const* proto = &itr.second;

        // // skip items with rank/rep requirements
        // if (proto->GetRequiredHonorRank() > 0 ||
        //     proto->RequiredSkillRank > 0 ||
        //     proto->RequiredCityRank > 0 ||
        //     proto->RequiredReputationRank > 0)
        //     continue;

        // if (proto->GetRequiredHonorRank() > 0 || proto->RequiredSkillRank > 0 || proto->RequiredCityRank > 0)
        //     continue;

        // // skip random enchant items
        // if (proto->RandomProperty)
        //     continue;

        // // skip heirloom items
        // if (proto->Quality == ITEM_QUALITY_HEIRLOOM)
        //     continue;

        // // check possible equip slots
        // EquipmentSlots slot = EQUIPMENT_SLOT_START;
        // for (std::map<EquipmentSlots, std::set<InventoryType> >::iterator i = viableSlots.begin();
        //      i != viableSlots.end(); ++i)
        // {
        //     std::set<InventoryType> slots = viableSlots[(EquipmentSlots)i->first];
        //     if (slots.find((InventoryType)proto->InventoryType) != slots.end())
        //         slot = i->first;
        // }

        // if (slot == EQUIPMENT_SLOT_START)
        //     continue;

        // Init Item cache
        // ItemInfoEntry cacheInfo;

        // for (uint8 clazz = CLASS_WARRIOR; clazz < MAX_CLASSES; ++clazz)
        // {
        //     // skip nonexistent classes
        //     if (!((1 << (clazz - 1)) & CLASSMASK_ALL_PLAYABLE) || !sChrClassesStore.LookupEntry(clazz))
        //         continue;

        //     // skip wrong classes
        //     if ((proto->AllowableClass & (1 << (clazz - 1))) == 0)
        //         continue;

        //     for (uint32 spec = 1; spec < 5; ++spec)
        //     {
        //         if (!m_weightScales[clazz][spec].info.id)
        //             continue;

        //         // check possible armor for spec
        //         if (m_weightScales)
        //         if (CreatureTemplate_GetClass(proto) == ITEM_CLASS_ARMOR && (
        //             slot == EQUIPMENT_SLOT_HEAD ||
        //             slot == EQUIPMENT_SLOT_SHOULDERS ||
        //             slot == EQUIPMENT_SLOT_CHEST ||
        //             slot == EQUIPMENT_SLOT_WAIST ||
        //             slot == EQUIPMENT_SLOT_LEGS ||
        //             slot == EQUIPMENT_SLOT_FEET ||
        //             slot == EQUIPMENT_SLOT_WRISTS ||
        //             slot == EQUIPMENT_SLOT_HANDS) &&
        //             !ShouldEquipArmorForSpec(clazz, spec, proto))
        //             continue;

        //         // check possible weapon for spec
        //         if ((CreatureTemplate_GetClass(proto) == ITEM_CLASS_WEAPON || (proto->SubClass == ITEM_SUBCLASS_ARMOR_SHIELD ||
        //         (proto->SubClass == ITEM_SUBCLASS_ARMOR_MISC && proto->InventoryType == INVTYPE_HOLDABLE))) &&
        //             !ShouldEquipWeaponForSpec(clazz, spec, proto))
        //             continue;

        //         StatWeight statWeight;
        //         statWeight.id = m_weightScales[clazz][spec].info.id;
        //         uint32 statW = CalculateStatWeight(clazz, spec, proto);
        //         // set stat weight = 1 for items that can be equipped but have no proper stats
        //         statWeight.weight = statW ? statW : 1;
        //         //statWeight.weight = statW;
        //         // save item statWeight into ItemCache
        //         cacheInfo.weights[statWeight.id] = statWeight.weight;
        //         TC_LOG_DEBUG("playerbots", "Item: {}, weight: {}, class: {}, spec: {}", proto->GetId(),
        //         statWeight.weight, clazz, m_weightScales[clazz][spec].info.name);
        //     }
        // }

        // cacheInfo.team = TEAM_NEUTRAL;

        // // check faction
        // if (proto->HasFlag2(ITEM_FLAG2_FACTION_HORDE))
        //     cacheInfo.team = TEAM_HORDE;

        // if (proto->HasFlag2(ITEM_FLAG2_FACTION_ALLIANCE))
        //     cacheInfo.team = TEAM_ALLIANCE;

        // if (cacheInfo.team == TEAM_NEUTRAL && proto->AllowableRace > 1 && proto->AllowableRace < 8388607)
        // {
        //     if (FactionEntry const* faction = sFactionStore.LookupEntry(HORDE))
        //         if ((proto->AllowableRace & faction->BaseRepRaceMask[0]) != 0)
        //             cacheInfo.team = TEAM_HORDE;

        //     if (FactionEntry const* faction = sFactionStore.LookupEntry(ALLIANCE))
        //         if ((proto->AllowableRace & faction->BaseRepRaceMask[0]) != 0)
        //             cacheInfo.team = TEAM_ALLIANCE;
        // }

        // if (cacheInfo.team < TEAM_NEUTRAL)
        //     TC_LOG_DEBUG("playerbots", "Item: {}, team (item): {}", proto->GetId(), cacheInfo.team == TEAM_ALLIANCE ?
        //     "Alliance" : "Horde");

        // // check min level
        // if (proto->GetBaseRequiredLevel())
        //     cacheInfo.minLevel = proto->GetBaseRequiredLevel();

        // // check item source

        // if (proto->HasFlag(ITEM_FLAG_NO_DISENCHANT))
        // {
        //     cacheInfo.source = ITEM_SOURCE_PVP;
        //     TC_LOG_DEBUG("playerbots", "Item: {}, source: PvP Reward", proto->GetId());
        // }

        // // check quests
        // if (cacheInfo.source == ITEM_SOURCE_NONE)
        // {
        //     std::vector<uint32> questIds = GetQuestIdsForItem(proto->GetId());
        //     if (questIds.size())
        //     {
        //         bool isAlly = false;
        //         bool isHorde = false;
        //         for (std::vector<uint32>::iterator i = questIds.begin(); i != questIds.end(); ++i)
        //         {
        //             Quest const* quest = sObjectMgr->GetQuestTemplate(*i);
        //             if (quest)
        //             {
        //                 cacheInfo.source = ITEM_SOURCE_QUEST;
        //                 cacheInfo.sourceId = *i;
        //                 if (!cacheInfo.minLevel)
        //                     cacheInfo.minLevel = quest->GetMinLevel();

        //                 // check quest team
        //                 if (cacheInfo.team == TEAM_NEUTRAL)
        //                 {
        //                     uint32 reqRace = quest->GetAllowableRaces();
        //                     if (reqRace)
        //                     {
        //                         if ((reqRace & RACEMASK_ALLIANCE) != 0)
        //                             isAlly = true;
        //                         else if ((reqRace & RACEMASK_HORDE) != 0)
        //                             isHorde = true;
        //                     }
        //                 }
        //             }
        //         }

        //         if (isAlly && isHorde)
        //             cacheInfo.team = TEAM_NEUTRAL;
        //         else if (isAlly)
        //             cacheInfo.team = TEAM_ALLIANCE;
        //         else if (isHorde)
        //             cacheInfo.team = TEAM_HORDE;

        //         TC_LOG_DEBUG("playerbots", "Item: {}, team (quest): {}", proto->GetId(), cacheInfo.team == TEAM_ALLIANCE
        //         ? "Alliance" : cacheInfo.team == TEAM_HORDE ? "Horde" : "Both"); TC_LOG_DEBUG("playerbots", "Item: {},
        //         source: quest {}, minlevel: {}", proto->GetId(), cacheInfo.sourceId, cacheInfo.minLevel);
        //     }
        // }

        // if (cacheInfo.minLevel)
        //     TC_LOG_DEBUG("playerbots", "Item: {}, minlevel: {}", proto->GetId(), cacheInfo.minLevel);

        // // check vendors
        // if (cacheInfo.source == ITEM_SOURCE_NONE)
        // {
        //     for (std::set<uint32>::iterator i = vendorItems.begin(); i != vendorItems.end(); ++i)
        //     {
        //         if (proto->GetId() == *i)
        //         {
        //             cacheInfo.source = ITEM_SOURCE_VENDOR;
        //             TC_LOG_DEBUG("playerbots", "Item: {} source: vendor", proto->GetId());
        //             break;
        //         }
        //     }
        // }

        // // check drops
        // std::vector<int32> creatures;
        // std::vector<int32> gameobjects;
        // auto range = dropMap->equal_range(itr.first);

        // for (auto iter = range.first; iter != range.second; ++iter)
        // {
        //     if (iter->second > 0)
        //         creatures.push_back(iter->second);
        //     else
        //         gameobjects.push_back(abs(iter->second));
        // }

        // // check creature drop
        // if (cacheInfo.source == ITEM_SOURCE_NONE)
        // {
        //     if (creatures.size())
        //     {
        //         if (creatures.size() == 1)
        //         {
        //             cacheInfo.source = ITEM_SOURCE_DROP;
        //             cacheInfo.sourceId = creatures.front();
        //             TC_LOG_DEBUG("playerbots", "Item: {}, source: creature drop, ID: {}", proto->GetId(),
        //             creatures.front());
        //         }
        //         else
        //         {
        //             cacheInfo.source = ITEM_SOURCE_DROP;
        //             TC_LOG_DEBUG("playerbots", "Item: {}, source: creatures drop, number: {}", proto->GetId(),
        //             creatures.size());
        //         }
        //     }
        // }

        // // check gameobject drop
        // if (cacheInfo.source == ITEM_SOURCE_NONE || (cacheInfo.source == ITEM_SOURCE_DROP && !cacheInfo.sourceId))
        // {
        //     if (gameobjects.size())
        //     {
        //         if (gameobjects.size() == 1)
        //         {
        //             cacheInfo.source = ITEM_SOURCE_DROP;
        //             cacheInfo.sourceId = gameobjects.front();
        //             TC_LOG_INFO("playerbots", "Item: {}, source: gameobject, ID: {}", proto->GetId(),
        //             gameobjects.front());
        //         }
        //         else
        //         {
        //             cacheInfo.source = ITEM_SOURCE_DROP;
        //             TC_LOG_INFO("playerbots", "Item: {}, source: gameobjects, number: {}", proto->GetId(),
        //             gameobjects.size());
        //         }
        //     }
        // }

        // // check faction
        // if (proto->RequiredReputationFaction > 0 && proto->RequiredReputationFaction != 35 &&
        // proto->RequiredReputationRank < 15)
        // {
        //     cacheInfo.repFaction = proto->RequiredReputationFaction;
        //     cacheInfo.repRank = proto->RequiredReputationRank;
        // }

        // cacheInfo.quality = proto->Quality;
        // cacheInfo.itemId = proto->GetId();
        // cacheInfo.slot = slot;

        // // save cache
        // PlayerbotsDatabasePreparedStatement* stmt =
        // PlayerbotsDatabase.GetPreparedStatement(PLAYERBOTS_DEL_EQUIP_CACHE_NEW); stmt->SetData(0, proto->GetId());
        // trans->Append(stmt);

        // stmt = PlayerbotsDatabase.GetPreparedStatement(PLAYERBOTS_INS_EQUIP_CACHE_NEW);
        // stmt->SetData(0, cacheInfo.itemId);
        // stmt->SetData(1, cacheInfo.quality);
        // stmt->SetData(2, cacheInfo.slot);
        // stmt->SetData(3, cacheInfo.source);
        // stmt->SetData(4, cacheInfo.sourceId);
        // stmt->SetData(5, cacheInfo.team);
        // stmt->SetData(6, cacheInfo.repFaction);
        // stmt->SetData(7, cacheInfo.repRank);
        // stmt->SetData(8, cacheInfo.minLevel);

        // for (uint8 i = 1; i <= MAX_STAT_SCALES; ++i)
        // {
        //     if (cacheInfo.weights[i])
        //         stmt->SetData(8 + i, cacheInfo.weights[i]);
        //     else
        //         stmt->SetData(8 + i, 0);
        // }

        // trans->Append(stmt);

        // itemInfoCache[cacheInfo.itemId] = std::move(cacheInfo);
    }

    PlayerbotsDatabase.CommitTransaction(trans);*/
}

void RandomItemMgr::BuildCacheAmmo()
{
    uint32 const oldMSTime = getMSTime();

    TC_LOG_INFO("server.loading", "Building ammo cache for {} levels...", DEFAULT_MAX_LEVEL);

    struct AmmoEntry
    {
        uint32 entry;
        uint32 subClass;
        uint32 itemLevel;
        uint32 requiredLevel;
        uint32 stackable;
    };

    std::vector<AmmoEntry> ammoItems;

    //By leewheel 2026-07-11: TC的GetItemTemplateStore()返回引用, 需要取地址
    ItemTemplateContainer const* itemTemplates = &sObjectMgr->GetItemTemplateStore();
    //End By leewheel 2026-07-11
    for (auto const& itr : *itemTemplates)
    {
        ItemTemplate const* proto = &itr.second;

        // skip non-projectile items
        if (proto->GetClass() != ITEM_CLASS_PROJECTILE)
            continue;

        // skip non-arrow and non-bullet subclasses
        if (proto->GetSubClass() != ITEM_SUBCLASS_ARROW && proto->GetSubClass() != ITEM_SUBCLASS_BULLET)
            continue;

        // skip items that fail basic validatoin (level, duration, flags, etc.)
        if (!IsValidItem(proto))
            continue;

        // skip items with no required level
        // NOTE: This filters only three items: 3464 (Feathered Arrow), 3465
        //       (Exploding Shot), and 4960 (Flash Pellet). In theory we could
        //       set RequiredLevel = ItemLevel and use that instead.
        if (proto->GetBaseRequiredLevel() == 0)
            continue;

        // skip items with no damage value
        if (proto->GetMinDamage(0) == 0.0f)
            continue;

        //By leewheel 2026-07-11: GetBaseRequiredLevel()返回int32, 需要显式转换为uint32避免narrowing
        ammoItems.push_back({itr.first, proto->GetSubClass(), proto->GetItemLevel(), uint32(proto->GetBaseRequiredLevel()),
                            proto->GetMaxStackSize()});
        //End By leewheel 2026-07-11
    }

    // we want higher stack sizes first, then higher item levels, to ensure bots
    // use the best available ammo for their current level
    std::sort(ammoItems.begin(), ammoItems.end(), [](AmmoEntry const& a, AmmoEntry const& b)
    {
        if (a.stackable != b.stackable)
            return a.stackable > b.stackable;
        return a.itemLevel > b.itemLevel;
    });

    uint32 count = 0;
    for (uint32 level = 1; level <= DEFAULT_MAX_LEVEL; ++level)
    {
        for (AmmoEntry const& ammo : ammoItems)
        {
            // skip ammo above bot's current level
            if (ammo.requiredLevel > level)
                continue;

            ammoCache[level][ammo.subClass].push_back(ammo.entry);
            ++count;
        }
    }

    TC_LOG_INFO("server.loading", ">> Cached total {} ammo entries in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    TC_LOG_INFO("server.loading", " ");
}

void RandomItemMgr::BuildCacheFood()
{
    uint32 const oldMSTime = getMSTime();

    TC_LOG_INFO("server.loading", "Building food cache for {} levels...", DEFAULT_MAX_LEVEL);

    constexpr uint32 MAX_FOOD_LEVEL_DELTA = 10;

    uint32 count = 0;
    //By leewheel 2026-07-11: TC的GetItemTemplateStore()返回引用, 需要取地址
    ItemTemplateContainer const* itemTemplates = &sObjectMgr->GetItemTemplateStore();
    //End By leewheel 2026-07-11
    for (auto const& itr : *itemTemplates)
    {
        ItemTemplate const* proto = &itr.second;

        // skip non-consumable items
        if (proto->GetClass() != ITEM_CLASS_CONSUMABLE)
            continue;

        // skip non-food and non-consumable subclasses
        if (proto->GetSubClass() != ITEM_SUBCLASS_CONSUMABLE && proto->GetSubClass() != ITEM_SUBCLASS_FOOD)
            continue;

        // skip bound items
        if (proto->GetBonding() != NO_BIND)
            continue;

        // skip items that fail basic validatoin (level, duration, flags, etc.)
        if (!IsValidItem(proto))
            continue;

        // skip items requiring a profession skill
        if (proto->GetRequiredSkill())
            continue;

        // skip items that are neither food nor drink
        if (proto->Effects.empty() || (proto->Effects[0]->SpellCategoryID != SPELL_CATEGORY_FOOD &&
            proto->Effects[0]->SpellCategoryID != SPELL_CATEGORY_DRINK))
            continue;

        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(proto->Effects.empty() ? 0 : proto->Effects[0]->SpellID);
        if (!spellInfo)
            continue;

        // skip items with spell effect inebriate (alcohol)
        if (spellInfo->HasEffect(SPELL_EFFECT_INEBRIATE))
            continue;

        uint32 requiredLevel = proto->GetBaseRequiredLevel();
        uint32 category = proto->Effects.empty() ? 0 : proto->Effects[0]->SpellCategoryID;
        for (uint32 level = 1; level <= DEFAULT_MAX_LEVEL; level += 10)
        {
            if (requiredLevel)
            {
                // skip items above the level bucket
                if (requiredLevel > level)
                    continue;

                // skip items below the level bucket window
                if (level > MAX_FOOD_LEVEL_DELTA && requiredLevel < level - MAX_FOOD_LEVEL_DELTA)
                    continue;
            }

            foodCache[(level - 1) / 10][category].push_back(itr.first);
            ++count;
        }
    }

    TC_LOG_INFO("server.loading", ">> Cached total {} food entries in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    TC_LOG_INFO("server.loading", " ");
}

void RandomItemMgr::BuildCachePotion()
{
    uint32 const oldMSTime = getMSTime();

    TC_LOG_INFO("server.loading", "Building potion cache for {} levels...", DEFAULT_MAX_LEVEL);

    constexpr uint32 MAX_POTION_LEVEL_DELTA = 13;

    uint32 count = 0;
    //By leewheel 2026-07-11: TC的GetItemTemplateStore()返回引用, 需要取地址
    ItemTemplateContainer const* itemTemplates = &sObjectMgr->GetItemTemplateStore();
    //End By leewheel 2026-07-11
    for (auto const& itr : *itemTemplates)
    {
        ItemTemplate const* proto = &itr.second;

        // skip non-consumable items
        if (proto->GetClass() != ITEM_CLASS_CONSUMABLE)
            continue;

        // skip non-potion and non-flask subclasses
        if (proto->GetSubClass() != ITEM_SUBCLASS_POTION && proto->GetSubClass() != ITEM_SUBCLASS_FLASK)
            continue;

        // skip bound items
        if (proto->GetBonding() != NO_BIND)
            continue;

        // skip items that fail basic validatoin (level, duration, flags, etc.)
        if (!IsValidItem(proto))
            continue;

        // skip items requiring a profession skill
        if (proto->GetRequiredSkill())
            continue;

        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(proto->Effects.empty() ? 0 : proto->Effects[0]->SpellID);
        if (!spellInfo)
            continue;

        // skip potions whose primary effect is not heal or energize
        if (spellInfo->GetEffects()[EFFECT_0].Effect != SPELL_EFFECT_HEAL &&
            spellInfo->GetEffects()[EFFECT_0].Effect != SPELL_EFFECT_ENERGIZE)
            continue;

        // do not accept potions/flasks with more than one spell effects, only
        // first one effect (EFFECT_0) should be set and eq. heal/energize
        bool hasOtherEffects = false;
        for (uint8 i = EFFECT_1; i < MAX_SPELL_EFFECTS; ++i)
        {
            if (spellInfo->GetEffects()[i].Effect != 0)
            {
                hasOtherEffects = true;
                break;
            }
        }

        // skip potions with secondary spell effects
        if (hasOtherEffects)
            continue;

        uint32 requiredLevel = proto->GetBaseRequiredLevel();
        uint32 effect = spellInfo->GetEffects()[EFFECT_0].Effect;
        for (uint32 level = std::max(1u, requiredLevel); level <= DEFAULT_MAX_LEVEL; ++level)
        {
            // skip levels outside the potion relevance window
            if (level > MAX_POTION_LEVEL_DELTA && level - requiredLevel > MAX_POTION_LEVEL_DELTA)
                break;

            potionCache[level][effect].push_back(itr.first);
            ++count;
        }
    }

    TC_LOG_INFO("server.loading", ">> Cached total {} potion entries in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    TC_LOG_INFO("server.loading", " ");
}

void RandomItemMgr::BuildCacheTrade()
{
    uint32 const oldMSTime = getMSTime();

    TC_LOG_INFO("server.loading", "Building trade cache for {} levels...", DEFAULT_MAX_LEVEL);

    constexpr uint32 MAX_TRADE_LEVEL_DELTA = 10;

    uint32 count = 0;
    //By leewheel 2026-07-11: TC的GetItemTemplateStore()返回引用, 需要取地址
    ItemTemplateContainer const* itemTemplates = &sObjectMgr->GetItemTemplateStore();
    //End By leewheel 2026-07-11
    for (auto const& itr : *itemTemplates)
    {
        ItemTemplate const* proto = &itr.second;

        // skip non-trade-goods
        if (proto->GetClass() != ITEM_CLASS_TRADE_GOODS)
            continue;

        // skip bound items
        if (proto->GetBonding() != NO_BIND)
            continue;

        // skip items that fail basic validatoin (level, duration, flags, etc.)
        if (!IsValidItem(proto))
            continue;

        // skip items requiring a profession skill
        if (proto->GetRequiredSkill())
            continue;

        uint32 requiredLevel = proto->GetBaseRequiredLevel();
        for (uint32 level = 1; level <= DEFAULT_MAX_LEVEL; level += 10)
        {
            // skip items whose item level is below the current bucket
            if (proto->GetItemLevel() < level)
                continue;

            if (requiredLevel)
            {
                // skip items above the level bucket
                if (requiredLevel > level)
                    continue;

                // skip items below the level bucket window
                if (level > MAX_TRADE_LEVEL_DELTA && requiredLevel < level - MAX_TRADE_LEVEL_DELTA)
                    continue;
            }

            tradeCache[(level - 1) / 10].push_back(itr.first);
            ++count;
        }
    }

    TC_LOG_INFO("server.loading", ">> Cached total {} trade entries in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    TC_LOG_INFO("server.loading", " ");
}

void RandomItemMgr::BuildCacheRarity()
{
    uint32 const oldMSTime = getMSTime();

    TC_LOG_INFO("server.loading", "Building items rarity cache...");

    QueryResult result = WorldDatabase.Query(
        "SELECT item, MAX(q.chance) AS max_chance FROM ( "
        // <-- creature
        "SELECT lt.item, "
        "    AVG(CASE "
        "        WHEN lt.GroupId = 0 THEN lt.Chance "
        "        WHEN lt.Chance > 0  THEN lt.Chance "
        "        ELSE IFNULL(100 - lt.group_sum, 100) / NULLIF(lt.group_zero, 0) "
        "    END) AS chance "
        "FROM ( "
        "    SELECT *, "
        "        SUM(CASE WHEN Chance > 0 THEN Chance ELSE 0 END) OVER (PARTITION BY entry, GroupId) AS group_sum, "
        "        SUM(CASE WHEN Chance = 0 THEN 1 ELSE 0 END)      OVER (PARTITION BY entry, GroupId) AS group_zero "
        "    FROM creature_loot_template WHERE item != 0) lt "
        "JOIN creature_template ct ON ct.LootId = lt.entry "
        //By leewheel 2026-08-09: 移植上游838a337f(#2586)——TC世界库creature表主键为id而非AC的id1，
        //原SQL查询失败导致稀有度缓存(creature部分)失效
        "JOIN creature c ON c.id = ct.entry "
        //End By leewheel
        "GROUP BY lt.item "
        "UNION ALL "
        // <-- gameobject
        "SELECT lt.item, "
        "    AVG(CASE "
        "        WHEN lt.GroupId = 0 THEN lt.Chance "
        "        WHEN lt.Chance > 0  THEN lt.Chance "
        "        ELSE IFNULL(100 - lt.group_sum, 100) / NULLIF(lt.group_zero, 0) "
        "    END) AS chance "
        "FROM (SELECT *, "
        "        SUM(CASE WHEN Chance > 0 THEN Chance ELSE 0 END) OVER (PARTITION BY entry, GroupId) AS group_sum, "
        "        SUM(CASE WHEN Chance = 0 THEN 1 ELSE 0 END)      OVER (PARTITION BY entry, GroupId) AS group_zero "
        "    FROM gameobject_loot_template WHERE item != 0) lt "
        "JOIN gameobject_template ct ON ct.data1 = lt.entry "
        "JOIN gameobject c ON c.id = ct.entry "
        "GROUP BY lt.item "
        "UNION ALL "
        // <-- disenchant
        "SELECT lt.item, "
        "    AVG(CASE "
        "        WHEN lt.GroupId = 0 THEN lt.Chance "
        "        WHEN lt.Chance > 0  THEN lt.Chance "
        "        ELSE IFNULL(100 - lt.group_sum, 100) / NULLIF(lt.group_zero, 0) "
        "    END) AS chance "
        "FROM (SELECT *, "
        "        SUM(CASE WHEN Chance > 0 THEN Chance ELSE 0 END) OVER (PARTITION BY entry, GroupId) AS group_sum, "
        "        SUM(CASE WHEN Chance = 0 THEN 1 ELSE 0 END)      OVER (PARTITION BY entry, GroupId) AS group_zero "
        "    FROM disenchant_loot_template WHERE item != 0) lt "
        "JOIN item_template ct ON ct.DisenchantID = lt.entry "
        "GROUP BY lt.item "
        "UNION ALL "
        // <-- fishing
        "SELECT lt.item, "
        "    AVG(CASE "
        "        WHEN lt.GroupId = 0 THEN lt.Chance "
        "        WHEN lt.Chance > 0  THEN lt.Chance "
        "        ELSE IFNULL(100 - lt.group_sum, 100) / NULLIF(lt.group_zero, 0) "
        "    END) AS chance "
        "FROM (SELECT *, "
        "        SUM(CASE WHEN Chance > 0 THEN Chance ELSE 0 END) OVER (PARTITION BY entry, GroupId) AS group_sum, "
        "        SUM(CASE WHEN Chance = 0 THEN 1 ELSE 0 END)      OVER (PARTITION BY entry, GroupId) AS group_zero "
        "    FROM fishing_loot_template WHERE item != 0) lt "
        "GROUP BY lt.item "
        "UNION ALL "
        // <-- skinning
        "SELECT lt.item, "
        "    AVG(CASE "
        "        WHEN lt.GroupId = 0 THEN lt.Chance "
        "        WHEN lt.Chance > 0  THEN lt.Chance "
        "        ELSE IFNULL(100 - lt.group_sum, 100) * IFNULL(1.0 / NULLIF(lt.group_zero, 0), 1) "
        "    END) AS chance "
        "FROM (SELECT *, "
        "       SUM(CASE WHEN Chance > 0 THEN Chance ELSE 0 END) OVER (PARTITION BY entry, GroupId) AS group_sum, "
        "       SUM(CASE WHEN Chance = 0 THEN 1 ELSE 0 END)      OVER (PARTITION BY entry, GroupId) AS group_zero "
        "   FROM skinning_loot_template WHERE item != 0) lt "
        "JOIN creature_template ct ON ct.skinloot = lt.entry "
        //By leewheel 2026-08-09: 移植上游838a337f(#2586)——同上，skinloot查询的creature连接列修正
        "JOIN creature c ON c.id = ct.entry "
        //End By leewheel
        "GROUP BY lt.item) q GROUP BY item HAVING max_chance > 0.01 ORDER BY item");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Cached 0 rarity entries. SQL query result is empty!");
        TC_LOG_INFO("server.loading", " ");
        return;
    }

    PlayerbotsDatabaseTransaction trans = PlayerbotsDatabase.BeginTransaction();

    do
    {
        Field* fields = result->Fetch();
        uint32 itemId = fields[0].Get<uint32>();
        float  rarity = fields[1].Get<float>();

        ItemTemplate const* proto = sObjectMgr->GetItemTemplate(itemId);
        if (!proto)
            continue;

        // skip poor, legendary, artifact and heirloom quality
        // NOTE: There are only 5 items with loot source and above Epic quality:
        //       Warglaive of Azzinoth (2), Bindings of the Windseeker (2), and
        //       Shadowfrost Shard. The last two are quest-related items.
        if (proto->GetQuality() < ITEM_QUALITY_NORMAL || proto->GetQuality() > ITEM_QUALITY_EPIC)
            continue;

        // skip items that fail basic validatoin (level, duration, flags, etc.)
        if (!IsValidItem(proto))
            continue;

        rarityCache[itemId] = rarity;

        PlayerbotsDatabasePreparedStatement* stmt =
            PlayerbotsDatabase.GetPreparedStatement(PLAYERBOTS_INS_RARITY_CACHE);
        stmt->SetData(0, itemId);
        stmt->SetData(1, rarity);
        trans->Append(stmt);
    } while (result->NextRow());

    PlayerbotsDatabase.CommitTransaction(trans);

    TC_LOG_INFO("server.loading", ">> Cached total {} rarity entries in {} ms", rarityCache.size(),
             GetMSTimeDiffToNow(oldMSTime));
    TC_LOG_INFO("server.loading", " ");
}

void RandomItemMgr::DebugCacheRandomItem()
{
    // NOTE: Not sure if anyone ever used this data, because it produces a huge
    //       amount of output to the file/console. It might be better to create
    //       separate command that shows the cache contents on demand.

    if (!sLog->ShouldLog("playerbots", LogLevel::LOG_LEVEL_DEBUG))
        return;

    uint32 const maxBucket = (DEFAULT_MAX_LEVEL - 1) / 10;

    for (uint32 bucket = 0; bucket <= maxBucket; ++bucket)
    {
        auto const bucketItr = randomItemCache.find(bucket);
        if (bucketItr == randomItemCache.end())
            continue;

        uint32 const minLevel = bucket * 10 + 1;
        uint32 const maxBucketLevel = std::min(minLevel + 9, static_cast<uint32>(DEFAULT_MAX_LEVEL));

        for (auto const& [type, items] : bucketItr->second)
        {
            //By leewheel 2026-09-03 修复C4189警告：按 AC 原版恢复被注释的调试日志（log 输出改为中文），maxBucketLevel 恢复实际引用
            TC_LOG_DEBUG("playerbots", "    桶 {} (等级 {}..{}) 类型 {} - {} 个随机物品已缓存",
                      bucket, minLevel, maxBucketLevel, type, items.size());
            //End By leewheel

            for (uint32 const itemId : items)
            {
                ItemTemplate const* proto = sObjectMgr->GetItemTemplate(itemId);
                if (!proto)
                    continue;

                // TC_LOG_DEBUG("playerbots", "        [{}] {}", itemId, proto->GetName(DEFAULT_LOCALE));
            }
        }
    }
}

void RandomItemMgr::AddItemStats(uint32 mod, uint8& sp, uint8& ap, uint8& tank)
{
    switch (mod)
    {
        case ITEM_MOD_HEALTH:
        case ITEM_MOD_STAMINA:
            ++sp; ++ap; ++tank;  // universal stats - benefit all roles
            break;
        case ITEM_MOD_AGILITY:
        case ITEM_MOD_STRENGTH:
            ++ap; ++tank;        // physical stats - no spellpower benefit
            break;
        case ITEM_MOD_MANA:
        case ITEM_MOD_INTELLECT:
        case ITEM_MOD_SPIRIT:
            ++sp;                // caster-only stats
            break;
        default:
            break;
    }
}

bool RandomItemMgr::CanUseItemStats(uint8 clazz, uint8 sp, uint8 ap, uint8 tank)
{
    switch (clazz)
    {
        case CLASS_MAGE:
        case CLASS_PRIEST:
        case CLASS_WARLOCK:
            // pure casters: must have sp, and sp must dominate
            if (!sp || ap > sp || tank > sp)
                return false;
            break;
        case CLASS_DEATH_KNIGHT:
        case CLASS_PALADIN:
        case CLASS_WARRIOR:
            // pure melee/tank: must have ap and tank, sp must not dominate
            if ((!ap && !tank) || sp > ap || sp > tank)
                return false;
            break;
        case CLASS_HUNTER:
        case CLASS_ROGUE:
            // physical DPS: must have ap, sp must not dominate
            if (!ap || sp > ap || sp > tank)
                return false;
            break;
        default:
            // CLASS_SHAMAN, CLASS_DRUID - hybrid classes, accept any item that
            // has at least one relevant stat
            break;
    }

    return sp || ap || tank;
}

bool RandomItemMgr::CanEquipItem(ItemTemplate const* proto, uint32 level) const
{
    if (!proto)
        return false;

    // skip items whose required level exceeds the bot's level
    //By leewheel 2026-09-03 修复C4018警告：GetBaseRequiredLevel()在本项目返回int32，与uint32 level比较产生有符号/无符号不匹配；AC原版RequiredLevel为uint32，此处显式转换对齐语义
    if (proto->GetBaseRequiredLevel() > 0 && static_cast<uint32>(proto->GetBaseRequiredLevel()) > level)
        return false;
    //End By leewheel

    // skip use-bound and quest-bound items
    if (proto->GetBonding() > BIND_WHEN_EQUIPPED)
        return false;

    // skip items with no viable equip slot
    if (!GetViableSlots(static_cast<InventoryType>(proto->GetInventoryType())))
        return false;

    if (proto->GetQuality() > ITEM_QUALITY_NORMAL)
    {
        uint32 requiredLevel = proto->GetBaseRequiredLevel();
        /*
         * DEADCODE: Uncomment the code below if BuildCacheItemInfo method will
         *           process itemInfoCache; otherwise remove it completely.
         */

        // if (!requiredLevel)
        //     requiredLevel = GetMinLevelFromCache(proto->GetId());

        if (!requiredLevel)
            requiredLevel = level;

        uint32 delta = 2;
        if (level < 15)
            delta = 15;
        else if (level < 40)
            delta = 10;
        else if (level < 60)
            delta = 6;
        else
            delta = 9;

        // junk (poor/normal) - take anything that fits the slot; real gear should
        // match the level relevance window
        if (requiredLevel > level || (level >= delta && requiredLevel < level - delta))
            return false;
    }

    return true;
}

std::vector<EquipmentSlots> const* RandomItemMgr::GetViableSlots(InventoryType invType) const
{
    auto it = viableSlots.find(invType);
    return it != viableSlots.end() ? &it->second : nullptr;
}

uint32 RandomItemMgr::NormalizeLevel(uint32 level) const
{
    uint32 const levelCap = std::min(sPlayerbotAIConfig.randomBotMaxLevel,
                                     static_cast<uint32>(DEFAULT_MAX_LEVEL));
    return std::min(level, levelCap);
}
