/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "AcceptQuestAction.h"

#include "ChatHelper.h"
#include "Event.h"
#include "PlayerbotAI.h"
#include "AiObjectContext.h"
#include "PlayerbotTextMgr.h"
#include "Playerbots.h"

bool AcceptAllQuestsAction::ProcessQuest(Quest const* quest, Object* questGiver)
{
    if (!AcceptQuest(quest, questGiver->GetGUID())) return false;

    auto text_quest = ChatHelper::FormatQuest(quest);
    bot->PlayDistanceSound(620);

    if (botAI->HasStrategy("debug quest", BotState::BOT_STATE_NON_COMBAT) || botAI->HasStrategy("debug rpg", BotState::BOT_STATE_COMBAT))
    {
        TC_LOG_INFO("playerbots", "{} => Quest [{}] accepted", bot->GetName(), quest->GetLogTitle());
        std::string text = PlayerbotTextMgr::instance().GetBotTextOrDefault(
            "quest_accept_debug",
            "Quest [%quest] accepted",
            {{"%quest", text_quest}});
        bot->Say(text, LANG_UNIVERSAL);
    }

    return true;
}

bool AcceptQuestAction::Execute(Event event)
{
    Player* requester = event.getOwner() ? event.getOwner() : GetMaster();
    if (!requester)
        return false;

    Player* bot = botAI->GetBot();
    ObjectGuid guid;
    uint32 quest = 0;

    std::string const text = event.getParam();
    PlayerbotChatHandler ch(requester);
    quest = ch.extractQuestId(text);

    bool hasAccept = false;

    if (event.getPacket().empty())
    {
        GuidVector npcs = AI_VALUE(GuidVector, "nearest npcs");
        for (auto i = npcs.begin(); i != npcs.end(); i++)
        {
            Unit* unit = botAI->GetUnit(*i);
            if (unit && quest && unit->hasQuest(quest))
            {
                guid = unit->GetGUID();
                break;
            }
            if (unit && text == "*" && bot->GetDistance(unit) <= INTERACTION_DISTANCE)
                hasAccept |= QuestAction::ProcessQuests(unit);
        }
        GuidVector gos = AI_VALUE(GuidVector, "nearest game objects no los");
        for (auto i = gos.begin(); i != gos.end(); i++)
        {
            GameObject* go = botAI->GetGameObject(*i);
            if (go && quest && go->hasQuest(quest))
            {
                guid = go->GetGUID();
                break;
            }
            if (go && text == "*" && bot->GetDistance(go) <= INTERACTION_DISTANCE)
                hasAccept |= QuestAction::ProcessQuests(go);
        }
    }
    else
    {
        WorldPacket& p = event.getPacket();
        //By leewheel 2026-08-03: 修复——客户端包rpos=2(opcode已消费)，
        //原rpos(0)把opcode 2字节当包体读导致guid/quest错乱(接任务功能静默失效)。
        //与核心一致：保留rpos(=2, opcode已消费)，从当前rpos读包体。
        //By leewheel 2026-08-04: 修复rpos(0)+长度检查
        if (p.wpos() < p.rpos() || (p.wpos() - p.rpos()) < 12)
            return false;
        //End By leewheel
        p >> guid >> quest;
    }

    if (!quest || guid.IsEmpty())
        return false;

    Quest const* qInfo = sObjectMgr->GetQuestTemplate(quest);
    if (!qInfo)
        return false;

    hasAccept |= AcceptQuest(qInfo, guid);

    if (hasAccept)
    {
        std::stringstream ss;
        ss << "AcceptQuestAction [" << qInfo->GetLogTitle() << "] - [" << std::to_string(qInfo->GetQuestId()) << "]";
        // TC_LOG_DEBUG("playerbots", "{}", ss.str().c_str());
        // botAI->TellMaster(ss.str());
    }

    return hasAccept;
}

