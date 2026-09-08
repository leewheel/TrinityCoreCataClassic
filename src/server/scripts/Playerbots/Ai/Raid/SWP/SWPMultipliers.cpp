/* 太阳之井高地 机器人策略 */
#include "SWPMultipliers.h"
#include "ChooseTargetActions.h"
#include "DruidActions.h"
#include "EncounterHelpers.h"
#include "FollowActions.h"
#include "GenericSpellActions.h"
#include "HunterActions.h"
//By leewheel 2026-09-04: 上游70808114——NoEncounterDrinking 乘数需要 InstanceScript 与 NonCombatActions(DrinkAction)
#include "InstanceScript.h"
#include "MageActions.h"
#include "NonCombatActions.h"
#include "ReachTargetActions.h"
#include "RogueActions.h"
#include "ShamanActions.h"
#include "SWPActions.h"
#include "SWPSharedConstants.h"
#include "SWPEncounter_Brut.h"
#include "SWPEncounter_Felmyst.h"
#include "SWPEncounter_Kalec.h"
#include "SWPEncounter_KJ.h"
#include "SWPEncounter_Muru.h"
#include "SWPEncounter_Twins.h"
#include "Timer.h"
#include "WipeAction.h"

using namespace SwpHelpers;
using namespace EncounterHelpers;

// General

//By leewheel 2026-09-04: 上游70808114——补 SunwellPlateauNoEncounterDrinkingMultiplier 实现
//(战斗中禁止喝水动作, 防开战后误触发回蓝)
float SunwellPlateauNoEncounterDrinkingMultiplier::GetValue(Action* action)
{
    if (bot->GetMapId() != SWP_MAP_ID)
        return 1.0f;

    InstanceScript* instance = bot->GetInstanceScript();
    if (!instance || !instance->IsEncounterInProgress())
        return 1.0f;

    return dynamic_cast<DrinkAction*>(action) ? 0.0f : 1.0f;
}
//End By leewheel

//By leewheel 2026-09-04: 上游70808114——新增 Trash 段: 内敛恶魔犬接近抑制乘数
// Trash

float VolatileFiendRestrictApproachMultiplier::GetValue(Action* action)
{
    if (!dynamic_cast<CastReachTargetSpellAction*>(action) &&
        !dynamic_cast<ReachTargetAction*>(action))
    {
        return 1.0f;
    }

    if (PlayerbotAI::IsTank(bot))
        return 1.0f;

    Creature* volatileFiend = botAI->GetCreature(AI_VALUE(ObjectGuid, "swp volatile fiend"));
    if (!volatileFiend || !volatileFiend->IsAlive())
        return 1.0f;

    return bot->GetExactDist2d(volatileFiend) < VOLATILE_FIEND_APPROACH_SUPPRESSION_RADIUS ?
        0.0f : 1.0f;
}
//End By leewheel

// Kalecgos

float KalecgosControlMisdirectionMultiplier::GetValueInEncounter(Action* action)
{
    if (botAI->GetState() == BOT_STATE_NON_COMBAT)
        return 1.0f;

    if (bot->getClass() != CLASS_HUNTER)
        return 1.0f;

    if (!dynamic_cast<CastMisdirectionOnMainTankAction*>(action))
        return 1.0f;

    if (AI_VALUE2(Unit*, "find target", "24850"))
        return 0.0f;

    return 1.0f;
}

float KalecgosWaitToDecurseMultiplier::GetValueInEncounter(Action* action)
{
    if (bot->getClass() != CLASS_DRUID && bot->getClass() != CLASS_MAGE &&
        bot->getClass() != CLASS_SHAMAN)
    {
        return 1.0f;
    }

    if (!dynamic_cast<CastRemoveCurseAction*>(action) &&
        !dynamic_cast<CastRemoveCurseOnPartyAction*>(action) &&
        !dynamic_cast<CastCleanseSpiritAction*>(action) &&
        !dynamic_cast<CastCleanseSpiritCurseOnPartyAction*>(action) &&
        !dynamic_cast<CastDruidRemoveCurseOnPartyAction*>(action))
    {
        return 1.0f;
    }

    if (!AI_VALUE2(Unit*, "find target", "24850"))
        return 1.0f;

    Unit* target = AI_VALUE2(Unit*, "party member to dispel", DISPEL_CURSE);
    if (!target)
        return 1.0f;

    // Like Illidan's Shadowfiends, the spread from player-to-player is a separate spell
    Aura* aura = target->GetAura(Id(SwpSpells::SPELL_CURSE_OF_BOUNDLESS_AGONY));
    if (!aura)
        aura = target->GetAura(Id(SwpSpells::SPELL_CURSE_OF_BOUNDLESS_AGONY_SEC));

//By leewheel 2026-08-23: the-lab 重构为常量 KALECGOS_DISPEL_REMAINING_MS(15000=15秒)
    return aura && aura->GetDuration() >= KALECGOS_DISPEL_REMAINING_MS ? 0.0f : 1.0f;
    //End By leewheel
}

