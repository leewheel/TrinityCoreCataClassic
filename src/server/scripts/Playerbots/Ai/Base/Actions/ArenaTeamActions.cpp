/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "ArenaTeamActions.h"

#include "ArenaTeamMgr.h"
#include "CharacterCache.h"
#include "ObjectAccessor.h"
#include "PlayerbotTextMgr.h"
#include "Playerbots.h"
#include "World.h"

bool ArenaTeamAcceptAction::Execute(Event event)
{
    WorldPacket p(event.getPacket());
    //By leewheel 2026-08-04: 修复rpos(0)+长度检查
    if (p.wpos() < p.rpos() || (p.wpos() - p.rpos()) < 4)
        return false;
    //End By leewheel
    Player* inviter = nullptr;
    std::string Invitedname;
    p >> Invitedname;

    if (normalizePlayerName(Invitedname))
        inviter = ObjectAccessor::FindPlayerByName(Invitedname.c_str());

    if (!inviter)
        return false;

    //By leewheel 2026-09-08: TC-Cata的竞技场系统不使用个人竞技场数据存储
    //AC的PersonalArenaTeamStore/AddPersonalArenaTeam在TC-Cata中不存在
    //机器人直接接受竞技场邀请,统计由TC正常竞技场系统处理
    std::string text = PlayerbotTextMgr::instance().GetBotTextOrDefault(
        "arena_team_thanks_for_invite", "感谢邀请我加入竞技场队伍！", {});
    bot->Say(text, LANG_UNIVERSAL);
    TC_LOG_INFO("playerbots", "机器人 {} <{}> 接受了竞技场队伍邀请",
             bot->GetGUID().ToString().c_str(), bot->GetName().c_str());
    return true;
    //End By leewheel
}
