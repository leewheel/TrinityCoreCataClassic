/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "SpellIdValue.h"

#include "ChatHelper.h"
#include "Playerbots.h"
//By leewheel 2026-07-26: 为诊断日志开关 spellIdDiagnosticLog 引入配置头
#include "PlayerbotAIConfig.h"
//End By leewheel
#include "Vehicle.h"

SpellIdValue::SpellIdValue(PlayerbotAI* botAI) : CalculatedValue<uint32>(botAI, "spell id", 20 * 1000) {}

VehicleSpellIdValue::VehicleSpellIdValue(PlayerbotAI* botAI) : CalculatedValue<uint32>(botAI, "vehicle spell id") {}

//By leewheel 2026-07-13: 重写SpellIdValue::Calculate支持多locale技能名称查找
// 核心改动：不再只查enUS locale，而是尝试所有可用locale匹配spell名称
// 这样既支持英文策略名(fireball)，也支持中文名(火球术)的查找
uint32 SpellIdValue::Calculate()
{
    std::string namepart = qualifier;
    ItemIds itemIds = ChatHelper::parseItems(namepart);

    PlayerbotChatHandler handler(bot);
    uint32 extractedSpellId = handler.extractSpellId(namepart);
    if (extractedSpellId)
        if (SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(extractedSpellId))
            if (spellInfo->SpellName)
                namepart = GetSpellNameBestLocaleWithCache(extractedSpellId, spellInfo->SpellName);

    std::wstring wnamepart;
    if (!Utf8toWStr(namepart, wnamepart))
        return 0;

    wstrToLower(wnamepart);
    //By leewheel 2026-09-03 修复C4189警告：firstSymbol/spellLength为AC原版手工匹配(首字母+长度)遗留变量，
    //本项目已改用SpellNameMatchesWithCache统一匹配(见下方78/101行)，两变量无引用，按移植清理
    //End By leewheel

    //By leewheel 2026-07-14: 诊断日志计数器
    static uint32 spellLookupCount = 0;
    static uint32 spellLookupFail = 0;
    static uint32 lastSuccessLogTime = 0;
    spellLookupCount++;
    //End By leewheel

    std::set<uint32> spellIds;
    uint32 spellbookSize = 0;
    for (PlayerSpellMap::iterator itr = bot->GetSpellMap().begin(); itr != bot->GetSpellMap().end(); ++itr)
    {
        spellbookSize++;
        uint32 spellId = itr->first;

        if (itr->second.state == PLAYERSPELL_REMOVED || !itr->second.active)
            continue;

        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);
        if (!spellInfo || spellInfo->IsPassive())
            continue;

        if (spellInfo->GetEffects()[0].Effect == SPELL_EFFECT_LEARN_SPELL)
            continue;

        bool useByItem = false;
        for (uint8 i = 0; i < 3; ++i)
        {
            if (spellInfo->GetEffects()[i].Effect == SPELL_EFFECT_CREATE_ITEM &&
                itemIds.find(spellInfo->GetEffects()[i].ItemType) != itemIds.end())
            {
                useByItem = true;
                break;
            }
        }

        //By leewheel 2026-07-14: 使用带spellnameeng缓存的匹配函数
        if (!useByItem && !SpellNameMatchesWithCache(spellId, spellInfo->SpellName, namepart))
            continue;

        spellIds.insert(spellId);
    }

    Pet* pet = bot->GetPet();
    if (spellIds.empty() && pet)
    {
        for (PetSpellMap::const_iterator itr = pet->m_spells.begin(); itr != pet->m_spells.end(); ++itr)
        {
            if (itr->second.state == PETSPELL_REMOVED)
                continue;

            uint32 spellId = itr->first;
            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);
            if (!spellInfo)
                continue;

            if (spellInfo->GetEffects()[0].Effect == SPELL_EFFECT_LEARN_SPELL)
                continue;

            //By leewheel 2026-07-14: 使用带spellnameeng缓存的匹配函数
            if (!SpellNameMatchesWithCache(spellId, spellInfo->SpellName, namepart))
                continue;

            spellIds.insert(spellId);
        }
    }

    //By leewheel 2026-07-14: 诊断日志 - 失败时节流打印(5秒)
    //By leewheel 2026-07-26: 受 Playerbot.SpellIdDiagnosticLog 开关控制，默认关闭避免刷屏
    if (spellIds.empty())
    {
        spellLookupFail++;
        static uint32 lastFailLogTime = 0;
        if (sPlayerbotAIConfig.spellIdDiagnosticLog && getMSTime() - lastFailLogTime > 5000)
        {
            lastFailLogTime = getMSTime();
            TC_LOG_INFO("playerbots", "SpellIdValue: FAIL bot={} qualifier='{}' spellbookSize={} total={} fail={}",
                bot->GetName(), qualifier, spellbookSize, spellLookupCount, spellLookupFail);

            //By leewheel 2026-07-17: 详细诊断 - 列出所有候选spellId及其本地化名+缓存名
            static std::map<std::string, uint32> failDetailCount;
            auto& fdc = failDetailCount[qualifier];
            if (fdc < 2)
            {
                ++fdc;
                uint32 idx = 0;
                for (PlayerSpellMap::iterator itr2 = bot->GetSpellMap().begin(); itr2 != bot->GetSpellMap().end() && idx < 30; ++itr2, ++idx)
                {
                    if (itr2->second.state == PLAYERSPELL_REMOVED || !itr2->second.active) continue;
                    uint32 sid = itr2->first;
                    SpellInfo const* si = sSpellMgr->GetSpellInfo(sid);
                    if (!si || si->IsPassive()) continue;
                    if (si->GetEffects()[0].Effect == SPELL_EFFECT_LEARN_SPELL) continue;

                    const char* enUS = (*si->SpellName)[LOCALE_enUS];
                    const char* zhCN = (*si->SpellName)[LOCALE_zhCN];
                    const char* cacheName = GetSpellNameEngFromCache(sid);
                    bool matchDB2 = SpellNameMatches(si->SpellName, qualifier);
                    bool matchCache = false;
                    if (cacheName && *cacheName)
                    {
                        std::wstring wq;
                        if (Utf8toWStr(qualifier, wq))
                        {
                            wstrToLower(wq);
                            if (tolower(cacheName[0]) == tolower(qualifier.empty() ? '\0' : qualifier[0]) &&
                                strlen(cacheName) == wq.length() &&
                                Utf8FitTo(cacheName, wq))
                                matchCache = true;
                        }
                    }
                    TC_LOG_INFO("playerbots", "SpellIdValueDetail: sid={} enUS='{}' zhCN='{}' cache='{}' matchDB2={} matchCache={}",
                        sid, enUS ? enUS : "", zhCN ? zhCN : "", cacheName ? cacheName : "<none>", matchDB2, matchCache);
                }
            }
            //End By leewheel
        }
    }
    else if (sPlayerbotAIConfig.spellIdDiagnosticLog && getMSTime() - lastSuccessLogTime > 5000)
    {
        lastSuccessLogTime = getMSTime();
        TC_LOG_INFO("playerbots", "SpellIdValue: OK bot={} qualifier='{}' found={} spellbookSize={}",
            bot->GetName(), qualifier, spellIds.size(), spellbookSize);
    }
    //End By leewheel

    if (spellIds.empty())
        return 0;

    //By leewheel 2026-07-22: 通过spell chain扩展候选集
    //DB2 SpellName可能只有部分等级有enUS名称，导致spellIds只匹配到1-2个等级
    //利用spell_ranks链数据，把bot已学会的同链法术全部加入候选，确保选到最高等级
    {
        std::set<uint32> chainExpansion;
        for (uint32 sid : spellIds)
        {
            SpellInfo const* si = sSpellMgr->GetSpellInfo(sid);
            if (!si || !si->ChainEntry)
                continue;

            // 从链首遍历到链尾
            SpellInfo const* chainSpell = si->GetFirstRankSpell();
            while (chainSpell)
            {
                uint32 chainId = chainSpell->Id;
                if (spellIds.find(chainId) == spellIds.end() && bot->HasSpell(chainId))
                    chainExpansion.insert(chainId);
                chainSpell = chainSpell->GetNextRankSpell();
            }
        }
        spellIds.insert(chainExpansion.begin(), chainExpansion.end());
    }
    //End By leewheel

    int32 saveMana = (int32)round(AI_VALUE(double, "mana save level"));
    uint32 rank = 1;
    uint32 highestRank = 0;
    uint32 highestSpellId = 0;
    uint32 lowestRank = 0;
    uint32 lowestSpellId = 0;
    if (saveMana <= 1)
    {
        for (auto it = spellIds.rbegin(); it != spellIds.rend(); ++it)
        {
            auto spellId = *it;
            const SpellInfo* pSpellInfo = sSpellMgr->GetSpellInfo(spellId);
            if (!pSpellInfo)
                continue;

            //By leewheel 2026-07-10: TC使用GetRank()替代SpellRank字符串解析
            int id = pSpellInfo->GetRank();
            //End By leewheel

            if (!id)
            {
                highestSpellId = spellId;
                continue;
            }

            if (!highestRank || (uint32)id > highestRank)
            {
                highestRank = id;
                highestSpellId = spellId;
            }

            if (!lowestRank || (lowestRank && (uint32)id < lowestRank))
            {
                lowestRank = id;
                lowestSpellId = spellId;
            }
        }
    }
    else
    {
        for (auto it = spellIds.rbegin(); it != spellIds.rend(); ++it)
        {
            auto spellId = *it;
            if (!highestSpellId)
                highestSpellId = spellId;
            if (saveMana == (int32)rank)
                return spellId;
            lowestSpellId = spellId;
            rank++;
        }
    }

    return saveMana > 1 ? lowestSpellId : highestSpellId;
}

