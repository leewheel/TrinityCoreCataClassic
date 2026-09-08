/* 太阳之井高地 机器人策略 */
/* 移植来源: brighton-chi mod-playerbots the-lab SWPActions.cpp(0123e1338..d3d21eef6 新提交) 移植适配 TC 框架 */
#include "SWPActions.h"
#include "EncounterHelpers.h"
#include "InstanceScript.h"
#include "Playerbots.h"
#include "SWPSharedConstants.h"
#include "SWPEncounter_Brut.h"
#include "SWPEncounter_Felmyst.h"
#include "SWPEncounter_Kalec.h"
#include "SWPEncounter_KJ.h"
#include "SWPEncounter_Muru.h"
#include "SWPEncounter_Twins.h"
//By leewheel 2026-09-04: 恢复重置动作的目标图标清理(对齐上游32d4f6ed/a6ca7bfb——骷髅标记清boss头图标)
#include "RtiTargetValue.h"
//End By leewheel
#include <list>

using namespace SwpHelpers;
using namespace EncounterHelpers;

bool SunwellPlateauResetEncounterStatesAction::Execute(Event /*event*/)
{
    ObjectGuid const guid = bot->GetGUID();
    uint32 const instanceId = bot->GetInstanceId();

    bool reset = false;

    // Kalecgos
    Action* kalecAction = context->GetAction("kalecgos disperse ranged");
    if (kalecAction && static_cast<KalecgosDisperseRangedAction*>(
            kalecAction)->ResetInitialRangedPositionReached())
    {
        reset = true;
    }

    // Brutallus
    auto const brutallusItr = brutallusEncounterStates.find(instanceId);
    if (brutallusItr != brutallusEncounterStates.end())
        reset |= brutallusItr->second.rangedBurnStates.erase(guid) > 0;

    reset |= ReleaseBrutallusBurnPad(bot);

    Action* brutallusAction = context->GetAction("brutallus tanks position and swap");
    if (brutallusAction && static_cast<BrutallusTanksPositionAndSwapAction*>(
            brutallusAction)->ResetInitialPositionReached())
    {
        reset = true;
    }

    // Eredar Twins
    Action* twinsAction = context->GetAction("eredar twins alythess tank move out of blaze");
    if (twinsAction && static_cast<EredarTwinsAlythessTankMoveOutOfBlazeAction*>(
            twinsAction)->ResetAlythessTankStep())
    {
        reset = true;
    }

    //By leewheel 2026-08-27: 对齐 the-lab 1e0caf61——M'uru: 重置远位到达标记
    Action* muruAction = context->GetAction("m'uru position ranged by phase");
    if (muruAction && static_cast<MuruPositionRangedByPhaseAction*>(
            muruAction)->ResetEntropiusRangedPositionReached())
    {
        reset = true;
    }
    //End By leewheel

    // Kil'jaeden
    reset |= kiljaedenDragonOrbUseTimes.erase(guid.GetCounter()) > 0;

    //By leewheel 2026-09-04: 恢复上游目标图标清理——KJ标手(MarkTargetWithSkull)打出的骷髅图标
    //在重置战斗状态时必须清除, 否则图标残留在旧目标上; 脱战状态才执行, 与上游语义一致
    if (!AI_VALUE2(bool, "combat", "self target"))
        reset |= ClearTargetIcon(bot, RtiTargetValue::skullIndex);
    //End By leewheel

    // 共享记录由mechanicTracker统一清理一次(全员), 其余bot只清自己的guid键
    if (!IsMechanicTrackerBot(bot, SWP_MAP_ID))
        return reset;

    reset |= kalecgosEncounterStates.erase(instanceId) > 0;
    reset |= brutallusEncounterStates.erase(instanceId) > 0;
    reset |= felmystEncounterStates.erase(instanceId) > 0;
    reset |= eredarTwinsIncomingConflagrationStates.erase(instanceId) > 0;
    reset |= eredarTwinsBlazeTargetStates.erase(instanceId) > 0;
    reset |= eredarTwinsDpsHoldStartMs.erase(instanceId) > 0;
    reset |= muruDarknessStates.erase(instanceId) > 0;
    reset |= muruVoidSentinelTankAssignments.erase(instanceId) > 0;
    reset |= kiljaedenEncounterStates.erase(instanceId) > 0;
    reset |= ResetKiljaedenDragonOrbUserAnnouncement(instanceId);
    reset |= kiljaedenHandControlClaims.erase(instanceId) > 0;

    //By leewheel 2026-08-14: GUID-keyed状态按本实例所有玩家清理，防止掉线bot残留串到下一场
    std::vector<ObjectGuid> const memberGuids = GetInstancePlayerGuids(bot);
    for (ObjectGuid const& memberGuid : memberGuids)
    {
        if (kiljaedenDragonOrbUseTimes.erase(memberGuid.GetCounter()) > 0)
            reset = true;
    }
    if (ClearEredarTwinsBlazeState(memberGuids))
        reset = true;
    if (kiljaedenTrackedArmageddonTargets.erase(bot->GetGUID()) > 0)
        reset = true;
    //End By leewheel

    return reset;
}

