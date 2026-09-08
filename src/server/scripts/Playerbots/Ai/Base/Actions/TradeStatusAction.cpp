/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "TradeStatusAction.h"

#include "CraftValue.h"
#include "Event.h"
#include "GuildTaskMgr.h"
#include "Item.h"
#include "ItemUsageValue.h"
#include "ItemVisitors.h"
#include "ObjectMgr.h"
#include "PlayerbotMgr.h"
#include "PlayerbotSecurity.h"
#include "PlayerbotTextMgr.h"
#include "Playerbots.h"
#include "RandomPlayerbotMgr.h"
#include "SetCraftAction.h"
#include "SpellInfo.h"
#include "TradeData.h"

bool TradeStatusAction::Execute(Event event)
{
    //By leewheel 2026-08-15: 对齐the-lab——selfbot(自身操控)的交易由玩家客户端处理，早退
    if (IsSelfBot(bot))
        return false;
    //End By leewheel

    if (!bot->GetSession())
        return false;

    Player* trader = bot->GetTrader();
    Player* master = GetMaster();
    if (!trader)
        return false;

    //By leewheel 2026-09-03 修复C4189警告：traderBotAI被traderIsGameClientPlayer取代后不再引用，
    //保留IsRealPlayer语义的等价判断用途已移除，删除死变量避免未引用告警
    //End By leewheel

    //By leewheel 2026-08-24: 对齐 master(00052def1 Fix selfbot trading)——
    //原用 !traderBotAI 判断"非bot"，但game client非bot玩家也含selfbot操控者；
    //改用 IsRealPlayer(trader) || IsSelfBot(trader) 精确筛真人/selfbot玩家。
    //Bots 拒绝与"既非主人也非队友"的真人(含selfbot)交易；bot交易者在下方单独处理。
    bool const traderIsGameClientPlayer = IsRealPlayer(trader) || IsSelfBot(trader);
    // Allow the master and group members to trade
    if (trader != master && traderIsGameClientPlayer &&
        (!bot->GetGroup() || !bot->GetGroup()->IsMember(trader->GetGUID())))
    //End By leewheel
    {
        bot->Whisper(PlayerbotTextMgr::instance().GetBotTextOrDefault(
                         //By leewheel 2026-08-01: 玩家可见文本中文化
                         "trade_busy_now", "我现在有点忙", {}),
                     //End By leewheel
                     LANG_UNIVERSAL, trader);
        //By leewheel 2026-08-15: 修复——拒绝后不CancelTrade会使bot自身m_trade残留，
        //玩家再点交易永远收到PLAYER_BUSY直到登出(交易窗口锁死)
        WPPCompat::CancelTrade(bot->GetSession());
        //End By leewheel
        return false;
    }

    if (sPlayerbotAIConfig.enableRandomBotTrading == 0 && (sRandomPlayerbotMgr.IsRandomBot(bot)|| sRandomPlayerbotMgr.IsAddclassBot(bot)))
    {
        bot->Whisper(PlayerbotTextMgr::instance().GetBotTextOrDefault(
                         //By leewheel 2026-08-01: 玩家可见文本中文化
                         "trade_disabled", "交易已被禁用", {}),
                     //End By leewheel
                     LANG_UNIVERSAL, trader);
        //By leewheel 2026-08-15: 同第一拒绝分支——补CancelTrade防交易锁死
        WPPCompat::CancelTrade(bot->GetSession());
        //End By leewheel
        return false;
    }

    // Allow trades from group members or bots
    if ((!bot->GetGroup() || !bot->GetGroup()->IsMember(trader->GetGUID())) &&
        (trader != master || !botAI->GetSecurity()->CheckLevelFor(PLAYERBOT_SECURITY_ALLOW_ALL, true, master)) &&
        //By leewheel 2026-08-24: 对齐 master(00052def1)——traderIsGameClientPlayer 取代 !traderBotAI
        traderIsGameClientPlayer)
    //End By leewheel
    {
        // By leewheel 2026-07-09: 使用WPPCompat兼容层替代直接WorldPacket调用
        WPPCompat::CancelTrade(bot->GetSession());
        // End By leewheel
        return false;
    }

    WorldPacket p(event.getPacket());
    //By leewheel 2026-08-04: 修复——TC的SMSG_TRADE_STATUS是bit-packed包,与AC扁平格式完全不同
    //AC格式: uint32 status (扁平4字节)
    //TC格式: WriteBit(PartnerIsSameBnetAccount) + WriteBits(Status, 5) + 根据Status分支写不同字段
    //原代码 p >> status(uint32) 读到的是bit-packed头部后的字节流,不是Status值
    //修复: 按TC bit-packed格式读取Status(跳过PartnerIsSameBnetAccount bit)
    //包体最小: 1bit + 5bits = 6bits, 对齐后1字节
    if (p.wpos() < p.rpos() || (p.wpos() - p.rpos()) < 1)
        return false;

    p.ReadBit();  // PartnerIsSameBnetAccount, 业务不需要
    uint32 status = p.ReadBits(5);
    p.ResetBitPos();
    //End By leewheel

    //By leewheel 2026-07-10: TC中没有TRADE_STATUS_BACK_TO_TRADE状态
    if (status == TRADE_STATUS_TRADE_ACCEPT /*|| (status == TRADE_STATUS_BACK_TO_TRADE && trader->GetTradeData() && trader->GetTradeData()->IsAccepted())*/)
    //End By leewheel
    {
        // By leewheel 2026-07-09: 使用WPPCompat兼容层替代直接WorldPacket调用
        uint32 discount = sRandomPlayerbotMgr.GetTradeDiscount(bot, trader);
        if (CheckTrade())
        // End By leewheel
        {
            std::map<uint32, uint32> givenItemIds, takenItemIds;
            for (uint32 slot = 0; slot < TRADE_SLOT_TRADED_COUNT; ++slot)
            {
                Item* item = trader->GetTradeData()->GetItem((TradeSlots)slot);
                if (item)
                    givenItemIds[item->GetTemplate()->GetId()] += item->GetCount();

                item = bot->GetTradeData()->GetItem((TradeSlots)slot);
                if (item)
                    takenItemIds[item->GetTemplate()->GetId()] += item->GetCount();
            }

            // By leewheel 2026-07-09: 使用WPPCompat兼容层替代直接WorldPacket调用
            WPPCompat::DoAcceptTrade(bot->GetSession());
            // End By leewheel
            if (bot->GetTradeData())
            {
                sRandomPlayerbotMgr.SetTradeDiscount(bot, trader, discount);
                return false;
            }

            for (std::map<uint32, uint32>::iterator i = givenItemIds.begin(); i != givenItemIds.end(); ++i)
            {
                uint32 itemId = i->first;
                uint32 count = i->second;

                CraftData& craftData = AI_VALUE(CraftData&, "craft");
                if (!craftData.IsEmpty() && craftData.IsRequired(itemId))
                {
                    craftData.AddObtained(itemId, count);
                }

                GuildTaskMgr::instance().CheckItemTask(itemId, count, trader, bot);
            }

            for (std::map<uint32, uint32>::iterator i = takenItemIds.begin(); i != takenItemIds.end(); ++i)
            {
                uint32 itemId = i->first;
                uint32 count = i->second;

                CraftData& craftData = AI_VALUE(CraftData&, "craft");
                if (!craftData.IsEmpty() && craftData.itemId == itemId)
                {
                    craftData.Crafted(count);
                }
            }

            return true;
        }
    }
    //By leewheel 2025-07-10
    // TC使用TRADE_STATUS_INITIATED，AC使用TRADE_STATUS_BEGIN_TRADE
    else if (status == TRADE_STATUS_INITIATED)
    {
        if (!bot->HasInArc(CAST_ANGLE_IN_FRONT, trader, sPlayerbotAIConfig.sightDistance))
            bot->SetFacingToObject(trader);

        BeginTrade();

        //By leewheel 2026-07-22: 法师交易时自动放入魔法水/面包
        TryGiveConjuredItems(trader);
        //End By leewheel

        return true;
    }
    //End By leewheel
    return false;
}

