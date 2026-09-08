/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "TradeAction.h"
#include "ChatHelper.h"
#include "Event.h"
#include "ItemCountValue.h"
#include "ItemVisitors.h"
#include "PlayerbotAI.h"
//By leewheel 2026-07-10: 需要包含TradeData.h和WPPCompat.h
#include "TradeData.h"
#include "PlayerbotsWPPCompat.h"
//End By leewheel

bool TradeAction::Execute(Event event)
{
    std::string const text = event.getParam();

    // If text starts with any excluded prefix, don't process it further.
    for (auto const& prefix : sPlayerbotAIConfig.tradeActionExcludedPrefixes)
    {
        if (text.find(prefix) == 0)
            return false;
    }

    if (!bot->GetTrader())
    {
        GuidVector guids = chat->parseGameobjects(text);
        Player* player = nullptr;

        for (auto& guid : guids)
            if (guid.IsPlayer())
                player = ObjectAccessor::FindPlayer(guid);

        if (!player && botAI->GetMaster())
            player = botAI->GetMaster();

        if (!player)
            return false;

        if (!player->GetTrader())
        {
            // By leewheel 2026-07-10: WPPCompat中的函数名是DoInitiateTrade和DoSetTradeGold
            WPPCompat::DoInitiateTrade(bot->GetSession(), player->GetGUID());
            // End By leewheel
            return true;
        }
        else if (player->GetTrader() != bot)
            return false;
    }

    uint32 copper = chat->parseMoney(text);
    if (copper > 0)
    {
        // By leewheel 2026-07-10: WPPCompat中的函数名是DoInitiateTrade和DoSetTradeGold
        WPPCompat::DoSetTradeGold(bot->GetSession(), copper);
        // End By leewheel
    }

    size_t pos = text.rfind(" ");
    int count = pos != std::string::npos ? atoi(text.substr(pos + 1).c_str()) : 1;

    std::vector<Item*> found = parseItems(text);
    if (found.empty())
        return false;

    uint32 traded = 0;
    for (Item* item : found)
    {
        if (!bot->GetTrader() || item->IsInTrade())
            continue;

        int8 slot = item->CanBeTraded() ? -1 : TRADE_SLOT_NONTRADED;
        if (TradeItem(item, slot) && slot != TRADE_SLOT_NONTRADED && ++traded >= uint32(count))
            break;
    }

    return true;
}

bool TradeAction::TradeItem(Item const* item, int8 slot)
{
    int8 tradeSlot = -1;
    Item* itemPtr = const_cast<Item*>(item);

    TradeData* pTrade = bot->GetTradeData();
    //By leewheel 2026-07-10: 显式使用TradeSlots枚举
    if ((slot >= 0 && slot < int8(TRADE_SLOT_COUNT)) && pTrade->GetItem(TradeSlots(slot)) == nullptr)
        tradeSlot = slot;
    //End By leewheel

    if (slot == int8(TRADE_SLOT_NONTRADED))
        pTrade->SetItem(TRADE_SLOT_NONTRADED, itemPtr);
    else
    {
        for (uint8 i = 0; i < uint8(TRADE_SLOT_TRADED_COUNT) && tradeSlot == -1; i++)
        {
            if (pTrade->GetItem(TradeSlots(i)) == itemPtr)
            {
                tradeSlot = i;

                //By leewheel 2026-07-10: WPPCompat中的函数名是DoClearTradeItem
                WPPCompat::DoClearTradeItem(bot->GetSession(), (uint8)tradeSlot);
                //End By leewheel
                pTrade->SetItem(TradeSlots(i), nullptr);
                return true;
            }
        }

        for (uint8 i = 0; i < TRADE_SLOT_TRADED_COUNT && tradeSlot == -1; i++)
        {
            if (pTrade->GetItem(TradeSlots(i)) == nullptr)
                tradeSlot = i;
        }
    }

    if (tradeSlot == -1)
        return false;

    // By leewheel 2026-07-10: 使用WPPCompat兼容层替代直接WorldPacket调用，函数名是DoSetTradeItem
    WPPCompat::DoSetTradeItem(bot->GetSession(), (uint8)tradeSlot, (uint8)item->GetBagSlot(), (uint8)item->GetSlot());
    // End By leewheel
    return true;
}