uint32 VehicleSpellIdValue::Calculate()
{
    Vehicle* vehicle = bot->GetVehicle();
    if (!vehicle)
        return 0;

    // do not allow if no spells
    VehicleSeatEntry const* seat = vehicle->GetSeatForPassenger(bot);
    //By leewheel 2026-07-10: TC的VehicleSeatEntry使用Flags而非m_flags
    if (!seat || !(seat->Flags & VEHICLE_SEAT_FLAG_CAN_CAST))
    //End By leewheel
        return 0;

    Unit* vehicleBase = vehicle->GetBase();
    if (!vehicleBase->IsAlive())
        return 0;

    std::string namepart = qualifier;

    //By leewheel 2026-07-14: 使用带spellnameeng缓存的匹配函数
    PlayerbotChatHandler handler(bot);
    uint32 extractedSpellId = handler.extractSpellId(namepart);
    if (extractedSpellId)
        if (SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(extractedSpellId))
            if (spellInfo->SpellName)
                namepart = GetSpellNameBestLocaleWithCache(extractedSpellId, spellInfo->SpellName);

    std::wstring wnamepart;
    if (!Utf8toWStr(namepart, wnamepart))
        return 0;

    wstrToLower(wnamepart);

    Creature* creature = vehicleBase->ToCreature();
    for (uint32 x = 0; x < MAX_CREATURE_SPELLS; ++x)
    {
        uint32 spellId = creature->m_spells[x];
        if (spellId == 2)
            continue;

        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);
        if (!spellInfo || spellInfo->IsPassive())
            continue;

        //By leewheel 2026-07-14: 使用带spellnameeng缓存的匹配函数
        if (!SpellNameMatchesWithCache(spellId, spellInfo->SpellName, namepart))
            continue;

        return spellId;
    }
    //End By leewheel

    return 0;
}
