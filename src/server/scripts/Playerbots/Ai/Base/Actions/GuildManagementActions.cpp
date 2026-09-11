/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "GuildManagementActions.h"

#include "GuildMgr.h"
#include "GuildPackets.h"
#include "Playerbots.h"
#include "ServerFacade.h"
#include "BroadcastHelper.h"

Player* GuidManageAction::GetPlayer(Event event)
{
    Player* player = nullptr;
    ObjectGuid guid = event.getObject();

    if (guid)
    {
        player = ObjectAccessor::FindPlayer(guid);

        if (player)
            return player;
    }

    std::string text = event.getParam();

    if (!text.empty())
    {
        if (normalizePlayerName(text))
        {
            player = ObjectAccessor::FindPlayerByName(text.c_str());

            if (player)
                return player;
        }

        return nullptr;
    }

    Player* master = GetMaster();
    if (!master)
        guid = bot->GetTarget();
    else
        guid = master->GetTarget();

    player = ObjectAccessor::FindPlayer(guid);

    if (player)
        return player;

    player = event.getOwner();

    if (player)
        return player;

    return nullptr;
}

void GuidManageAction::SendPacket(WorldPacket const& packet)
{
    // make a heap copy because QueuePacket takes ownership
    WorldPacket* data = new WorldPacket(packet);

    bot->GetSession()->QueuePacket(data);
}

bool GuidManageAction::Execute(Event event)
{
    Player* player = GetPlayer(event);

    if (!player || !PlayerIsValid(player) || player == bot)
        return false;

    //By leewheel 2026-08-15: 修复——原扁平名字串经TC bit-packed(CMSG_GUILD_INVITE_BY_NAME)或ObjectGuid格式
    //(PROMOTE/DEMOTE/OFFICER_REMOVE)解析必然错位/查无此人，公会邀请/晋升/降职/踢人全部静默失效。
    //改用TC Guild的直接操作方法
    Guild* guild = sGuildMgr->GetGuildById(bot->GetGuildId());
    if (!guild)
        return false;

    switch (opcode)
    {
        case CMSG_GUILD_INVITE_BY_NAME:
            guild->HandleInviteMember(bot->GetSession(), player->GetName());
            break;
        case CMSG_GUILD_PROMOTE_MEMBER:
            guild->HandleUpdateMemberRank(bot->GetSession(), player->GetGUID(), false);
            break;
        case CMSG_GUILD_DEMOTE_MEMBER:
            guild->HandleUpdateMemberRank(bot->GetSession(), player->GetGUID(), true);
            break;
        case CMSG_GUILD_OFFICER_REMOVE_MEMBER:
            guild->HandleRemoveMember(bot->GetSession(), player->GetGUID());
            break;
        default:
            return false;
    }
    //End By leewheel

    return true;
}

bool GuidManageAction::PlayerIsValid(Player* member) { return !member->GetGuildId(); }

//By leewheel 2026-09-09: TC-Cata的GetMember非const版本是private，使用GetMembers()查找
uint8 GuidManageAction::GetRankId(Player* member)
{
    Guild* guild = sGuildMgr->GetGuildById(member->GetGuildId());
    if (!guild)
        return 0xFF;
    auto const& members = guild->GetMembers();
    auto itr = members.find(member->GetGUID());
    return itr != members.end() ? static_cast<uint8>(itr->second.GetRankId()) : 0xFF;
}
//End By leewheel

bool GuildInviteAction::isUseful()
{
    if (!bot->GetGuildId())
        return false;
    
    Guild* guild = sGuildMgr->GetGuildById(bot->GetGuildId());
    if (!guild)
        return false;

    return Guild_HasRankRight(guild, bot->GetGUID(), GR_RIGHT_INVITE);
}

bool GuildInviteAction::PlayerIsValid(Player* member)
{
    return !member->GetGuildId() && (sWorld->getBoolConfig(CONFIG_ALLOW_TWO_SIDE_INTERACTION_GUILD) ||
                                     (bot->GetTeamId() == member->GetTeamId()));
}

bool GuildPromoteAction::isUseful()
{
    if (!bot->GetGuildId())
        return false;
    
    Guild* guild = sGuildMgr->GetGuildById(bot->GetGuildId());
    if (!guild)
        return false;

    return Guild_HasRankRight(guild, bot->GetGUID(), GR_RIGHT_PROMOTE);
}

bool GuildPromoteAction::PlayerIsValid(Player* member)
{
    return member->GetGuildId() == bot->GetGuildId() && GetRankId(bot) < GetRankId(member) - 1;
}

bool GuildDemoteAction::isUseful()
{
    if (!bot->GetGuildId())
        return false;
    
    Guild* guild = sGuildMgr->GetGuildById(bot->GetGuildId());
    if (!guild)
        return false;

    return Guild_HasRankRight(guild, bot->GetGUID(), GR_RIGHT_DEMOTE);
}