float KalecgosControlMovementMultiplier::GetValueInEncounter(Action* action)
{
    if (botAI->GetState() == BOT_STATE_NON_COMBAT)
        return 1.0f;

    if (!dynamic_cast<FollowAction*>(action) && !dynamic_cast<FleeAction*>(action) &&
        !dynamic_cast<CombatFormationMoveAction*>(action))
    {
        return 1.0f;
    }

    if (dynamic_cast<SetBehindTargetAction*>(action))
        return 1.0f;

    Unit* kalecgos = AI_VALUE2(Unit*, "find target", "24850");
    if (kalecgos && !kalecgos->IsFriendlyTo(bot))
        return 0.0f;

    return 1.0f;
}

// Avoid dueling taunts in the surface and spectral realms
float KalecgosRestrictTauntMultiplier::GetValueInEncounter(Action* action)
{
    if (botAI->GetState() == BOT_STATE_NON_COMBAT)
        return 1.0f;

    if (!PlayerbotAI::IsTank(bot))
        return 1.0f;

    if (!IsTauntAction(bot, action))
        return 1.0f;

    if (!AI_VALUE2(Unit*, "find target", "24850"))
        return 1.0f;

    if (!IsInSpectralRealm(bot))
        return FindKalecgosDesignatedTank(bot) == bot ? 1.0f : 0.0f;

    Unit* sathrovarr = AI_VALUE2(Unit*, "find target", "sathrovarr the corruptor");
    if (!sathrovarr)
        return 1.0f;

    Unit* victim = sathrovarr->GetVictim();
    Player* victimPlayer = victim ? victim->ToPlayer() : nullptr;
    return victimPlayer && PlayerbotAI::IsTank(victimPlayer) ? 0.0f : 1.0f;
}

float KalecgosSuppressAssistTankPullThreatMultiplier::GetValueInEncounter(Action* action)
{
    if (!dynamic_cast<CastSpellAction*>(action) && !dynamic_cast<AttackAction*>(action))
        return 1.0f;

    if (!AI_VALUE2(Unit*, "find target", "24850"))
        return 1.0f;

    if (!PlayerbotAI::IsAssistTank(bot))
        return 1.0f;

    auto const stateItr = kalecgosEncounterStates.find(bot->GetInstanceId());
    if (stateItr == kalecgosEncounterStates.end() || !stateItr->second.encounterStartMs)
        return 1.0f;

    //By leewheel 2026-08-23: the-lab 重构为常量 KALECGOS_PULL_THREAT_SUPPRESSION_MS(5000)
    return getMSTimeDiff(stateItr->second.encounterStartMs, getMSTime()) <
        KALECGOS_PULL_THREAT_SUPPRESSION_MS ? 0.0f : 1.0f;
    //End By leewheel
}

float KalecgosEnterSpectralRiftMultiplier::GetValueInEncounter(Action* action)
{
    if (!dynamic_cast<MovementAction*>(action) &&
        !dynamic_cast<CastReachTargetSpellAction*>(action))
    {
        return 1.0f;
    }

    if (dynamic_cast<AttackAction*>(action))
        return 1.0f;

    if (dynamic_cast<KalecgosEnterSpectralRiftAction*>(action))
        return 1.0f;

    if (!AI_VALUE2(Unit*, "find target", "kalecgos"))
        return 1.0f;

    if (!ShouldEnterKalecgosPortal(bot))
        return 1.0f;

    return botAI->GetGameObject(AI_VALUE(ObjectGuid, "kalecgos spectral rift")) ? 0.0f : 1.0f;
}

float KalecgosDelayCooldownsForSathrovarrMultiplier::GetValueInEncounter(Action* action)
{
    if (botAI->GetState() == BOT_STATE_NON_COMBAT)
        return 1.0f;

    if (!IsDpsCooldownAction(bot, action))
        return 1.0f;

    if (!AI_VALUE2(Unit*, "find target", "24850"))
        return 1.0f;

    if (!IsInSpectralRealm(bot))
        return 0.0f;

    return 1.0f;
}

// Brutallus

float BrutallusControlMisdirectionMultiplier::GetValueInEncounter(Action* action)
{
    if (botAI->GetState() == BOT_STATE_NON_COMBAT)
        return 1.0f;

    if (bot->getClass() != CLASS_HUNTER)
        return 1.0f;

    if (!dynamic_cast<CastMisdirectionOnMainTankAction*>(action))
        return 1.0f;

    if (AI_VALUE2(Unit*, "find target", "24882"))
        return 0.0f;

    return 1.0f;
}

