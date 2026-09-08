/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "GuildAcceptAction.h"

#include "Event.h"
#include "GuildPackets.h"
#include "PlayerbotSecurity.h"
#include "PlayerbotTextMgr.h"
#include "Playerbots.h"

bool GuildAcceptAction::Execute(Event event)
{
    WorldPacket p(event.getPacket());
    //By leewheel 2026-08-04: 修复——TC的SMSG_GUILD_INVITE是bit-packed包,与AC扁平格式完全不同
    //AC格式: null-terminated string(InviterName) + ...
    //TC格式: WriteBits(InviterName.len,6)+WriteBits(GuildName.len,7)+WriteBits(OldGuildName.len,7)+FlushBits
    //  +uint32 InviterVirtualRealmAddress+uint32 GuildVirtualRealmAddress+ObjectGuid GuildGUID
    //  +uint32 OldGuildVirtualRealmAddress+ObjectGuid OldGuildGUID
    //  +uint32 EmblemStyle+uint32 EmblemColor+uint32 BorderStyle+uint32 BorderColor+uint32 Background
    //  +int32 AchievementPoints
    //  +WriteString(InviterName)+WriteString(GuildName)+WriteString(OldGuildName)
    //原代码 p >> Invitedname 读null-terminated string,在TC下读到的是bit-packed头部后的字节流
    //(InviterVirtualRealmAddress等),不是InviterName,导致normalizePlayerName失败,bot永不接受公会邀请
    //修复: 按TC bit-packed格式读取InviterName
    //包体最小: 3bits(对齐1字节) + 9*4 + 2*2(packed GUID最小) = 41字节 + 3个空字符串
    if (p.wpos() < p.rpos() || (p.wpos() - p.rpos()) < 1)
        return false;

    uint32 inviterNameLen = p.ReadBits(6);
    p.ReadBits(7);  // GuildName.length, 业务不需要
    p.ReadBits(7);  // OldGuildName.length, 业务不需要
    p.ResetBitPos();

    // 跳过扁平字段: 9个uint32 + 2个ObjectGuid(packed)
    uint32 inviterRealmAddr, guildRealmAddr, oldGuildRealmAddr;
    uint32 emblemStyle, emblemColor, borderStyle, borderColor, background;
    int32 achievementPoints;
    ObjectGuid guildGuid, oldGuildGuid;
    p >> inviterRealmAddr >> guildRealmAddr >> guildGuid;
    p >> oldGuildRealmAddr >> oldGuildGuid;
    p >> emblemStyle >> emblemColor >> borderStyle >> borderColor >> background >> achievementPoints;

    // 读InviterName
    std::string Invitedname(p.ReadString(inviterNameLen));
    //End By leewheel

    Player* inviter = nullptr;

    if (normalizePlayerName(Invitedname))
        inviter = ObjectAccessor::FindPlayerByName(Invitedname.c_str());

    if (!inviter)
        return false;

    bool accept = true;
    uint32 guildId = inviter->GetGuildId();
    if (!guildId)
    {
        botAI->TellError(PlayerbotTextMgr::instance().GetBotTextOrDefault(
            "guild_accept_inviter_not_in_guild", "You are not in a guild!", {}));
        accept = false;
    }
    else if (bot->GetGuildId())
    {
        botAI->TellError(PlayerbotTextMgr::instance().GetBotTextOrDefault(
            "guild_accept_already_in_guild", "Sorry, I am in a guild already", {}));
        accept = false;
    }
    else if (!botAI->GetSecurity()->CheckLevelFor(PLAYERBOT_SECURITY_INVITE, false, inviter, true))
    {
        botAI->TellError(PlayerbotTextMgr::instance().GetBotTextOrDefault(
            "guild_accept_declined", "Sorry, I don't want to join your guild :(", {}));
        accept = false;
    }

    if (accept)
    {
        //By leewheel 2026-08-27: 修复崩溃——原 CMSG_GUILD_ACCEPT(=CMSG_GUILD_ACCEPT_MEMBER=0) 构造的包
        //在 AcceptGuildInvite 的 ClientPacket 构造里 ASSERT(GetOpcode()==CMSG_ACCEPT_GUILD_INVITE) 失败崩溃。
        //改用 TC 真实 opcode CMSG_ACCEPT_GUILD_INVITE
        WorldPacket data(CMSG_ACCEPT_GUILD_INVITE);
        WPPCompat::GuildAcceptInvite(bot->GetSession(), data);
        //End By leewheel
    }
    else
    {
        //By leewheel 2026-08-27: 修复崩溃——原 CMSG_GUILD_DECLINE(=0) 同上，改用 CMSG_GUILD_DECLINE_INVITATION
        WorldPacket data(CMSG_GUILD_DECLINE_INVITATION);
        WPPCompat::GuildDeclineInvitation(bot->GetSession(), data);
        //End By leewheel
    }

    return true;
}
