/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "SellAction.h"

//By leewheel 2026-09-05: 上游44afe232——解析品质关键字需要ChatHelper
#include "ChatHelper.h"
//End By leewheel
#include "Event.h"
#include "ItemUsageValue.h"
#include "ItemVisitors.h"
#include "Playerbots.h"
#include "ItemPackets.h"

class SellItemsVisitor : public IterateItemsVisitor
{
public:
    SellItemsVisitor(SellAction* action) : IterateItemsVisitor(), action(action) {}

    bool Visit(Item* item) override
    {
        action->Sell(item);
        return true;
    }

private:
    SellAction* action;
};

//By leewheel 2026-09-05: 上游44afe232——SellGrayItemsVisitor 扩展为 SellQualityItemsVisitor(支持按品质出售)
class SellQualityItemsVisitor : public SellItemsVisitor
{
public:
    SellQualityItemsVisitor(SellAction* action, uint32 maxQuality, bool allClasses)
        : SellItemsVisitor(action), maxQuality(maxQuality), allClasses(allClasses)
    {
    }

    bool Visit(Item* item) override
    {
        ItemTemplate const* proto = item->GetTemplate();
        if (proto->GetQuality() > maxQuality)
            return true;

        // 专业工具(矿锄/鱼竿/剥皮小刀等)不是废品, 不能自动出售
        if (IsProfessionTool(proto))
            return true;

        // 非all模式: 普通及更高品质的绿色装备以上, 只卖护甲/武器, 其余(如附魔材料、雕文)保留
        if (!allClasses && proto->GetQuality() > ITEM_QUALITY_POOR && !IsEquipment(proto))
            return true;

        return SellItemsVisitor::Visit(item);
    }

private:
    static bool IsEquipment(ItemTemplate const* proto)
    {
        //By leewheel 2026-09-09: CreatureTemplate_GetClass仅用于CreatureTemplate，ItemTemplate用GetClass()
        return proto->GetClass() == ITEM_CLASS_ARMOR || proto->GetClass() == ITEM_CLASS_WEAPON;
    }

    static bool IsProfessionTool(ItemTemplate const* proto)
    {
        //By leewheel 2026-09-09: CreatureTemplate_GetClass仅用于CreatureTemplate，ItemTemplate用GetClass()
        if (proto->GetClass() != ITEM_CLASS_WEAPON)
            return false;

        if (proto->SubClass == ITEM_SUBCLASS_WEAPON_MISC || proto->SubClass == ITEM_SUBCLASS_WEAPON_FISHING_POLE)
            return true;

        //By leewheel 2026-09-05: TC343的ItemTemplate无TotemCategory成员字段,等价API为GetTotemCategory()(ItemTemplate.h:822)
        return proto->GetTotemCategory() != 0;
        //End By leewheel
    }

    uint32 maxQuality;
    bool allClasses;
};
//End By leewheel

class SellVendorItemsVisitor : public SellItemsVisitor
{
public:
    SellVendorItemsVisitor(SellAction* action, AiObjectContext* con) : SellItemsVisitor(action) { context = con; }

    AiObjectContext* context;

    bool Visit(Item* item) override
    {
        ItemUsage usage = context->GetValue<ItemUsage>("item usage", item->GetEntry())->Get();
        if (usage != ITEM_USAGE_VENDOR && usage != ITEM_USAGE_AH)
            return true;

        return SellItemsVisitor::Visit(item);
    }
};

bool SellAction::Execute(Event event)
{
    std::string const text = event.getParam();
    if (text == "gray" || text == "*")
    {
        //By leewheel 2026-09-05: 上游44afe232——灰色出售走通用品质出售器(POOR, 非all)
        SellQualityItemsVisitor visitor(this, ITEM_QUALITY_POOR, false);
        //End By leewheel
        IterateItems(&visitor);
        return true;
    }

    if (text == "vendor")
    {
        SellVendorItemsVisitor visitor(this, context);
        IterateItems(&visitor);
        return true;
    }

    //By leewheel 2026-09-05: 上游44afe232——支持 "s <quality>" 与 "s <quality> all"(all=连非装备类的普通/优秀物品也卖)
    std::string quality = text;
    bool allClasses = false;

    size_t const split = quality.rfind(' ');
    if (split != std::string::npos && quality.substr(split + 1) == "all")
    {
        quality.erase(split);
        allClasses = true;
    }

    uint32 const maxQuality = ChatHelper::parseItemQuality(quality);
    if (maxQuality != MAX_ITEM_QUALITY)
    {
        SellQualityItemsVisitor visitor(this, maxQuality, allClasses);
        IterateItems(&visitor);
        return true;
    }
    //End By leewheel

    if (text != "")
    {
        std::vector<Item*> items = parseItems(text, ITERATE_ITEMS_IN_BAGS);
        for (Item* item : items)
        {
            Sell(item);
        }
        return true;
    }

    //By leewheel 2026-08-01: 玩家可见文本中文化
    //By leewheel 2026-09-05: 上游44afe232——用法增加 <quality> [all]
    botAI->TellError("用法: s gray/*/vendor/<品质> [all]/[物品链接]");
    //End By leewheel 2026-09-05
    //End By leewheel 2026-08-01
    return false;
}

void SellAction::Sell(FindItemVisitor* visitor)
{
    IterateItems(visitor);
    std::vector<Item*> items = visitor->GetResult();
    for (Item* item : items)
    {
        Sell(item);
    }
}

void SellAction::Sell(Item* item)
{
    std::ostringstream out;

    GuidVector vendors = botAI->GetAiObjectContext()->GetValue<GuidVector>("nearest npcs")->Get();

    for (ObjectGuid const vendorguid : vendors)
    {
        Creature* pCreature = bot->GetNPCIfCanInteractWith(vendorguid, UNIT_NPC_FLAG_VENDOR, UNIT_NPC_FLAG_2_NONE); // By leewheel 2026-07-08
        if (!pCreature)
            continue;

        ObjectGuid itemguid = item->GetGUID();
        uint32 count = item->GetCount();

        uint32 botMoney = bot->GetMoney();

        WorldPacket p(CMSG_SELL_ITEM);
        p << vendorguid << itemguid << count;

        WorldPackets::Item::SellItem nicePacket(std::move(p));
        nicePacket.Read();
        bot->GetSession()->HandleSellItemOpcode(nicePacket);

        if (botAI->HasCheat(BotCheatMask::gold))
        {
            bot->SetMoney(botMoney);
        }

        //By leewheel 2026-08-01: 玩家可见文本中文化
        out << "正在出售 " << chat->FormatItem(item->GetTemplate());
        //End By leewheel
        botAI->TellMaster(out);

        bot->PlayDistanceSound(120);
        break;
    }
}
