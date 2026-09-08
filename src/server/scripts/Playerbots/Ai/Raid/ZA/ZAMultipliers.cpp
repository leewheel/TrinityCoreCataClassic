/* 副本机器人策略 */
#include "ZAMultipliers.h"
#include "ChooseTargetActions.h"
#include "DKActions.h"
#include "DruidBearActions.h"
#include "FollowActions.h"
#include "GenericSpellActions.h"
#include "HunterActions.h"
#include "MageActions.h"
#include "PaladinActions.h"
#include "Playerbots.h"
#include "PriestActions.h"
#include "ReachTargetActions.h"
#include "RogueActions.h"
#include "ShamanActions.h"
#include "WarlockActions.h"
#include "WarriorActions.h"
#include "ZAActions.h"
#include "ZAHelpers.h"
//By leewheel 2026-08-23: ZA 使用 EncounterHelpers 的 IsTauntAction/GetFirstAliveUnitByEntry
#include "EncounterHelpers.h"
//End By leewheel

using namespace ZaHelpers;
//By leewheel 2026-08-23: 与其它 Raid 策略一致引入 EncounterHelpers 命名空间
using namespace EncounterHelpers;
//End By leewheel

namespace
{

//By leewheel 2026-09-04: 上游14413ee2——新增接近类移动判定(用于旋风/冰霜陷阱驻留乘数), 修正IsHazardousMovement签名
// Actions that place the bot relative to its current target: closing to melee or spell range, the
// gap-closers, and the formation moves that circle one. These are what fight a flee, so a hazard a
// bot has been moved away from holds them until it is clear of it. Distinct from the below, which
// is every movement at all - reach for that only where the bot should not move for any reason.
bool IsApproachMovement(Action* action)
{
    return dynamic_cast<ReachTargetAction*>(action) ||
        dynamic_cast<CastReachTargetSpellAction*>(action) ||
        dynamic_cast<CombatFormationMoveAction*>(action);
}

bool IsHazardousMovement(Action* action)
{
    return (dynamic_cast<MovementAction*>(action) && !dynamic_cast<AttackAction*>(action)) ||
        dynamic_cast<CastReachTargetSpellAction*>(action) ||
        dynamic_cast<CastKillingSpreeAction*>(action) ||
        dynamic_cast<CastBlinkBackAction*>(action) ||
        dynamic_cast<CastDisengageAction*>(action);
}

}

// General

float ZulAmanDelayDpsCooldownsMultiplier::GetValueInEncounter(Action* action)
{
    if (botAI->GetState() == BOT_STATE_NON_COMBAT)
        return 1.0f;

    if (bot->GetMapId() != ZA_MAP_ID) // In case strategy persists outside (e.g., server reset)
        return 1.0f;

    if (!IsDpsCooldownAction(bot, action))
        return 1.0f;

    // Every Zul'Aman boss, and nothing else in the instance, runs a BossAI.
    Unit* boss = AI_VALUE(Unit*, "boss target");
    if (!boss)
        return 0.0f;

    if (boss->GetHealthPct() > BOSS_ENGAGED_HEALTH_PCT)
        return 0.0f;

    // Further restrictions on Bloodlust for Zul'jin and Jan'alai
    if (bot->getClass() != CLASS_SHAMAN)
        return 1.0f;

    if (!dynamic_cast<CastBloodlustAction*>(action) &&
        !dynamic_cast<CastHeroismAction*>(action))
    {
        return 1.0f;
    }

    // Zul'jin: hold until Phase 3 (or later)
    if (boss->GetEntry() == Id(ZaNpcs::NPC_ZULJIN))
    {
        return (boss->HasAura(Id(ZaSpells::SPELL_SHAPE_OF_THE_EAGLE)) ||
            boss->HasAura(Id(ZaSpells::SPELL_SHAPE_OF_THE_LYNX)) ||
            boss->HasAura(Id(ZaSpells::SPELL_SHAPE_OF_THE_DRAGONHAWK))) ? 1.0f : 0.0f;
    }

    // Jan'alai: hold until time to burn Hatchlings (see comments to the constants in ZAHelpers.h)
    if (boss->GetEntry() == Id(ZaNpcs::NPC_JANALAI))
    {
        if (boss->GetHealthPct() <= JANALAI_HATCH_ALL_HEALTH_PCT)
            return 1.0f;

        return CountJanalaiHatchlingsByEntry(botAI) >= JANALAI_BLOODLUST_HATCHLING_COUNT ?
            1.0f : 0.0f;
    }

    return 1.0f;
}

