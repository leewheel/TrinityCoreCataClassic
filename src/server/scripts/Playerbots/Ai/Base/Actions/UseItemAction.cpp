/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "UseItemAction.h"

#include "ChatHelper.h"
#include "Event.h"
#include "ItemPackets.h"
#include "ItemUsageValue.h"
#include "PlayerbotTextMgr.h"
#include "Playerbots.h"
#include "SpellPackets.h" // By leewheel 2026-07-09

bool UseItemAction::Execute(Event event)
{
    std::string name = event.getParam();
    if (name.empty())
        name = getName();

    std::vector<Item*> items = AI_VALUE2(std::vector<Item*>, "inventory items", name);
    GuidVector gos = chat->parseGameobjects(name);

    if (gos.empty())
    {
        if (!items.empty())
        {
            return UseItemAuto(*items.begin());
        }
    }
    else
    {
        if (items.empty())
            return UseGameObject(*gos.begin());
        else
            return UseItemOnGameObject(*items.begin(), *gos.begin());
    }

    botAI->TellError(PlayerbotTextMgr::instance().GetBotTextOrDefault(
        "use_item_none_available", "没有可用的物品(或游戏对象)", {})); //By leewheel 2026-08-01: 玩家可见文本中文化
        //End By leewheel
    return false;
}

bool UseItemAction::UseGameObject(ObjectGuid guid)
{
    GameObject* go = botAI->GetGameObject(guid);
    if (!go || !go->isSpawned() /* || go->GetGoState() != GO_STATE_READY*/)
        return false;

    go->Use(bot);

    std::ostringstream out;
    botAI->TellMasterNoFacing(PlayerbotTextMgr::instance().GetBotTextOrDefault(
        "use_gameobject",
        "正在使用 %gameobject",
        {{"%gameobject", chat->FormatGameobject(go)}})); //By leewheel 2026-08-01: 玩家可见文本中文化
        //End By leewheel
    return true;
}

bool UseItemAction::UseItemAuto(Item* item) { return UseItem(item, ObjectGuid::Empty, nullptr); }

bool UseItemAction::UseItemOnGameObject(Item* item, ObjectGuid go) { return UseItem(item, go, nullptr); }

bool UseItemAction::UseItemOnItem(Item* item, Item* itemTarget) { return UseItem(item, ObjectGuid::Empty, itemTarget); }

