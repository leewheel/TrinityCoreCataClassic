/* 海加尔山 机器人策略 */
/*By leewheel 2026-08-18: 对齐 brighton-chi the-lab HEAD(e92a52db)——
  移植 8df2134e/33f67223/7f293f95/ab50f6a1/fdefbb16/0a3a0367/5f34b67d:
  - Archimonde 恢复月神之佑(Protection of Elune)判定 + 新增 ControlDoomfireAvoidance 乘数
  - 远程控制回避不再豁免 DetermineDpsPriority/AttackAction(清理)
  - 近战控制回避半径改 DEATH_AND_DECAY_RADIUS+10(统一共享常量)
  - Doomfire 危险半径收窄(3→2y)、缓存取值(IsNearDoomfire 走 "hyjal doomfire trail")
  - Kazrogal 低蓝乘数恢复 IsKazrogalManaUser 判定。
  By leewheel 2026-09-04: 上游6d5d68cd——Boss 乘数继承 HyjalSummitEncounterMultiplier 基类,
  实现函数改名 GetValueInEncounter(GetValue 由基类 final 提供, 内含 IsEncounterInProgress 闸门)。*/
#include "HyjalMultipliers.h"
#include "AiFactory.h"
#include "ChooseTargetActions.h"
#include "DKActions.h"
#include "DruidBearActions.h"
#include "EncounterHelpers.h"
#include "HunterActions.h"
#include "HyjalActions.h"
#include "HyjalHelpers.h"
#include "PaladinActions.h"
#include "ReachTargetActions.h"
#include "ShamanActions.h"
#include "WarriorActions.h"

using namespace HyjalHelpers;
using namespace EncounterHelpers;

float HyjalSummitDelayDpsCooldownsMultiplier::GetValue(Action* action)
{
    if (botAI->GetState() == BOT_STATE_NON_COMBAT)
        return 1.0f;

    if (bot->GetMapId() != HYJAL_MAP_ID) // Needed in case strategy isn't cleared outside
        return 1.0f;

    if (!IsDpsCooldownAction(bot, action)) // This includes Bloodlust & Heroism
        return 1.0f;

    //By leewheel 2026-09-04: 对齐上游0b605890——改用带2s缓存的"boss target"值(BossTargetValue,
    //遍历攻击者匹配BossAI, 返回boss本体), 替代原5个英文名逐个find target(1ms缓存×5次, 且与
    //同文件其余乘数全entry化的风格不一致); 无boss在场(小怪波次)时压制嗜血/英勇
    Unit* boss = AI_VALUE(Unit*, "boss target");

    // Suppress Bloodlust/Heroism when no boss is present (trash waves). Asked only on this branch,
    // since it is the only one the answer differs on--everything else here is a dps cooldown too
    if (!boss)
    {
        return bot->getClass() == CLASS_SHAMAN &&
            (dynamic_cast<CastBloodlustAction*>(action) ||
             dynamic_cast<CastHeroismAction*>(action)) ? 0.0f : 1.0f;
    }

    return boss->GetHealthPct() > BOSS_ENGAGED_HEALTH_PCT ? 0.0f : 1.0f;
}

// Rage Winterchill

float RageWinterchillDisableCombatFormationMoveMultiplier::GetValueInEncounter(Action* action)
{
    if (botAI->GetState() == BOT_STATE_NON_COMBAT)
        return 1.0f;

    if (!dynamic_cast<CombatFormationMoveAction*>(action))
        return 1.0f;

    if (dynamic_cast<SetBehindTargetAction*>(action))
        return 1.0f;

    return AI_VALUE2(Unit*, "find target", "17767") ? 0.0f : 1.0f;
}