// Malacrass siphoning a Warrior soul and Zul'jin have very similar Whirlwinds
float ZulAmanAvoidWhirlwindMultiplier::GetValueInEncounter(Action* action)
{
    if (!IsApproachMovement(action) && !dynamic_cast<CastKillingSpreeAction*>(action))
        return 1.0f;

    if (Unit* zuljin = AI_VALUE2(Unit*, "find target", "23863"))
    {
        if (zuljin->GetVictim() == bot)
            return 1.0f;

        if (!zuljin->HasAura(Id(ZaSpells::SPELL_ZULJIN_WHIRLWIND)))
            return 1.0f;

        return bot->GetExactDist2d(zuljin) <= ZA_WHIRLWIND_HOLD_DISTANCE ? 0.0f : 1.0f;
    }

    if (Unit* malacrass = AI_VALUE2(Unit*, "find target", "24239"))
    {
        if (malacrass->GetVictim() == bot)
            return 1.0f;

        if (!malacrass->HasAura(Id(ZaSpells::SPELL_HEX_LORD_WHIRLWIND)))
            return 1.0f;

        return bot->GetExactDist2d(malacrass) <= ZA_WHIRLWIND_HOLD_DISTANCE ? 0.0f : 1.0f;
    }

    return 1.0f;
}

// CombatFormationMoveAction is the action for the "disperse" command. It is also the parent class
// for SetBehindTargetAction and TankFaceAction.
float ZulAmanDisableCombatFormationMoveMultiplier::GetValueInEncounter(Action* action)
{
    if (botAI->GetState() == BOT_STATE_NON_COMBAT)
        return 1.0f;

    if (!dynamic_cast<CombatFormationMoveAction*>(action))
        return 1.0f;

    if (dynamic_cast<SetBehindTargetAction*>(action))
        return 1.0f;

    return AI_VALUE2(Unit*, "find target", "23578") ||
        AI_VALUE2(Unit*, "find target", "23574") ? 0.0f : 1.0f;
}

// Akil'zon <Eagle Avatar>

float AkilzonStayInEyeOfTheStormMultiplier::GetValueInEncounter(Action* action)
{
    if (!IsHazardousMovement(action))
        return 1.0f;

    if (!AI_VALUE2(Unit*, "find target", "23574"))
        return 1.0f;

    if (dynamic_cast<AkilzonMoveToEyeOfTheStormAction*>(action))
        return 1.0f;

    auto it = akilzonStormTimer.find(bot->GetInstanceId());
    if (it == akilzonStormTimer.end())
        return 1.0f;

    return IsInStormWindow(it->second) ? 0.0f : 1.0f;
}
//End By leewheel

// Nalorakk <Bear Avatar>

//By leewheel 2026-09-04: 上游14413ee2——统一坦克动作抑制语义(isTankFace/isTankAssist/isTaunt三分类);
//  Nalorakk按形态压制嘲讽: 熊形态主坦持有, 人形态副坦持有
float NalorakkDisableTankActionsMultiplier::GetValueInEncounter(Action* action)
{
    if (botAI->GetState() == BOT_STATE_NON_COMBAT)
        return 1.0f;

    if (!PlayerbotAI::IsTank(bot))
        return 1.0f;

    bool const isTankFace = dynamic_cast<TankFaceAction*>(action);
    bool const isTankAssist = dynamic_cast<TankAssistAction*>(action);
    bool const isTaunt = !isTankFace && !isTankAssist && IsTauntAction(bot, action);

    if (!isTankFace && !isTankAssist && !isTaunt)
        return 1.0f;

    Unit* nalorakk = AI_VALUE2(Unit*, "find target", "23576");
    if (!nalorakk)
        return 1.0f;

    if (!isTaunt)
        return 0.0f;

    // Nalorakk: A tank swap is used by form so suppress taunts for the opposite-form tank.
    bool const isInBearForm = IsNalorakkInBearForm(nalorakk);

    if (!isInBearForm && PlayerbotAI::IsAssistTankOfIndex(bot, 0, true))
        return 0.0f;

    return isInBearForm && PlayerbotAI::IsMainTank(bot) ? 0.0f : 1.0f;
}

// Nalorakk: Don't Misdirect the boss in troll form to the Main Tank.
float NalorakkControlMisdirectionMultiplier::GetValueInEncounter(Action* action)
{
    if (botAI->GetState() == BOT_STATE_NON_COMBAT)
        return 1.0f;

    if (bot->getClass() != CLASS_HUNTER)
        return 1.0f;

    if (!dynamic_cast<CastMisdirectionOnMainTankAction*>(action))
        return 1.0f;

    return AI_VALUE2(Unit*, "find target", "23576") ? 0.0f : 1.0f;
}

