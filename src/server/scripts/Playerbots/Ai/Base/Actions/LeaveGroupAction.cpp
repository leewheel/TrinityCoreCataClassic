/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "LeaveGroupAction.h"

#include "Event.h"
#include "PartyPackets.h"
#include "PlayerbotAIConfig.h"
#include "PlayerbotTextMgr.h"
#include "Playerbots.h"

bool LeaveGroupAction::Execute(Event event)
{
    Player* player = event.getOwner();
    if (player == botAI->GetMaster())
        return Leave();

    return false;
}

bool PartyCommandAction::Execute(Event event)
{
    WorldPacket& p = event.getPacket();
    //By leewheel 2026-08-04: 修复——TC的SMSG_PARTY_COMMAND_RESULT是bit-packed包,与AC扁平格式完全不同
    //AC格式: uint8 operation + uint32 result + string member (扁平)
    //TC格式(Write逆序读取): ReadBits(9)nameLen + ReadBits(4)Command + ReadBits(6)Result
    //                       + ResetBitPos + uint32 ResultData + ObjectGuid ResultGUID + ReadString(nameLen)
    //原代码 p.rpos(0) + p >> operation(uint32) >> member(string) 完全错位:
    //  - SMSG包rpos已是0, rpos(0)多余但无害
    //  - operation读到的是bit-packed头部(9+4+6=19bits=2.375字节)后的ResultData,不是Command
    //  - member读到的是ResultGUID的字节流作为string,可能无null终止符导致越界
    //修复: 按TC bit-packed格式解析,提取Command和Name
    //包体最小: 19bits头(对齐后3字节) + 4字节ResultData + 8字节ResultGUID = 15字节
    if (p.wpos() < p.rpos() || (p.wpos() - p.rpos()) < 15)
        return false;
    //End By leewheel

    //By leewheel 2026-08-04: 按TC bit-packed格式解析
    uint32 nameLen = p.ReadBits(9);
    uint32 operation = p.ReadBits(4);  // Command
    /*uint32 result =*/ p.ReadBits(6); // Result, 业务不需要
    p.ResetBitPos();

    uint32 resultData;
    ObjectGuid resultGuid;
    p >> resultData >> resultGuid;

    std::string member(p.ReadString(nameLen));
    //End By leewheel

    if (operation != PARTY_OP_LEAVE)
        return false;
    // Only leave if master has left the party, and randombot cannot set new master.
    Player* master = GetMaster();
    if (master && member == master->GetName())
    {
        if (sRandomPlayerbotMgr.IsRandomBot(bot))
        {
            Player* newMaster = botAI->FindNewMaster();
            if (newMaster || bot->InBattleground())
            {
                botAI->SetMaster(newMaster);
                return false;
            }
        }
        return Leave();
    }
    return false;
}

bool UninviteAction::Execute(Event event)
{
    WorldPacket& p = event.getPacket();
    //By leewheel 2026-07-26: 适配TC的CMSG_PARTY_UNINVITE(bit-packed PartyUninvite{TargetGUID})
    //AC原实现按玩家名(字符串)解析,与TC线格式不兼容,直接读会误解析甚至抛异常
    //TC客户端只按GUID踢人,改用WorldPackets解析TargetGUID与本机器人GUID比对
    if (p.GetOpcode() == CMSG_PARTY_UNINVITE)
    {
        //By leewheel 2026-08-03: 修复崩溃——客户端包经WorldSocket读opcode后rpos=2，
        //原copy.rpos(0)把opcode 2字节当包体解析导致位错位reasonLen=垃圾值→ReadString越界抛异常→崩溃。
        //与核心一致：保留rpos（=2，opcode已消费），PartyUninvite::Read从当前rpos读包体。
        //By leewheel 2026-08-04: 加强防御——投票踢人等场景可能发空body的CMSG_PARTY_UNINVITE，
        //PartyUninvite::Read最少需要1字节(9bits: hasPartyIndex+reasonLen)+8字节(TargetGUID)=9字节，
        //剩余可读数据不足时直接跳过，避免Read()越界抛异常依赖try-catch兜底。
        WorldPacket copy(p);
        if (copy.wpos() < copy.rpos() || (copy.wpos() - copy.rpos()) < sizeof(uint64) + 1)
            return false;

        WorldPackets::Party::PartyUninvite packet(std::move(copy));
        packet.Read();

        // 被踢的是本机器人自己 → 优雅离队
        if (bot->GetGUID() == packet.TargetGUID)
            return Leave();
    }
    //End By leewheel

    return false;
}

bool LeaveGroupAction::Leave()
{
    if (!botAI)
        return false;

    Player* master = botAI -> GetMaster();

    //By leewheel 2026-07-14: 组队离开日志已清理
    //End By leewheel

    if (master)
        //By leewheel 2026-08-01: 玩家可见文本中文化
        botAI->TellMaster(
            PlayerbotTextMgr::instance().GetBotTextOrDefault("goodbye", "再见！", {}),
            PLAYERBOT_SECURITY_TALK);
        //End By leewheel

    botAI->LeaveOrDisbandGroup();
    return true;
}

bool LeaveFarAwayAction::Execute(Event /*event*/)
{
    // allow bot to leave party when they want
    return Leave();
}

bool LeaveFarAwayAction::isUseful()
{
    if (bot->InBattleground())
        return false;

    if (bot->InBattlegroundQueue())
        return false;

    if (!bot->GetGroup())
        return false;

    Player* groupLeader = botAI->GetGroupLeader();
    Player* trueMaster = botAI->GetMaster();
    if (!groupLeader || (bot == groupLeader && !botAI->IsRealPlayer()))
        return false;

    PlayerbotAI* groupLeaderBotAI = nullptr;
    if (groupLeader)
        groupLeaderBotAI = GET_PLAYERBOT_AI(groupLeader);
    if (groupLeader && !groupLeaderBotAI)
        return false;

    if (trueMaster && !GET_PLAYERBOT_AI(trueMaster))
        return false;

    //By leewheel 2026-08-01: 按上游(4fb82ed0)统一命名约定，IsAlt改为IsAltBot
    if (botAI->IsAltBot() &&
        (!groupLeaderBotAI || groupLeaderBotAI->IsRealPlayer()))  // 当alt与玩家领队组队时不要离开队伍
    //End By leewheel
        return false;

    if (botAI->GetGrouperType() == GrouperType::SOLO)
        return true;

    uint32 dCount = AI_VALUE(uint32, "death count");

    if (dCount > 9)
        return true;

    if (dCount > 4 && !botAI->HasRealPlayerMaster())
        return true;

    if (bot->GetGuildId() == groupLeader->GetGuildId())
    {
        if (bot->GetLevel() > groupLeader->GetLevel() + 5)
        {
            if (AI_VALUE(bool, "should get money"))
                return false;
        }
    }

    if (abs(int32(groupLeader->GetLevel() - bot->GetLevel())) > 4)
        return true;

    if (bot->GetMapId() != groupLeader->GetMapId() || bot->GetDistance2d(groupLeader) >= 2 * sPlayerbotAIConfig.rpgDistance)
    {
        return true;
    }

    return false;
}