float BrutallusControlMovementMultiplier::GetValueInEncounter(Action* action)
{
    if (botAI->GetState() == BOT_STATE_NON_COMBAT)
        return 1.0f;

    if (!dynamic_cast<FollowAction*>(action) &&
        !dynamic_cast<ReachTargetAction*>(action) &&
        !dynamic_cast<CombatFormationMoveAction*>(action) &&
        !dynamic_cast<FleeAction*>(action) &&
        !dynamic_cast<CastReachTargetSpellAction*>(action) &&
        !dynamic_cast<CastDisengageAction*>(action) &&
        !dynamic_cast<CastBlinkBackAction*>(action))
    {
        return 1.0f;
    }

    if (AI_VALUE2(Unit*, "find target", "24882"))
        return 0.0f;

    return 1.0f;
}

// Don't use KS if any melee member (other than the Brutallus tanks) has Burn
float BrutallusNoKillingSpreeWhenNearbyBurnMultiplier::GetValueInEncounter(Action* action)
{
    if (botAI->GetState() == BOT_STATE_NON_COMBAT)
        return 1.0f;

    if (bot->getClass() != CLASS_ROGUE)
        return 1.0f;

    if (!dynamic_cast<CastKillingSpreeAction*>(action))
        return 1.0f;

    if (!AI_VALUE2(Unit*, "find target", "24882"))
        return 1.0f;

    Group* group = bot->GetGroup();
    if (!group)
        return 1.0f;

    for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
    {
        Player* member = ref->GetSource();
        if (!member || !member->HasAura(Id(SwpSpells::SPELL_BURN)))
            continue;

        if (PlayerbotAI::IsMelee(member) && !PlayerbotAI::IsMainTank(member) &&
            !PlayerbotAI::IsAssistTankOfIndex(member, 0, true))
        {
            return 0.0f;
        }
    }

    return 1.0f;
}

float BrutallusRestrictTauntMultiplier::GetValueInEncounter(Action* action)
{
    if (botAI->GetState() == BOT_STATE_NON_COMBAT)
        return 1.0f;

    if (!IsTauntAction(bot, action))
        return 1.0f;

    //By leewheel 2026-08-21: 移植 brighton-chi 2eaa83f3——只限制当前受害者是坦克时的嘲讽,
    //避免误拦非坦玩家被击时用于救场的嘲讽
    Unit* brutallus = AI_VALUE2(Unit*, "find target", "24882");
    if (!brutallus)
        return 1.0f;

    Unit* victim = brutallus->GetVictim();
    if (!victim)
        return 1.0f;

    Player* playerVictim = victim->ToPlayer();
    return playerVictim && PlayerbotAI::IsTank(playerVictim) ? 0.0f : 1.0f;
    //End By leewheel
}

float BrutallusDelayCooldownsMultiplier::GetValueInEncounter(Action* action)
{
    if (botAI->GetState() == BOT_STATE_NON_COMBAT)
        return 1.0f;

    if (!IsDpsCooldownAction(bot, action))
        return 1.0f;

    Unit* brutallus = AI_VALUE2(Unit*, "find target", "24882");
    if (brutallus && brutallus->GetHealthPct() > BOSS_ENGAGED_HEALTH_PCT)
        return 0.0f;

    return 1.0f;
}

// Felmyst

float FelmystControlMovementMultiplier::GetValueInEncounter(Action* action)
{
    if (botAI->GetState() == BOT_STATE_NON_COMBAT)
        return 1.0f;

    if (!dynamic_cast<CombatFormationMoveAction*>(action) &&
        !dynamic_cast<FleeAction*>(action) &&
        !dynamic_cast<CastDisengageAction*>(action) &&
        !dynamic_cast<CastBlinkBackAction*>(action))
    {
        return 1.0f;
    }

    if (AI_VALUE2(Unit*, "find target", "25038"))
        return 0.0f;

    return 1.0f;
}

float FelmystWaitForLandingDpsMultiplier::GetValueInEncounter(Action* action)
{
    if (!dynamic_cast<CastSpellAction*>(action) && !dynamic_cast<AttackAction*>(action))
        return 1.0f;

    if (dynamic_cast<CastHealingSpellAction*>(action))
        return 1.0f;

    Unit* felmyst = AI_VALUE2(Unit*, "find target", "25038");
    if (!felmyst)
        return 1.0f;

    if (PlayerbotAI::IsMainTank(bot))
        return 1.0f;

    //By leewheel 2026-08-23: the-lab 重构——find + 短路判断, 语义等价
    auto const stateItr = felmystEncounterStates.find(felmyst->GetInstanceId());
    return stateItr != felmystEncounterStates.end() && stateItr->second.landingDpsWaitStartMs ?
        0.0f : 1.0f;
    //End By leewheel
}