void TradeStatusAction::BeginTrade()
{
    Player* trader = bot->GetTrader();
    if (!trader)
        return;

    //By leewheel 2026-08-01: 移植brighton-chi(254d2240)——允许selfbot与主人交易，
    //其他bot交易仍被拒绝
    PlayerbotAI* traderBotAI = GET_PLAYERBOT_AI(trader);
    if (traderBotAI && !traderBotAI->IsRealPlayer())
        return;
    //End By leewheel

    // By leewheel 2026-07-09: 使用WPPCompat兼容层替代直接WorldPacket调用
    WPPCompat::BeginTrade(bot->GetSession());
    // End By leewheel

    ListItemsVisitor visitor;
    IterateItems(&visitor);

    //By leewheel 2026-08-01: 玩家可见文本中文化
    botAI->TellMaster("=== 背包物品 ===");
    //End By leewheel
    TellItems(visitor.items, visitor.soulbound);

    if (sRandomPlayerbotMgr.IsRandomBot(bot))
    {
        uint32 discount = sRandomPlayerbotMgr.GetTradeDiscount(bot, botAI->GetMaster());
        if (discount)
        {
            std::ostringstream out;
            //By leewheel 2026-08-01: 玩家可见文本中文化
            out << "最高折扣：" << chat->formatMoney(discount);
            //End By leewheel
            botAI->TellMaster(out);
        }
    }
}

