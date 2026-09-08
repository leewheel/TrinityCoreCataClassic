/* 风暴要塞 机器人策略 */
#include "TKMultipliers.h"
#include "ChooseTargetActions.h"
#include "EncounterHelpers.h"
#include "EquipAction.h"
#include "FollowActions.h"
#include "HunterActions.h"
#include "MageActions.h"
#include "Playerbots.h"
#include "ReachTargetActions.h"
#include "RogueActions.h"
#include "ShamanActions.h"
#include "TKActions.h"
#include "TKHelpers.h"
//By leewheel 2026-08-21: 移植 brighton-chi 5232cb9d——灵魂碎裂乘数需要术士动作头文件
#include "WarlockActions.h"
//End By leewheel
//By leewheel 2026-08-21: 移植 brighton-chi 615c78d2——阶段读取改用 GetKaelthasPhase,不再需要 boss_kaelthas 镜像类
//End By leewheel
#include <ctime>

//By leewheel 2026-08-21: 移植 brighton-chi 2eaa83f3——显式引入 TkHelpers 命名空间
using namespace TkHelpers;
//By leewheel 2026-08-21: 移植 brighton-chi 2eaa83f3——显式引入 TkHelpers 命名空间
//End By leewheel
//By leewheel 2026-08-23: the-lab 引入 —— TK 使用 EncounterHelpers 工具函数
using namespace EncounterHelpers;
//End By leewheel

// Al'ar <Phoenix God>

//By leewheel 2026-09-04: 上游4f9815d1——乘数继承 TempestKeepEncounterMultiplier 基类, 实现函数改名 GetValueInEncounter

float AlarSuppressGapClosersMultiplier::GetValueInEncounter(Action* action)
{
    if (botAI->GetState() == BOT_STATE_NON_COMBAT)
        return 1.0f;

    bool const isBlockedMovement =
        dynamic_cast<TankFaceAction*>(action) ||
        dynamic_cast<CastKillingSpreeAction*>(action) ||
        dynamic_cast<CastDisengageAction*>(action) ||
        dynamic_cast<CastBlinkBackAction*>(action) ||
        dynamic_cast<ReachTargetAction*>(action);

    if (!isBlockedMovement && !dynamic_cast<CastReachTargetSpellAction*>(action))
        return 1.0f;

    Unit* alar = AI_VALUE2(Unit*, "find target", "19514");
    //By leewheel 2026-08-21: 移植 brighton-chi 78f030ba——GetInstanceId() 取代 GetMap()->GetInstanceId()
    if (!alar || IsAlarInPhase2(alar->GetInstanceId()))
        return 1.0f;
    //End By leewheel

    if (isBlockedMovement)
        return 0.0f;

    if (PlayerbotAI::IsTank(bot))
        return 1.0f;

    // Block Charge, etc. for non-tanks when not at a platform
    int8 const currentLocationIndex = GetAlarCurrentLocationIndex(alar);
    if (currentLocationIndex < PLATFORM_0_IDX || currentLocationIndex > PLATFORM_3_IDX)
        return 0.0f;

    return 1.0f;
}

float AlarControlMovementMultiplier::GetValueInEncounter(Action* action)
{
    bool const isDisperseOrFlee =
        dynamic_cast<CombatFormationMoveAction*>(action) || dynamic_cast<FleeAction*>(action);

    if (!isDisperseOrFlee && !dynamic_cast<FollowAction*>(action))
        return 1.0f;

    if (dynamic_cast<TankFaceAction*>(action) || dynamic_cast<SetBehindTargetAction*>(action))
        return 1.0f;

    Unit* alar = AI_VALUE2(Unit*, "find target", "19514");
    if (!alar)
        return 1.0f;

    if (isDisperseOrFlee)
        return 0.0f;

    //By leewheel 2026-08-21: 移植 brighton-chi 78f030ba——GetInstanceId() 取代 GetMap()->GetInstanceId()
    if (!IsAlarInPhase2(alar->GetInstanceId()))
        return 1.0f;
    //End By leewheel

    // Enable FollowAction only in the non-combat engine in Phase 2.
    return botAI->GetState() == BOT_STATE_COMBAT ? 0.0f : 1.0f;
}