bool UseItemAction::UseItem(Item* item, ObjectGuid goGuid, Item* itemTarget, Unit* unitTarget)
{
    if (bot->CanUseItem(item) != EQUIP_ERR_OK)
        return false;

    if (bot->IsNonMeleeSpellCast(false))
        return false;

    uint8 bagIndex = item->GetBagSlot();
    uint8 slot = item->GetSlot();
    //By leewheel 2026-09-03 修复C4189警告：cast_count/glyphIndex为AC手工构造包时代遗留死变量，
    //TC 343经UseItem结构体传递无对应字段，删除
    ObjectGuid item_guid = item->GetGUID();
    uint8 castFlags = 0;
    //End By leewheel
    uint32 targetFlag = TARGET_FLAG_NONE;
    ObjectGuid targetGuid; // By leewheel 2026-07-09: 跟踪目标GUID
    uint32 spellId = 0;
    //By leewheel 2026-07-21: TC的ItemTemplate::Spells[5]是AC兼容字段, 从不填充(恒为0),
    //物品使用法术实际存在Effects[](来自ItemEffect.db2), 须遍历Effects并取TriggerType==ON_USE的SpellID。
    //原代码读Spells[]导致spellId恒为0, 后续if(!spellId)return false使机器人永远无法使用食物/饮料。
    ItemTemplate const* useProto = item->GetTemplate();
    for (uint8 idx = 0; idx < useProto->Effects.size(); ++idx)
    {
        if (useProto->Effects[idx]->TriggerType == ITEM_SPELLTRIGGER_ON_USE && useProto->Effects[idx]->SpellID > 0)
        {
            spellId = useProto->Effects[idx]->SpellID;
            if (!botAI->CanCastSpell(spellId, bot, false, itemTarget, item))
            {
                return false;
            }
        }
    }
    //End By leewheel

    // By leewheel 2026-07-09: 不再使用WorldPacket构造CMSG_USE_ITEM，改用WorldPackets::Spells::UseItem
    bool targetSelected = false;

    std::string itemText = chat->FormatItem(item->GetTemplate());
    std::string targetText;

    //By leewheel 2026-07-10: TC中Stackable是ExtendedData成员，使用GetMaxStackSize()方法
    if (item->GetTemplate()->GetMaxStackSize() > 1)
    //End By leewheel
    {
        uint32 count = item->GetCount();
        if (count > 1)
            //By leewheel 2026-08-01: 玩家可见文本中文化
            itemText += " (" + std::to_string(count) + " 个可用)";
            //End By leewheel
        else
            //By leewheel 2026-08-01: 玩家可见文本中文化
            itemText += " (最后一个!)";
            //End By leewheel
    }

    if (goGuid)
    {
        GameObject* go = botAI->GetGameObject(goGuid);
        if (!go || !go->isSpawned())
            return false;

        targetFlag = TARGET_FLAG_GAMEOBJECT;
        targetGuid = goGuid; // By leewheel 2026-07-09
        targetText = chat->FormatGameobject(go);
        targetSelected = true;
    }

    if (itemTarget)
    {
        if (item->GetTemplate()->GetClass() == ITEM_CLASS_GEM)
        {
            bool fit = SocketItem(itemTarget, item) || SocketItem(itemTarget, item, true);
            if (!fit)
                botAI->TellMaster(PlayerbotTextMgr::instance().GetBotTextOrDefault(
                    "socket_does_not_fit", "宝石插槽不匹配", {})); //By leewheel 2026-08-01: 玩家可见文本中文化
                    //End By leewheel

            return fit;
        }
        else
        {
            targetFlag = TARGET_FLAG_ITEM;
            targetGuid = itemTarget->GetGUID(); // By leewheel 2026-07-09
            targetText = chat->FormatItem(itemTarget->GetTemplate());
            targetSelected = true;
        }
    }

    Player* master = GetMaster();
    if (!targetSelected && item->GetTemplate()->GetClass() != ITEM_CLASS_CONSUMABLE && master &&
        botAI->HasActivePlayerMaster() && !selfOnly)
    {
        if (ObjectGuid masterSelection = master->GetTarget())
        {
            Unit* unit = botAI->GetUnit(masterSelection);
            if (unit)
            {
                targetFlag = TARGET_FLAG_UNIT;
                targetGuid = masterSelection; // By leewheel 2026-07-09
                targetText = unit->GetName();
                targetSelected = true;
            }
        }
    }

    if (!targetSelected && item->GetTemplate()->GetClass() != ITEM_CLASS_CONSUMABLE && unitTarget)
    {
        targetFlag = TARGET_FLAG_UNIT;
        targetGuid = unitTarget->GetGUID(); // By leewheel 2026-07-09
        targetText = unitTarget->GetName();
        targetSelected = true;
    }

    if (uint32 questid = item->GetTemplate()->GetStartQuest()) //By leewheel 2026-09-09: TC中方法名为GetStartQuest()
    {
        if (Quest const* qInfo = sObjectMgr->GetQuestTemplate(questid))
        {
            WorldPacket packet(CMSG_QUESTGIVER_ACCEPT_QUEST, 8 + 4 + 4);
            packet << item_guid;
            packet << questid;
            packet << uint32(0);
            // By leewheel 2026-07-09: 使用WPPCompat兼容层
            WPPCompat::HandleQuestgiverAcceptQuestOpcode(bot->GetSession(), packet);
            // End By leewheel

            //By leewheel 2026-08-01: 玩家可见文本中文化
            botAI->TellMasterNoFacing("接到任务 " + chat->FormatQuest(qInfo));
            //End By leewheel
            return true;
        }
    }

    bot->ClearUnitState(UNIT_STATE_CHASE);
    bot->ClearUnitState(UNIT_STATE_FOLLOW);

    if (bot->isMoving())
    {
        bot->StopMoving();
        botAI->SetNextCheckDelay(sPlayerbotAIConfig.globalCoolDown);
        return false;
    }

    //By leewheel 2026-07-21: 同上, TC的Spells[]恒为空, 改遍历Effects[]查找物品目标法术(如附魔)
    for (uint8 idx = 0; idx < useProto->Effects.size(); ++idx)
    {
        if (useProto->Effects[idx]->TriggerType != ITEM_SPELLTRIGGER_ON_USE)
            continue;
        uint32 spellId = useProto->Effects[idx]->SpellID;
        if (!spellId)
            continue;

        if (!botAI->CanCastSpell(spellId, bot, false))
            continue;

        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);
        if (spellInfo->Targets & TARGET_FLAG_ITEM)
        {
            Item* itemForSpell = AI_VALUE2(Item*, "item for spell", spellId);
            if (!itemForSpell)
                continue;

            if (itemForSpell->GetEnchantmentId(TEMP_ENCHANTMENT_SLOT))
                continue;

            if (bot->GetTrader())
            {
                if (selfOnly)
                    return false;

                targetFlag = TARGET_FLAG_TRADE_ITEM;
                //By leewheel 2025-01-16
                // TC中ObjectGuid使用Create<T>模板构造
                targetGuid = ObjectGuid::Create<HighGuid::Item>(TRADE_SLOT_NONTRADED);
                //End By leewheel 2025-01-16
                targetSelected = true;
                //By leewheel 2026-08-01: 玩家可见文本中文化
                targetText = "交易物品";
                //End By leewheel
            }
            else
            {
                targetFlag = TARGET_FLAG_ITEM;
                targetGuid = itemForSpell->GetGUID(); // By leewheel 2026-07-09
                targetSelected = true;
                targetText = chat->FormatItem(itemForSpell->GetTemplate());
            }
            uint32 castTime = spellInfo->CalcCastTime();
            botAI->SetNextCheckDelay(castTime + sPlayerbotAIConfig.reactDelay);
        }

        break;
    }

    if (!targetSelected)
    {
        // Use the actual target if provided
        if (unitTarget)
        {
            //By leewheel 2026-07-22: 消耗品对他人施法须设TARGET_FLAG_UNIT，否则TC默认self-cast
            targetFlag = TARGET_FLAG_UNIT;
            targetGuid = unitTarget->GetGUID();
            targetSelected = true;

            //By leewheel 2026-08-01: 玩家可见文本中文化
            if (unitTarget == bot || !unitTarget->IsInWorld() || unitTarget->IsDuringRemoveFromWorld())
                targetText = "自己";
            else if (unitTarget->IsHostileTo(bot))
                targetText = "自己";
            else
                targetText = unitTarget->GetName();
            //End By leewheel
        }
        else
        {
            targetFlag = TARGET_FLAG_NONE;
            targetGuid = bot->GetGUID(); // By leewheel 2026-07-09
            targetSelected = true;
            //By leewheel 2026-08-01: 玩家可见文本中文化
            targetText = "自己";
            //End By leewheel
        }
    }

    ItemTemplate const* proto = item->GetTemplate();
        bool isDrink = !proto->Effects.empty() && proto->Effects[0]->SpellCategoryID == 59;
        bool isFood = !proto->Effects.empty() && proto->Effects[0]->SpellCategoryID == 11;
        if (proto->GetClass() == ITEM_CLASS_CONSUMABLE &&
            (proto->GetSubClass() == ITEM_SUBCLASS_FOOD || proto->GetSubClass() == ITEM_SUBCLASS_CONSUMABLE) && (isFood || isDrink))
    {
        if (bot->IsInCombat())
            return false;

        // bot->SetStandState(UNIT_STAND_STATE_SIT);
        bot->CastStop();
        float hp = bot->GetHealthPct();
        float mp = bot->GetPower(POWER_MANA) * 100.0f / bot->GetMaxPower(POWER_MANA);
        float p = 0.f;
        if (isDrink && isFood)
        {
            //By leewheel 2026-08-01: 玩家可见文本中文化
            p = std::min(hp, mp);
            TellConsumableUse(item, "盛宴", p);
        }
        else if (isDrink)
        {
            p = mp;
            TellConsumableUse(item, "饮水", p);
        }
        else if (isFood)
        {
            p = std::min(hp, mp);
            TellConsumableUse(item, "进食", p);
        }
            //End By leewheel

        if (!bot->IsInCombat() && !bot->InBattleground())
            botAI->SetNextCheckDelay(std::max(10000.0f, 27000.0f * (100 - p) / 100.0f));

        if (!bot->IsInCombat() && bot->InBattleground())
            botAI->SetNextCheckDelay(std::max(10000.0f, 20000.0f * (100 - p) / 100.0f));

        // botAI->SetNextCheckDelay(27000.0f * (100 - p) / 100.0f);
        //  botAI->SetNextCheckDelay(20000);
        //By leewheel 2026-07-14: 消耗品路径也需要检查spellId，防止spellId=0导致HandleUseItemOpcode报错
        if (!spellId)
            return false;
        //End By leewheel
        // By leewheel 2026-07-09: TC的HandleUseItemOpcode需要WorldPackets::Spells::UseItem而不是WorldPacket
        // By leewheel 2026-07-11: 修复most vexing parse, 用WorldPacket变量避免被解析为函数声明
        {
            WorldPacket useItemPacket(CMSG_USE_ITEM);
            WorldPackets::Spells::UseItem useItem(std::move(useItemPacket));
            useItem.PackSlot = bagIndex;
            useItem.Slot = slot;
            useItem.CastItem = item_guid;
            useItem.Cast.SpellID = spellId;
            useItem.Cast.SendCastFlags = castFlags;
            useItem.Cast.Target.Flags = targetFlag;
            if (targetFlag & TARGET_FLAG_ITEM)
                useItem.Cast.Target.Item = targetGuid;
            else
                useItem.Cast.Target.Unit = targetGuid;
            bot->GetSession()->HandleUseItemOpcode(useItem);
        }
        // End By leewheel

        return true;
    }

    if (!spellId)
        return false;

    // botAI->SetNextCheckDelay(sPlayerbotAIConfig.globalCoolDown);
    //By leewheel 2026-08-01: 玩家可见文本中文化
    std::string useText = targetSelected
        ? PlayerbotTextMgr::instance().GetBotTextOrDefault(
            "use_item_on_target", "正在对 %target 使用 %item", {{"%item", itemText}, {"%target", targetText}})
        : PlayerbotTextMgr::instance().GetBotTextOrDefault(
            "use_item", "正在使用 %item", {{"%item", itemText}});
    //End By leewheel
    botAI->TellMasterNoFacing(useText);
    // By leewheel 2026-07-09: TC的HandleUseItemOpcode需要WorldPackets::Spells::UseItem而不是WorldPacket
    // By leewheel 2026-07-11: 修复most vexing parse, 用WorldPacket变量避免被解析为函数声明
    {
        WorldPacket useItemPacket(CMSG_USE_ITEM);
        WorldPackets::Spells::UseItem useItem(std::move(useItemPacket));
        useItem.PackSlot = bagIndex;
        useItem.Slot = slot;
        useItem.CastItem = item_guid;
        useItem.Cast.SpellID = spellId;
        useItem.Cast.SendCastFlags = castFlags;
        useItem.Cast.Target.Flags = targetFlag;
        if (targetFlag & TARGET_FLAG_ITEM)
            useItem.Cast.Target.Item = targetGuid;
        else
            useItem.Cast.Target.Unit = targetGuid;
        bot->GetSession()->HandleUseItemOpcode(useItem);
    }
    // End By leewheel
    return true;
}