bool GuildDemoteAction::PlayerIsValid(Player* member)
{
    return member->GetGuildId() == bot->GetGuildId() && GetRankId(bot) < GetRankId(member);
}

bool GuildRemoveAction::isUseful()
{
    if (!bot->GetGuildId())
        return false;
    
    Guild* guild = sGuildMgr->GetGuildById(bot->GetGuildId());
    if (!guild)
        return false;

    return Guild_HasRankRight(guild, bot->GetGUID(), GR_RIGHT_REMOVE);
}

bool GuildRemoveAction::PlayerIsValid(Player* member)
{
    return member->GetGuildId() == bot->GetGuildId() && GetRankId(bot) < GetRankId(member);
};

bool GuildManageNearbyAction::Execute(Event /*event*/)
{
    uint32 found = 0;

    Guild* guild = sGuildMgr->GetGuildById(bot->GetGuildId());
    //By leewheel 20260710: TC的Member是private类，不再直接获取Member指针
    ObjectGuid botGuid = bot->GetGUID();
    //End By leewheel

    GuidVector nearGuids = botAI->GetAiObjectContext()->GetValue<GuidVector>("nearest friendly players")->Get();
    for (auto& guid : nearGuids)
    {
        Player* player = ObjectAccessor::FindPlayer(guid);

        if (!player || bot == player)
            continue;

        if (player->isDND())
            continue;

        // Promote or demote nearby members based on chance.
        if (player->GetGuildId() && player->GetGuildId() == bot->GetGuildId())
        {
            uint32 dCount = AI_VALUE(uint32, "death count");

if (!urand(0, 30) && dCount < 2 && Guild_HasRankRight(guild, botGuid, GR_RIGHT_PROMOTE))
            {
                BroadcastHelper::BroadcastGuildMemberPromotion(botAI, bot, player);

                botAI->DoSpecificAction("guild promote", Event("guild management", guid), true);
                continue;
            }

if (!urand(0, 30) && dCount > 2 && Guild_HasRankRight(guild, botGuid, GR_RIGHT_DEMOTE))
            {
                BroadcastHelper::BroadcastGuildMemberDemotion(botAI, bot, player);

                botAI->DoSpecificAction("guild demote", Event("guild management", guid), true);
                continue;
            }

            continue;
        }

        if (!sPlayerbotAIConfig.randomBotGuildNearby)
            return false;

        if (guild->GetMembersCount() > 1000)
            return false;

if (!Guild_HasRankRight(guild, botGuid, GR_RIGHT_INVITE))
            continue;

        if (player->GetGuildIdInvited())
            continue;

        PlayerbotAI* botAi = GET_PLAYERBOT_AI(player);

        if (!sPlayerbotAIConfig.randomBotInvitePlayer && botAi && botAi->IsRealPlayer())
            continue;

        if (botAi)
        {
            if (botAi->GetGuilderType() == GuilderType::SOLO && !botAi->HasRealPlayerMaster()) //Do not invite solo players.
                continue;

            if (botAi->HasActivePlayerMaster() && !sRandomPlayerbotMgr.IsRandomBot(player)) //Do not invite alts of active players.
                continue;
        }

        bool sameGroup = bot->GetGroup() && bot->GetGroup()->IsMember(player->GetGUID());

        if (!sameGroup && ServerFacade::instance().GetDistance2d(bot, player) > sPlayerbotAIConfig.spellDistance)
            continue;

        if (sPlayerbotAIConfig.inviteChat && (sRandomPlayerbotMgr.IsRandomBot(bot) || !botAI->HasActivePlayerMaster()))
        {
            /* std::map<std::string, std::string> placeholders;
            placeholders["%name"] = player->GetName();
            placeholders["%members"] = std::to_string(guild->GetMemberSize());
            placeholders["%guildname"] = guild->GetName();
            AreaTableEntry const* current_area = botAI->GetCurrentArea();
            AreaTableEntry const* current_zone = botAI->GetCurrentZone();
            placeholders["%area_name"] = current_area ? current_area->area_name[BroadcastHelper::GetLocale()] : PlayerbotTextMgr::instance().GetBotText("string_unknown_area");
            placeholders["%zone_name"] = current_zone ? current_zone->area_name[BroadcastHelper::GetLocale()] : PlayerbotTextMgr::instance().GetBotText("string_unknown_area");

            std::vector<std::string> lines;

            //TODO - Move these hardcoded texts to sql!
            switch ((urand(0, 10) * urand(0, 10)) / 10)
            {
            case 0:
                lines.push_back(PlayerbotTextMgr::instance().GetBotText("Hey %name do you want to join my guild?", placeholders));
                break;
            case 1:
                lines.push_back(PlayerbotTextMgr::instance().GetBotText("Hey man you wanna join my guild %name?", placeholders));
                break;
            case 2:
                lines.push_back(PlayerbotTextMgr::instance().GetBotText("I think you would be a good contribution to %guildname. Would you like to join %name?", placeholders));
                break;
            case 3:
                lines.push_back(PlayerbotTextMgr::instance().GetBotText("My guild %guildname has %members quality members. Would you like to make it 1 more %name?", placeholders));
                break;
            case 4:
                lines.push_back(PlayerbotTextMgr::instance().GetBotText("Hey %name do you want to join %guildname? We have %members members and looking to become number 1 of the server.", placeholders));
                break;
            case 5:
                lines.push_back(PlayerbotTextMgr::instance().GetBotText("I'm not really good at smalltalk. Do you wanna join my guild %name/r?", placeholders));
                break;
            case 6:
                lines.push_back(PlayerbotTextMgr::instance().GetBotText("Welcome to %zone_name.... do you want to join my guild %name?", placeholders));
                break;
            case 7:
                lines.push_back(PlayerbotTextMgr::instance().GetBotText("%name, you should join my guild!", placeholders));
                break;
            case 8:
                lines.push_back(PlayerbotTextMgr::instance().GetBotText("%name, I got this guild....", placeholders));
                break;
            case 9:
                lines.push_back(PlayerbotTextMgr::instance().GetBotText("You are actually going to join my guild %name?", placeholders));
                lines.push_back(PlayerbotTextMgr::instance().GetBotText("Haha.. you are the man! We are going to raid Molten...", placeholders));
                break;
            case 10:
                lines.push_back(PlayerbotTextMgr::instance().GetBotText("Hey Hey! do you guys wanna join my gild????", placeholders));
                lines.push_back(PlayerbotTextMgr::instance().GetBotText("We've got a bunch of high levels and we are really super friendly..", placeholders));
                lines.push_back(PlayerbotTextMgr::instance().GetBotText("..and watch your dog and do your homework...", placeholders));
                lines.push_back(PlayerbotTextMgr::instance().GetBotText("..and we raid once a week and are working on MC raids...", placeholders));
                lines.push_back(PlayerbotTextMgr::instance().GetBotText("..and we have more members than just me...", placeholders));
                lines.push_back(PlayerbotTextMgr::instance().GetBotText("..and please stop I'm lonenly and we can get a ride the whole time...", placeholders));
                lines.push_back(PlayerbotTextMgr::instance().GetBotText("..and it's really beautifull and I feel like crying...", placeholders));
                lines.push_back(PlayerbotTextMgr::instance().GetBotText("So what do you guys say are you going to join are you going to join?", placeholders));
                break;
            }

            for (auto line : lines)
                if (sameGroup)
                {
                    WorldPacket data;
                    ChatHandler::BuildChatPacket(data, bot->GetGroup()->isRaidGroup() ? CHAT_MSG_RAID : CHAT_MSG_PARTY, line.c_str(), LANG_UNIVERSAL, CHAT_TAG_NONE, bot->GetGUID(), bot->GetName());
                    bot->GetGroup()->BroadcastPacket(&data, true);
                }
                else
                    bot->Say(line, (bot->GetTeamId() == TEAM_ALLIANCE ? LANG_COMMON : LANG_ORCISH));*/
        }

        if (botAI->DoSpecificAction("guild invite", Event("guild management", guid), true))
        {
            if (sPlayerbotAIConfig.inviteChat)
                return true;
            found++;
        }
    }

    return found > 0;
}

