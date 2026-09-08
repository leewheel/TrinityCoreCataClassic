/* 太阳之井高地 机器人策略 */
#include "SWPActions.h"
#include "EncounterHelpers.h"
#include "Playerbots.h"
#include "SWPEncounter_Twins.h"
#include "SWPSharedConstants.h"

using namespace SwpHelpers;
using namespace EncounterHelpers;

bool EredarTwinsMeleeJumpFromBalconyAction::Execute(Event /*event*/)
{
    Unit* alythess = AI_VALUE2(Unit*, "find target", "25166");
    Position const& jumpPosition = EREDAR_TWINS_P1_RANGED_POSITION;
    Position const landingPosition = GetEredarTwinsP2MeleePosition(alythess);

    constexpr float arrivalDistance = 2.0f;
    float const distanceToJumpPos = bot->GetExactDist2d(jumpPosition);

    if (distanceToJumpPos > arrivalDistance)
    {
        return MoveTo(
            SWP_MAP_ID, jumpPosition.GetPositionX(), jumpPosition.GetPositionY(),
            jumpPosition.GetPositionZ(), false, false, false, false,
            MovementPriority::MOVEMENT_FORCED, true, false);
    }

    return JumpTo(
        SWP_MAP_ID, landingPosition.GetPositionX(), landingPosition.GetPositionY(),
        landingPosition.GetPositionZ(), MovementPriority::MOVEMENT_FORCED);
}

bool EredarTwinsMisdirectBossesToTanksAction::Execute(Event /*event*/)
{
    Group* group = bot->GetGroup();
    if (!group)
        return false;

    std::vector<Player*> hunters;
    for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
    {
        Player* member = ref->GetSource();
        if (member && member->IsAlive() && member->GetMapId() == SWP_MAP_ID &&
            member->getClass() == CLASS_HUNTER && GET_PLAYERBOT_AI(member))
        {
            hunters.push_back(member);
        }

        if (hunters.size() >= 3)
            break;
    }

    int8 hunterIndex = -1;
    for (size_t i = 0; i < hunters.size(); ++i)
    {
        if (hunters[i] == bot)
        {
            hunterIndex = static_cast<int8>(i);
            break;
        }
    }
    if (hunterIndex == -1)
        return false;

    Unit* bossTarget = nullptr;
    Player* tankTarget = nullptr;
    if (hunterIndex == 0)
    {
        bossTarget = AI_VALUE2(Unit*, "find target", "25166"); // 艾莉希丝(Alythess)
        //By leewheel 2026-08-23: 合并 the-lab——GetGroupAssistTank/MainTank 单参数 API
        tankTarget = GetGroupAssistTank(bot, 0);
    }
    else if (hunterIndex == 1)
    {
        bossTarget = AI_VALUE2(Unit*, "find target", "25165"); // 萨库拉什(Sacrolash)
        tankTarget = GetGroupMainTank(bot);
    }
    else if (hunterIndex == 2)
    {
        bossTarget = AI_VALUE2(Unit*, "find target", "25165"); // 萨库拉什(Sacrolash)
        tankTarget = GetGroupAssistTank(bot, 1);
        //End By leewheel
    }

    if (!tankTarget || !tankTarget->IsAlive())
        return false;

    if (botAI->CanCastSpell("misdirection", tankTarget))
        return botAI->CastSpell("misdirection", tankTarget);

    if (bot->HasAura(Id(SwpSpells::SPELL_MISDIRECTION)) &&
        botAI->CanCastSpell("steady shot", bossTarget))
    {
        return botAI->CastSpell("steady shot", bossTarget);
    }

    return false;
}

bool EredarTwinsPositionSacrolashTanksAction::Execute(Event /*event*/)
{
    Unit* sacrolash = AI_VALUE2(Unit*, "find target", "25165");
    if (!sacrolash)
        return false;

    if (AI_VALUE(Unit*, "current target") != sacrolash)
        return Attack(sacrolash);

    if (sacrolash->GetVictim() != bot || !bot->IsWithinMeleeRange(sacrolash))
        return false;

    constexpr float arrivalDist = 2.0f;
    float moveX;
    float moveY;
    bool backwards;
    if (!GetStepToPosition(
            bot, SACROLASH_TANK_POSITION, arrivalDist, sacrolash, moveX, moveY, backwards))
    {
        return false;
    }

    return MoveTo(
        SWP_MAP_ID, moveX, moveY, bot->GetPositionZ(), false, false,
        false, false, MovementPriority::MOVEMENT_COMBAT, true, backwards);
}