bool SunwellPlateauRemoveAuraAction::Execute(Event /*event*/)
{
    if (bot->getClass() == CLASS_MAGE && bot->HasAura(Id(SwpSpells::SPELL_ICE_BLOCK)))
    {
        bot->RemoveAura(Id(SwpSpells::SPELL_ICE_BLOCK));
        return true;
    }

    if (bot->getClass() == CLASS_PALADIN && !PlayerbotAI::IsHeal(bot) &&
        bot->HasAura(Id(SwpSpells::SPELL_DIVINE_SHIELD)))
    {
        bot->RemoveAura(Id(SwpSpells::SPELL_DIVINE_SHIELD));
        return true;
    }

    InstanceScript* instance = bot->GetInstanceScript();
    if (!instance || instance->IsEncounterInProgress())
        return false;

    // It is Blizzlike for Burn to persist after the kill, but bots will murder the raid without
    // a dedicated non-combat strategy for it. It's no fun to do that and wait around for expiry
    // so I'm just wiping the aura after the encounter.
    if (!HasBrutallusBurn(bot))
        return false;

    bot->RemoveAura(Id(SwpSpells::SPELL_BURN));
    return true;
}

namespace SwpHelpers
{

ObjectGuid FindSwpVolatileFiendGuid(Player* bot)
{
    //By leewheel 2026-09-04: 上游89a4c459——FindNearestCreature 第三参默认 true, 去掉显式实参
    Creature* fiend = bot->FindNearestCreature(
        Id(SwpNpcs::NPC_VOLATILE_FIEND), VOLATILE_FIEND_SEARCH_RADIUS);
    //End By leewheel

    return fiend ? fiend->GetGUID() : ObjectGuid::Empty;
}

}

bool VolatileFiendKeepEnemyAwayFromGroupAction::Execute(Event /*event*/)
{
    Creature* volatileFiend = botAI->GetCreature(AI_VALUE(ObjectGuid, "swp volatile fiend"));
    if (!volatileFiend || !volatileFiend->IsAlive())
        return false;

    if (PlayerbotAI::IsTank(bot))
        return AI_VALUE(Unit*, "current target") != volatileFiend && Attack(volatileFiend);

    //By leewheel 2026-09-04: 上游70808114——安全距离常量化 VOLATILE_FIEND_SAFE_DISTANCE, 距离判定改精确 GetExactDist2d
    float const currentDistance = bot->GetExactDist2d(volatileFiend);
    if (currentDistance >= VOLATILE_FIEND_SAFE_DISTANCE)
        return false;

    bot->CastStop();
    return MoveAway(volatileFiend, VOLATILE_FIEND_SAFE_DISTANCE - currentDistance);
    //End By leewheel
}

// At low health, Infernal Defense is cast, granting immunity to all damage but holy
bool ApocalypseGuardAttackWithHolyMagicAction::Execute(Event /*event*/)
{
    Unit* target = nullptr;
    constexpr float searchRadius = 40.0f;
    std::list<Creature*> apocalypseGuards;
    bot->GetCreatureListWithEntryInGrid(
        apocalypseGuards, Id(SwpNpcs::NPC_APOCALYPSE_GUARD), searchRadius);

    for (Creature* apocalypseGuard : apocalypseGuards)
    {
        if (!apocalypseGuard || !apocalypseGuard->IsAlive() ||
            !apocalypseGuard->HasAura(Id(SwpSpells::SPELL_INFERNAL_DEFENSE)))
        {
            continue;
        }

        if (!target || apocalypseGuard->GetGUID() < target->GetGUID())
            target = apocalypseGuard;
    }

    if (bot->HasAura(Id(SwpSpells::SPELL_SHADOWFORM)))
        bot->RemoveAura(Id(SwpSpells::SPELL_SHADOWFORM));

    return botAI->CanCastSpell("smite", target) && botAI->CastSpell("smite", target);
}

//By leewheel 2026-08-21: 移植 brighton-chi b49a9cc2——太阳之井通用误导动作(boss名用entry)
bool SunwellPlateauMisdirectBossToMainTankAction::Execute(Event /*event*/)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", _bossName);
    if (!boss)
        return false;

    Player* mainTank = GetGroupMainTank(bot);
    if (!mainTank || !mainTank->IsAlive())
        return false;

    if (botAI->CanCastSpell("misdirection", mainTank))
        return botAI->CastSpell("misdirection", mainTank);

    if (!bot->HasAura(Id(SwpSpells::SPELL_MISDIRECTION)))
        return false;

    return botAI->CanCastSpell("steady shot", boss) && botAI->CastSpell("steady shot", boss);
}
//End By leewheel