float AlarDisableAutomaticTargetingMultiplier::GetValueInEncounter(Action* action)
{
    if (botAI->GetState() == BOT_STATE_NON_COMBAT)
        return 1.0f;

    if (!dynamic_cast<TankAssistAction*>(action) && !dynamic_cast<DpsAssistAction*>(action))
        return 1.0f;

    if (AI_VALUE2(Unit*, "find target", "19514"))
        return 0.0f;

    return 1.0f;
}

float AlarStayAwayFromRebirthMultiplier::GetValueInEncounter(Action* action)
{
    if (PlayerbotAI::IsRanged(bot) || PlayerbotAI::IsTank(bot))
        return 1.0f;

    if (!dynamic_cast<MovementAction*>(action))
        return 1.0f;

    if (dynamic_cast<AlarMoveAwayFromRebirthAction*>(action))
        return 1.0f;

    // Don't block Flame Quills avoidance in case of bad timing for the transition.
    if (dynamic_cast<AlarJumpFromPlatformAction*>(action))
        return 1.0f;

    Unit* alar = AI_VALUE2(Unit*, "find target", "19514");
    //By leewheel 2026-08-21: 移植 brighton-chi 78f030ba——GetInstanceId() 取代 GetMap()->GetInstanceId()
    if (!alar || IsAlarInPhase2(alar->GetInstanceId()))
        return 1.0f;
    //End By leewheel

    Creature* alarCreature = alar->ToCreature();
    if (alarCreature && alarCreature->GetReactState() == REACT_PASSIVE)
        return 0.0f;

    if (alar->GetHealthPct() <= 5.0f) // Melee dps activate logic for the P2 transition at 5% HP
        return 0.0f;

    return 1.0f;
}

float AlarControlTauntingMultiplier::GetValueInEncounter(Action* action)
{
    if (botAI->GetState() == BOT_STATE_NON_COMBAT)
        return 1.0f;

    if (!IsTauntAction(bot, action))
        return 1.0f;

    bool const isFirstAlarTank = IsFirstAlarTank(bot);

    if (!isFirstAlarTank && !IsSecondAlarTank(bot))
        return 1.0f;

    Unit* alar = AI_VALUE2(Unit*, "find target", "19514");
    if (!alar)
        return 1.0f;

    if (bot->HasAura(Id(TkSpells::SPELL_MELT_ARMOR)) && AI_VALUE(Unit*, "current target") == alar)
        return 0.0f;

    //By leewheel 2026-08-21: 移植 brighton-chi 78f030ba——GetInstanceId() 取代 GetMap()->GetInstanceId()
    if (IsAlarInPhase2(alar->GetInstanceId()))
        return 1.0f;
    //End By leewheel

    int8 platformIndex = GetAlarPlatformIndex(alar);
    if (isFirstAlarTank)
    {
        if (platformIndex != PLATFORM_0_IDX && platformIndex != PLATFORM_2_IDX)
            return 0.0f;
    }
    else // isSecondAlarTank
    {
        if (platformIndex != PLATFORM_1_IDX && platformIndex != PLATFORM_3_IDX)
            return 0.0f;
    }

    return 1.0f;
}

// Void Reaver

float VoidReaverMaintainPositionsMultiplier::GetValueInEncounter(Action* action)
{
    if (botAI->GetState() == BOT_STATE_NON_COMBAT)
        return 1.0f;

    if (!dynamic_cast<CombatFormationMoveAction*>(action))
        return 1.0f;

    if (dynamic_cast<SetBehindTargetAction*>(action))
        return 1.0f;

    if (AI_VALUE2(Unit*, "find target", "19516"))
        return 0.0f;

    return 1.0f;
}

