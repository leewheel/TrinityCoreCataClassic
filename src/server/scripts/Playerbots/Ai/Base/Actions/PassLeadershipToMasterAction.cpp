/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "PassLeadershipToMasterAction.h"

#include "Event.h"
#include "PlayerbotOperations.h"
#include "PlayerbotWorldThreadProcessor.h"

bool PassLeadershipToMasterAction::Execute(Event /*event*/)
{
    if (Player* master = GetMaster())
        if (master && master != bot && bot->GetGroup() && bot->GetGroup()->IsMember(master->GetGUID()))
        {
            auto setLeaderOp = std::make_unique<GroupSetLeaderOperation>(bot->GetGUID(), master->GetGUID());
            PlayerbotWorldThreadProcessor::instance().QueueOperation(std::move(setLeaderOp));

            if (!message.empty())
                botAI->TellMasterNoFacing(message);

            if (sRandomPlayerbotMgr.IsRandomBot(bot))
            {
                botAI->ResetStrategies();
                botAI->Reset();
            }

            return true;
        }

    return false;
}

bool PassLeadershipToMasterAction::isUseful()
{
    //By leewheel 2026-08-01: 按上游(4fb82ed0)统一命名约定，IsAlt改为IsAltBot
    return botAI->IsAltBot() && bot->GetGroup() && bot->GetGroup()->IsLeader(bot->GetGUID());
    //End By leewheel
}

bool GiveLeaderAction::isUseful()
{
    //By leewheel 2026-09-05: 上游9daa24f2——允许 selfbot 被指派为队长
    return (IsRealPlayer(botAI->GetMaster()) || IsSelfBot(botAI->GetMaster())) && bot->GetGroup() &&
           bot->GetGroup()->IsLeader(bot->GetGUID());
    //End By leewheel
}