bool TradeStatusAction::CheckTrade()
{
    Player* trader = bot->GetTrader();
    if (!bot->GetTradeData() || !trader || !trader->GetTradeData())
        return false;

    //By leewheel 2026-08-01: 移植brighton-chi(254d2240)——允许selfbot与主人交易，
    //否则selfbot会被下方随机bot限制拦截
    PlayerbotAI* traderBotAI = GET_PLAYERBOT_AI(trader);
    if (traderBotAI && traderBotAI->IsRealPlayer() && botAI->GetMaster() == trader)
        return true;
    //End By leewheel

    //By leewheel 2026-08-15: 对齐the-lab——bot间分支排除selfbot trader(自身操控的bot走正常
    //交易路径)，且返回isGettingItem(对方给物品才接受)，防随机bot市场被单方面白送/白嫖
    if (!botAI->HasActivePlayerMaster() && traderBotAI && !IsSelfBot(trader))
    {
        bool isGettingItem = false;
        for (uint32 slot = 0; slot < TRADE_SLOT_TRADED_COUNT; ++slot)
        {
            if (trader->GetTradeData()->GetItem((TradeSlots)slot))
            {
                isGettingItem = true;
                break;
            }
        }

        if (isGettingItem)
        {
            if (bot->GetGroup() && bot->GetGroup()->IsMember(bot->GetTrader()->GetGUID()) &&
                botAI->HasRealPlayerMaster())
                botAI->TellMasterNoFacing(PlayerbotTextMgr::instance().GetBotTextOrDefault(
                    "trade_thank_you_player",
                    //By leewheel 2026-08-01: 玩家可见文本中文化
                    "谢谢你 %player",
                    //End By leewheel
                    {{"%player", chat->FormatWorldobject(bot->GetTrader())}}));
            else
                bot->Say(PlayerbotTextMgr::instance().GetBotTextOrDefault(
                             //By leewheel 2026-08-01: 玩家可见文本中文化
                             "trade_thank_you_player",
                             "谢谢你 %player",
                             //End By leewheel
                             {{"%player", chat->FormatWorldobject(bot->GetTrader())}}),
                         (bot->GetTeamId() == TEAM_ALLIANCE ? LANG_COMMON : LANG_ORCISH));
        }
        return isGettingItem;
        //End By leewheel
    }
    if (!bot->GetSession())
    {
        return false;
    }
    uint32 accountId = bot->GetSession()->GetAccountId();
    if (!sPlayerbotAIConfig.IsInRandomAccountList(accountId))
    {
        int32 botItemsMoney = CalculateCost(bot, true);
        int32 botMoney = bot->GetTradeData()->GetMoney() + botItemsMoney;
        int32 playerItemsMoney = CalculateCost(trader, false);
        int32 playerMoney = trader->GetTradeData()->GetMoney() + playerItemsMoney;
        if (playerMoney || botMoney)
            botAI->PlaySound(playerMoney < botMoney ? TEXT_EMOTE_SIGH : TEXT_EMOTE_THANK);

        return true;
    }

    int32 botItemsMoney = CalculateCost(bot, true);
    int32 botMoney = bot->GetTradeData()->GetMoney() + botItemsMoney;
    int32 playerItemsMoney = CalculateCost(trader, false);
    int32 playerMoney = trader->GetTradeData()->GetMoney() + playerItemsMoney;
    if (botItemsMoney > 0 && sPlayerbotAIConfig.enableRandomBotTrading == 2 && (sRandomPlayerbotMgr.IsRandomBot(bot)|| sRandomPlayerbotMgr.IsAddclassBot(bot)))
    {
        bot->Whisper(PlayerbotTextMgr::instance().GetBotTextOrDefault(
                         //By leewheel 2026-08-01: 玩家可见文本中文化
                         "trade_selling_disabled", "出售功能已被禁用。", {}),
                     //End By leewheel
                     LANG_UNIVERSAL, trader);
        return false;
    }
    if (playerItemsMoney && sPlayerbotAIConfig.enableRandomBotTrading == 3 && (sRandomPlayerbotMgr.IsRandomBot(bot)|| sRandomPlayerbotMgr.IsAddclassBot(bot)))
    {
        bot->Whisper(PlayerbotTextMgr::instance().GetBotTextOrDefault(
                         //By leewheel 2026-08-01: 玩家可见文本中文化
                         "trade_buying_disabled", "购买功能已被禁用。", {}),
                     //End By leewheel
                     LANG_UNIVERSAL, trader);
        return false;
    }
    for (uint32 slot = 0; slot < TRADE_SLOT_TRADED_COUNT; ++slot)
    {
        Item* item = bot->GetTradeData()->GetItem((TradeSlots)slot);
        //By leewheel 2025-07-10
        // TC使用GetSellPrice()方法，AC使用SellPrice成员
        if (item && !item->GetTemplate()->GetSellPrice() && !item->GetTemplate()->IsConjuredConsumable())
        //End By leewheel
        {
            std::ostringstream out;
            botAI->TellMaster(PlayerbotTextMgr::instance().GetBotTextOrDefault(
                //By leewheel 2026-08-01: 玩家可见文本中文化
                "trade_item_not_for_sale",
                "%item - 此物品不出售",
                //End By leewheel
                {{"%item", chat->FormatItem(item->GetTemplate())}}));
            botAI->PlaySound(TEXT_EMOTE_NO);
            return false;
        }

        item = trader->GetTradeData()->GetItem((TradeSlots)slot);
        if (item)
        {
            std::ostringstream out;
            out << item->GetTemplate()->GetId();
            ItemUsage usage = AI_VALUE2(ItemUsage, "item usage", out.str());
            //By leewheel 2025-07-10
            // TC使用GetBuyPrice()方法而非直接访问成员
            if ((botMoney && !item->GetTemplate()->GetBuyPrice()) || usage == ITEM_USAGE_NONE)
            //End By leewheel 2025-07-10
            {
                std::ostringstream out;
                botAI->TellMaster(PlayerbotTextMgr::instance().GetBotTextOrDefault(
                    //By leewheel 2026-08-01: 玩家可见文本中文化
                    "trade_item_not_needed",
                    "%item - 我不需要这个",
                    //End By leewheel
                    {{"%item", chat->FormatItem(item->GetTemplate())}}));
                botAI->PlaySound(TEXT_EMOTE_NO);
                return false;
            }
        }
    }

    if (!botMoney && !playerMoney)
        return true;

    if (!botItemsMoney && !playerItemsMoney)
    {
        botAI->TellError(PlayerbotTextMgr::instance().GetBotTextOrDefault(
            //By leewheel 2026-08-01: 玩家可见文本中文化
            "trade_no_items_error", "没有可以交易的物品", {}));
        //End By leewheel
        return false;
    }

    int32 discount = (int32)sRandomPlayerbotMgr.GetTradeDiscount(bot, trader);
    int32 delta = playerMoney - botMoney;
    int32 moneyDelta = (int32)trader->GetTradeData()->GetMoney() - (int32)bot->GetTradeData()->GetMoney();
    bool success = false;
    if (delta < 0)
    {
        if (delta + discount >= 0)
        {
            if (moneyDelta < 0)
            {
                botAI->TellError(PlayerbotTextMgr::instance().GetBotTextOrDefault(
                    //By leewheel 2026-08-01: 玩家可见文本中文化
                    "trade_discount_buy_only", "折扣只能用于购买物品", {}));
                //End By leewheel
                botAI->PlaySound(TEXT_EMOTE_NO);
                return false;
            }
            success = true;
        }
    }
    else
        success = true;

    if (success)
    {
        sRandomPlayerbotMgr.AddTradeDiscount(bot, trader, delta);
        switch (urand(0, 4))
        {
            //By leewheel 2026-08-01: 玩家可见文本中文化
            case 0:
                botAI->TellMaster(PlayerbotTextMgr::instance().GetBotTextOrDefault(
                    "trade_success_pleasure", "和你交易真是愉快", {}));
                break;
            case 1:
                botAI->TellMaster(PlayerbotTextMgr::instance().GetBotTextOrDefault(
                    "trade_success_fair_trade", "公平交易", {}));
                break;
            case 2:
                botAI->TellMaster(PlayerbotTextMgr::instance().GetBotTextOrDefault(
                    "trade_success_thanks", "谢谢", {}));
                break;
            case 3:
                botAI->TellMaster(PlayerbotTextMgr::instance().GetBotTextOrDefault(
                    "trade_success_off_with_you", "你可以走了", {}));
                break;
            //End By leewheel
        }

        botAI->PlaySound(TEXT_EMOTE_THANK);
        return true;
    }

    std::ostringstream out;
    botAI->TellMaster(PlayerbotTextMgr::instance().GetBotTextOrDefault(
        //By leewheel 2026-08-01: 玩家可见文本中文化
        "trade_want_money_for_this",
        "这个我要 %money",
        //End By leewheel
        {{"%money", chat->formatMoney(-(delta + discount))}}));
    botAI->PlaySound(TEXT_EMOTE_NO);
    return false;
}