// High Astromancer Solarian

float HighAstromancerSolarianWrathStayAwayMultiplier::GetValueInEncounter(Action* action)
{
    if (!dynamic_cast<MovementAction*>(action) &&
        !dynamic_cast<CastReachTargetSpellAction*>(action))
    {
        return 1.0f;
    }

    if (dynamic_cast<AttackAction*>(action))
        return 1.0f;

    if (dynamic_cast<HighAstromancerSolarianMoveAwayFromGroupAction*>(action))
        return 1.0f;

    Unit* astromancer = AI_VALUE2(Unit*, "find target", "18805");
    if (!astromancer || astromancer->HasAura(Id(TkSpells::SPELL_SOLARIAN_TRANSFORM)))
        return 1.0f;

    if (HasWrathOfTheAstromancer(bot))
        return 0.0f;

    return 1.0f;
}

float HighAstromancerSolarianDisableMeleeTargetingMultiplier::GetValueInEncounter(Action* action)
{
    if (botAI->GetState() == BOT_STATE_NON_COMBAT)
        return 1.0f;

    if (!PlayerbotAI::IsMelee(bot))
        return 1.0f;

    if (!dynamic_cast<TankAssistAction*>(action) && !dynamic_cast<DpsAssistAction*>(action))
        return 1.0f;

    Unit* astromancer = AI_VALUE2(Unit*, "find target", "18805");
    if (!astromancer)
        return 1.0f;

    if (PlayerbotAI::IsMainTank(bot))
    {
        Creature* astromancerCreature = astromancer->ToCreature();
        if (astromancerCreature && astromancerCreature->GetReactState() != REACT_PASSIVE)
            return 0.0f;
    }
    else if (AI_VALUE2(Unit*, "find target", "18806"))
    {
        return 0.0f;
    }

    return 1.0f;
}

// Kael'thas Sunstrider <Lord of the Blood Elves>

float KaelthasSunstriderWaitForDpsMultiplier::GetValueInEncounter(Action* action)
{
    if (!dynamic_cast<CastSpellAction*>(action) && !dynamic_cast<AttackAction*>(action))
        return 1.0f;

    if (dynamic_cast<CastHealingSpellAction*>(action))
        return 1.0f;

    //By leewheel 2026-08-21: 移植 brighton-chi 2eaa83f3——移除对 MisdirectAdvisorsToTanksAction 的豁免
    //End By leewheel

    Unit* kaelthas = AI_VALUE2(Unit*, "find target", "19622");
    if (!kaelthas)
        return 1.0f;

    //By leewheel 2026-08-21: 移植 brighton-chi 615c78d2——阶段读取改用 GetKaelthasPhase
    if (GetKaelthasPhase(kaelthas) != PHASE_SINGLE_ADVISOR)
        return 1.0f;
    //End By leewheel

    constexpr uint32 dpsWaitMs = 10 * IN_MILLISECONDS;
    auto it = advisorDpsWaitTimer.find(kaelthas->GetInstanceId());
    //End By leewheel
    if (it != advisorDpsWaitTimer.end() && it->second != ADVISOR_DPS_WAIT_NOT_STARTED &&
        getMSTimeDiff(it->second, getMSTime()) >= dpsWaitMs)
    {
        return 1.0f;
    }

    // Only the applicable tank may attack during the first 10 seconds of an advisor in phase 1
    if (IsAdvisorActive(AI_VALUE2(Unit*, "find target", "20060")))
        return PlayerbotAI::IsMainTank(bot) ? 1.0f : 0.0f;

    if (IsAdvisorActive(AI_VALUE2(Unit*, "find target", "20062")))
        return bot->getClass() == CLASS_WARLOCK && GetCapernianTank(bot) == bot ? 1.0f : 0.0f;

    if (IsAdvisorActive(AI_VALUE2(Unit*, "find target", "20063")))
        return PlayerbotAI::IsAssistTankOfIndex(bot, 0, true) ? 1.0f : 0.0f;

    return 1.0f;
}

