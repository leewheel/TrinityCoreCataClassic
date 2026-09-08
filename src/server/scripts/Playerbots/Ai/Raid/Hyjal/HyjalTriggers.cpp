/* 海加尔山 机器人策略 */
/*By leewheel 2026-08-18: 对齐 brighton-chi the-lab HEAD(e92a52db)——
  移植 47abcff2(压缩: 拉怪/主坦接战触发)、33f67223/7f293f95(命名)、
  8df2134e/ab50f6a1(Archimonde: 恐惧/气爆/散开/地狱火触发, 月神之佑豁免)、
  5f34b67d(IsNearDoomfire 走缓存)。
  By leewheel 2026-09-04: 上游6d5d68cd——Boss 触发器继承 HyjalSummitEncounterTrigger 基类,
  实现函数改名 IsActiveInEncounter(IsActive 由基类 final 提供, 内含 IsEncounterInProgress 闸门)。*/
#include "HyjalTriggers.h"
#include "EncounterHelpers.h"
#include "HyjalActions.h"
#include "HyjalHelpers.h"
#include "InstanceScript.h"
#include "Playerbots.h"

using namespace HyjalHelpers;
using namespace EncounterHelpers;

// General

bool HyjalSummitNoEncounterInProgress::IsActive()
{
    if (bot->GetMapId() != HYJAL_MAP_ID)
        return false;

    InstanceScript* instance = bot->GetInstanceScript();
    return instance && !instance->IsEncounterInProgress();
}

bool HyjalPullingBossTrigger::IsActive()
{
    if (bot->getClass() != CLASS_HUNTER)
        return false;

    Unit* boss = AI_VALUE2(Unit*, "find target", _bossName);
    return boss && boss->GetHealthPct() > BOSS_ENGAGED_HEALTH_PCT;
}

bool HyjalBossShouldBeTankedTrigger::IsActiveInEncounter()
{
    if (!PlayerbotAI::IsTank(bot))
        return false;

    // IsMainTank() does not require an actual tank (by strategy or spec), but the raid strategy
    // assumes the main tank will be a tank.
    if (_mainTankOnly && !PlayerbotAI::IsMainTank(bot))
        return false;

    Unit* boss = AI_VALUE2(Unit*, "find target", _bossName);
    return boss && boss->GetHealthPct() > _activeAboveHealthPct;
}

// Rage Winterchill

bool RageWinterchillRangedShouldSpreadTrigger::IsActiveInEncounter()
{
    return PlayerbotAI::IsRanged(bot) && AI_VALUE2(Unit*, "find target", "17767");
}

bool RageWinterchillMeleeNearDeathAndDecayTrigger::IsActiveInEncounter()
{
    if (!PlayerbotAI::IsMelee(bot))
        return false;

    Unit* winterchill = AI_VALUE2(Unit*, "find target", "17767");
    if (!winterchill || winterchill->GetVictim() == bot)
        return false;

    if (PlayerbotAI::IsMainTank(bot))
        return false;

    // Reaches as far as the suppression that accompanies it, not just to the pool. Everything
    // inside that radius has had its path back to the boss zeroed, so this action has to keep
    // running across the whole of it--it is the only thing left that can walk the bot anywhere,
    // including back onto Winterchill once the pool no longer blocks the ring
    return IsNearDeathAndDecay(bot, DEATH_AND_DECAY_MELEE_CONTROL_RADIUS);
}

bool RageWinterchillRangedInDeathAndDecayTrigger::IsActiveInEncounter()
{
    if (!PlayerbotAI::IsRanged(bot))
        return false;

    if (!AI_VALUE2(Unit*, "find target", "17767"))
        return false;

    return IsInDeathAndDecay(bot);
}

// Anetheron

bool AnetheronPullingBossOrInfernalTrigger::IsActive()
{
    return bot->getClass() == CLASS_HUNTER && AI_VALUE2(Unit*, "find target", "17808");
}

bool AnetheronRangedShouldSpreadTrigger::IsActiveInEncounter()
{
    if (PlayerbotAI::IsMelee(bot))
        return false;

    Unit* anetheron = AI_VALUE2(Unit*, "find target", "17808");
    if (!anetheron)
        return false;

    if (GetInfernoTarget(anetheron) == bot)
        return false;

    return !GetInfernalToAttack(botAI, anetheron);
}

// Whoever is holding Anetheron stays put: walking him across the platform costs the raid more than
// a two second stun costs one bot. The Inferno target itself is excluded because it has its own job
// -- carrying the summon to the gathering spot -- and nothing it does avoids a stun centred on it
bool AnetheronBotIsNearInfernoTargetTrigger::IsActiveInEncounter()
{
    Unit* anetheron = AI_VALUE2(Unit*, "find target", "17808");
    if (!anetheron || anetheron->GetVictim() == bot)
        return false;

    Player* infernoTarget = GetInfernoTarget(anetheron);
    if (!infernoTarget || infernoTarget == bot)
        return false;

    return bot->GetExactDist2d(infernoTarget) < INFERNAL_ESCAPE_DISTANCE;
}