float RageWinterchillMeleeControlAvoidanceMultiplier::GetValueInEncounter(Action* action)
{
    if (PlayerbotAI::IsRanged(bot))
        return 1.0f;

    bool const isAvoidAoe = dynamic_cast<AvoidAoeAction*>(action);

    if (!isAvoidAoe &&
        !dynamic_cast<ReachTargetAction*>(action) &&
        !dynamic_cast<CastReachTargetSpellAction*>(action) &&
        !dynamic_cast<SetBehindTargetAction*>(action))
    {
        return 1.0f;
    }

    Unit* winterchill = AI_VALUE2(Unit*, "find target", "17767");
    if (!winterchill)
        return 1.0f;

    // Shares its radius with the trigger that runs the maneuver action, so that action owns melee
    // movement across exactly the area this clears for it and there is no band between them
    if (!IsNearDeathAndDecay(bot, DEATH_AND_DECAY_MELEE_CONTROL_RADIUS))
        return 1.0f;

    if (isAvoidAoe)
        return 0.0f;

    return winterchill->GetVictim() == bot || PlayerbotAI::IsMainTank(bot) ? 1.0f : 0.0f;
}

// Stock avoid-aoe discards Death and Decay outright: it drops any hazard whose own radius exceeds
// AiPlayerbot.MaxAoeAvoidRadius, and at the default 15 a 20 yard pool never qualifies. Where it
// does run it flees to the raw radius, which still sits inside the aura once the target's combat
// reach is added. The hardcoded action handles both, so keep the two from fighting over the bot
float RageWinterchillRangedControlAvoidanceMultiplier::GetValueInEncounter(Action* action)
{
    if (!PlayerbotAI::IsRanged(bot))
        return 1.0f;

    if (!dynamic_cast<MovementAction*>(action))
        return 1.0f;

    if (dynamic_cast<RageWinterchillRangedGetOutOfDeathAndDecayAction*>(action))
        return 1.0f;

    // Acquiring a target is not movement. It only reads as such because AttackAction derives from
    // MovementAction, and Attack itself paths nowhere--it sets selection, faces the target, and if
    // anything stops movement. Unlike Azgalor there is no hardcoded targeting action here to spare,
    // so what this would otherwise suppress is stock "dps assist", which every dps bot runs and
    // which nothing else at this fight disables: ranged near a pool could not pick up a target at
    // all until it expired
    if (dynamic_cast<AttackAction*>(action))
        return 1.0f;

    if (!AI_VALUE2(Unit*, "find target", "17767"))
        return 1.0f;

    if (dynamic_cast<AvoidAoeAction*>(action))
        return 0.0f;

    // The spread settles once a bot reaches its point on the circle and then stops asking, but a
    // bot pushed off course before it ever arrives never settles and keeps trying for the rest of
    // the fight--including back into the pool it was just moved out of
    //By leewheel 2026-08-21: 移植上游——改用共享常量 DEATH_AND_DECAY_RANGED_CONTROL_RADIUS
    return IsNearDeathAndDecay(bot, DEATH_AND_DECAY_RANGED_CONTROL_RADIUS) ? 0.0f : 1.0f;
}

// Anetheron

float AnetheronDisableAssistTargetingMultiplier::GetValueInEncounter(Action* action)
{
    if (botAI->GetState() == BOT_STATE_NON_COMBAT)
        return 1.0f;

    bool const isTankAssist = dynamic_cast<TankAssistAction*>(action) != nullptr;
    if (!isTankAssist && !dynamic_cast<DpsAssistAction*>(action))
        return 1.0f;

    if (isTankAssist && IsInfernalTank(bot))
        return 1.0f;

    return AI_VALUE2(Unit*, "find target", "17808") ? 0.0f : 1.0f;
}

float AnetheronAvoidAccidentalInfernalAggroMultiplier::GetValueInEncounter(Action* action)
{
    if (botAI->GetState() == BOT_STATE_NON_COMBAT)
        return 1.0f;

    if (!IsAoeThreatAction(bot, action))
        return 1.0f;

    //By leewheel 2026-08-21: 移植 brighton-chi 8f1b9273——先按距离过滤地狱火再判坦克归属
    constexpr float holdTankAoeRadius = 20.0f; // 任意但大于AOE威胁技能半径
    Unit* infernal = AI_VALUE2(Unit*, "find target", "17818");
    if (!infernal || infernal->GetExactDist2d(bot) > holdTankAoeRadius)
        return 1.0f;

    return IsInfernalTank(bot) ? 1.0f : 0.0f;
    //End By leewheel
}