float FelmystPrioritizeEncapsulateAvoidanceMultiplier::GetValueInEncounter(Action* action)
{
    if (!dynamic_cast<MovementAction*>(action) &&
        !dynamic_cast<CastReachTargetSpellAction*>(action))
    {
        return 1.0f;
    }

    if (dynamic_cast<FelmystRunAwayFromEncapsulatedPlayerAction*>(action))
        return 1.0f;

    Unit* felmyst = AI_VALUE2(Unit*, "find target", "25038");
    if (!felmyst || felmyst->IsFlying())
        return 1.0f;

    if (GetFelmystEncapsulateTarget(bot))
        return 0.0f;

    return 1.0f;
}

float FelmystPrioritizeFogAvoidanceMultiplier::GetValueInEncounter(Action* action)
{
    if (botAI->GetState() == BOT_STATE_NON_COMBAT)
        return 1.0f;

    if (!dynamic_cast<MovementAction*>(action) &&
        !dynamic_cast<CastReachTargetSpellAction*>(action))
    {
        return 1.0f;
    }

    if (dynamic_cast<AttackAction*>(action))
        return 1.0f;

    if (dynamic_cast<FelmystMoveToSafeFogLaneAction*>(action))
        return 1.0f;

    Unit* felmyst = AI_VALUE2(Unit*, "find target", "25038");
    if (!felmyst || !felmyst->IsFlying())
        return 1.0f;

    //By leewheel 2026-08-23: the-lab 重构为 IsFelmystFogMovementSuppressed helper
    return IsFelmystFogMovementSuppressed(felmyst) ? 0.0f : 1.0f;
    //End By leewheel
}

float FelmystPrioritizeDemonicVaporAvoidanceMultiplier::GetValueInEncounter(Action* action)
{
    if (!dynamic_cast<FollowAction*>(action) &&
        !dynamic_cast<ReachTargetAction*>(action) &&
        !dynamic_cast<CastReachTargetSpellAction*>(action))
    {
        return 1.0f;
    }

    Unit* felmyst = AI_VALUE2(Unit*, "find target", "25038");
    if (!felmyst || !felmyst->IsFlying())
        return 1.0f;

    if (IsFelmystFogActiveForBot(bot, felmyst))
        return 1.0f;

    if (IsFelmystLanding(felmyst))
        return 1.0f;

    return 0.0f;
}

float FelmystFocusAttacksOnCharmedPlayerMultiplier::GetValueInEncounter(Action* action)
{
    if (!PlayerbotAI::IsDps(bot))
        return 1.0f;

    // The charmed player is still friendly to group members so is considered to be an
    // invalid target by bots; blocking "drop target" allows them to be attacked
    if (!dynamic_cast<DpsAssistAction*>(action) && !dynamic_cast<DropTargetAction*>(action))
        return 1.0f;

    Unit* felmyst = AI_VALUE2(Unit*, "find target", "25038");
    if (!felmyst)
        return 1.0f;

    Player* charmedPlayer = GetFelmystCharmedTarget(bot, felmyst);
    if (!charmedPlayer)
        return 1.0f;

    // Melee: attack only during flight phase when the charmed player is in melee range
    if (PlayerbotAI::IsMelee(bot) &&
        (!felmyst->IsFlying() || !bot->IsWithinMeleeRange(charmedPlayer)))
    {
        return 0.0f;
    }

//By leewheel 2026-08-23: the-lab 重构为常量 FELMYST_CHARMED_TARGET_RANGE(30y), 变量名统一 charmedPlayer
    // Ranged: attack at any time the charmed player is in general spell range
    return PlayerbotAI::IsRanged(bot) &&
        bot->GetExactDist2d(charmedPlayer) < FELMYST_CHARMED_TARGET_RANGE ? 0.0f : 1.0f;
    //End By leewheel
}

float FelmystDontDotAddsMultiplier::GetValueInEncounter(Action* action)
{
    if (botAI->GetState() == BOT_STATE_NON_COMBAT)
        return 1.0f;

    if (!dynamic_cast<CastDebuffSpellOnAttackerAction*>(action))
        return 1.0f;

    Unit* felmyst = AI_VALUE2(Unit*, "find target", "25038");
    if (!felmyst || !felmyst->IsFlying())
        return 1.0f;

    if (action->GetTarget() != felmyst)
        return 0.0f;

    return 1.0f;
}

float FelmystDelayCooldownsMultiplier::GetValueInEncounter(Action* action)
{
    if (botAI->GetState() == BOT_STATE_NON_COMBAT)
        return 1.0f;

    if (!IsDpsCooldownAction(bot, action))
        return 1.0f;

    Unit* felmyst = AI_VALUE2(Unit*, "find target", "25038");
    if (felmyst && (felmyst->IsFlying() || felmyst->GetHealthPct() > BOSS_ENGAGED_HEALTH_PCT))
        return 0.0f;

    return 1.0f;
}

