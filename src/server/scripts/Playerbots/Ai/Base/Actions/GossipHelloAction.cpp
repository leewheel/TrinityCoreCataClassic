/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

//By leewheel 2026-07-10: TC中没有DB2Manager.h，使用DB2Stores.h替代
#include "GossipHelloAction.h"

#include "DB2Stores.h"
#include "Event.h"
#include "GossipDef.h"
#include "ObjectMgr.h"
#include "Playerbots.h"
//End By leewheel

bool GossipHelloAction::Execute(Event event)
{
    ObjectGuid guid;

    WorldPacket& p = event.getPacket();
    if (p.empty())
    {
        Player* master = GetMaster();
        if (master)
            guid = master->GetTarget();
    }
    else
    {
        //By leewheel 2026-08-03: 修复——客户端包rpos=2(opcode已消费)，去掉rpos(0)防止把opcode当包体读
        //By leewheel 2026-08-04: 修复rpos(0)+长度检查
        if (p.wpos() < p.rpos() || (p.wpos() - p.rpos()) < 8)
            return false;
        //End By leewheel
        p >> guid;
    }

    std::string const text = event.getParam();
    int32 menuToSelect = -1;
    if (!text.empty())
        menuToSelect = atoi(text.c_str());

    return Execute(guid, menuToSelect, false);
}

void GossipHelloAction::TellGossipText(uint32 textId)
{
    if (!textId)
        return;

    //By leewheel 2026-07-10: TC的NpcText使用Data[i].BroadcastTextID，需要通过BroadcastTextEntry获取文本
    if (NpcText const* text = sObjectMgr->GetNpcText(textId))
    {
        for (uint8 i = 0; i < MAX_NPC_TEXT_OPTIONS; i++)
        {
            if (text->Data[i].BroadcastTextID)
            {
                if (BroadcastTextEntry const* bct = sBroadcastTextStore.LookupEntry(text->Data[i].BroadcastTextID))
                {
                    //By leewheel 2026-07-10: GetBroadcastTextValue返回char const*，不能对std::string使用&&
                    char const* text0 = DB2Manager::GetBroadcastTextValue(bct, DEFAULT_LOCALE, GENDER_MALE, true);
                    if (text0 && text0[0])
                        botAI->TellMasterNoFacing(text0);

                    char const* text1 = DB2Manager::GetBroadcastTextValue(bct, DEFAULT_LOCALE, GENDER_FEMALE, true);
                    if (text1 && text1[0])
                        botAI->TellMasterNoFacing(text1);
                    //End By leewheel
                }
            }
        }
    }
    //End By leewheel
}

void GossipHelloAction::TellGossipMenus()
{
    if (!bot->PlayerTalkClass)
        return;

    Creature* pCreature = bot->GetNPCIfCanInteractWith(GetMaster()->GetTarget(), UNIT_NPC_FLAG_NONE, UNIT_NPC_FLAG_2_NONE); // By leewheel 2026-07-08
    GossipMenu& menu = bot->PlayerTalkClass->GetGossipMenu();
    if (pCreature)
    {
        uint32 textId = bot->GetGossipTextId(menu.GetMenuId(), pCreature);
        TellGossipText(textId);
    }

    GossipMenuItemContainer const& items = menu.GetMenuItems();
    for (size_t i = 0; i < items.size(); ++i)
    {
        GossipMenuItem const& item = items[i];
        std::ostringstream out;
        out << "[" << i << "] " << item.OptionText;
        botAI->TellMasterNoFacing(out.str());
    }
}

bool GossipHelloAction::ProcessGossip(int32 menuToSelect, bool silent)
{
    GossipMenu& menu = bot->PlayerTalkClass->GetGossipMenu();
    if (menuToSelect != -1 && !menu.GetItem(menuToSelect))
    {
        if (!silent)
            //By leewheel 2026-08-01: 玩家可见文本中文化
            botAI->TellError("未知的对话选项");
            //End By leewheel
        return false;
    }

    //By leewheel 2026-07-10: TC使用空字符串作为code参数
    WPPCompat::DoGossipSelectOption(bot->GetSession(), GetMaster()->GetTarget(), menuToSelect, menu.GetMenuId(), "");
    //End By leewheel

    if (!silent)
        TellGossipMenus();

    return true;
}

bool GossipHelloAction::Execute(ObjectGuid guid, int32 menuToSelect, bool silent)
{
    if (!guid)
        return false;

    Creature* pCreature = bot->GetNPCIfCanInteractWith(guid, UNIT_NPC_FLAG_NONE, UNIT_NPC_FLAG_2_NONE); // By leewheel 2026-07-08
    if (!pCreature)
    {
        // TC_LOG_DEBUG("playerbots",
        //           "[PlayerbotMgr]: HandleMasterIncomingPacket - Received  CMSG_GOSSIP_HELLO {} not found or you can't "
        //           "interact with him.",
        //           guid.ToString().c_str());
        return false;
    }

    //By leewheel 2026-07-10: TC中GossipMenuId是方法而非成员变量，需要加括号
    auto pMenuItemBounds =
        sObjectMgr->GetGossipMenuItemsMapBounds(pCreature->GetCreatureTemplate()->GossipMenuId());
    //End By leewheel
    //By leewheel 2026-07-10: TC的Trinity::IteratorPair没有first/second，使用begin()/end()
    if (pMenuItemBounds.begin() == pMenuItemBounds.end())
        return false;
    //End By leewheel

    if (menuToSelect == -1)
    {
        WPPCompat::DoGossipHello(bot->GetSession(), guid);
        bot->SetFacingToObject(pCreature);

        if (!silent)
        {
            std::ostringstream out;
            out << "--- " << pCreature->GetName() << " ---";
            botAI->TellMasterNoFacing(out.str());
            TellGossipMenus();
        }
    }
    else if (!bot->PlayerTalkClass)
    {
        if (!silent)
            //By leewheel 2026-08-01: 玩家可见文本中文化
            botAI->TellError("我需要先交谈");
            //End By leewheel
        return false;
    }
    else
    {
        if (!ProcessGossip(menuToSelect, silent))
            return false;
    }

    bot->TalkedToCreature(pCreature->GetEntry(), pCreature->GetGUID());
    return true;
}
