/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "AcceptInvitationAction.h"

#include "Engine.h"
#include "Event.h"
#include "ObjectAccessor.h"
#include "PlayerbotAIConfig.h"
#include "PlayerbotSecurity.h"
#include "PlayerbotTextMgr.h"
#include "Playerbots.h"
#include "PartyPackets.h"
#include "WorldPacket.h"

//By leewheel 2026-09-03 修复C4100警告：event参数在本动作中未使用（邀请信息从GetGroupInvite()获取），显式省略参数名
bool AcceptInvitationAction::Execute(Event /*event*/)
//End By leewheel
{
    Group* grp = bot->GetGroupInvite();
    if (!grp)
        return false;

    //By leewheel 2026-07-14: TC的SMSG_PARTY_INVITE包格式与AC完全不同，不尝试解析包数�?    //TC使用bot->GetGroupInvite()获取邀请信息，不需要从包中读取
    //TC的PartyInvite包包�? bits, GUIDs, realm info等复杂结�?    //End By leewheel 2026-07-14

    Player* inviter = ObjectAccessor::FindPlayer(grp->GetLeaderGUID());
    if (!inviter)
    {
        //By leewheel 2026-07-14: 组队邀请全流程日志
        if (sRandomPlayerbotMgr.IsAllBotsLoggedIn())
        {

        }
        //End By leewheel
        return false;
    }

    //By leewheel 2026-07-14: 组队邀请日志已清理
    //End By leewheel

    if (!botAI->GetSecurity()->CheckLevelFor(PLAYERBOT_SECURITY_INVITE, false, inviter))
    {
        WorldPacket data(SMSG_GROUP_DECLINE, 10);
        data << bot->GetName();
        inviter->SendDirectMessage(&data);
        bot->UninviteFromGroup();
        return false;
    }

    //By leewheel 2026-09-05: 上游6704d553——selfbot的AFK标志由客户端管理,不得在服务器侧清除
    if (bot->isAFK() && !IsSelfBot(bot))
        bot->ToggleAFK();
    //End By leewheel

    // TC使用WorldPackets::Party::PartyInviteResponse来处理组队接受
    WorldPacket packetData(CMSG_PARTY_INVITE_RESPONSE);
    WorldPackets::Party::PartyInviteResponse response(std::move(packetData));
    response.Accept = true;
    bot->GetSession()->HandlePartyInviteResponseOpcode(response);

    //By leewheel 2026-07-14: 组队接受结果日志已清理
    //End By leewheel

    if (!bot->GetGroup() || !bot->GetGroup()->IsMember(inviter->GetGUID()))
        return false;

    //By leewheel 2026-07-15: 始终为真实玩家邀请者设置master
    //原代码只对random bot设置master，导致非random bot组队后不知道跟随谁
    if (!GET_PLAYERBOT_AI(inviter))  // inviter是真实玩家
        botAI->SetMaster(inviter);
    else if (sRandomPlayerbotMgr.IsRandomBot(bot))
        botAI->SetMaster(inviter);
    //End By leewheel

    // By leewheel 2026-07-17: 诊断日志已清理
    //End By leewheel

    botAI->ResetStrategies();
    botAI->ChangeStrategy("+follow,-lfg,-bg", BOT_STATE_NON_COMBAT);
    botAI->Reset();

    //By leewheel 2026-07-18: 诊断组队打招呼空消息问题
    //By leewheel 2026-08-01: 玩家可见文本中文化
    {
        std::string helloText = PlayerbotTextMgr::instance().GetBotTextOrDefault("hello", "你好", {});
        // TC_LOG_INFO("playerbots", "[BotGreet] bot=\"{}\" inviter=\"{}\" helloText=\"{}\" textLen={} localePriority={}",
        //     bot->GetName(), inviter->GetName(), helloText, helloText.size(),
        //     PlayerbotTextMgr::instance().GetLocalePriority());
        botAI->TellMaster(helloText);
    }
    //End By leewheel

    if (sPlayerbotAIConfig.summonWhenGroup && bot->GetDistance(inviter) > sPlayerbotAIConfig.sightDistance)
    {
        Teleport(inviter, bot, true);
    }
    return true;
}
