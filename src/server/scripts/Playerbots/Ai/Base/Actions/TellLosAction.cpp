/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "TellLosAction.h"
#include <sstream>

#include "ChatHelper.h"
#include "Event.h"
#include "ItemTemplate.h"
#include "ObjectMgr.h"
#include "Playerbots.h"
#include "StatsWeightCalculator.h"
#include "World.h"

bool TellLosAction::Execute(Event event)
{
    std::string const param = event.getParam();

    if (param.empty() || param == "targets")
    {
        //By leewheel 2026-08-01: 玩家可见文本中文化
        ListUnits("--- 目标 ---", *context->GetValue<GuidVector>("possible targets"));
        ListUnits("--- 全部目标 ---", *context->GetValue<GuidVector>("all targets"));
        //End By leewheel
    }

    if (param.empty() || param == "npcs")
    {
        //By leewheel 2026-08-01: 玩家可见文本中文化
        ListUnits("--- NPC ---", *context->GetValue<GuidVector>("nearest npcs"));
        //End By leewheel
    }

    if (param.empty() || param == "corpses")
    {
        //By leewheel 2026-08-01: 玩家可见文本中文化
        ListUnits("--- 尸体 ---", *context->GetValue<GuidVector>("nearest corpses"));
        //End By leewheel
    }

    if (param.empty() || param == "gos" || param == "game objects")
    {
        //By leewheel 2026-08-01: 玩家可见文本中文化
        ListGameObjects("--- 游戏物体 ---", *context->GetValue<GuidVector>("nearest game objects"));
        //End By leewheel
    }

    if (param.empty() || param == "players")
    {
        //By leewheel 2026-08-01: 玩家可见文本中文化
        ListUnits("--- 友善玩家 ---", *context->GetValue<GuidVector>("nearest friendly players"));
        //End By leewheel
    }

    if (param.empty() || param == "triggers")
    {
        //By leewheel 2026-08-01: 玩家可见文本中文化
        ListUnits("--- 触发器 ---", *context->GetValue<GuidVector>("possible triggers"));
        //End By leewheel
    }

    return true;
}

void TellLosAction::ListUnits(std::string const title, GuidVector units)
{
    botAI->TellMaster(title);

    for (ObjectGuid const guid : units)
    {
        if (Unit* unit = botAI->GetUnit(guid))
        {
            //By leewheel 2026-07-10: TC的GetNameForLocaleIdx返回std::string，直接使用
            botAI->TellMaster(unit->GetNameForLocaleIdx(sWorld->GetDefaultDbcLocale()));
            //End By leewheel
        }
    }
}
void TellLosAction::ListGameObjects(std::string const title, GuidVector gos)
{
    botAI->TellMaster(title);

    for (ObjectGuid const guid : gos)
    {
        if (GameObject* go = botAI->GetGameObject(guid))
            botAI->TellMaster(chat->FormatGameobject(go));
    }
}

bool TellAuraAction::Execute(Event /*event*/)
{
    //By leewheel 2026-08-01: 玩家可见文本中文化
    botAI->TellMaster("--- 光环 ---");
    //End By leewheel
    //By leewheel 2026-07-10: 替换sLog->outMessage为TC_LOG_DEBUG
    // TC_LOG_DEBUG("playerbots", "--- Auras ---");
    //End By leewheel
    Unit::AuraApplicationMap& map = bot->GetAppliedAuras();
    for (Unit::AuraApplicationMap::iterator i = map.begin(); i != map.end(); ++i)
    {
        Aura* aura = i->second->GetBase();
        if (!aura)
            continue;
        //By leewheel 2026-07-14: 使用带spellnameeng缓存的辅助函数
        const std::string auraName = GetSpellNameBestLocaleWithCache(aura->GetSpellInfo()->Id, aura->GetSpellInfo()->SpellName);
        // TC_LOG_DEBUG("playerbots", "Info of Aura - name: {}", auraName);
        //End By leewheel
        AuraObjectType type = aura->GetType();
        WorldObject* owner = aura->GetOwner();
        //By leewheel 2026-08-01: 玩家可见文本中文化
        std::string owner_name = owner ? owner->GetName() : "未知";
        //End By leewheel
        float distance = bot->GetDistance2d(owner);
        Unit* caster = aura->GetCaster();
        //By leewheel 2026-08-01: 玩家可见文本中文化
        std::string caster_name = caster ? caster->GetName() : "未知";
        //End By leewheel
        bool is_area = aura->IsArea();
        int32 duration = aura->GetDuration();
        int32 spellId = aura->GetSpellInfo()->Id;
        bool isPositive = aura->GetSpellInfo()->IsPositive();
        //By leewheel 2026-07-10: 使用TC_LOG_DEBUG替代sLog->outMessage
        // TC_LOG_DEBUG("playerbots", "Info of Aura - name: {} caster: {} type: {} owner: {} distance: {} isArea: {} duration: {} spellId: {} isPositive: {}",
        //              auraName, caster_name, type, owner_name, distance, is_area, duration, spellId, isPositive);
        //End By leewheel

        //By leewheel 2026-08-01: 玩家可见文本中文化
        botAI->TellMaster("光环信息 - 名称：" + auraName + " 施法者：" + caster_name + " 类型：" +
                          std::to_string(type) + " 拥有者：" + owner_name + " 距离：" + std::to_string(distance) +
                          " 是否区域：" + std::to_string(is_area) + " 持续时间：" + std::to_string(duration) +
                          " 法术ID：" + std::to_string(spellId) + " 是否增益：" + std::to_string(isPositive));
        //End By leewheel

        if (type == DYNOBJ_AURA_TYPE)
        {
            DynamicObject* dyn_owner = aura->GetDynobjOwner();
            float radius = dyn_owner->GetRadius();
            int32 spellId = dyn_owner->GetSpellId();
            int32 duration = dyn_owner->GetDuration();
            //By leewheel 2026-07-10: 使用TC_LOG_DEBUG替代sLog->outMessage
            // TC_LOG_DEBUG("playerbots", "Info of DynamicObject - name: {} radius: {} spell id: {} duration: {}",
            //              dyn_owner->GetName(), radius, spellId, duration);
            //End By leewheel

            //By leewheel 2026-08-01: 玩家可见文本中文化
            botAI->TellMaster(std::string("动态物体信息 -") + " 名称：" + dyn_owner->GetName() +
                              " 半径：" + std::to_string(radius) + " 法术ID：" + std::to_string(spellId) +
                              " 持续时间：" + std::to_string(duration));
            //End By leewheel
        }
    }
    return true;
}

bool TellEstimatedDpsAction::Execute(Event /*event*/)
{
    float dps = AI_VALUE(float, "estimated group dps");
    //By leewheel 2026-08-01: 玩家可见文本中文化
    botAI->TellMaster("估算队伍DPS：" + std::to_string(dps));
    //End By leewheel
    return true;
}

bool TellCalculateItemAction::Execute(Event event)
{
    std::string const text = event.getParam();
    ItemWithRandomProperty item = chat->parseItemWithRandomProperty(text);
    StatsWeightCalculator calculator(bot);

    const ItemTemplate* proto = sObjectMgr->GetItemTemplate(item.itemId);
    if (!proto)
        return false;
    float score = calculator.CalculateItem(item.itemId, item.randomPropertyId);

    std::ostringstream out;
    //By leewheel 2026-08-01: 玩家可见文本中文化
    out << "物品评分：" << chat->FormatItem(proto) << "：" << score;
    //End By leewheel
    botAI->TellMasterNoFacing(out.str());
    return true;
}