bool AcceptQuestShareAction::Execute(Event event)
{
    Player* master = GetMaster();
    Player* bot = botAI->GetBot();

    WorldPacket& p = event.getPacket();
    //By leewheel 2026-08-03: 修复——客户端包rpos=2(opcode已消费)，去掉rpos(0)防止把opcode当包体读
    //By leewheel 2026-08-04: 修复rpos(0)+长度检查
    if (p.wpos() < p.rpos() || (p.wpos() - p.rpos()) < 4)
        return false;
    //End By leewheel
    uint32 quest;
    p >> quest;

    Quest const* qInfo = sObjectMgr->GetQuestTemplate(quest);
    if (!qInfo || !Player_GetDivider(bot))
        return false;

    quest = qInfo->GetQuestId();

    if (bot->IsActiveQuest(quest))
    {
        Player_SetDivider(bot, ObjectGuid::Empty);
        botAI->TellError(PlayerbotTextMgr::instance().GetBotTextOrDefault(
            "quest_already_have_error", "I have this quest", {}));
        return false;
    }

    //By leewheel 2026-08-01: 移植brighton-chi(5ae21f70)——altbot(玩家分身)可接受灰色任务
    if (!botAI->IsAltBot() && !bot->CanTakeQuest(qInfo, false))
    {
        // can't take quest
        Player_SetDivider(bot, ObjectGuid::Empty);
        botAI->TellError(PlayerbotTextMgr::instance().GetBotTextOrDefault(
            "quest_cant_take_error", "I can't take this quest", {}));

        return false;
    }

    if (Player_GetDivider(bot))
    {
        // send msg to quest giving player
        master->SendPushToPartyResponse(bot, QuestPushReason::Accepted);
        Player_SetDivider(bot, ObjectGuid::Empty);
    }

    if (bot->CanAddQuest(qInfo, false))
    {
        bot->AddQuest(qInfo, master);

        if (bot->CanCompleteQuest(quest))
            bot->CompleteQuest(quest);

        // Runsttren: did not add typeid switch from WorldSession::HandleQuestgiverAcceptQuestOpcode!
        // I think it's not needed, cause typeid should be TYPEID_PLAYER - and this one is not handled
        // there and there is no default case also.

        if (qInfo->GetSrcSpell() > 0)
        {
            bot->CastSpell(bot, qInfo->GetSrcSpell(), true);
        }

        botAI->TellMaster(PlayerbotTextMgr::instance().GetBotTextOrDefault(
            "quest_accept", "Quest accepted", {}));
        return true;
    }

    return false;
}

bool ConfirmQuestAction::Execute(Event event)
{
    Player* bot = botAI->GetBot();
    Player* requester = event.getOwner() ? event.getOwner() : GetMaster();

    WorldPacket& p = event.getPacket();
    //By leewheel 2026-08-03: 修复——客户端包rpos=2(opcode已消费)，去掉rpos(0)防止把opcode当包体读
    //By leewheel 2026-08-04: 修复rpos(0)+长度检查
    if (p.wpos() < p.rpos() || (p.wpos() - p.rpos()) < 4)
        return false;
    //End By leewheel
    uint32 quest;
    p >> quest;
    Quest const* qInfo = sObjectMgr->GetQuestTemplate(quest);

    if (!qInfo)
        return false;

    quest = qInfo->GetQuestId();
    //By leewheel 2026-08-01: 移植brighton-chi(5ae21f70)——altbot可接受灰色任务
    if (!botAI->IsAltBot() && !bot->CanTakeQuest(qInfo, false))
    {
        // can't take quest
        // botAI->TellError("quest_cant_take");
        return false;
    }

    if (bot->CanAddQuest(qInfo, false))
    {
        bot->AddQuest(qInfo, requester);

        if (bot->CanCompleteQuest(quest))
            bot->CompleteQuest(quest);

        if (qInfo->GetSrcSpell() > 0)
        {
            bot->CastSpell(bot, qInfo->GetSrcSpell(), true);
        }

        // botAI->TellMaster("quest_accept");
        return true;
    }

    return false;
}