bool AnetheronBotIsTargetedByInfernalTrigger::IsActiveInEncounter()
{
    Unit* anetheron = AI_VALUE2(Unit*, "find target", "17808");
    if (!anetheron || anetheron->GetVictim() == bot)
        return false;

    if (GetInfernoTarget(anetheron) == bot)
        return true;

    if (IsInfernalTank(bot))
        return false;

    return GetInfernalTargetingBot(bot);
}

bool AnetheronInfernalsPulseImmolationTrigger::IsActiveInEncounter()
{
    if (PlayerbotAI::IsTank(bot))
        return false;

    if (!AI_VALUE2(Unit*, "find target", "anetheron"))
        return false;

    Unit* infernal = GetNearestInfernal(bot);
    return infernal && infernal->GetVictim() != bot &&
        bot->GetExactDist2d(infernal) < INFERNAL_DANGER_RADIUS;
}

bool AnetheronInfernalsShouldBeTankedAwayTrigger::IsActiveInEncounter()
{
    if (!IsInfernalTank(bot))
        return false;

    if (!AI_VALUE2(Unit*, "find target", "17808"))
        return false;

    Unit* infernal = GetInfernalTargetingBot(bot);
    return infernal && bot->IsWithinMeleeRange(infernal);
}

//By leewheel 2026-08-30: 对齐上游类名——AnetheronShouldDetermineDpsPriorityTrigger 改 AnetheronShouldDivideDpsTrigger
//  逻辑同步上游(IsDps判定)
bool AnetheronShouldDivideDpsTrigger::IsActiveInEncounter()
{
    return PlayerbotAI::IsDps(bot) && AI_VALUE2(Unit*, "find target", "17808");
}
//End By leewheel

// Kaz'rogal

//By leewheel 2026-08-30: 对齐上游类名——KazrogalMalevolentCleaveSplitsDamageTrigger 改 KazrogalCanSplitMalevolentCleaveDamageTrigger
bool KazrogalCanSplitMalevolentCleaveDamageTrigger::IsActiveInEncounter()
{
    if (!PlayerbotAI::IsAssistTank(bot))
        return false;

    if (!AI_VALUE2(Unit*, "find target", "17888"))
        return false;

    if (bot->getClass() != CLASS_PALADIN)
        return true;

    return !botsBelowManaThreshold.contains(bot->GetGUID());
}
//End By leewheel

//By leewheel 2026-08-30: 对齐上游类名——KazrogalLowManaBotsNeedEscapePathTrigger 改 KazrogalRangedShouldAvoidWarStompTrigger
bool KazrogalRangedShouldAvoidWarStompTrigger::IsActiveInEncounter()
{
    if (!PlayerbotAI::IsRanged(bot))
        return false;

    if (!AI_VALUE2(Unit*, "find target", "17888"))
        return false;

    return !botsBelowManaThreshold.contains(bot->GetGUID());
}
//End By leewheel

bool KazrogalBotIsLowOnManaTrigger::IsActiveInEncounter()
{
    if (!IsKazrogalManaUser(botAI, bot))
        return false;

    // Hunters never run. They rely only on Aspect of the Viper.
    if (bot->getClass() == CLASS_HUNTER)
        return false;

    Unit* kazrogal = AI_VALUE2(Unit*, "find target", "17888");
    if (!kazrogal || kazrogal->GetVictim() == bot)
        return false;

    if (bot->GetPower(POWER_MANA) <= MARK_DANGER_MANA)
    {
        botsBelowManaThreshold.insert(bot->GetGUID());
        return true;
    }

    return botsBelowManaThreshold.contains(bot->GetGUID());
}

bool KazrogalHunterShouldPreserveManaTrigger::IsActiveInEncounter()
{
    if (bot->getClass() != CLASS_HUNTER)
        return false;

    if (!AI_VALUE2(Unit*, "find target", "17888"))
        return false;

    if (bot->HasAura(Id(HyjalSpells::SPELL_ASPECT_OF_THE_VIPER)))
        return false;

    // Activate at 3200 mana; switch back based on normal Hunter strategy
    return bot->GetPower(POWER_MANA) <= MARK_DANGER_MANA;
}