// Jan'alai <Dragonhawk Avatar>

//By leewheel 2026-09-04: 上游14413ee2——Jan'alai允许副坦坦克辅助接手幼龙, 仅主坦被压制; 正面朝向由动作/编队乘数处理
float JanalaiDisableTankActionsMultiplier::GetValueInEncounter(Action* action)
{
    if (botAI->GetState() == BOT_STATE_NON_COMBAT)
        return 1.0f;

    if (!PlayerbotAI::IsTank(bot))
        return 1.0f;

    bool const isTankFace = dynamic_cast<TankFaceAction*>(action);
    bool const isTankAssist = dynamic_cast<TankAssistAction*>(action);
    bool const isTaunt = !isTankFace && !isTankAssist && IsTauntAction(bot, action);

    if (!isTankFace && !isTankAssist && !isTaunt)
        return 1.0f;

    if (!AI_VALUE2(Unit*, "find target", "23578"))
        return 1.0f;

    if (isTaunt)
        return 1.0f;

    // Jan'alai: Allow tank assist for the assist tank to pick up the Hatchlings. Tank face is
    // already disabled as part of ZulAmanDisableCombatFormationMoveMultiplier so the addition here
    // is just belt-and-suspenders for no cost.
    return isTankFace || PlayerbotAI::IsMainTank(bot) ? 0.0f : 1.0f;
}

float JanalaiStayAwayFromFireBombsMultiplier::GetValueInEncounter(Action* action)
{
    if (!IsHazardousMovement(action))
        return 1.0f;

    Unit* janalai = AI_VALUE2(Unit*, "find target", "23578");
    if (!janalai)
        return 1.0f;

    if (dynamic_cast<JanalaiAvoidFireBombsAction*>(action))
        return 1.0f;

    return IsJanalaiBombing(janalai) ? 0.0f : 1.0f;
}

float JanalaiDoNotCrowdControlHatchersMultiplier::GetValueInEncounter(Action* action)
{
    if (botAI->GetState() == BOT_STATE_NON_COMBAT)
        return 1.0f;

    if (!dynamic_cast<CastCrowdControlSpellAction*>(action))
        return 1.0f;

    //By leewheel 2026-09-04 对齐上游: 回归按动作目标判定——CC 目标恰为孵化者时压制;
    //原写法"场上存在孵化者即压制所有 CC"会过度禁止 CC 其他小怪
    Unit* target = action->GetTarget();
    return target && target->GetEntry() == Id(ZaNpcs::NPC_AMANISHI_HATCHER) ? 0.0f : 1.0f;
}

// Halazzi <Lynx Avatar>

//By leewheel 2026-09-04: 上游14413ee2——Halazzi: 主坦禁止嘲讽山猫之灵(由副坦接手), 其余坦克动作照常压制
float HalazziDisableTankActionsMultiplier::GetValueInEncounter(Action* action)
{
    if (botAI->GetState() == BOT_STATE_NON_COMBAT)
        return 1.0f;

    if (!PlayerbotAI::IsTank(bot))
        return 1.0f;

    bool const isTankFace = dynamic_cast<TankFaceAction*>(action);
    bool const isTankAssist = dynamic_cast<TankAssistAction*>(action);
    bool const isTaunt = !isTankFace && !isTankAssist && IsTauntAction(bot, action);

    if (!isTankFace && !isTankAssist && !isTaunt)
        return 1.0f;

    if (!AI_VALUE2(Unit*, "find target", "23577"))
        return 1.0f;

    if (!isTaunt)
        return 0.0f;

    // The assist tank picks up the Spirit of the Lynx so disable the main tank from taunting it
    if (!PlayerbotAI::IsMainTank(bot))
        return 1.0f;

    Unit* target = action->GetTarget();
    return target && target->GetEntry() == Id(ZaNpcs::NPC_SPIRIT_OF_THE_LYNX) ? 0.0f : 1.0f;
}

// Halazzi: Don't Misdirect the Spirit of the Lynx to the Main Tank.
float HalazziControlMisdirectionMultiplier::GetValueInEncounter(Action* action)
{
    if (botAI->GetState() == BOT_STATE_NON_COMBAT)
        return 1.0f;

    if (bot->getClass() != CLASS_HUNTER)
        return 1.0f;

    if (!dynamic_cast<CastMisdirectionOnMainTankAction*>(action))
        return 1.0f;

    return AI_VALUE2(Unit*, "find target", "23577") ? 0.0f : 1.0f;
}