// Eredar Twins

float EredarTwinsDisableAutomaticTargetingMultiplier::GetValueInEncounter(Action* action)
{
    if (botAI->GetState() == BOT_STATE_NON_COMBAT)
        return 1.0f;

    if (!dynamic_cast<DpsAssistAction*>(action) && !dynamic_cast<TankAssistAction*>(action) &&
        !dynamic_cast<CastDebuffSpellOnAttackerAction*>(action))
    {
        return 1.0f;
    }

    if (AI_VALUE2(Unit*, "find target", "25166"))
        return 0.0f;

    return 1.0f;
}

float EredarTwinsControlMisdirectionMultiplier::GetValueInEncounter(Action* action)
{
    if (botAI->GetState() == BOT_STATE_NON_COMBAT)
        return 1.0f;

    if (bot->getClass() != CLASS_HUNTER)
        return 1.0f;

    if (!dynamic_cast<CastMisdirectionOnMainTankAction*>(action))
        return 1.0f;

    if (AI_VALUE2(Unit*, "find target", "25166"))
        return 0.0f;

    return 1.0f;
}

float EredarTwinsHoldDpsAtStartMultiplier::GetValueInEncounter(Action* action)
{
    if (PlayerbotAI::IsTank(bot))
        return 1.0f;

    // No AttackAction block. Commencing auto-attack gets bots positioned, but don't use abilities.
    if (!dynamic_cast<CastSpellAction*>(action))
        return 1.0f;

    if (dynamic_cast<CastHealingSpellAction*>(action))
        return 1.0f;

    if (dynamic_cast<EredarTwinsMisdirectBossesToTanksAction*>(action))
        return 1.0f;

    if (PlayerbotAI::IsMelee(bot) && bot->GetPositionZ() > EREDAR_TWINS_BALCONY_Z)
        return 1.0f;

    if (!AI_VALUE2(Unit*, "find target", "25165"))
        return 1.0f;

    auto const it = eredarTwinsDpsHoldStartMs.find(bot->GetInstanceId());
    if (it == eredarTwinsDpsHoldStartMs.end())
        return 0.0f;

    return getMSTimeDiff(it->second, getMSTime()) < EREDAR_TWINS_DPS_HOLD_MS ? 0.0f : 1.0f;
    //End By leewheel
}

float EredarTwinsControlThreatMultiplier::GetValueInEncounter(Action* action)
{
    if (PlayerbotAI::IsHeal(bot)) // early return; already excluded from ShouldHoldTwinThreat()
        return 1.0f;

    if (!dynamic_cast<CastSpellAction*>(action) && !dynamic_cast<AttackAction*>(action))
        return 1.0f;

    if (dynamic_cast<EredarTwinsDpsPrioritizeSacrolashAction*>(action))
        return 1.0f;

    Unit* alythess = AI_VALUE2(Unit*, "find target", "25166");
    Unit* sacrolash = AI_VALUE2(Unit*, "find target", "25165");

    bool const shouldHoldSacrolashThreat = sacrolash && !PlayerbotAI::IsTank(bot) &&
        ShouldHoldTwinThreat(bot, sacrolash, SACROLASH_THREAT_HOLD_RATIO, IsAnySacrolashTank);
    bool const shouldHoldAlythessThreat = alythess &&
        ShouldHoldTwinThreat(bot, alythess, ALYTHESS_THREAT_HOLD_RATIO, IsAlythessTank);

    if (!shouldHoldSacrolashThreat && !shouldHoldAlythessThreat)
        return 1.0f;

    Unit* actionTarget = action->GetTarget();
    bool const suppressSacrolashAttack = shouldHoldSacrolashThreat &&
        (actionTarget == sacrolash || AI_VALUE(Unit*, "current target") == sacrolash);
    bool const suppressAlythessAttack = shouldHoldAlythessThreat &&
        (actionTarget == alythess || AI_VALUE(Unit*, "current target") == alythess);

    if (suppressSacrolashAttack || suppressAlythessAttack)
        return 0.0f;

    return 1.0f;
}