int32 TradeStatusAction::CalculateCost(Player* player, bool sell)
{
    Player* trader = bot->GetTrader();
    TradeData* data = player->GetTradeData();
    if (!data)
        return 0;

    uint32 sum = 0;
    for (uint32 slot = 0; slot < TRADE_SLOT_TRADED_COUNT; ++slot)
    {
        Item* item = data->GetItem((TradeSlots)slot);
        if (!item)
            continue;

        ItemTemplate const* proto = item->GetTemplate();
        if (!proto)
            continue;

        if (proto->GetQuality() < ITEM_QUALITY_NORMAL)
            return 0;

        CraftData& craftData = AI_VALUE(CraftData&, "craft");
        if (!craftData.IsEmpty())
        {
            if (player == trader && !sell && craftData.IsRequired(proto->GetId()))
                continue;

            if (player == bot && sell && craftData.itemId == proto->GetId() && craftData.IsFulfilled())
            {
                sum += item->GetCount() * SetCraftAction::GetCraftFee(craftData);
                continue;
            }
        }

        if (sell)
            sum += item->GetCount() * proto->GetSellPrice() * sRandomPlayerbotMgr.GetSellMultiplier(bot);

        else
            sum += item->GetCount() * proto->GetBuyPrice() * sRandomPlayerbotMgr.GetBuyMultiplier(bot);

    }

    return sum;
}

