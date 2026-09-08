/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "ResetAiAction.h"

#include "Event.h"
#include "Group.h"
#include "ObjectGuid.h"
#include "PlayerbotRepository.h"
#include "Playerbots.h"
#include "WorldPacket.h"

bool ResetAiAction::Execute(Event event)
{
    if (!event.getPacket().empty())
    {
        WorldPacket packet = event.getPacket();
        if (packet.GetOpcode() == SMSG_GROUP_LIST)
        {
            //By leewheel 2026-08-04: 修复——TC的SMSG_GROUP_LIST(=SMSG_PARTY_UPDATE)格式与AC完全不同
            //AC格式: groupType(1)+slot.group(1)+slot.flags(1)+slot.roles(1)+[LFG:status(1)+dungeon(4)]
            //        +guid(8)+counter(4)+membersCount(4)
            //TC格式(PartyUpdate): PartyFlags(2)+PartyIndex(1)+PartyType(1)+MyIndex(4)+PartyGUID(8)
            //        +SequenceNum(4)+LeaderGUID(8)+LeaderFactionGroup(1)+PlayerList.size()(4)+bits...
            //原代码按AC格式读,所有字段全部错位,membersCount读到的是LeaderGUID的字节流,判断永远错误
            //修复: 按TC格式读取,提取PlayerList.size()作为membersCount
            //包体最小: 2+1+1+4+8+4+8+1+4 = 33字节(到PlayerList.size()结束)
            if (packet.wpos() < packet.rpos() || (packet.wpos() - packet.rpos()) < 33)
                return false;

            uint16 partyFlags;
            uint8 partyIndex, partyType;
            int32 myIndex;
            ObjectGuid partyGuid;
            uint32 sequenceNum;
            ObjectGuid leaderGuid;
            uint8 leaderFactionGroup;
            uint32 membersCount;
            packet >> partyFlags >> partyIndex >> partyType >> myIndex;
            packet >> partyGuid >> sequenceNum >> leaderGuid >> leaderFactionGroup;
            packet >> membersCount;  // PlayerList.size()
            //End By leewheel

            if (membersCount != 0)
            {
                return false;
            }
        }
    }
    if (Player* master = botAI->GetMaster())
    {
        Group* botGroup = bot->GetGroup();
        Group* masterGroup = master->GetGroup();
        if (botGroup && (!masterGroup || masterGroup != botGroup))
            botAI->SetMaster(nullptr);
    }
    if (sRandomPlayerbotMgr.IsRandomBot(bot) && !bot->InBattleground())
    {
        if (bot->GetGroup() && (!botAI->GetMaster() || GET_PLAYERBOT_AI(botAI->GetMaster())))
        {
            if (Player* newMaster = botAI->FindNewMaster())
                botAI->SetMaster(newMaster);
        }
    }
    //By leewheel 2026-08-07: 修复崩溃——Map线程执行数据库操作可能因连接异常抛出C++异常
    //崩溃日志显示: ResetAiAction::Execute -> PlayerbotRepository::Reset -> PlayerbotsDatabase.Execute
    //            -> my_make_scrambled_password (libmysql.dll) -> _CxxThrowException -> terminate -> abort
    //根因: 数据库连接断开或认证失败时, Map线程(无try-catch)的数据库调用抛异常跨线程终止进程
    //修复: 用 try-catch 包裹数据库调用, 异常时仅记录日志不崩溃
    try
    {
        PlayerbotRepository::instance().Reset(botAI);
    }
    catch (std::exception const& e)
    {
        LOG_ERROR("playerbots", "ResetAiAction: 数据库重置失败, bot={}, 异常={}", bot->GetName(), e.what());
    }
    catch (...)
    {
        LOG_ERROR("playerbots", "ResetAiAction: 数据库重置失败(未知异常), bot={}", bot->GetName());
    }
    //End By leewheel
    botAI->ResetStrategies(false);
    //By leewheel 2026-08-01: 玩家可见文本中文化
    botAI->TellMaster("AI已重置为默认设置");
    //End By leewheel
    return true;
}