float EredarTwinsControlMovementMultiplier::GetValueInEncounter(Action* action)
{
    if (botAI->GetState() == BOT_STATE_NON_COMBAT)
        return 1.0f;

    bool const isReachAction =
        dynamic_cast<ReachTargetAction*>(action) ||
        dynamic_cast<CastReachTargetSpellAction*>(action);

    if (!isReachAction &&
        !dynamic_cast<CombatFormationMoveAction*>(action) &&
        !dynamic_cast<FleeAction*>(action) &&
        !dynamic_cast<CastDisengageAction*>(action) &&
        !dynamic_cast<CastBlinkBackAction*>(action) &&
        !dynamic_cast<CastKillingSpreeAction*>(action) &&
        //By leewheel 2026-08-21: 移植 brighton-chi 2eaa83f3——修复逻辑取反bug:
        //坦克的规避AOE动作本应放行, 原代码取反后反而被拦截
        !(PlayerbotAI::IsTank(bot) && dynamic_cast<AvoidAoeAction*>(action)))
        //End By leewheel
    {
        return 1.0f;
    }

    if (!AI_VALUE2(Unit*, "find target", "25166"))
        return 1.0f;

    if (!isReachAction)
        return 0.0f;

    if (PlayerbotAI::IsRanged(bot) || IsAlythessTank(bot))
        return 0.0f;

    return 1.0f;
}

//By leewheel 2026-09-04: 上游——签名改 GetValueInEncounter; 逻辑重构: 拦截条件改为
//  ReachTargetAction/CombatFormationMoveAction/CastReachTargetSpellAction 三类, 新增伊莎利丝坦(需继续本职)放行,
//  尾段简化为"萨洛拉丝当前目标即燃烧目标则拦截"单一判定
float EredarTwinsIsolateConflagrationMultiplier::GetValueInEncounter(Action* action)
{
    if (!dynamic_cast<ReachTargetAction*>(action) &&
        !dynamic_cast<CombatFormationMoveAction*>(action) &&
        !dynamic_cast<CastReachTargetSpellAction*>(action))
    {
        return 1.0f;
    }

    if (dynamic_cast<EredarTwinsConflagrationTargetMoveFromGroupAction*>(action) ||
        dynamic_cast<EredarTwinsMoveAwayFromSacrolashVictimAction*>(action))
    {
        return 1.0f;
    }

    if (!AI_VALUE2(Unit*, "find target", "25166"))
        return 1.0f;

    Player* conflagTarget = GetEredarTwinsConflagrationTarget(bot);
    if (!conflagTarget)
        return 1.0f;

    // Block movement for bot targeted by Conflagration, unless the target is a Rogue that has
    // vanished and caused Alythess to drop the target.
    if (conflagTarget == bot)
        return bot->getClass() == CLASS_ROGUE && botAI->HasAura("vanish", bot) ? 1.0f : 0.0f;

    if (IsAlythessTank(bot)) // This bot needs to keep doing its job.
        return 1.0f;

    // If Sacrolash's victim is targeted by Conflagration, block actions that move toward Sacrolash.
    Unit* sacrolash = AI_VALUE2(Unit*, "find target", "25165");
    if (!sacrolash)
        return 1.0f;

    Unit* victim = sacrolash->GetVictim();
    return victim && victim != bot && conflagTarget == victim ? 0.0f : 1.0f;
}
//End By leewheel

float EredarTwinsDelayCooldownsMultiplier::GetValueInEncounter(Action* action)
{
    if (botAI->GetState() == BOT_STATE_NON_COMBAT)
        return 1.0f;

    if (!IsDpsCooldownAction(bot, action))
        return 1.0f;

    Unit* alythess = AI_VALUE2(Unit*, "find target", "25166");
    if (!alythess)
        return 1.0f;

    Unit* sacrolash = AI_VALUE2(Unit*, "find target", "25165");
    //By leewheel 2026-09-04 修复: 统一用 EREDAR_TWINS_MAX_DPS_HP_PERCENT 常量(值同为80.0f, 消除硬编码)
    if (sacrolash && sacrolash->GetHealthPct() > EREDAR_TWINS_MAX_DPS_HP_PERCENT)
        return 0.0f;

    //By leewheel 2026-09-04 修复: 上游安全写法恢复——sacrolash 为 null(萨库拉丝已死, P2阶段)时
    //直接解引用必空指针崩溃(此前 entry 化重构引入的回归), 补回空判
    return sacrolash && sacrolash->GetHealthPct() > EREDAR_TWINS_MAX_DPS_HP_PERCENT ? 0.0f : 1.0f;
    //End By leewheel
}

// M'uru