float HalazziDisableAutoDpsTargetingMultiplier::GetValueInEncounter(Action* action)
{
    if (botAI->GetState() == BOT_STATE_NON_COMBAT)
        return 1.0f;

    if (!PlayerbotAI::IsDps(bot))
        return 1.0f;

    if (!dynamic_cast<DpsAssistAction*>(action) &&
        !dynamic_cast<CastDebuffSpellOnAttackerAction*>(action))
    {
        return 1.0f;
    }

    return AI_VALUE2(Unit*, "find target", "23577") ? 0.0f : 1.0f;
}

// Hex Lord Malacrass

// Weirdly, Unstable Affliction is considered a magic effect, not a curse.
float HexLordMalacrassUnstableAfflictionMultiplier::GetValueInEncounter(Action* action)
{
    if (botAI->GetState() == BOT_STATE_NON_COMBAT)
        return 1.0f;

    if (bot->getClass() == CLASS_PRIEST)
    {
        if (!dynamic_cast<CastDispelMagicOnPartyAction*>(action) &&
            !dynamic_cast<CastDispelMagicAction*>(action) &&
            !dynamic_cast<CastMassDispelAction*>(action))
        {
            return 1.0f;
        }
    }
    else if (bot->getClass() == CLASS_PALADIN)
    {
        if (!dynamic_cast<CastCleanseMagicOnPartyAction*>(action) &&
            !dynamic_cast<CastCleanseMagicAction*>(action))
        {
            return 1.0f;
        }
    }
    else
    {
        return 1.0f;
    }

    if (!AI_VALUE2(Unit*, "find target", "24239"))
        return 1.0f;

    Unit* target = AI_VALUE2(Unit*, "party member to dispel", DISPEL_MAGIC);
    return target && target->HasAura(Id(ZaSpells::SPELL_UNSTABLE_AFFLICTION)) ? 0.0f : 1.0f;
}

float HexLordMalacrassSpellReflectionMultiplier::GetValueInEncounter(Action* action)
{
    if (!PlayerbotAI::IsCaster(bot))
        return 1.0f;

    if (dynamic_cast<CastHealingSpellAction*>(action))
        return 1.0f;

    if (!dynamic_cast<CastSpellAction*>(action))
        return 1.0f;

    Unit* malacrass = AI_VALUE2(Unit*, "find target", "24239");
    return malacrass &&
        malacrass->HasAura(Id(ZaSpells::SPELL_HEX_LORD_SPELL_REFLECTION)) ? 0.0f : 1.0f;
}

//By leewheel 2026-09-04: 上游14413ee2——冰霜陷阱驻留乘数(接近类动作在陷阱存在时暂停, 与逃跑动作互不拉扯)
float HexLordMalacrassStayAwayFromFreezingTrapMultiplier::GetValueInEncounter(Action* action)
{
    if (!IsApproachMovement(action))
        return 1.0f;

    return GetNearbyFreezingTrap(botAI) ? 0.0f : 1.0f;
}
//End By leewheel

// Zul'jin

//By leewheel 2026-09-04: 上游14413ee2——Zul'jin阶段5允许坦克正面(把龙头离人群), 龙鹰形态禁止
float ZuljinDisableTankFaceMultiplier::GetValueInEncounter(Action* action)
{
    if (botAI->GetState() == BOT_STATE_NON_COMBAT)
        return 1.0f;

    if (!PlayerbotAI::IsTank(bot))
        return 1.0f;

    if (!dynamic_cast<TankFaceAction*>(action))
        return 1.0f;

    //By leewheel 2026-09-04 对齐上游: zuljin 查不到时必须放行 1.0f——原写法空指针方向反了,
    //ZA 内打其他 boss/清怪时坦克面向动作被误压制(压制的条件是"boss是祖金且非龙鹰形态")
    Unit* zuljin = AI_VALUE2(Unit*, "find target", "23863");
    return zuljin && zuljin->HasAura(Id(ZaSpells::SPELL_SHAPE_OF_THE_DRAGONHAWK)) ? 0.0f : 1.0f;
}

// AvoidAoeAction is otherwise triggered by the Feather Vortices, and it is useless as they chase
// players at player run speed (the bot runs away when it gets hit, and the vortex just chases the
// bot at the same speed).
float ZuljinEagleDisableAvoidAoeMultiplier::GetValueInEncounter(Action* action)
{
    if (botAI->GetState() == BOT_STATE_NON_COMBAT)
        return 1.0f;

    if (!dynamic_cast<AvoidAoeAction*>(action))
        return 1.0f;

    Unit* zuljin = AI_VALUE2(Unit*, "find target", "23863");
    return zuljin && zuljin->HasAura(Id(ZaSpells::SPELL_SHAPE_OF_THE_EAGLE)) ? 0.0f : 1.0f;
}