void UseItemAction::TellConsumableUse(Item* item, std::string const action, float percent)
{
    std::ostringstream out;
    out << action << " " << chat->FormatItem(item->GetTemplate());

    if (item->GetTemplate()->GetMaxStackSize() > 1) //By leewheel 2026-09-09: TC中用GetMaxStackSize()
        out << "/x" << item->GetCount();

    out << " (" << round(percent) << "%)";
    botAI->TellMasterNoFacing(out.str());
}

bool UseItemAction::SocketItem(Item* item, Item* gem, bool replace)
{
    WorldPacket packet(CMSG_SOCKET_GEMS);
    packet << item->GetGUID();

    bool fits = false;
    for (uint32 enchant_slot = SOCK_ENCHANTMENT_SLOT; enchant_slot < SOCK_ENCHANTMENT_SLOT + MAX_GEM_SOCKETS;
         ++enchant_slot)
    {
        //By leewheel 2026-07-11: TC用GetSocketColor方法, 需要static_cast<uint8>消除重载歧义
        uint8 SocketColor = item->GetTemplate()->GetSocketColor(static_cast<uint8>(enchant_slot - SOCK_ENCHANTMENT_SLOT));
        //End By leewheel
        GemPropertiesEntry const* gemProperty = sGemPropertiesStore.LookupEntry(gem->GetTemplate()->GetGemProperties());
        if (gemProperty && (gemProperty->Type & SocketColor))
        //By leewheel 2026-09-09: TC中GemPropertiesEntry用Type字段表示宝石颜色类型，非color()方法
        //End By leewheel
        {
            if (fits)
            {
                packet << ObjectGuid::Empty;
                continue;
            }

            uint32 enchant_id = item->GetEnchantmentId(EnchantmentSlot(enchant_slot));
            if (!enchant_id)
            {
                packet << gem->GetGUID();
                fits = true;
                continue;
            }

            SpellItemEnchantmentEntry const* enchantEntry = sSpellItemEnchantmentStore.LookupEntry(enchant_id);
        //By leewheel 2026-09-09: TC的SpellItemEnchantmentEntry用GemItemID字段，非GemID()方法
        if (!enchantEntry || !enchantEntry->GemItemID)
        {
            packet << gem->GetGUID();
            fits = true;
            continue;
        }

        if (replace && enchantEntry->GemItemID != gem->GetTemplate()->GetId())
        {
            packet << gem->GetGUID();
            fits = true;
            continue;
        }
        //End By leewheel
        }

        packet << ObjectGuid::Empty;
    }

    if (fits)
    {
        botAI->TellMaster(PlayerbotTextMgr::instance().GetBotTextOrDefault(
            "socketing_item_with_gem",
            "正在向 %item 镶嵌 %gem",
            {{"%item", chat->FormatItem(item->GetTemplate())}, {"%gem", chat->FormatItem(gem->GetTemplate())}})); //By leewheel 2026-08-01: 玩家可见文本中文化
            //End By leewheel

        WorldPackets::Item::SocketGems nicePacket(std::move(packet));
        nicePacket.Read();
        // By leewheel 2026-07-09: 修复函数名称，TC中使用HandleSocketGems而非HandleSocketOpcode
        bot->GetSession()->HandleSocketGems(nicePacket);
        // End By leewheel
    }

    return fits;
}

