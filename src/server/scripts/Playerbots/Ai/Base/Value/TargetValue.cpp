/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "TargetValue.h"

#include "CombatManager.h"
#include "LastMovementValue.h"
#include "ObjectGuid.h"
#include "Playerbots.h"
#include "RtiTargetValue.h"
#include "ScriptedCreature.h"
#include "Strategy.h"
#include "ThreatManager.h"
#include <algorithm>
#include <cstdlib>
#include <cctype>
#include <unordered_map>

//By leewheel 2026-07-26: 移植目标排除汇总。仅当战斗引擎确实有策略声明排除时才遍历；
//因目前无策略重写HasTargetExclusions，此函数总返回空集，选目标行为与移植前一致。
GuidSet GatherStrategyTargetExclusions(PlayerbotAI* botAI, TargetValueExclusionType type)
{
    GuidSet exclusions;
    if (!botAI || type == TargetValueExclusionType::None || !botAI->HasTargetExclusions())
        return exclusions;

    for (auto const& strategyName : botAI->GetStrategies(BOT_STATE_COMBAT))
    {
        Strategy* strategy = botAI->GetStrategy(strategyName, BOT_STATE_COMBAT);
        if (!strategy)
            continue;

        strategy->AppendTargetExclusions(exclusions, type);
    }

    return exclusions;
}
//End By leewheel

Unit* FindTargetStrategy::GetResult() { return result; }

//By leewheel 2026-08-21 目标解析: 中文名 / Entry 数字 → 统一得到 creature entry
// 背景: 本项目中文客户端+中文DB2, creature_template.name 直接存中文(如 24850=卡雷苟斯),
//       运行时 Creature::GetName() 返回该中文名。机器人 "find target" 的目标应使用中文名或 Entry,
//       英文名调用处需逐一改为对应 Entry(依老大要求, 不依赖 locale 表)。
// 实现: 缓存仅从 creature_template 建「中文名(现主locale名)→entry」, 配合 Entry 数字, 真正中文本地化兼容。
namespace
{
    using CreatureNameEntryMap = std::unordered_map<std::string, uint32>;

    // 名称(小写) → entry 缓存; 懒加载一次(进程级复用, 避免每次全表遍历); 只依赖 creature_template
    CreatureNameEntryMap const& GetCreatureNameToEntryCache()
    {
        static CreatureNameEntryMap cache;
        if (cache.empty())
        {
            // 模板名: 本项目 creature_template.name 为主 locale 显示名(中文) → 中文名→entry
            CreatureTemplateContainer const& creatures = sObjectMgr->GetCreatureTemplates();
            cache.reserve(creatures.size());
            for (auto const& itr : creatures)
            {
                CreatureTemplate const& ct = itr.second;
                if (ct.Name.empty())
                    continue;
                std::string lowerName = ct.Name;
                std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                cache.try_emplace(std::move(lowerName), ct.Entry);
            }
        }
        return cache;
    }

    // 把 qualifier 归一化为 creature entry; 无法识别(既非数字、也非 creature_template 内名称)返回 0
    uint32 ResolveTargetCreatureEntry(std::string const& qualifier)
    {
        if (qualifier.empty())
            return 0;
        if (std::all_of(qualifier.begin(), qualifier.end(), [](unsigned char c) { return std::isdigit(c); }))
            return static_cast<uint32>(std::strtoul(qualifier.c_str(), nullptr, 10));
        std::string lowerName = qualifier;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        auto const& cache = GetCreatureNameToEntryCache();
        auto itr = cache.find(lowerName);
        return itr != cache.end() ? itr->second : 0;
    }
}
//End By leewheel

//By leewheel 2026-07-26: 移植排除类型接口默认实现。
TargetValueExclusionType FindTargetStrategy::GetExclusionType() { return TargetValueExclusionType::None; }
//End By leewheel

Unit* TargetValue::FindTarget(FindTargetStrategy* strategy)
{
    GuidVector attackers = botAI->GetAiObjectContext()->GetValue<GuidVector>("attackers")->Get();
    //By leewheel 2026-07-26: 移植动态排除过滤(休眠时dynamicExclusions为空，与原行为等价)。
    GuidSet const dynamicExclusions = GatherStrategyTargetExclusions(botAI, strategy->GetExclusionType());
    for (ObjectGuid const guid : attackers)
    {
        Unit* unit = botAI->GetUnit(guid);
        if (!unit || dynamicExclusions.find(guid) != dynamicExclusions.end())
            continue;
        //End By leewheel

        ThreatManager& threatMgr = unit->GetThreatMgr();
        strategy->CheckAttacker(unit, &threatMgr);
    }

    return strategy->GetResult();
}

