/*
 * 动作基类实现
 * 从 AzerothCore mod-playerbots 移植到 TrinityCore
 */

#include "Action.h"

#include "AiObjectContext.h"
#include "PlayerbotAI.h"
#include "Playerbots.h"
#include "Timer.h"

Value<Unit*>* Action::GetTargetValue() { return context->GetValue<Unit*>(GetTargetName()); }

Unit* Action::GetTarget() { return GetTargetValue()->Get(); }

ActionBasket::ActionBasket(ActionNode* action, float relevance, bool skipPrerequisites, Event event)
    : action(action), relevance(relevance), skipPrerequisites(skipPrerequisites), event(event), created(getMSTime())
{
}

bool ActionBasket::isExpired(uint32_t msecs) { return getMSTime() - created >= msecs; }