bool EredarTwinsAlythessTankMoveOutOfBlazeAction::Execute(Event /*event*/)
{
    Unit* alythess = AI_VALUE2(Unit*, "find target", "25166");
    if (!alythess)
        return false;

    if (AI_VALUE(Unit*, "current target") != alythess)
        return Attack(alythess);

    auto const findSafeAlythessTankIndex =
        [&](uint8 startIndex, bool includeStart, uint8& safeIndex)
    {
        size_t const offsetStart = includeStart ? 0 : 1;
        for (size_t offset = offsetStart; offset < ALYTHESS_TANK_POSITIONS.size(); ++offset)
        {
            uint8 const candidateIndex =
                static_cast<uint8>((startIndex + offset) % ALYTHESS_TANK_POSITIONS.size());

            if (IsAlythessTankPositionSafe(
                    botAI, GetAlythessTankPosition(alythess, candidateIndex)))
            {
                safeIndex = candidateIndex;
                return true;
            }
        }

        return false;
    };

    uint8 index = _alythessTankStep;

    if (!IsAlythessTankPositionSafe(botAI, GetAlythessTankPosition(alythess, index)))
    {
        uint8 safeIndex = index;
        if (!findSafeAlythessTankIndex(index, false, safeIndex))
            return false;

        index = safeIndex;
        _alythessTankStep = index;
    }

    Position const position = GetAlythessTankPosition(alythess, index);
    constexpr float maxDistance = 1.0f;
    float const distToPosition = bot->GetExactDist2d(position);

    if (alythess->GetVictim() != bot)
        return false;

    if (distToPosition <= maxDistance && ShouldAdvanceAlythessTankPosition(alythess, bot))
    {
        uint8 safeIndex = index;
        if (!findSafeAlythessTankIndex(index, false, safeIndex))
            return false;

        index = safeIndex;
        _alythessTankStep = index;
        Position const newPos = GetAlythessTankPosition(alythess, index);

        if (bot->GetExactDist2d(newPos) > maxDistance)
        {
            return MoveTo(
                SWP_MAP_ID, newPos.GetPositionX(), newPos.GetPositionY(), newPos.GetPositionZ(),
                false, false, false, false, MovementPriority::MOVEMENT_COMBAT, true, false);
        }
    }
    else if (distToPosition > maxDistance)
    {
        return MoveTo(
            SWP_MAP_ID, position.GetPositionX(), position.GetPositionY(), position.GetPositionZ(),
            false, false, false, false, MovementPriority::MOVEMENT_COMBAT, true, false);
    }

    return false;
}

bool EredarTwinsRangedStackAtBalconyEdgeAction::Execute(Event /*event*/)
{
    Unit* sacrolash = AI_VALUE2(Unit*, "find target", "25165");
    if (sacrolash && sacrolash->GetVictim() != bot && GetEredarTwinsBlazeTarget(bot) != bot)
    {
        Position const& position = EREDAR_TWINS_P1_RANGED_POSITION;
        if (bot->GetExactDist2d(position) <= 1.0f)
            return false;

        return MoveTo(
            SWP_MAP_ID, position.GetPositionX(), position.GetPositionY(), position.GetPositionZ(),
            false, false, false, false, MovementPriority::MOVEMENT_FORCED, true, false);
    }
    // Jump down during Phase 2 or if the bot pulls aggro on Sacrolash
    else if (bot->GetPositionZ() > EREDAR_TWINS_BALCONY_Z)
    {
        Unit* alythess = AI_VALUE2(Unit*, "find target", "25166");
        Position const& jumpPosition = EREDAR_TWINS_P1_RANGED_POSITION;
        Position const landingPosition = GetEredarTwinsP2RangedPosition(alythess);

        constexpr float arrivalDistance = 2.0f;
        float const distanceToJumpPos = bot->GetExactDist2d(jumpPosition);

        if (distanceToJumpPos > arrivalDistance)
        {
            return MoveTo(
                SWP_MAP_ID, jumpPosition.GetPositionX(), jumpPosition.GetPositionY(),
                jumpPosition.GetPositionZ(), false, false, false, false,
                MovementPriority::MOVEMENT_FORCED, true, false);
        }

        return JumpTo(
            SWP_MAP_ID, landingPosition.GetPositionX(), landingPosition.GetPositionY(),
            landingPosition.GetPositionZ(), MovementPriority::MOVEMENT_FORCED);
    }

    return false;
}

bool EredarTwinsStackInRoomCenterAction::Execute(Event /*event*/)
{
    Unit* alythess = AI_VALUE2(Unit*, "find target", "25166");
    if (!alythess)
        return false;

    Position const position = PlayerbotAI::IsRanged(bot) ?
        GetEredarTwinsP2RangedPosition(alythess) :
        GetEredarTwinsP2MeleePosition(alythess);

    if (bot->GetExactDist2d(position) <= 0.5f)
        return false;

    return MoveTo(
        SWP_MAP_ID, position.GetPositionX(), position.GetPositionY(), position.GetPositionZ(),
        false, false, false, false, MovementPriority::MOVEMENT_FORCED, true, false);
}

