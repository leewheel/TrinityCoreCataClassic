/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option) any later version.
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

    // TC中竞技场系统已重构，不再使用持久的竞技场队伍邀请流程。
    // TC的竞技场队伍是在排队时临时创建的(ArenaTeam(Group*, uint8 type))，
    // 个人竞技场数据通过 sArenaTeamMgr 的 PersonalArenaTeamStore 管理。
    //
    // 当机器人收到竞技场邀请时，正确的TC适配方式是：
    // 1. 检查机器人是否已有该类型的个人竞技场数据
    // 2. 如果没有，通过 AddPersonalArenaTeam 注册初始数据
    // 3. 让邀请者知道机器人已接受

    bool accept = true;

    // 检查机器人是否已有个人竞技场数据
    auto const& personalTeams = sArenaTeamMgr->GetPersonalArenaTeams();
    bool hasArenaStats = personalTeams.find(bot->GetGUID()) != personalTeams.end();

    if (hasArenaStats)
    {
        // 机器人已有竞技场数据，检查各slot
        auto const& stats = personalTeams.at(bot->GetGUID());
        bool inAnySlot = false;
        for (uint8 slot = 0; slot < MAX_ARENA_SLOT; ++slot)
        {
            if (stats[slot].PersonalRating > 0)
            {
                inAnySlot = true;
                break;
            }
        }

        if (inAnySlot)
        {
            std::string text = PlayerbotTextMgr::instance().GetBotTextOrDefault(
                "arena_team_already_in_team", "抱歉，我已经在竞技场队伍中了", {});
            bot->Say(text, LANG_UNIVERSAL);
            accept = false;
        }
    }

    if (accept)
    {
        // 为机器人注册个人竞技场数据
        // TC中竞技场队伍是临时的，每次排队时由Group创建
        // 机器人只需要有个人竞技场统计即可参与
        for (uint8 slot = 0; slot < MAX_ARENA_SLOT; ++slot)
        {
            PersonalArenaStats stats;
            stats.PersonalRating = sWorld->getIntConfig(CONFIG_ARENA_START_PERSONAL_RATING);
            stats.MatchMakerRating = sWorld->getIntConfig(CONFIG_ARENA_START_MATCHMAKER_RATING);
            sArenaTeamMgr->AddPersonalArenaTeam(bot, slot, stats);
        }

        // 将竞技场数据保存到数据库
        CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
        for (uint8 slot = 0; slot < MAX_ARENA_SLOT; ++slot)
        {
            CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_REP_CHARACTER_ARENA_STATS);
            stmt->setUInt64(0, bot->GetGUID().GetCounter());
            stmt->setUInt8(1, slot);
            stmt->setUInt16(2, 0);  // weekGames
            stmt->setUInt16(3, 0);  // weekWins
            stmt->setUInt16(4, 0);  // seasonGames
            stmt->setUInt16(5, 0);  // seasonWins
            stmt->setUInt16(6, static_cast<uint16>(sWorld->getIntConfig(CONFIG_ARENA_START_PERSONAL_RATING)));
            stmt->setUInt16(7, static_cast<uint16>(sWorld->getIntConfig(CONFIG_ARENA_START_MATCHMAKER_RATING)));
            stmt->setUInt16(8, 0);  // rank
            trans->Append(stmt);
        }
        CharacterDatabase.CommitTransaction(trans);

        std::string text = PlayerbotTextMgr::instance().GetBotTextOrDefault(
            "arena_team_thanks_for_invite", "感谢邀请我加入竞技场队伍！", {});
        bot->Say(text, LANG_UNIVERSAL);
        TC_LOG_INFO("playerbots", "机器人 {} <{}> 接受了竞技场队伍邀请",
                 bot->GetGUID().ToString().c_str(), bot->GetName().c_str());
        return true;
    }
    else
    {
        TC_LOG_INFO("playerbots", "机器人 {} <{}> 拒绝了竞技场队伍邀请",
                 bot->GetGUID().ToString().c_str(), bot->GetName().c_str());
        return false;
    }
}