float KaelthasSunstriderKiteThaladredMultiplier::GetValueInEncounter(Action* action)
{
    if (!dynamic_cast<MovementAction*>(action) &&
        !dynamic_cast<CastReachTargetSpellAction*>(action))
    {
        return 1.0f;
    }

    if (dynamic_cast<KaelthasSunstriderKiteThaladredAction*>(action))
        return 1.0f;

    Unit* kaelthas = AI_VALUE2(Unit*, "find target", "19622");
    if (!kaelthas)
        return 1.0f;

    //By leewheel 2026-08-21: 移植 brighton-chi 615c78d2——阶段读取改用 GetKaelthasPhase
    uint32 const phase = GetKaelthasPhase(kaelthas);
    if (phase == PHASE_NONE)
        return 1.0f;

    if (PlayerbotAI::IsTank(bot) && phase == PHASE_ALL_ADVISORS)
        return 1.0f;

    Unit* thaladred = AI_VALUE2(Unit*, "find target", "20064");
    if (thaladred && thaladred->GetVictim() == bot)
        return 0.0f;
    //End By leewheel

    return 1.0f;
}

float KaelthasSunstriderControlMisdirectionMultiplier::GetValueInEncounter(Action* action)
{
    if (botAI->GetState() == BOT_STATE_NON_COMBAT)
        return 1.0f;

    if (bot->getClass() != CLASS_HUNTER)
        return 1.0f;

    if (!dynamic_cast<CastMisdirectionOnMainTankAction*>(action))
        return 1.0f;

    Unit* kaelthas = AI_VALUE2(Unit*, "find target", "19622");
    if (!kaelthas)
        return 1.0f;

    //By leewheel 2026-08-21: 移植 brighton-chi 615c78d2——阶段读取改用 GetKaelthasPhase
    uint32 const phase = GetKaelthasPhase(kaelthas);
    return phase != PHASE_NONE && phase != PHASE_FINAL ? 0.0f : 1.0f;
    //End By leewheel
}

//By leewheel 2026-08-21: 移植 brighton-chi 5232cb9d——卡波妮娅术士坦禁用灵魂碎裂。
//目前该乘数暂未生效, 因为灵魂碎裂只在多个敌人时施放——这大概不是正确思路, 应修正;
//乘数保留在此以备未来修正灵魂碎裂用法
float KaelthasSunstriderDisableWarlockTankSoulshatterMultiplier::GetValueInEncounter(Action* action)
{
    if (botAI->GetState() == BOT_STATE_NON_COMBAT)
        return 1.0f;

    if (bot->getClass() != CLASS_WARLOCK)
        return 1.0f;

    if (!dynamic_cast<CastSoulshatterAction*>(action))
        return 1.0f;

    Unit* kaelthas = AI_VALUE2(Unit*, "find target", "19622");
    if (!kaelthas)
        return 1.0f;

    uint32 const phase = GetKaelthasPhase(kaelthas);
    if (phase != PHASE_SINGLE_ADVISOR && phase != PHASE_ALL_ADVISORS)
        return 1.0f;

    Unit* capernian = AI_VALUE2(Unit*, "find target", "20062");
    if (!IsAdvisorActive(capernian))
        return 1.0f;

    return GetCapernianTank(bot) == bot ? 0.0f : 1.0f;
}
//End By leewheel

