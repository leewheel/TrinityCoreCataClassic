/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option) any later version.
 */
// 移植来源: AC mod-playerbots PetitionSignAction.cpp 移植适配 TC 框架
// 业务对标: AC azerothcore-wotlk-with-PB PetitionSignAction.cpp

#include "PetitionSignAction.h"

#include "ArenaTeam.h"
#include "Event.h"
#include "PetitionMgr.h"
#include "Playerbots.h"

bool PetitionSignAction::Execute(Event event)
{
    WorldPacket p(event.getPacket());
    //By leewheel 2026-08-04: 修复rpos(0)把opcode当包体读 + 长度检查
    //SMSG_PETITION_SHOW_SIGNATURES需2个ObjectGuid=16字节
    if (p.wpos() < p.rpos() || (p.wpos() - p.rpos()) < 16)
        return false;
    //End By leewheel

    ObjectGuid petitionGuid;
    ObjectGuid inviter;
    p >> petitionGuid >> inviter;

    //By leewheel 2026-08-04: 区分公会/竞技场签名表,分别检查不同条件
    //原代码只检查公会条件(GetGuildId/GetGuildIdInvited),导致玩家用竞技场签名表找机器人签名时
    //机器人因"已有公会"而拒绝,即使签名表是竞技场类型的
    //修复: 从petition store获取PetitionType,公会/竞技场分别检查
    Petition const* petition = sPetitionMgr->GetPetition(petitionGuid);
    if (!petition)
        return false;

    //By leewheel 2026-09-09: TC-Cata的Petition结构体无PetitionType字段
    //通过机器人自身状态判断：无公会=公会签名表，有公会=竞技场签名表
    bool isArenaPetition = (bot->GetGuildId() != 0);
    uint8 petitionType = 0;  // 竞技场签名表类型未知时默认0，后续跳过slot检查
    //End By leewheel

    bool accept = true;

    //By leewheel 2026-08-04: 根据签名表类型检查不同条件
    if (isArenaPetition)
    {
        // 竞技场签名表: 检查等级、已有战队、战队邀请状态
        if (bot->GetLevel() < sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL))
        {
            botAI->TellError("抱歉，我的等级还不够创建竞技场战队");
            accept = false;
        }

        uint8 slot = ArenaTeam::GetSlotByType(petitionType);
        if (slot < MAX_ARENA_SLOT && bot->GetArenaTeamId(slot))
        {
            botAI->TellError("抱歉，我已经有竞技场战队了");
            accept = false;
        }

        if (bot->GetArenaTeamIdInvited())
        {
            botAI->TellError("抱歉，我已经被邀请加入竞技场战队了");
            accept = false;
        }
    }
    else
    {
        // 公会签名表: 检查已有公会、公会邀请状态(原逻辑)
        if (bot->GetGuildId())
        {
            botAI->TellError("抱歉，我已经有公会了");
            accept = false;
        }

        if (bot->GetGuildIdInvited())
        {
            botAI->TellError("抱歉，我已经被邀请加入公会了");
            accept = false;
        }
    }
    //End By leewheel

    Player* _inviter = ObjectAccessor::FindPlayer(inviter);
    if (!_inviter)
        return false;

    if (_inviter == bot)
        return false;

    if (!accept || !botAI->GetSecurity()->CheckLevelFor(PLAYERBOT_SECURITY_INVITE, false, _inviter, true))
    {
        //By leewheel 2026-08-27: 修复崩溃——原代码用 MSG_PETITION_DECLINE(=0) 构造 WorldPacket，
        //WPPCompat::DeclinePetition 转 DeclinePetition 时 ClientPacket 构造 ASSERT(GetOpcode()==CMSG_DECLINE_PETITION) 失败崩溃。
        //改用 TC 真实 opcode CMSG_DECLINE_PETITION
        WorldPacket data(CMSG_DECLINE_PETITION);
        data << petitionGuid;
        WPPCompat::DeclinePetition(bot->GetSession(), data);
        //End By leewheel
        TC_LOG_INFO("playerbots", "Bot {} <{}> declines {} invite",
                 bot->GetGUID().ToString().c_str(), bot->GetName().c_str(),
                 isArenaPetition ? "arena team" : "guild");
        return false;
    }

    if (accept)
    {
        //By leewheel 2026-08-27: 修复崩溃——原代码用 CMSG_PETITION_SIGN(=0) 构造 WorldPacket，
        //WPPCompat::PetitionSignOpcode 转 SignPetition 时 ClientPacket 构造 ASSERT(GetOpcode()==CMSG_SIGN_PETITION) 失败崩溃。
        //改用 WPPCompat::SignPetition(直接设 PetitionGUID, opcode 正确为 CMSG_SIGN_PETITION)
        WPPCompat::SignPetition(bot->GetSession(), petitionGuid);
        //End By leewheel
        //By leewheel 2026-08-01: 玩家可见文本中文化
        bot->Say("感谢邀请！", LANG_UNIVERSAL);
        //End By leewheel
        TC_LOG_INFO("playerbots", "Bot {} <{}> accepts {} petition sign",
                 bot->GetGUID().ToString().c_str(), bot->GetName().c_str(),
                 isArenaPetition ? "arena team" : "guild");
        return true;
    }

    return false;
}