bool KazrogalMarkOnMageOrPaladinTrigger::IsActiveInEncounter()
{
    if (bot->getClass() != CLASS_MAGE && bot->getClass() != CLASS_PALADIN)
        return false;

    Unit* kazrogal = AI_VALUE2(Unit*, "find target", "17888");
    if (!kazrogal || kazrogal->GetVictim() == bot)
        return false;

    Aura* aura = bot->GetAura(Id(HyjalSpells::SPELL_MARK_OF_KAZROGAL));
    if (!aura)
        return false;

    uint32 const mana = bot->GetPower(POWER_MANA);
    constexpr float markFullyDrainedMana = 3000.0f;
    if (mana >= markFullyDrainedMana)
        return false;

    // Blowing Ice Block/Divine Shield is worth it only where the Mark outlasts mana.
    //   2400-2999  needs 5s left      1200-1799  needs 3s left      0-599  needs 1s left
    //   1800-2399  needs 4s left       600-1199  needs 2s left
    uint32 const tickDrain = static_cast<uint32>(MARK_TICK_DRAIN);
    int32 const requiredMs = static_cast<int32>(mana / tickDrain + 1) * IN_MILLISECONDS;

    return aura->GetDuration() >= requiredMs;
}

bool KazrogalWarlockShouldManageManaTrigger::IsActiveInEncounter()
{
    if (bot->getClass() != CLASS_WARLOCK)
        return false;

    if (!AI_VALUE2(Unit*, "find target", "17888"))
        return false;

    if (bot->GetPower(POWER_MANA) <= MARK_LIFE_TAP_MANA &&
        bot->GetHealthPct() > sPlayerbotAIConfig.lowHealth)
    {
        return true;
    }

    if (!HasMarkOfKazrogal(bot) || botAI->HasAura("shadow ward", bot))
        return false;

    return bot->GetPower(POWER_MANA) <= MARK_TICK_DRAIN;
}

bool KazrogalImmunityNoLongerNeededTrigger::IsActiveInEncounter()
{
    if (bot->getClass() != CLASS_MAGE &&
        (bot->getClass() != CLASS_PALADIN || PlayerbotAI::IsHeal(bot)))
    {
        return false;
    }

    uint32 const spellId = GetKazrogalImmunitySpell(bot);
    if (!spellId || !bot->HasAura(spellId))
        return false;

    if (HasMarkOfKazrogal(bot))
        return false;

    // 50% is a proxy for the bot potentially being in range of getting blown up by other bots,
    // so don't wipe the immunity if below that HP.
    constexpr float keepImmunityHealthPct = 50.0f;
    if (bot->GetHealthPct() <= keepImmunityHealthPct)
        return false;

    return AI_VALUE2(Unit*, "find target", "kaz'rogal");
}

// Azgalor

bool AzgalorRangedShouldSpreadTrigger::IsActiveInEncounter()
{
    if (!PlayerbotAI::IsRanged(bot))
        return false;

    Unit* azgalor = AI_VALUE2(Unit*, "find target", "17842");
    if (!azgalor || azgalor->GetVictim() == bot)
        return false;

    if (IsDoomed(bot))
        return false;

    //By leewheel 2026-08-21: 移植上游——改用共享常量 RAIN_OF_FIRE_RANGED_CONTROL_RADIUS
    return !IsNearRainOfFire(bot, RAIN_OF_FIRE_RANGED_CONTROL_RADIUS);
}

bool AzgalorMeleeNearRainOfFireTrigger::IsActiveInEncounter()
{
    if (!PlayerbotAI::IsMelee(bot))
        return false;

    Unit* azgalor = AI_VALUE2(Unit*, "find target", "17842");
    if (!azgalor || azgalor->GetVictim() == bot)
        return false;

    if (IsDoomed(bot))
        return false;

    // The Doomguard tank keeps its corner. This action walks bots onto Azgalor's melee ring, which
    // for that one would haul the Doomguard into the raid behind it--and at ACTION_EMERGENCY it
    // outranks the positioning that would walk it back, so it would not return until the pool died.
    //
    // That leaves the Doomguard tank with no Rain of Fire avoidance at all: stock avoid-aoe is off
    // for the whole fight and the ranged escape does not apply to it either. Deliberate, not an
    // oversight--the corner is a fixed position and shifting it far enough to clear a pool costs
    // more than it saves. One tank standing in fire is a healing problem; a loose Doomguard is not
    if (IsDoomguardTank(bot))
        return false;

    // Reaches as far as the suppression that accompanies it, not just to the fire. Everything
    // inside that radius has had its other movement zeroed, so this action has to keep running
    // across the whole of it--it is the only thing left that can walk the bot anywhere, including
    // back onto Azgalor once the tank has dragged him clear of the pool
    return IsNearRainOfFire(bot, RAIN_OF_FIRE_MELEE_CONTROL_RADIUS);
}