float KaelthasSunstriderKeepDistanceFromCapernianMultiplier::GetValueInEncounter(Action* action)
{
    if (!dynamic_cast<MovementAction*>(action) &&
        !dynamic_cast<CastReachTargetSpellAction*>(action))
    {
        return 1.0f;
    }

    if (dynamic_cast<AttackAction*>(action))
        return 1.0f;

    if (dynamic_cast<KaelthasSunstriderSpreadAndMoveAwayFromCapernianAction*>(action))
        return 1.0f;

    Unit* kaelthas = AI_VALUE2(Unit*, "find target", "19622");
    if (!kaelthas)
        return 1.0f;

    //By leewheel 2026-08-21: 移植 brighton-chi 615c78d2——阶段读取改用 GetKaelthasPhase
    if (GetKaelthasPhase(kaelthas) != PHASE_SINGLE_ADVISOR)
        return 1.0f;
    //End By leewheel

    Unit* capernian = AI_VALUE2(Unit*, "find target", "20062");
    if (capernian && !capernian->HasUnitFlag(UNIT_FLAG_NON_ATTACKABLE) &&
        //By leewheel 2026-08-27: TC适配——thelab Acore Unit::IsFeigningDeath 在TC3.4.3不存在,
        //改用与 IsAdvisorActive 一致的"永久假死光环"判断
        !capernian->HasAura(Id(TkSpells::SPELL_PERMANENT_FEIGN_DEATH)))
    {
        return 0.0f;
    }

    return IsAdvisorActive(capernian) ? 0.0f : 1.0f;
}

float KaelthasSunstriderManageWeaponTankingMultiplier::GetValueInEncounter(Action* action)
{
    if (botAI->GetState() == BOT_STATE_NON_COMBAT)
        return 1.0f;

    // Try to keep main tank from grabbing aggro on any weapon other than the axe
    if (!IsTauntAction(bot, action) && !IsAoeThreatAction(bot, action))
        return 1.0f;

    if (!PlayerbotAI::IsMainTank(bot))
        return 1.0f;

    Unit* kaelthas = AI_VALUE2(Unit*, "find target", "19622");
    if (!kaelthas)
        return 1.0f;

    //By leewheel 2026-08-21: 移植 brighton-chi 615c78d2——阶段读取改用 GetKaelthasPhase
    return GetKaelthasPhase(kaelthas) == PHASE_WEAPONS ? 0.0f : 1.0f;
    //End By leewheel
}

float KaelthasSunstriderSuppressEquipUpgradeMultiplier::GetValueInEncounter(Action* action)
{
    if (!dynamic_cast<EquipUpgradeAction*>(action) &&
        !dynamic_cast<EquipUpgradesPacketAction*>(action))
    {
        return 1.0f;
    }

    if (AI_VALUE2(Unit*, "find target", "19622"))
        return 0.0f;

    return 1.0f;
}

float KaelthasSunstriderManageAutomaticTargetingMultiplier::GetValueInEncounter(Action* action)
{
    if (botAI->GetState() == BOT_STATE_NON_COMBAT)
        return 1.0f;

    bool const isDpsAssist = dynamic_cast<DpsAssistAction*>(action);

    if (!isDpsAssist && !dynamic_cast<TankAssistAction*>(action))
        return 1.0f;

    Unit* kaelthas = AI_VALUE2(Unit*, "find target", "19622");
    if (!kaelthas)
        return 1.0f;

    //By leewheel 2026-08-21: 移植 brighton-chi 615c78d2——阶段读取改用 GetKaelthasPhase
    uint32 const phase = GetKaelthasPhase(kaelthas);
    if (phase == PHASE_NONE)
        return 1.0f;

    if (isDpsAssist)
        return 0.0f;

    // TankAssistAction
    if (PlayerbotAI::IsMainTank(bot))
        return 0.0f;

    return phase == PHASE_SINGLE_ADVISOR || phase == PHASE_ALL_ADVISORS ? 0.0f : 1.0f;
    //End By leewheel
}

float KaelthasSunstriderDisableDisperseMultiplier::GetValueInEncounter(Action* action)
{
    if (botAI->GetState() == BOT_STATE_NON_COMBAT)
        return 1.0f;

    if (!dynamic_cast<CombatFormationMoveAction*>(action))
        return 1.0f;

    if (dynamic_cast<SetBehindTargetAction*>(action))
        return 1.0f;

    if (AI_VALUE2(Unit*, "find target", "19622"))
        return 0.0f;

    return 1.0f;
}