bool GuildManageNearbyAction::isUseful()
{
    if (!bot->GetGuildId())
        return false;

    Guild* guild = sGuildMgr->GetGuildById(bot->GetGuildId());

    //By leewheel 2026-09-09: TC-Cata只能用HasAnyRankRight检查单个权限
    return Guild_HasRankRight(guild, bot->GetGUID(), GR_RIGHT_DEMOTE) ||
           Guild_HasRankRight(guild, bot->GetGUID(), GR_RIGHT_PROMOTE) ||
           Guild_HasRankRight(guild, bot->GetGUID(), GR_RIGHT_INVITE);
    //End By leewheel
}

bool GuildLeaveAction::Execute(Event event)
{
    Player* owner = event.getOwner();
    if (owner && !botAI->GetSecurity()->CheckLevelFor(PLAYERBOT_SECURITY_INVITE, false, owner, true))
    {
        //By leewheel 2026-08-01: 玩家可见文本中文化
        botAI->TellError("抱歉，我在我的公会里很开心：)");
        //End By leewheel
        return false;
    }

    // By leewheel 2026-07-09: 使用WPPCompat兼容层替代直接调用
    WorldPacket data(CMSG_GUILD_LEAVE);
    WPPCompat::GuildLeaveOpcode(bot->GetSession(), data);
    // End By leewheel
    return true;
}

bool GuildLeaveAction::isUseful() { return bot->GetGuildId(); }