bool UseItemAction::isPossible() { return getName() == "use" || AI_VALUE2(uint32, "item count", getName()) > 0; }

bool UseSpellItemAction::isUseful() { return AI_VALUE2(bool, "spell cast useful", getName()); }

bool UseHealingPotion::isUseful() { return AI_VALUE2(bool, "combat", "self target"); }

bool UseManaPotion::isUseful() { return AI_VALUE2(bool, "combat", "self target"); }

bool UseHearthStone::Execute(Event event)
{
    if (bot->isMoving())
    {
        MotionMaster& mm = *bot->GetMotionMaster();
        bot->StopMoving();
        mm.Clear();
    }

    bool used = UseItemAction::Execute(event);
    if (used)
    {
        RESET_AI_VALUE(bool, "combat::self target");
        RESET_AI_VALUE(WorldPosition, "current position");
        botAI->SetNextCheckDelay(10 * IN_MILLISECONDS);
    }

    return used;
}

bool UseHearthStone::isUseful() { return !bot->InBattleground(); }

bool UseRandomRecipe::Execute(Event /*event*/)
{
    std::vector<Item*> recipes = AI_VALUE2(std::vector<Item*>, "inventory items", "recipe");

    std::string recipeName = "";

    for (auto& recipe : recipes)
    {
        recipeName = ItemTemplate_GetName(recipe->GetTemplate());
    }

    if (recipeName.empty())
        return false;

    bool used = UseItemAction::Execute(Event(name, recipeName));

    if (used)
        //By leewheel 2026-09-03 修复C5055警告：IN_MILLISECONDS为枚举常量，与double相乘已弃用，显式转float
        botAI->SetNextCheckDelay(3.0f * float(IN_MILLISECONDS));
        //End By leewheel

    return used;
}