float AnetheronInfernalTargetRunToPositionMultiplier::GetValueInEncounter(Action* action)
{
    if (!dynamic_cast<MovementAction*>(action) &&
        !dynamic_cast<CastReachTargetSpellAction*>(action))
    {
        return 1.0f;
    }

    if (dynamic_cast<AnetheronBringInfernalToInfernalTankAction*>(action))
        return 1.0f;

    Unit* anetheron = AI_VALUE2(Unit*, "find target", "17808");
    if (!anetheron || anetheron->GetVictim() == bot)
        return 1.0f;

    if (IsInfernalTank(bot))
        return 1.0f;

    return GetInfernoTarget(anetheron) == bot || GetInfernalTargetingBot(bot) ? 0.0f : 1.0f;
}

float AnetheronControlMovementMultiplier::GetValueInEncounter(Action* action)
{
    if (botAI->GetState() == BOT_STATE_NON_COMBAT)
        return 1.0f;

    bool const isTankAvoidAoe =
        PlayerbotAI::IsTank(bot) && dynamic_cast<AvoidAoeAction*>(action);

    if (!isTankAvoidAoe && !dynamic_cast<CombatFormationMoveAction*>(action))
        return 1.0f;

    if (dynamic_cast<SetBehindTargetAction*>(action))
        return 1.0f;

    return AI_VALUE2(Unit*, "find target", "17808") ? 0.0f : 1.0f;
}

float AnetheronControlMisdirectionMultiplier::GetValueInEncounter(Action* action)
{
    if (botAI->GetState() == BOT_STATE_NON_COMBAT)
        return 1.0f;

    if (bot->getClass() != CLASS_HUNTER)
        return 1.0f;

    if (!dynamic_cast<CastMisdirectionOnMainTankAction*>(action))
        return 1.0f;

    return AI_VALUE2(Unit*, "find target", "17808") ? 0.0f : 1.0f;
}

// Kaz'rogal

float KazrogalDisableDisperseAndTankFaceMultiplier::GetValueInEncounter(Action* action)
{
    if (botAI->GetState() == BOT_STATE_NON_COMBAT)
        return 1.0f;

    if (!dynamic_cast<CombatFormationMoveAction*>(action))
        return 1.0f;

    if (dynamic_cast<SetBehindTargetAction*>(action))
        return 1.0f;

    return AI_VALUE2(Unit*, "find target", "17888") ? 0.0f : 1.0f;
}

float KazrogalControlLowManaMovementMultiplier::GetValueInEncounter(Action* action)
{
    // Hunters are excluded alongside the classes the Mark cannot reach: it reaches them, but their
    // whole answer to it is Viper, so there is no escape here to clear the way for
    if (!IsKazrogalManaUser(botAI, bot) || bot->getClass() == CLASS_HUNTER)
        return 1.0f;

    if (!dynamic_cast<MovementAction*>(action) &&
        !dynamic_cast<CastReachTargetSpellAction*>(action))
    {
        return 1.0f;
    }

    if (dynamic_cast<AttackAction*>(action))
        return 1.0f;

    if (dynamic_cast<KazrogalMoveAwayFromGroupAction*>(action))
        return 1.0f;

    Unit* kazrogal = AI_VALUE2(Unit*, "find target", "17888");
    if (!kazrogal || kazrogal->GetVictim() == bot)
        return 1.0f;

    return botsBelowManaThreshold.contains(bot->GetGUID()) ? 0.0f : 1.0f;
}

float KazrogalKeepAspectOfTheViperActiveMultiplier::GetValueInEncounter(Action* action)
{
    if (botAI->GetState() == BOT_STATE_NON_COMBAT)
        return 1.0f;

    if (bot->getClass() != CLASS_HUNTER)
        return 1.0f;

    if (!dynamic_cast<CastAspectOfTheHawkAction*>(action) &&
        !dynamic_cast<CastAspectOfTheWildAction*>(action) &&
        !dynamic_cast<CastAspectOfTheDragonhawkAction*>(action) &&
        !dynamic_cast<CastAspectOfTheCheetahAction*>(action) &&
        !dynamic_cast<CastAspectOfThePackAction*>(action) &&
        !dynamic_cast<CastAspectOfTheMonkeyAction*>(action))
    {
        return 1.0f;
    }

    if (!AI_VALUE2(Unit*, "find target", "17888"))
        return 1.0f;

    return bot->GetPower(POWER_MANA) <= MARK_DANGER_MANA ? 0.0f : 1.0f;
}

