/* 地下城机器人策略 */
#include "SethMultipliers.h"
#include "EncounterHelpers.h"
#include "FollowActions.h"
#include "GenericSpellActions.h"
#include "HunterActions.h"
#include "MageActions.h"
#include "Playerbots.h"
#include "ReachTargetActions.h"
#include "SethActions.h"
#include "SethShared.h"
#include "ShamanActions.h"

using namespace SethShared;
using namespace EncounterHelpers;

float SethekkProphetSetTremorTotemMultiplier::GetValue(Action* action)
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

    //By leewheel 2026-08-18: 移植 brighton-chi the-lab 7453e2d8(测试分支警告修复+格式整理)——三元表达式简化
    return AI_VALUE2(Unit*, "find target", "18325") ? 0.0f : 1.0f;
    //End By leewheel
}

float AnzuControlSpellCastingWithSpellBombMultiplier::GetValue(Action* action)
{
    if (!dynamic_cast<CastSpellAction*>(action))
        return 1.0f;

    if (bot->getPowerType() != POWER_MANA || PlayerbotAI::IsTank(bot))
        return 1.0f;

    if (!bot->HasAura(Id(SethSpells::SPELL_SPELL_BOMB)))
        return 1.0f;

    if (PlayerbotAI::IsDps(bot))
        return 0.0f;

    // For healer
    Player* mainTank = GetGroupMainTank(bot);
    //By leewheel 2026-08-18: 移植 brighton-chi the-lab 7453e2d8(测试分支警告修复)——先判空再取血量,等价但更清晰
    if (!mainTank)
        return 1.0f;

    return mainTank->GetHealthPct() > 50.0f ? 0.0f : 1.0f;
    //End By leewheel
}

float TalonKingIkissDelayBloodlustAndHeroismMultiplier::GetValue(Action* action)
{
    if (botAI->GetState() == BOT_STATE_NON_COMBAT)
        return 1.0f;

    if (bot->getClass() != CLASS_SHAMAN)
        return 1.0f;

    if (!dynamic_cast<CastHeroismAction*>(action) &&
        !dynamic_cast<CastBloodlustAction*>(action))
    {
        return 1.0f;
    }

    Unit* ikiss = AI_VALUE2(Unit*, "find target", "18473");
    //By leewheel 2026-08-18: 移植 brighton-chi the-lab 7453e2d8(测试分支警告修复)——先判空再取血量
    if (!ikiss)
        return 1.0f;

    return ikiss->GetHealthPct() > BOSS_ENGAGED_HEALTH_PCT ? 0.0f : 1.0f;
    //End By leewheel
}

float TalonKingIkissControlMovementMultiplier::GetValue(Action* action)
{
    bool const isAlwaysDisabled =
        dynamic_cast<CombatFormationMoveAction*>(action) ||
        dynamic_cast<FleeAction*>(action) ||
        dynamic_cast<CastBlinkBackAction*>(action) ||
        dynamic_cast<CastDisengageAction*>(action);

    if (!isAlwaysDisabled &&
        !dynamic_cast<MovementAction*>(action) &&
        !dynamic_cast<CastReachTargetSpellAction*>(action))
    {
        return 1.0f;
    }

    if (dynamic_cast<TankFaceAction*>(action) ||
        dynamic_cast<SetBehindTargetAction*>(action) ||
        dynamic_cast<TalonKingIkissLosArcaneExplosionAction*>(action))
    {
        return 1.0f;
    }

    Unit* ikiss = AI_VALUE2(Unit*, "find target", "18473");
    if (!ikiss)
        return 1.0f;

    if (isAlwaysDisabled)
        return 0.0f;

    //By leewheel 2026-08-18: 移植 brighton-chi the-lab 7453e2d8(测试分支警告修复+格式整理)——三元表达式简化
    return ikiss->HasAura(Id(SethSpells::SPELL_ARCANE_BUBBLE)) ? 0.0f : 1.0f; // Movement generally
    //End By leewheel
}
