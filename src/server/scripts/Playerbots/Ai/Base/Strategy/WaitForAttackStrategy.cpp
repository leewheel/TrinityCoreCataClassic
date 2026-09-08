/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "WaitForAttackStrategy.h"

#include "Action.h"
#include "PlayerbotAI.h"
#include "PlayerbotAIConfig.h"
#include "Playerbots.h"
#include "Strategy.h"

void WaitForAttackStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "wait for attack safe distance",
        {
            NextAction("wait for attack keep safe distance", ACTION_RAID)
        }
    ));
}

void WaitForAttackStrategy::InitMultipliers(std::vector<Multiplier*>& multipliers)
{
    multipliers.push_back(new WaitForAttackMultiplier(botAI));
}

bool WaitForAttackStrategy::ShouldWait(PlayerbotAI* botAI)
{
    if (botAI->HasStrategy("wait for attack", BOT_STATE_COMBAT))
    {
        Player* bot = botAI->GetBot();
        if (bot->GetGroup() && botAI->HasRealPlayerMaster())
        {
            // Don't wait if the current target is an enemy player
            Unit* target = botAI->GetAiObjectContext()->GetValue<Unit*>("current target")->Get();
            if (target && target->IsPlayer())
                return false;

            AiObjectContext* context = botAI->GetAiObjectContext();
            time_t combatStartTime = context->GetValue<time_t>("combat start time")->Get();

            //By leewheel 2026-08-18: 移植 brighton-chi the-lab c6a7d104(修复等待攻击战斗状态检测#2650)
            //改用AI状态判断战斗,避免进战斗前就提前开打;战斗开始时间改由ChangeEngineOnCombat维护
            if (botAI->GetState() == BOT_STATE_COMBAT)
            {
                if (combatStartTime == 0)
                {
                    combatStartTime = time(nullptr);
                    context->GetValue<time_t>("combat start time")->Set(combatStartTime);
                }

                return time(nullptr) - combatStartTime < GetWaitTime(botAI);
            }
            //End By leewheel
        }
    }

    return false;
}

uint8 WaitForAttackStrategy::GetWaitTime(PlayerbotAI* botAI)
{
    return botAI->GetAiObjectContext()->GetValue<uint8>("wait for attack time")->Get();
}

float WaitForAttackStrategy::GetSafeDistance()
{
    return sPlayerbotAIConfig.spellDistance;
}

float WaitForAttackMultiplier::GetValue(Action* action)
{
    std::string const& actionName = action->getName();

    if (actionName != "wait for attack keep safe distance" &&
        actionName != "dps assist" &&
        actionName != "set facing" &&
        actionName != "pull my target" &&
        actionName != "pull rti target" &&
        actionName != "reach pull" &&
        actionName != "pull start" &&
        actionName != "pull action" &&
        actionName != "pull end")
    {
        return WaitForAttackStrategy::ShouldWait(botAI) ? 0.0f : 1.0f;
    }

    return 1.0f;
}