bool FindNonCcTargetStrategy::IsCcTarget(Unit* attacker)
{
    if (Group* group = botAI->GetBot()->GetGroup())
    {
        Group::MemberSlotList const& groupSlot = group->GetMemberSlots();
        for (Group::member_citerator itr = groupSlot.begin(); itr != groupSlot.end(); itr++)
        {
            Player* member = ObjectAccessor::FindPlayer(itr->guid);
            if (!member || !member->IsAlive())
                continue;

            if (PlayerbotAI* botAI = GET_PLAYERBOT_AI(member))
            {
                if (botAI->GetAiObjectContext()->GetValue<Unit*>("rti cc target")->Get() == attacker)
                    return true;

                std::string const rti = botAI->GetAiObjectContext()->GetValue<std::string>("rti cc")->Get();
                int32 index = RtiTargetValue::GetRtiIndex(rti);
                if (index != -1)
                {
                    if (ObjectGuid guid = group->GetTargetIcon(index))
                        if (attacker->GetGUID() == guid)
                            return true;
                }
            }
        }

        if (ObjectGuid guid = group->GetTargetIcon(4))
            if (attacker->GetGUID() == guid)
                return true;
    }

    return false;
}

void FindTargetStrategy::GetPlayerCount(Unit* creature, uint32* tankCount, uint32* dpsCount)
{
    Player* bot = botAI->GetBot();
    if (tankCountCache.find(creature) != tankCountCache.end())
    {
        *tankCount = tankCountCache[creature];
        *dpsCount = dpsCountCache[creature];
        return;
    }

    *tankCount = 0;
    *dpsCount = 0;

    Unit::AttackerSet attackers(creature->getAttackers());
    for (Unit* attacker : attackers)
    {
        if (!attacker || !attacker->IsAlive() || attacker == bot)
            continue;

        Player* player = attacker->ToPlayer();
        if (!player)
            continue;

        if (botAI->IsTank(player))
            ++(*tankCount);
        else
            ++(*dpsCount);
    }

    tankCountCache[creature] = *tankCount;
    dpsCountCache[creature] = *dpsCount;
}

bool FindTargetStrategy::IsHighPriority(Unit* attacker)
{
    if (Group* group = botAI->GetBot()->GetGroup())
    {
        ObjectGuid guid = group->GetTargetIcon(7);
        if (guid && attacker->GetGUID() == guid)
        {
            return true;
        }
    }
    GuidVector prioritizedTargets = botAI->GetAiObjectContext()->GetValue<GuidVector>("prioritized targets")->Get();
    for (ObjectGuid targetGuid : prioritizedTargets)
    {
        if (targetGuid && attacker->GetGUID() == targetGuid)
        {
            return true;
        }
    }
    return false;
}

WorldPosition LastLongMoveValue::Calculate()
{
    LastMovement& lastMove = *context->GetValue<LastMovement&>("last movement");
    if (lastMove.lastPath.empty())
        return WorldPosition();

    return lastMove.lastPath.getBack();
}

WorldPosition HomeBindValue::Calculate()
{
    //By leewheel 2026-07-09: TC使用m_homebind.WorldLocation而非分开的成员
    return WorldPosition(bot->m_homebind.GetMapId(), bot->m_homebind.GetPositionX(), 
                         bot->m_homebind.GetPositionY(), bot->m_homebind.GetPositionZ(), 0.f);
    //End By leewheel
}

Unit* FindTargetValue::Calculate()
{
    if (qualifier == "")
    {
        return nullptr;
    }
    Group* group = bot->GetGroup();
    if (!group)
    {
        return nullptr;
    }

    //By leewheel 2026-08-21 三合一目标解析: 中文名 / 英文名 / Entry数字 → 统一按entry优先匹配
    uint32 const entry = ResolveTargetCreatureEntry(qualifier);
    //End By leewheel

    for (auto const& [guid, ref] : bot->GetThreatMgr().GetThreatenedByMeList())
    {
        Unit* unit = ref->GetOwner();
        if (!unit)
            continue;

        //By leewheel 2026-08-21 能解析出entry(数字或英文名)时, 直接按 entry 匹配, 不依赖运行时名称(中文DB2友好)
        if (entry)
        {
            if (unit->GetEntry() == entry)
                return unit;
            continue;
        }
        //End By leewheel

        // 无法解析成entry时(如传中文名), 回退到按运行时名称匹配
        std::wstring wnamepart;
        Utf8toWStr(unit->GetName(), wnamepart);
        wstrToLower(wnamepart);
        if (!qualifier.empty() && qualifier.length() == wnamepart.length() && Utf8FitTo(qualifier, wnamepart))
            return unit;
    }

    //By leewheel 2026-08-21 entry兜底: 目标未必在bot威胁列表(如SWP老一Kalecgos), 用附近查找补全
    if (entry)
        if (Creature* c = bot->FindNearestCreature(entry, 100.0f, true))
            return c;
    //End By leewheel

    return nullptr;
}

void FindBossTargetStrategy::CheckAttacker(Unit* attacker, ThreatManager* /*threatManager*/)
{
    UnitAI* unitAI = attacker->GetAI();
    BossAI* bossAI = dynamic_cast<BossAI*>(unitAI);
    if (bossAI)
    {
        result = attacker;
    }
}

Unit* BossTargetValue::Calculate()
{
    FindBossTargetStrategy strategy(botAI);
    return FindTarget(&strategy);
}