// Azgalor

float AzgalorDisableAutoTargetingAndPositioningMultiplier::GetValueInEncounter(Action* action)
{
    if (botAI->GetState() == BOT_STATE_NON_COMBAT)
        return 1.0f;

    if (!dynamic_cast<DpsAssistAction*>(action) &&
        !dynamic_cast<TankAssistAction*>(action) &&
        !dynamic_cast<CombatFormationMoveAction*>(action) &&
        !dynamic_cast<AvoidAoeAction*>(action))
    {
        return 1.0f;
    }

    // Still disabled in RoF, below
    if (dynamic_cast<SetBehindTargetAction*>(action))
        return 1.0f;

    return AI_VALUE2(Unit*, "find target", "17842") ? 0.0f : 1.0f;
}

float AzgalorDoomedBotPrioritizePositioningMultiplier::GetValueInEncounter(Action* action)
{
    if (!IsDoomed(bot))
        return 1.0f;

    if (!dynamic_cast<MovementAction*>(action))
        return 1.0f;

    if (dynamic_cast<AttackAction*>(action))
        return 1.0f;

    return dynamic_cast<AzgalorMoveToDoomguardTankAction*>(action) ? 1.0f : 0.0f;
}

// Leave the escape action as the only thing that moves melee while Rain of Fire is a threat
float AzgalorMeleeDpsControlAvoidanceMultiplier::GetValueInEncounter(Action* action)
{
    if (!PlayerbotAI::IsMelee(bot) || PlayerbotAI::IsTank(bot))
        return 1.0f;

    if (!dynamic_cast<MovementAction*>(action) &&
        !dynamic_cast<CastReachTargetSpellAction*>(action))
    {
        return 1.0f;
    }

    // Doom outranks standing in fire, and it has its own positioning to do
    if (IsDoomed(bot))
        return 1.0f;

    if (dynamic_cast<AzgalorMeleeManeuverThroughFireAction*>(action))
        return 1.0f;

    // Acquiring a target is not movement. It only reads as such because AttackAction derives from
    // MovementAction, and Attack itself paths nowhere--it sets selection, faces the target, and if
    // anything stops movement. Suppressing it would leave a melee bot that entered the fire without
    // a live target unable to pick one up until the pool expired
    if (dynamic_cast<AzgalorDetermineDpsPriorityAction*>(action))
        return 1.0f;

    if (!AI_VALUE2(Unit*, "find target", "17842"))
        return 1.0f;

    // Shares its radius with the trigger that runs the maneuver action, so that action owns melee
    // movement across exactly the area this clears for it and there is no band between them
    return IsNearRainOfFire(bot, RAIN_OF_FIRE_MELEE_CONTROL_RADIUS) ? 0.0f : 1.0f;
}

// Rain of Fire is 15 yards, so unlike Death and Decay it does scrape past the default
// MaxAoeAvoidRadius and stock avoid-aoe does handle it--but only out to the raw radius, which
// leaves the bot inside the aura. The hardcoded action owns this instead.
//
// The dispersal action is the other thing that moves ranged here, and it runs for the whole fight
// rather than settling like the spreads at the other bosses do. It only loses to the escape on the
// ticks the escape actually returns true, so on any tick FleePosition declines an angle it would
// be free to walk the bot back into the fire it has just left
float AzgalorRangedControlAvoidanceMultiplier::GetValueInEncounter(Action* action)
{
    if (!PlayerbotAI::IsRanged(bot))
        return 1.0f;

    if (!dynamic_cast<MovementAction*>(action))
        return 1.0f;

    // Doom outranks standing in fire, and it has its own positioning to do
    if (IsDoomed(bot))
        return 1.0f;

    if (dynamic_cast<AzgalorRangedGetOutOfRainOfFireAction*>(action))
        return 1.0f;

    // Spared for the same reason as on the melee side: acquiring a target is not movement, it only
    // reads as such because AttackAction derives from MovementAction. Suppressing it would leave a
    // ranged bot near a pool stuck on whatever it was already hitting--unable to switch onto a
    // Doomguard as one spawns, or back onto Azgalor once it dies
    if (dynamic_cast<AzgalorDetermineDpsPriorityAction*>(action))
        return 1.0f;

    if (!AI_VALUE2(Unit*, "find target", "17842"))
        return 1.0f;

    // Wider than the escape trigger's own radius on purpose. The band between them is where a bot
    // that has just left a pool waits it out: the dispersal action is what would otherwise move it,
    // and letting that resume at the pool's edge is the tug-of-war this exists to prevent. Nothing
    // is lost by holding it there--the dispersal trigger stops firing across the same band anyway
    //By leewheel 2026-08-21: 移植上游——改用共享常量 RAIN_OF_FIRE_RANGED_CONTROL_RADIUS
    return IsNearRainOfFire(bot, RAIN_OF_FIRE_RANGED_CONTROL_RADIUS) ? 0.0f : 1.0f;
}