//By leewheel 2026-07-22: 法师交易时自动放入魔法水/面包
//法系职业(法师/术士/牧师/德鲁伊/萨满/圣骑士)给魔法水+魔法面包
//物理系职业(战士/盗贼/死亡骑士/猎人)只给魔法面包
//如果背包没有则当场制造

// 辅助：从conjure法术ID获取创建的物品ID
static uint32 GetConjuredItemId(uint32 spellId)
{
    if (!spellId)
        return 0;
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);
    if (!spellInfo)
        return 0;
    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
    {
        if (spellInfo->GetEffect((SpellEffIndex)i).Effect == SPELL_EFFECT_CREATE_ITEM)
            return spellInfo->GetEffect((SpellEffIndex)i).ItemType;
    }
    return 0;
}

// 辅助：直接创建物品到法师背包，返回创建的Item*
static Item* CreateConjuredItem(Player* mage, uint32 itemId)
{
    if (!itemId)
        return nullptr;

    ItemTemplate const* proto = sObjectMgr->GetItemTemplate(itemId);
    if (!proto)
        return nullptr;

    // 魔法水/面包通常可堆叠20个
    uint32 count = proto->Stackable() > 1 ? 20 : 1;

    ItemPosCountVec dest;
    InventoryResult msg = mage->CanStoreItem(NULL_BAG, NULL_SLOT, dest, itemId, count);
    if (msg != EQUIP_ERR_OK)
    {
        // 背包满，尝试只创建1个
        count = 1;
        msg = mage->CanStoreItem(NULL_BAG, NULL_SLOT, dest, itemId, count);
        if (msg != EQUIP_ERR_OK)
            return nullptr;
    }

    Item* newItem = Item::CreateItem(itemId, count, ItemContext::NONE, mage);
    if (!newItem)
        return nullptr;

    mage->StoreItem(dest, newItem, true);
    return newItem;
}

