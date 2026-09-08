/*
 * 魔导师平台 机器人策略
 */
//By leewheel 2026-09-05: 移植来源 AC mod-playerbots MgTTriggers.cpp 移植适配 TC 框架(上游 feat tbc-mgt #2663)
//业务对标: AC azerothcore-wotlk modules/mod-playerbots src/Ai/Dungeon/MgT/MgTTriggers.cpp
//End By leewheel

#include "MgTTriggers.h"
#include "AiObjectContext.h"
#include "MgTShared.h"
#include "Playerbots.h"

bool MgTCrystalActiveTrigger::IsActive()
{
    return !AI_VALUE(ObjectGuid, "mgt crystal target").IsEmpty();
}

bool MgTOutOfRoomTrigger::IsActive()
{
    if (bot->GetPositionX() >= MagistersTerrace::SELIN_SAFE_X)
        return false;

    return MagistersTerrace::GetSelin(bot) != nullptr;
}

bool MgTInDampeningFieldTrigger::IsActive()
{
    auto const& spots = AI_VALUE_REF(MagistersTerrace::EscapeSpots, "mgt dampening escape");
    return !spots.empty();
}

bool MgTMageGuardAtRangeTrigger::IsActive()
{
    return !AI_VALUE(ObjectGuid, "mgt mage guard target").IsEmpty();
}

bool MgTArcaneNovaRangeTrigger::IsActive()
{
    auto const& spots = AI_VALUE_REF(MagistersTerrace::EscapeSpots, "mgt nova escape");
    return !spots.empty();
}

bool MgTPriorityInterruptTrigger::IsActive()
{
    return !AI_VALUE(ObjectGuid, "mgt interrupt target").IsEmpty();
}

bool MgTFocusTargetTrigger::IsActive()
{
    if (!bot->IsInCombat())
        return false;

    if (!PlayerbotAI::IsDps(bot) || PlayerbotAI::IsTank(bot))
        return false;

    if (!AI_VALUE(ObjectGuid, "mgt crystal target").IsEmpty())
        return false;

    ObjectGuid const order = AI_VALUE(ObjectGuid, "mgt focus target");
    return !order.IsEmpty() && MagistersTerrace::ResolveFocusOrder(botAI) == order;
}

bool MgTEnragedWretchedTrigger::IsActive()
{
    return !AI_VALUE(ObjectGuid, "mgt enraged wretched").IsEmpty();
}

bool MgTDelrissaInterruptTrigger::IsActive()
{
    auto const& order = AI_VALUE_REF(GuidVector, "mgt delrissa interrupt order");
    return !order.empty();
}

bool MgTDelrissaFocusTargetTrigger::IsActive()
{
    if (!bot->IsInCombat())
        return false;

    if (!PlayerbotAI::IsDps(bot) || PlayerbotAI::IsTank(bot))
        return false;

    ObjectGuid const order = AI_VALUE(ObjectGuid, "mgt delrissa focus target");
    return !order.IsEmpty() && MagistersTerrace::ResolveFocusOrder(botAI) == order;
}

bool MgTDelrissaTremorTotemTrigger::IsActive()
{
    if (!AI_VALUE(bool, "mgt delrissa tremor totem"))
        return false;

    return !AI_VALUE2(bool, "has totem", "tremor totem");
}

bool MgTFlameStrikeTrigger::IsActive()
{
    auto const& spots = AI_VALUE_REF(MagistersTerrace::EscapeSpots, "mgt flame strike escape");
    return !spots.empty();
}

bool MgTPhoenixBurnTrigger::IsActive()
{
    auto const& spots = AI_VALUE_REF(MagistersTerrace::EscapeSpots, "mgt phoenix escape");
    return !spots.empty();
}

bool MgTKaelFocusTargetTrigger::IsActive()
{
    if (!bot->IsInCombat())
        return false;

    if (!PlayerbotAI::IsDps(bot) || PlayerbotAI::IsTank(bot))
        return false;

    return !AI_VALUE(ObjectGuid, "mgt kael focus target").IsEmpty();
}

bool MgTKaelInterruptTrigger::IsActive()
{
    return !AI_VALUE(ObjectGuid, "mgt kael interrupt target").IsEmpty();
}

bool MgTGravityLapseTrigger::IsActive()
{
    return AI_VALUE(bool, "mgt gravity lapse");
}