// Archimonde

float ArchimondeDisableCombatFormationMoveMultiplier::GetValueInEncounter(Action* action)
{
    if (botAI->GetState() == BOT_STATE_NON_COMBAT)
        return 1.0f;

    if (!dynamic_cast<CombatFormationMoveAction*>(action))
        return 1.0f;

    if (dynamic_cast<SetBehindTargetAction*>(action))
        return 1.0f;

    if (!AI_VALUE2(Unit*, "find target", "17968"))
        return 1.0f;

    return !HasProtectionOfElune(bot) ? 0.0f : 1.0f;
}

// Leave the Doomfire avoidance as the only thing that moves a bot near a trail. Its push tapers to
// nothing at DOOMFIRE_DANGER_RADIUS, so without this anything that wants the bot elsewhere--closing
// to spell range, stock avoid-aoe on the same patches, the ranged spread--takes over the instant it
// stops being pushed, drags it back inside, and the two swap the bot every tick
float ArchimondeControlDoomfireAvoidanceMultiplier::GetValueInEncounter(Action* action)
{
    if (!dynamic_cast<MovementAction*>(action) &&
        !dynamic_cast<CastReachTargetSpellAction*>(action))
    {
        return 1.0f;
    }

    // Air Burst outranks Doomfire: a knockback lands the bot somewhere unpredictable anyway, and
    // its own action already reaches further than this suppression does
    if (dynamic_cast<ArchimondeAvoidDoomfireAction*>(action) ||
        dynamic_cast<ArchimondeKeepAirBurstAwayFromTankAction*>(action))
    {
        return 1.0f;
    }

    if (!AI_VALUE2(Unit*, "find target", "17968"))
        return 1.0f;

    // Stock avoid-aoe goes for the whole fight, not merely near a trail. Between them the hardcoded
    // actions cover both hazards here, and stock flees each trail patch to its own 6y radius--a
    // different figure, reached by a different route, pulling against the repulsion the moment the
    // two disagree about which patch matters
    if (dynamic_cast<AvoidAoeAction*>(action))
        return 0.0f;

    if (HasProtectionOfElune(bot))
        return 1.0f;

    // Wider than the radius the avoidance reacts at, so a bot pushed to the edge is still held
    return IsNearDoomfire(bot, DOOMFIRE_CONTROL_RADIUS) ? 0.0f : 1.0f;
}

float ArchimondeSetTremorTotemMultiplier::GetValueInEncounter(Action* action)
{
    if (botAI->GetState() == BOT_STATE_NON_COMBAT)
        return 1.0f;

    if (bot->getClass() != CLASS_SHAMAN)
        return 1.0f;

    if (!dynamic_cast<CastStrengthOfEarthTotemAction*>(action) &&
        !dynamic_cast<CastStoneskinTotemAction*>(action) &&
        !dynamic_cast<CastStoneclawTotemAction*>(action) &&
        !dynamic_cast<CastEarthbindTotemAction*>(action))
    {
        return 1.0f;
    }

    Unit* archimonde = AI_VALUE2(Unit*, "find target", "17968");
    if (!archimonde || archimonde->GetHealthPct() > BOSS_ENGAGED_HEALTH_PCT)
        return 1.0f;

    return !HasProtectionOfElune(bot) ? 0.0f : 1.0f;
}
