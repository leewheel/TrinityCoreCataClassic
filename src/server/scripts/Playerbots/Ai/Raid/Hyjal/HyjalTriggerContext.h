/* 海加尔山 机器人策略 */
/*By leewheel 2026-08-18: 对齐 brighton-chi the-lab HEAD(e92a52db)——
  移植 47abcff2(压缩: 拉怪/主坦接战触发收敛为通用 HyjalPullingBossTrigger/
  HyjalBossEngagedByMainTankTrigger, 参数化Boss名与接战血量)。*/
#ifndef PLAYERBOTS_HYJALTRIGGERCONTEXT_H
#define PLAYERBOTS_HYJALTRIGGERCONTEXT_H

#include "EncounterHelpers.h"
#include "HyjalHelpers.h"
#include "HyjalTriggers.h"
#include "NamedObjectContext.h"

class //By leewheel 2026-09-04: 上游——RaidHyjalSummitTriggerContext 改名 RaidHyjalTriggerContext
RaidHyjalTriggerContext : public NamedObjectContext<Trigger>
{
public:
    //By leewheel 2026-09-04: 上游——RaidHyjalSummitTriggerContext 改名 RaidHyjalTriggerContext
RaidHyjalTriggerContext()
    {
        // General
        creators["hyjal summit no encounter in progress"] =
            &//By leewheel 2026-09-04: 上游——RaidHyjalSummitTriggerContext 改名 RaidHyjalTriggerContext
RaidHyjalTriggerContext::hyjal_summit_no_encounter_in_progress;

        // Rage Winterchill
        creators["rage winterchill pulling boss"] =
            &//By leewheel 2026-09-04: 上游——RaidHyjalSummitTriggerContext 改名 RaidHyjalTriggerContext
RaidHyjalTriggerContext::rage_winterchill_pulling_boss;

        creators["rage winterchill should be tanked"] =
            &//By leewheel 2026-09-04: 上游——RaidHyjalSummitTriggerContext 改名 RaidHyjalTriggerContext
RaidHyjalTriggerContext::rage_winterchill_should_be_tanked;

        creators["rage winterchill ranged should spread"] =
            &//By leewheel 2026-09-04: 上游——RaidHyjalSummitTriggerContext 改名 RaidHyjalTriggerContext
RaidHyjalTriggerContext::rage_winterchill_ranged_should_spread;

        creators["rage winterchill melee near death and decay"] =
            &//By leewheel 2026-09-04: 上游——RaidHyjalSummitTriggerContext 改名 RaidHyjalTriggerContext
RaidHyjalTriggerContext::rage_winterchill_melee_near_death_and_decay;

        creators["rage winterchill ranged in death and decay"] =
            &//By leewheel 2026-09-04: 上游——RaidHyjalSummitTriggerContext 改名 RaidHyjalTriggerContext
RaidHyjalTriggerContext::rage_winterchill_ranged_in_death_and_decay;

        // Anetheron
        creators["anetheron pulling boss or infernal"] =
            &//By leewheel 2026-09-04: 上游——RaidHyjalSummitTriggerContext 改名 RaidHyjalTriggerContext
RaidHyjalTriggerContext::anetheron_pulling_boss_or_infernal;

        creators["anetheron should be tanked"] =
            &//By leewheel 2026-09-04: 上游——RaidHyjalSummitTriggerContext 改名 RaidHyjalTriggerContext
RaidHyjalTriggerContext::anetheron_should_be_tanked;

        creators["anetheron ranged should spread"] =
            &//By leewheel 2026-09-04: 上游——RaidHyjalSummitTriggerContext 改名 RaidHyjalTriggerContext
RaidHyjalTriggerContext::anetheron_ranged_should_spread;

        creators["anetheron bot is near inferno target"] =
            &//By leewheel 2026-09-04: 上游——RaidHyjalSummitTriggerContext 改名 RaidHyjalTriggerContext
RaidHyjalTriggerContext::anetheron_bot_is_near_inferno_target;

        creators["anetheron bot is targeted by infernal"] =
            &//By leewheel 2026-09-04: 上游——RaidHyjalSummitTriggerContext 改名 RaidHyjalTriggerContext
RaidHyjalTriggerContext::anetheron_bot_is_targeted_by_infernal;

        creators["anetheron infernals pulse immolation"] =
            &//By leewheel 2026-09-04: 上游——RaidHyjalSummitTriggerContext 改名 RaidHyjalTriggerContext
RaidHyjalTriggerContext::anetheron_infernals_pulse_immolation;

        creators["anetheron infernals should be tanked away"] =
            &//By leewheel 2026-09-04: 上游——RaidHyjalSummitTriggerContext 改名 RaidHyjalTriggerContext
RaidHyjalTriggerContext::anetheron_infernals_should_be_tanked_away;

        creators["anetheron should divide dps"] =
            &//By leewheel 2026-09-04: 上游——RaidHyjalSummitTriggerContext 改名 RaidHyjalTriggerContext
RaidHyjalTriggerContext::anetheron_should_divide_dps;

        // Kaz'rogal
        creators["kaz'rogal pulling boss"] =
            &//By leewheel 2026-09-04: 上游——RaidHyjalSummitTriggerContext 改名 RaidHyjalTriggerContext
RaidHyjalTriggerContext::kazrogal_pulling_boss;

        creators["kaz'rogal should be tanked"] =
            &//By leewheel 2026-09-04: 上游——RaidHyjalSummitTriggerContext 改名 RaidHyjalTriggerContext
RaidHyjalTriggerContext::kazrogal_should_be_tanked;

        creators["kaz'rogal can split malevolent cleave damage"] =
            &//By leewheel 2026-09-04: 上游——RaidHyjalSummitTriggerContext 改名 RaidHyjalTriggerContext
RaidHyjalTriggerContext::kazrogal_can_split_malevolent_cleave_damage;

        creators["kaz'rogal ranged should avoid war stomp"] =
            &//By leewheel 2026-09-04: 上游——RaidHyjalSummitTriggerContext 改名 RaidHyjalTriggerContext
RaidHyjalTriggerContext::kazrogal_ranged_should_avoid_war_stomp;

        creators["kaz'rogal bot is low on mana"] =
            &//By leewheel 2026-09-04: 上游——RaidHyjalSummitTriggerContext 改名 RaidHyjalTriggerContext
RaidHyjalTriggerContext::kazrogal_bot_is_low_on_mana;

        creators["kaz'rogal hunter should preserve mana"] =
            &//By leewheel 2026-09-04: 上游——RaidHyjalSummitTriggerContext 改名 RaidHyjalTriggerContext
RaidHyjalTriggerContext::kazrogal_hunter_should_preserve_mana;

        creators["kaz'rogal mark on mage or paladin"] =
            &//By leewheel 2026-09-04: 上游——RaidHyjalSummitTriggerContext 改名 RaidHyjalTriggerContext
RaidHyjalTriggerContext::kazrogal_mark_on_mage_or_paladin;

        creators["kaz'rogal immunity no longer needed"] =
            &//By leewheel 2026-09-04: 上游——RaidHyjalSummitTriggerContext 改名 RaidHyjalTriggerContext
RaidHyjalTriggerContext::kazrogal_immunity_no_longer_needed;

        creators["kaz'rogal warlock should manage mana"] =
            &//By leewheel 2026-09-04: 上游——RaidHyjalSummitTriggerContext 改名 RaidHyjalTriggerContext
RaidHyjalTriggerContext::kazrogal_warlock_should_manage_mana;

        // Azgalor
        creators["azgalor pulling boss"] =
            &//By leewheel 2026-09-04: 上游——RaidHyjalSummitTriggerContext 改名 RaidHyjalTriggerContext
RaidHyjalTriggerContext::azgalor_pulling_boss;

        creators["azgalor should be tanked"] =
            &//By leewheel 2026-09-04: 上游——RaidHyjalSummitTriggerContext 改名 RaidHyjalTriggerContext
RaidHyjalTriggerContext::azgalor_should_be_tanked;

        creators["azgalor ranged should spread"] =
            &//By leewheel 2026-09-04: 上游——RaidHyjalSummitTriggerContext 改名 RaidHyjalTriggerContext
RaidHyjalTriggerContext::azgalor_ranged_should_spread;

        creators["azgalor melee near rain of fire"] =
            &//By leewheel 2026-09-04: 上游——RaidHyjalSummitTriggerContext 改名 RaidHyjalTriggerContext
RaidHyjalTriggerContext::azgalor_melee_near_rain_of_fire;

        creators["azgalor ranged in rain of fire"] =
            &//By leewheel 2026-09-04: 上游——RaidHyjalSummitTriggerContext 改名 RaidHyjalTriggerContext
RaidHyjalTriggerContext::azgalor_ranged_in_rain_of_fire;

        creators["azgalor bot is doomed"] =
            &//By leewheel 2026-09-04: 上游——RaidHyjalSummitTriggerContext 改名 RaidHyjalTriggerContext
RaidHyjalTriggerContext::azgalor_bot_is_doomed;

        creators["azgalor should control doomguards"] =
            &//By leewheel 2026-09-04: 上游——RaidHyjalSummitTriggerContext 改名 RaidHyjalTriggerContext
RaidHyjalTriggerContext::azgalor_should_control_doomguards;

        creators["azgalor should divide dps"] =
            &//By leewheel 2026-09-04: 上游——RaidHyjalSummitTriggerContext 改名 RaidHyjalTriggerContext
RaidHyjalTriggerContext::azgalor_should_divide_dps;

        // Archimonde
        creators["archimonde pulling boss"] =
            &//By leewheel 2026-09-04: 上游——RaidHyjalSummitTriggerContext 改名 RaidHyjalTriggerContext
RaidHyjalTriggerContext::archimonde_pulling_boss;

        creators["archimonde should be tanked"] =
            &//By leewheel 2026-09-04: 上游——RaidHyjalSummitTriggerContext 改名 RaidHyjalTriggerContext
RaidHyjalTriggerContext::archimonde_boss_engaged_by_main_tank;

        creators["archimonde boss casts fear"] =
            &//By leewheel 2026-09-04: 上游——RaidHyjalSummitTriggerContext 改名 RaidHyjalTriggerContext
RaidHyjalTriggerContext::archimonde_boss_casts_fear;

        creators["archimonde boss casting air burst"] =
            &//By leewheel 2026-09-04: 上游——RaidHyjalSummitTriggerContext 改名 RaidHyjalTriggerContext
RaidHyjalTriggerContext::archimonde_boss_casting_air_burst;

        creators["archimonde ranged should spread"] =
            &//By leewheel 2026-09-04: 上游——RaidHyjalSummitTriggerContext 改名 RaidHyjalTriggerContext
RaidHyjalTriggerContext::archimonde_ranged_should_spread;

        creators["archimonde bot is near doomfire"] =
            &//By leewheel 2026-09-04: 上游——RaidHyjalSummitTriggerContext 改名 RaidHyjalTriggerContext
RaidHyjalTriggerContext::archimonde_bot_is_near_doomfire;

        creators["archimonde bot stood in doomfire"] =
            &//By leewheel 2026-09-04: 上游——RaidHyjalSummitTriggerContext 改名 RaidHyjalTriggerContext
RaidHyjalTriggerContext::archimonde_bot_stood_in_doomfire;
    }

private:
    // General
    static Trigger* hyjal_summit_no_encounter_in_progress(PlayerbotAI* botAI) {
        return new HyjalSummitNoEncounterInProgress(botAI);
    }

    // Rage Winterchill
    static Trigger* rage_winterchill_pulling_boss(PlayerbotAI* botAI) {
        return new HyjalPullingBossTrigger(
            botAI, "rage winterchill pulling boss", "rage winterchill");
    }
    static Trigger* rage_winterchill_should_be_tanked(PlayerbotAI* botAI) {
        return new HyjalBossShouldBeTankedTrigger(
            botAI, "rage winterchill should be tanked", "rage winterchill", 0.0f, false);
    }
    static Trigger* rage_winterchill_ranged_should_spread(PlayerbotAI* botAI) {
        return new RageWinterchillRangedShouldSpreadTrigger(botAI);
    }
    static Trigger* rage_winterchill_melee_near_death_and_decay(PlayerbotAI* botAI) {
        return new RageWinterchillMeleeNearDeathAndDecayTrigger(botAI);
    }
    static Trigger* rage_winterchill_ranged_in_death_and_decay(PlayerbotAI* botAI) {
        return new RageWinterchillRangedInDeathAndDecayTrigger(botAI);
    }

    // Anetheron
    static Trigger* anetheron_pulling_boss_or_infernal(PlayerbotAI* botAI) {
        return new AnetheronPullingBossOrInfernalTrigger(botAI);
    }
    static Trigger* anetheron_should_be_tanked(PlayerbotAI* botAI) {
        return new HyjalBossShouldBeTankedTrigger(
            botAI, "anetheron should be tanked", "anetheron");
    }
    static Trigger* anetheron_ranged_should_spread(PlayerbotAI* botAI) {
        return new AnetheronRangedShouldSpreadTrigger(botAI);
    }
    static Trigger* anetheron_bot_is_near_inferno_target(PlayerbotAI* botAI) {
        return new AnetheronBotIsNearInfernoTargetTrigger(botAI);
    }
    static Trigger* anetheron_bot_is_targeted_by_infernal(PlayerbotAI* botAI) {
        return new AnetheronBotIsTargetedByInfernalTrigger(botAI);
    }
    static Trigger* anetheron_infernals_pulse_immolation(PlayerbotAI* botAI) {
        return new AnetheronInfernalsPulseImmolationTrigger(botAI);
    }
    static Trigger* anetheron_infernals_should_be_tanked_away(PlayerbotAI* botAI) {
        return new AnetheronInfernalsShouldBeTankedAwayTrigger(botAI);
    }
    static Trigger* anetheron_should_divide_dps(PlayerbotAI* botAI) {
        return new AnetheronShouldDivideDpsTrigger(botAI);
    }

    // Kaz'rogal
    static Trigger* kazrogal_pulling_boss(PlayerbotAI* botAI) {
        return new HyjalPullingBossTrigger(botAI, "kaz'rogal pulling boss", "kaz'rogal");
    }
    static Trigger* kazrogal_should_be_tanked(PlayerbotAI* botAI) {
        return new HyjalBossShouldBeTankedTrigger(
            botAI, "kaz'rogal should be tanked", "kaz'rogal");
    }
    static Trigger* kazrogal_can_split_malevolent_cleave_damage(PlayerbotAI* botAI) {
        return new KazrogalCanSplitMalevolentCleaveDamageTrigger(botAI);
    }
    static Trigger* kazrogal_ranged_should_avoid_war_stomp(PlayerbotAI* botAI) {
        return new KazrogalRangedShouldAvoidWarStompTrigger(botAI);
    }
    static Trigger* kazrogal_bot_is_low_on_mana(PlayerbotAI* botAI) {
        return new KazrogalBotIsLowOnManaTrigger(botAI);
    }
    static Trigger* kazrogal_hunter_should_preserve_mana(PlayerbotAI* botAI) {
        return new KazrogalHunterShouldPreserveManaTrigger(botAI);
    }
    static Trigger* kazrogal_mark_on_mage_or_paladin(PlayerbotAI* botAI) {
        return new KazrogalMarkOnMageOrPaladinTrigger(botAI);
    }
    static Trigger* kazrogal_immunity_no_longer_needed(PlayerbotAI* botAI) {
        return new KazrogalImmunityNoLongerNeededTrigger(botAI);
    }
    static Trigger* kazrogal_warlock_should_manage_mana(PlayerbotAI* botAI) {
        return new KazrogalWarlockShouldManageManaTrigger(botAI);
    }

    // Azgalor
    static Trigger* azgalor_pulling_boss(PlayerbotAI* botAI) {
        return new HyjalPullingBossTrigger(botAI, "azgalor pulling boss", "azgalor");
    }
    static Trigger* azgalor_should_be_tanked(PlayerbotAI* botAI) {
        return new HyjalBossShouldBeTankedTrigger(
            botAI, "azgalor should be tanked", "azgalor");
    }
    static Trigger* azgalor_ranged_should_spread(PlayerbotAI* botAI) {
        return new AzgalorRangedShouldSpreadTrigger(botAI);
    }
    static Trigger* azgalor_melee_near_rain_of_fire(PlayerbotAI* botAI) {
        return new AzgalorMeleeNearRainOfFireTrigger(botAI);
    }
    static Trigger* azgalor_ranged_in_rain_of_fire(PlayerbotAI* botAI) {
        return new AzgalorRangedInRainOfFireTrigger(botAI);
    }
    static Trigger* azgalor_bot_is_doomed(PlayerbotAI* botAI) {
        return new AzgalorBotIsDoomedTrigger(botAI);
    }
    static Trigger* azgalor_should_control_doomguards(PlayerbotAI* botAI) {
        return new AzgalorShouldControlDoomguardsTrigger(botAI);
    }
    static Trigger* azgalor_should_divide_dps(PlayerbotAI* botAI) {
        return new AzgalorShouldDivideDpsTrigger(botAI);
    }

    // Archimonde
    static Trigger* archimonde_pulling_boss(PlayerbotAI* botAI) {
        return new HyjalPullingBossTrigger(botAI, "archimonde pulling boss", "archimonde");
    }
    static Trigger* archimonde_boss_engaged_by_main_tank(PlayerbotAI* botAI) {
        return new HyjalBossShouldBeTankedTrigger(
            botAI, "archimonde should be tanked", "archimonde",
            EncounterHelpers::BOSS_ENGAGED_HEALTH_PCT, false);
    }
    static Trigger* archimonde_boss_casts_fear(PlayerbotAI* botAI) {
        return new ArchimondeBossCastsFearTrigger(botAI);
    }
    static Trigger* archimonde_boss_casting_air_burst(PlayerbotAI* botAI) {
        return new ArchimondeBossCastingAirBurstTrigger(botAI);
    }
    static Trigger* archimonde_ranged_should_spread(PlayerbotAI* botAI) {
        return new ArchimondeRangedShouldSpreadTrigger(botAI);
    }
    static Trigger* archimonde_bot_is_near_doomfire(PlayerbotAI* botAI) {
        return new ArchimondeBotIsNearDoomfireTrigger(botAI);
    }
    static Trigger* archimonde_bot_stood_in_doomfire(PlayerbotAI* botAI) {
        return new ArchimondeBotStoodInDoomfireTrigger(botAI);
    }
};

#endif