bool EredarTwinsRemoveFlameSearAction::Execute(Event /*event*/)
{
    switch (bot->getClass())
    {
        case CLASS_MAGE:
            return botAI->CanCastSpell(Id(SwpSpells::SPELL_ICE_BLOCK), bot) &&
                botAI->CastSpell(Id(SwpSpells::SPELL_ICE_BLOCK), bot);

        case CLASS_PALADIN:
            return botAI->CanCastSpell(Id(SwpSpells::SPELL_DIVINE_SHIELD), bot) &&
                botAI->CastSpell(Id(SwpSpells::SPELL_DIVINE_SHIELD), bot);

        case CLASS_ROGUE:
            return botAI->CanCastSpell(Id(SwpSpells::SPELL_CLOAK_OF_SHADOWS), bot) &&
                botAI->CastSpell(Id(SwpSpells::SPELL_CLOAK_OF_SHADOWS), bot);

        default:
            return false;
    }
}

bool EredarTwinsDpsPrioritizeSacrolashAction::Execute(Event /*event*/)
{
    RecordEredarTwinsDpsHoldStart(bot);

    Unit* twinTarget = AI_VALUE2(Unit*, "find target", "lady sacrolash");
    float threatHoldRatio = SACROLASH_THREAT_HOLD_RATIO;
    //End By leewheel
    bool (*isTwinTank)(Player*) = IsAnySacrolashTank;

    if (!twinTarget)
    {
        twinTarget = AI_VALUE2(Unit*, "find target", "25166");
        threatHoldRatio = ALYTHESS_THREAT_HOLD_RATIO;
        isTwinTank = IsAlythessTank;
    }

    if (!twinTarget)
        return false;

    // Healers are excluded from ShouldHoldTwinThreat() but need this action so that they focus
    // their "healer dps"/wanding on Sacrolash in phase 1.
    if (ShouldHoldTwinThreat(bot, twinTarget, threatHoldRatio, isTwinTank))
    {
        bot->AttackStop();
        bot->InterruptSpell(CURRENT_MELEE_SPELL);
        bot->CastStop();
        context->GetValue<Unit*>("current target")->Set(nullptr);
        bot->SetSelection(ObjectGuid());
        return true;
    }

    if (AI_VALUE(Unit*, "current target") != twinTarget)
        return Attack(twinTarget);

    return false;
}

bool EredarTwinsConflagrationTargetMoveFromGroupAction::Execute(Event /*event*/)
{
    if (bot->getClass() == CLASS_ROGUE && botAI->CanCastSpell("vanish", bot) &&
        botAI->CastSpell("vanish", bot))
    {
        return true;
    }

    if (AI_VALUE2(Unit*, "find target", "25165"))
    {
        Position const& position = PlayerbotAI::IsRanged(bot) ?
            EREDAR_TWINS_RANGED_CONFLAG_POSITION : EREDAR_TWINS_MELEE_CONFLAG_POSITION;

        if (bot->GetExactDist2d(position) <= 1.0f)
            return false;

        return MoveTo(
            SWP_MAP_ID, position.GetPositionX(), position.GetPositionY(), position.GetPositionZ(),
            false, false, false, false, MovementPriority::MOVEMENT_FORCED, true, false);
    }

    Player* nearestPlayer = GetNearestPlayerInRadius(bot, CONFLAGRATION_SAFE_DISTANCE);
    if (!nearestPlayer)
        return false;

    float const distanceToPlayer = bot->GetExactDist2d(nearestPlayer);
    if (distanceToPlayer >= CONFLAGRATION_SAFE_DISTANCE)
        return false;

    bot->CastStop();
    return MoveAway(nearestPlayer, CONFLAGRATION_SAFE_DISTANCE - distanceToPlayer);
}

bool EredarTwinsMoveAwayFromSacrolashVictimAction::Execute(Event /*event*/)
{
    Unit* sacrolash = AI_VALUE2(Unit*, "find target", "25165");
    if (!sacrolash)
        return false;

    Unit* victim = sacrolash->GetVictim();
    if (!victim)
        return false;

    //By leewheel 2026-09-04: 上游70808114——距离判定改精确 GetExactDist2d
    if (bot->GetExactDist2d(victim) >= CONFLAGRATION_SAFE_DISTANCE)
    //End By leewheel
        return false;

    bot->CastStop();
    return MoveFromGroup(CONFLAGRATION_SAFE_DISTANCE);
}