float MuruDisableDefaultTargetingMultiplier::GetValueInEncounter(Action* action)
{
    if (botAI->GetState() == BOT_STATE_NON_COMBAT)
        return 1.0f;

    bool const isDpsAssist = dynamic_cast<DpsAssistAction*>(action);
    bool const isTankAssist = dynamic_cast<TankAssistAction*>(action);

    if (!isDpsAssist && !isTankAssist && !dynamic_cast<CastDebuffSpellOnAttackerAction*>(action))
        return 1.0f;

    if (!AI_VALUE2(Unit*, "find target", "25741"))
        return 1.0f;

    if (isDpsAssist)
        return 0.0f;

    if (isTankAssist && PlayerbotAI::IsAssistTankOfIndex(bot, 0, true) &&
        AI_VALUE2(Unit*, "find target", "25772"))
    {
        return 0.0f;
    }

    //By leewheel 2026-08-21: 移植 brighton-chi 2eaa83f3——虚空幼体存在时禁用对其的次要DOT
    //By leewheel 2026-08-22: 随上游(9e8e7718..70b60954)改为按当前目标entry判定, 去掉40码邻近搜索
    Unit* currentTarget = AI_VALUE(Unit*, "current target");
    return currentTarget && currentTarget->GetEntry() == Id(SwpNpcs::NPC_VOID_SPAWN) ? 0.0f : 1.0f;
    //End By leewheel
}

float MuruControlMisdirectionMultiplier::GetValueInEncounter(Action* action)
{
    if (botAI->GetState() == BOT_STATE_NON_COMBAT)
        return 1.0f;

    if (bot->getClass() != CLASS_HUNTER)
        return 1.0f;

    if (!dynamic_cast<CastMisdirectionOnMainTankAction*>(action))
        return 1.0f;

    if (AI_VALUE2(Unit*, "find target", "25840"))
        return 1.0f;

    if (AI_VALUE2(Unit*, "find target", "25741"))
        return 0.0f;

    return 1.0f;
}

float MuruControlMovementMultiplier::GetValueInEncounter(Action* action)
{
    if (botAI->GetState() == BOT_STATE_NON_COMBAT)
        return 1.0f;

    bool const isReachAction =
        dynamic_cast<ReachTargetAction*>(action) ||
        dynamic_cast<CastReachTargetSpellAction*>(action);

    if (!isReachAction &&
        !dynamic_cast<FollowAction*>(action) &&
        !dynamic_cast<FleeAction*>(action) &&
        !dynamic_cast<CombatFormationMoveAction*>(action) &&
        !dynamic_cast<CastDisengageAction*>(action) &&
        !dynamic_cast<CastBlinkBackAction*>(action))
    {
        return 1.0f;
    }

    if (dynamic_cast<SetBehindTargetAction*>(action))
        return 1.0f;

    Unit* muru = AI_VALUE2(Unit*, "find target", "25741");
    if (!muru)
        return 1.0f;

    if (!isReachAction)
        return 0.0f;

    // Remainder is checking only for validity of reach actions

    if (PlayerbotAI::IsAssistTankOfIndex(bot, 0, true) &&
        AI_VALUE2(Unit*, "find target", "25772"))
    {
        return 1.0f;
    }

    if (!TryGetMuruDarknessActiveState(bot, muru))
        return 1.0f;

    auto const isReachTargetSafeFromDarkness = [&](Action* action) -> bool
    {
        Unit* actionTarget = action->GetTarget();
        if (!actionTarget)
            return false;

        float const targetDistFromMuru = muru->GetExactDist2d(actionTarget);
        Position const& refPosition = PlayerbotAI::IsAssistTankOfIndex(bot, 1, true) ?
            MURU_ENTRANCE_POSITION : MURU_STACK_POSITION;
        float const targetDistFromRef = actionTarget->GetExactDist2d(refPosition);

        //By leewheel 2026-09-04: 上游1e110a5f——跟随改名 MURU_DARKNESS_SAFE_DISTANCE
        return targetDistFromMuru > MURU_DARKNESS_SAFE_DISTANCE &&
            targetDistFromRef < MURU_HOLDING_POSITION_RADIUS;
        //End By leewheel
    };

    if (isReachTargetSafeFromDarkness(action))
        return 1.0f;

    if (PlayerbotAI::IsTank(bot) && !TryGetMuruDarknessEarlyState(bot, muru))
        return 1.0f;

    return 0.0f;
}

float MuruDelayCooldownsMultiplier::GetValueInEncounter(Action* action)
{
    if (botAI->GetState() == BOT_STATE_NON_COMBAT)
        return 1.0f;

    if (!IsDpsCooldownAction(bot, action))
        return 1.0f;

    Unit* muru = AI_VALUE2(Unit*, "find target", "25741");
    if (!muru)
        return 1.0f;

    Unit* entropius = AI_VALUE2(Unit*, "find target", "25840");
    if (entropius && entropius->GetHealthPct() < BOSS_ENGAGED_HEALTH_PCT)
        return 1.0f;

    // Bloodlust is saved for Entropius
    if (bot->getClass() == CLASS_SHAMAN &&
        (dynamic_cast<CastHeroismAction*>(action) || dynamic_cast<CastBloodlustAction*>(action)))
    {
        return 0.0f;
    }

    // Other dps cooldowns can be used on M'uru after the pull
    return muru->GetHealthPct() > MURU_MAX_DPS_HP_PERCENT ? 0.0f : 1.0f;
}