float KaelthasSunstriderPrepareForPhase3Multiplier::GetValueInEncounter(Action* action)
{
    if (!dynamic_cast<MovementAction*>(action))
        return 1.0f;

    if (dynamic_cast<KaelthasSunstriderHandleAdvisorRolesInPhase3Action*>(action))
        return 1.0f;

    Unit* kaelthas = AI_VALUE2(Unit*, "find target", "19622");
    if (!kaelthas)
        return 1.0f;

    //By leewheel 2026-08-21: 移植 brighton-chi 615c78d2——阶段读取改用 GetKaelthasPhase
    if (GetKaelthasPhase(kaelthas) != PHASE_ALL_ADVISORS)
        return 1.0f;
    //End By leewheel

    Unit* sanguinar = AI_VALUE2(Unit*, "find target", "20060");
    if (PlayerbotAI::IsAssistHealOfIndex(bot, 0, true))
    {
        if (dynamic_cast<KaelthasSunstriderKiteThaladredAction*>(action))
            return 1.0f;

        return sanguinar && sanguinar->IsAlive() ? 0.0f : 1.0f;
    }

    // The Sanguinar check is a proxy for the revival/Kael talk phase (any non-selectable advisor
    // would do, since all four revive together, but Sanguinar is already needed for the healer).
    if (!sanguinar || !sanguinar->HasUnitFlag(UNIT_FLAG_NOT_SELECTABLE))
        return 1.0f;

    if (PlayerbotAI::IsMainTank(bot) ||
        PlayerbotAI::IsAssistTankOfIndex(bot, 0, true) ||
        (bot->getClass() == CLASS_WARLOCK && GetCapernianTank(bot) == bot))
    {
        return 0.0f;
    }

    return 1.0f;
}

// Bloodlust/Heroism and other major cooldowns should be saved until Phase 3.
float KaelthasSunstriderDelayCooldownsMultiplier::GetValueInEncounter(Action* action)
{
    if (botAI->GetState() == BOT_STATE_NON_COMBAT)
        return 1.0f;

    if (!IsDpsCooldownAction(bot, action))
        return 1.0f;

    Unit* kaelthas = AI_VALUE2(Unit*, "find target", "19622");
    if (!kaelthas)
        return 1.0f;

    //By leewheel 2026-08-21: 移植 brighton-chi 615c78d2——阶段读取改用 GetKaelthasPhase
    uint32 const phase = GetKaelthasPhase(kaelthas);
    if (phase == PHASE_NONE)
        return 1.0f;

    bool const isLustAction = bot->getClass() == CLASS_SHAMAN &&
        (dynamic_cast<CastBloodlustAction*>(action) ||
         dynamic_cast<CastHeroismAction*>(action));

    if (isLustAction && phase == PHASE_WEAPONS)
        return 0.0f;

    return phase == PHASE_SINGLE_ADVISOR || phase == PHASE_TRANSITION ? 0.0f : 1.0f;
    //End By leewheel
}

float KaelthasSunstriderStaySpreadDuringGravityLapseMultiplier::GetValueInEncounter(Action* action)
{
    if (!bot->HasAura(Id(TkSpells::SPELL_GRAVITY_LAPSE)))
        return 1.0f;

    if (!dynamic_cast<MovementAction*>(action) &&
        !dynamic_cast<CastReachTargetSpellAction*>(action))
    {
        return 1.0f;
    }

    if (PlayerbotAI::IsRanged(bot) && dynamic_cast<AttackAction*>(action))
        return 1.0f;

    if (!dynamic_cast<KaelthasSunstriderSpreadOutInMidairAction*>(action))
        return 0.0f;

    return 1.0f;
}