bool UseRandomRecipe::isUseful()
{
    return !bot->IsInCombat() && !botAI->HasActivePlayerMaster() && !bot->InBattleground();
}

bool UseRandomRecipe::isPossible() { return AI_VALUE2(uint32, "item count", "recipe") > 0; }

bool UseRandomQuestItem::Execute(Event /*event*/)
{
    Unit* unitTarget = nullptr;
    ObjectGuid goTarget;

    std::vector<Item*> questItems = AI_VALUE2(std::vector<Item*>, "inventory items", "quest");
    if (questItems.empty())
        return false;

    Item* item = nullptr;
    for (uint8 i = 0; i < 5; i++)
    {
        auto itr = questItems.begin();
        std::advance(itr, urand(0, questItems.size() - 1));
        Item* questItem = *itr;

        ItemTemplate const* proto = questItem->GetTemplate();
        if (proto->GetStartQuest())
        {
            Quest const* qInfo = sObjectMgr->GetQuestTemplate(proto->GetStartQuest());
            if (bot->CanTakeQuest(qInfo, false))
            {
                item = questItem;
                break;
            }
        }
    }

    if (!item)
        return false;

    bool used = UseItem(item, goTarget, nullptr, unitTarget);
    if (used)
        botAI->SetNextCheckDelay(sPlayerbotAIConfig.globalCoolDown);

    return used;
}

bool UseRandomQuestItem::isUseful()
{
    return !botAI->HasActivePlayerMaster() && !bot->InBattleground() && !bot->HasUnitState(UNIT_STATE_IN_FLIGHT);
}

bool UseRandomQuestItem::isPossible() { return AI_VALUE2(uint32, "item count", "quest") > 0; }