//By leewheel 2026-08-30: 对齐上游类名——AzgalorRangedIsStandingInRainOfFireTrigger 改 AzgalorRangedInRainOfFireTrigger
bool AzgalorRangedInRainOfFireTrigger::IsActiveInEncounter()
{
    if (!PlayerbotAI::IsRanged(bot))
        return false;

    if (!AI_VALUE2(Unit*, "find target", "17842"))
        return false;

    if (IsDoomed(bot))
        return false;

    return IsInRainOfFire(bot);
}
//End By leewheel

bool AzgalorBotIsDoomedTrigger::IsActiveInEncounter()
{
    return IsDoomed(bot);
}

//By leewheel 2026-08-30: 对齐上游类名——AzgalorDoomguardsMustBeControlledTrigger 改 AzgalorShouldControlDoomguardsTrigger
bool AzgalorShouldControlDoomguardsTrigger::IsActiveInEncounter()
{
    if (!PlayerbotAI::IsTank(bot))
        return false;

    if (!AI_VALUE2(Unit*, "find target", "17842"))
        return false;

    if (!IsDoomguardTank(bot))
        return false;

    return AI_VALUE2(Unit*, "find target", "17864") || AnyGroupMemberHasDoom(bot);
}
//End By leewheel

bool AzgalorShouldDivideDpsTrigger::IsActiveInEncounter()
{
    return PlayerbotAI::IsDps(bot) && AI_VALUE2(Unit*, "find target", "17842");
}

// Archimonde

bool ArchimondeBossCastsFearTrigger::IsActiveInEncounter()
{
    //By leewheel 2026-09-04: 对齐上游b8304144——牧师fear ward走通用策略，此触发只保留萨满战栗图腾
    if (bot->getClass() != CLASS_SHAMAN)
        return false;

    Unit* archimonde = AI_VALUE2(Unit*, "find target", "17968");
    if (!archimonde || archimonde->GetHealthPct() > BOSS_ENGAGED_HEALTH_PCT)
        return false;

    return !HasProtectionOfElune(bot);
}

bool ArchimondeBossCastingAirBurstTrigger::IsActiveInEncounter()
{
    Unit* archimonde = AI_VALUE2(Unit*, "find target", "17968");
    if (!archimonde || archimonde->GetVictim() == bot)
        return false;

    if (HasProtectionOfElune(bot))
        return false;

    //By leewheel 2026-08-21: 移植上游——移除"主坦豁免"(动作已改为参照当前坦克, 坦克本身由
    // GetVictim()==bot 排除); GetPendingAirBurstCast 改为 bool+出参
    AirBurstData airBurst;
    return GetPendingAirBurstCast(bot->GetInstanceId(), airBurst);
    //End By leewheel
}

// No longer gated to the opening. Ranged drift back together across a fight this long, and the
// spread is cheap: the action rate limits itself and the Doomfire multiplier removes it near a
// trail, so leaving it live costs nothing where it would otherwise get in the way
bool ArchimondeRangedShouldSpreadTrigger::IsActiveInEncounter()
{
    if (!PlayerbotAI::IsRanged(bot))
        return false;

    if (!AI_VALUE2(Unit*, "find target", "17968"))
        return false;

    return !HasProtectionOfElune(bot);
}

bool ArchimondeBotIsNearDoomfireTrigger::IsActiveInEncounter()
{
    Unit* archimonde = AI_VALUE2(Unit*, "find target", "17968");
    if (!archimonde)
        return false;

    if (HasProtectionOfElune(bot))
        return false;

    // The same radius the multiplier suppresses at, so the action owns movement across exactly the
    // area cleared for it. Nothing it does reaches further: the push only has a magnitude inside
    // DOOMFIRE_DANGER_RADIUS, the trapped sweep needs a patch inside DOOMFIRE_BURN_RADIUS, and past
    // this ordinary movement is free again and closes the gap to Archimonde perfectly well. Gating
    // here rather than in the action is what keeps the 18y field sweep off every bot on every tick
    // of a fight where a trail is usually nowhere near them
    return IsNearDoomfire(bot, DOOMFIRE_CONTROL_RADIUS);
}

bool ArchimondeBotStoodInDoomfireTrigger::IsActiveInEncounter()
{
    if (bot->getClass() != CLASS_MAGE && bot->getClass() != CLASS_ROGUE &&
        bot->getClass() != CLASS_PALADIN)
    {
        return false;
    }

    if (HasProtectionOfElune(bot))
        return false;

    return bot->GetHealthPct() < 40.0f && // Arbitrary high risk-of-death threshold
        (bot->HasAura(Id(HyjalSpells::SPELL_DOOMFIRE)) ||
         bot->HasAura(Id(HyjalSpells::SPELL_DOOMFIRE_DOT)));
}