void TradeStatusAction::TryGiveConjuredItems(Player* trader)
{
    // 只有法师才提供魔法水和面包
    if (bot->getClass() != CLASS_MAGE)
        return;

    // 确认交易已建立
    TradeData* myTrade = bot->GetTradeData();
    if (!myTrade)
        return;

    // 判断对方是法系还是物理系
    uint8 traderClass = trader->getClass();
    bool isCaster = (traderClass == CLASS_MAGE || traderClass == CLASS_WARLOCK ||
                     traderClass == CLASS_PRIEST || traderClass == CLASS_DRUID ||
                     traderClass == CLASS_SHAMAN || traderClass == CLASS_PALADIN);

    // === 魔法面包 ===
    Item* bestFood = nullptr;
    std::vector<Item*> foodItems = parseItems("conjured food", ITERATE_ITEMS_IN_BAGS);
    uint32 bestFoodLevel = 0;
    for (Item* item : foodItems)
    {
        if (!item || !item->GetTemplate())
            continue;
        if (myTrade->HasItem(item->GetGUID()))
            continue;
        uint32 reqLevel = item->GetTemplate()->RequiredLevel();
        if (reqLevel >= bestFoodLevel)
        {
            bestFoodLevel = reqLevel;
            bestFood = item;
        }
    }

    // 背包没有面包→当场制造
    if (!bestFood)
    {
        uint32 foodSpellId = AI_VALUE2(uint32, "spell id", "conjure food");
        uint32 foodItemId = GetConjuredItemId(foodSpellId);
        if (foodItemId)
            bestFood = CreateConjuredItem(bot, foodItemId);
    }

    // === 魔法水（仅法系需要）===
    Item* bestWater = nullptr;
    if (isCaster)
    {
        std::vector<Item*> waterItems = parseItems("conjured water", ITERATE_ITEMS_IN_BAGS);
        uint32 bestWaterLevel = 0;
        for (Item* item : waterItems)
        {
            if (!item || !item->GetTemplate())
                continue;
            if (myTrade->HasItem(item->GetGUID()))
                continue;
            uint32 reqLevel = item->GetTemplate()->RequiredLevel();
            if (reqLevel >= bestWaterLevel)
            {
                bestWaterLevel = reqLevel;
                bestWater = item;
            }
        }

        // 背包没有水→当场制造
        if (!bestWater)
        {
            uint32 waterSpellId = AI_VALUE2(uint32, "spell id", "conjure water");
            uint32 waterItemId = GetConjuredItemId(waterSpellId);
            if (waterItemId)
                bestWater = CreateConjuredItem(bot, waterItemId);
        }
    }

    // 放入交易栏
    uint8 tradeSlot = 0;
    if (bestFood && tradeSlot < TRADE_SLOT_TRADED_COUNT)
    {
        myTrade->SetItem((TradeSlots)tradeSlot, bestFood);
        ++tradeSlot;
    }
    if (bestWater && tradeSlot < TRADE_SLOT_TRADED_COUNT)
    {
        myTrade->SetItem((TradeSlots)tradeSlot, bestWater);
        ++tradeSlot;
    }

    // 告知玩家
    if (bestFood || bestWater)
    {
        std::ostringstream msg;
        msg << "给你";
        if (bestFood)
            msg << " [魔法面包]";
        if (bestWater)
            msg << " [魔法水]";
        botAI->TellMasterNoFacing(msg.str());
    }
}
//End By leewheel