// Kil'jaeden <The Deceiver>

float KiljaedenDelayCooldownsMultiplier::GetValue(Action* action)
{
    if (botAI->GetState() == BOT_STATE_NON_COMBAT)
        return 1.0f;

    if (!IsDpsCooldownAction(bot, action))
        return 1.0f;

    Unit* kiljaeden = AI_VALUE2(Unit*, "find target", "25608");
    if (!kiljaeden)
        return 1.0f;

    if (AI_VALUE2(Unit*, "find target", "25588"))
        return 0.0f;

    //By leewheel 2026-08-21: 移植 brighton-chi 096512a0——阶段HP阈值常量
    if (kiljaeden->GetHealthPct() <= KILJAEDEN_PHASE5_HP_THRESHOLD) // 嗜血留到第五阶段
        return 1.0f;

    if (bot->getClass() == CLASS_SHAMAN &&
        (dynamic_cast<CastHeroismAction*>(action) || dynamic_cast<CastBloodlustAction*>(action)))
    {
        return 0.0f;
    }

    if (kiljaeden->GetHealthPct() > KILJAEDEN_PHASE3_HP_THRESHOLD) // 其他爆发留到第三阶段
        return 0.0f;
    //End By leewheel

    return 1.0f;
}

//By leewheel 2026-09-05: 对齐 the-lab c9774e50——坦克/DPS 双乘数合并为单目标之手乘数
float KiljaedenSingleTargetHandsMultiplier::GetValue(Action* action)
{
    if (botAI->GetState() == BOT_STATE_NON_COMBAT)
        return 1.0f;

    // 萨满没有铺开的DOT, 唯一归类为 ActionThreatType::Aoe 的法术是闪电链,
    // 而闪电链本身就是强力的单体法术(兼具Aoe伤害), 因此萨满直接放行
    if (bot->getClass() == CLASS_SHAMAN)
        return 1.0f;

    if (!PlayerbotAI::IsDps(bot))
        return 1.0f;

    if (!dynamic_cast<CastDebuffSpellOnAttackerAction*>(action) &&
        action->getThreatType() != Action::ActionThreatType::Aoe)
    {
        return 1.0f;
    }

    // 远离太阳井中心时(不在平台上)不做压制
    if (bot->GetExactDist2d(SUNWELL_CENTER_POSITION) > SUNWELL_CENTER_RADIUS)
        return 1.0f;

    return AI_VALUE(GuidVector, "kiljaeden hands").empty() ? 1.0f : 0.0f;
}
//End By leewheel

float KiljaedenControlMovementAndTargetingMultiplier::GetValueInEncounter(Action* action)
{
    if (botAI->GetState() == BOT_STATE_NON_COMBAT)
        return 1.0f;

    if (!dynamic_cast<FleeAction*>(action) &&
        !dynamic_cast<CombatFormationMoveAction*>(action) &&
        !dynamic_cast<CastDisengageAction*>(action) &&
        !dynamic_cast<CastBlinkBackAction*>(action) &&
        !(dynamic_cast<TankAssistAction*>(action) && PlayerbotAI::IsMainTank(bot)))
    {
        return 1.0f;
    }

    if (AI_VALUE2(Unit*, "find target", "25608"))
        return 0.0f;

    return 1.0f;
}

float KiljaedenPrioritizeDarknessProtectionMultiplier::GetValueInEncounter(Action* action)
{
    if (botAI->GetState() == BOT_STATE_NON_COMBAT)
        return 1.0f;

    if (!dynamic_cast<MovementAction*>(action))
        return 1.0f;

    if (dynamic_cast<AttackAction*>(action))
        return 1.0f;

    if (dynamic_cast<KiljaedenStackForShieldOfTheBlueAction*>(action))
        return 1.0f;

    Unit* kiljaeden = AI_VALUE2(Unit*, "find target", "25608");
    if (!kiljaeden)
        return 1.0f;

    if (HasKiljaedenDragonAura(bot))
        return 1.0f;

    if (IsKiljaedenCastingDarknessOfAThousandSouls(kiljaeden))
        return 0.0f;

    return 1.0f;
}

float KiljaedenControlDragonMultiplier::GetValueInEncounter(Action* action)
{
    if (!AI_VALUE2(Unit*, "find target", "25608"))
        return 1.0f;

    if (dynamic_cast<KiljaedenDragonBuffAndProtectRaidAction*>(action))
        return 1.0f;

    if (dynamic_cast<WipeAction*>(action))
        return 1.0f;

    if (HasKiljaedenDragonAura(bot))
        return 0.0f;

    return 1.0f;
}
