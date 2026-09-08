/* 毒蛇神殿 机器人策略 */
#ifndef PLAYERBOTS_SSCTRIGGERCONTEXT_H
#define PLAYERBOTS_SSCTRIGGERCONTEXT_H

#include "NamedObjectContext.h"
#include "SSCTriggers.h"

class //By leewheel 2026-09-04: 上游——RaidSSCTriggerContext 改名 RaidSscTriggerContext
RaidSscTriggerContext : public NamedObjectContext<Trigger>
{
public:
    //By leewheel 2026-09-04: 上游——RaidSSCTriggerContext 改名 RaidSscTriggerContext
RaidSscTriggerContext()
    {
        // General
        creators["serpent shrine cavern no encounter in progress"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCTriggerContext 改名 RaidSscTriggerContext
RaidSscTriggerContext::serpent_shrine_cavern_no_encounter_in_progress;

        // Trash
        creators["underbog colossus spawned toxic pool after death"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCTriggerContext 改名 RaidSscTriggerContext
RaidSscTriggerContext::underbog_colossus_spawned_toxic_pool_after_death;

        creators["greyheart tidecaller water elemental totem spawned"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCTriggerContext 改名 RaidSscTriggerContext
RaidSscTriggerContext::greyheart_tidecaller_water_elemental_totem_spawned;

        // Hydross the Unstable <Duke of Currents>
        creators["hydross the unstable bot is frost tank"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCTriggerContext 改名 RaidSscTriggerContext
RaidSscTriggerContext::hydross_the_unstable_bot_is_frost_tank;

        creators["hydross the unstable bot is nature tank"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCTriggerContext 改名 RaidSscTriggerContext
RaidSscTriggerContext::hydross_the_unstable_bot_is_nature_tank;

        creators["hydross the unstable elementals spawned"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCTriggerContext 改名 RaidSscTriggerContext
RaidSscTriggerContext::hydross_the_unstable_elementals_spawned;

        creators["hydross the unstable danger from water tombs"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCTriggerContext 改名 RaidSscTriggerContext
RaidSscTriggerContext::hydross_the_unstable_danger_from_water_tombs;

        creators["hydross the unstable tank needs aggro upon phase change"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCTriggerContext 改名 RaidSscTriggerContext
RaidSscTriggerContext::hydross_the_unstable_tank_needs_aggro_upon_phase_change;

        creators["hydross the unstable aggro resets upon phase change"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCTriggerContext 改名 RaidSscTriggerContext
RaidSscTriggerContext::hydross_the_unstable_aggro_resets_upon_phase_change;

        creators["hydross the unstable need to manage timers"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCTriggerContext 改名 RaidSscTriggerContext
RaidSscTriggerContext::hydross_the_unstable_need_to_manage_timers;

        // The Lurker Below
        creators["the lurker below spout is active"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCTriggerContext 改名 RaidSscTriggerContext
RaidSscTriggerContext::the_lurker_below_spout_is_active;

        creators["the lurker below boss is active for main tank"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCTriggerContext 改名 RaidSscTriggerContext
RaidSscTriggerContext::the_lurker_below_boss_is_active_for_main_tank;

        creators["the lurker below boss casts geyser"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCTriggerContext 改名 RaidSscTriggerContext
RaidSscTriggerContext::the_lurker_below_boss_casts_geyser;

        creators["the lurker below boss is submerged"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCTriggerContext 改名 RaidSscTriggerContext
RaidSscTriggerContext::the_lurker_below_boss_is_submerged;

        creators["the lurker below need to prepare timer for spout"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCTriggerContext 改名 RaidSscTriggerContext
RaidSscTriggerContext::the_lurker_below_need_to_prepare_timer_for_spout;

        // Leotheras the Blind
        //By leewheel 2026-09-04: 上游——删除 leotheras the blind boss is inactive(触发器随上游一并移除)
        creators["leotheras the blind boss transformed into demon form"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCTriggerContext 改名 RaidSscTriggerContext
RaidSscTriggerContext::leotheras_the_blind_boss_transformed_into_demon_form;

        creators["leotheras the blind only warlock should tank demon form"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCTriggerContext 改名 RaidSscTriggerContext
RaidSscTriggerContext::leotheras_the_blind_only_warlock_should_tank_demon_form;

        creators["leotheras the blind boss engaged by ranged"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCTriggerContext 改名 RaidSscTriggerContext
RaidSscTriggerContext::leotheras_the_blind_boss_engaged_by_ranged;

        creators["leotheras the blind boss channeling whirlwind"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCTriggerContext 改名 RaidSscTriggerContext
RaidSscTriggerContext::leotheras_the_blind_boss_channeling_whirlwind;

        creators["leotheras the blind bot has too many chaos blast stacks"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCTriggerContext 改名 RaidSscTriggerContext
RaidSscTriggerContext::leotheras_the_blind_bot_has_too_many_chaos_blast_stacks;

        creators["leotheras the blind inner demon has awakened"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCTriggerContext 改名 RaidSscTriggerContext
RaidSscTriggerContext::leotheras_the_blind_inner_demon_has_awakened;

        creators["leotheras the blind entered final phase"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCTriggerContext 改名 RaidSscTriggerContext
RaidSscTriggerContext::leotheras_the_blind_entered_final_phase;

        creators["leotheras the blind demon form tank needs aggro"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCTriggerContext 改名 RaidSscTriggerContext
RaidSscTriggerContext::leotheras_the_blind_demon_form_tank_needs_aggro;

        creators["leotheras the blind boss wipes aggro upon phase change"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCTriggerContext 改名 RaidSscTriggerContext
RaidSscTriggerContext::leotheras_the_blind_boss_wipes_aggro_upon_phase_change;

        // Fathom-Lord Karathress
        creators["fathom-lord karathress boss engaged by main tank"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCTriggerContext 改名 RaidSscTriggerContext
RaidSscTriggerContext::fathom_lord_karathress_boss_engaged_by_main_tank;

        creators["fathom-lord karathress caribdis engaged by first assist tank"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCTriggerContext 改名 RaidSscTriggerContext
RaidSscTriggerContext::fathom_lord_karathress_caribdis_engaged_by_first_assist_tank;

        creators["fathom-lord karathress sharkkis engaged by second assist tank"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCTriggerContext 改名 RaidSscTriggerContext
RaidSscTriggerContext::fathom_lord_karathress_sharkkis_engaged_by_second_assist_tank;

        creators["fathom-lord karathress tidalvess engaged by third assist tank"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCTriggerContext 改名 RaidSscTriggerContext
RaidSscTriggerContext::fathom_lord_karathress_tidalvess_engaged_by_third_assist_tank;

        creators["fathom-lord karathress caribdis tank needs dedicated healer"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCTriggerContext 改名 RaidSscTriggerContext
RaidSscTriggerContext::fathom_lord_karathress_caribdis_tank_needs_dedicated_healer;

        creators["fathom-lord karathress pulling bosses"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCTriggerContext 改名 RaidSscTriggerContext
RaidSscTriggerContext::fathom_lord_karathress_pulling_bosses;

        creators["fathom-lord karathress determining kill order"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCTriggerContext 改名 RaidSscTriggerContext
RaidSscTriggerContext::fathom_lord_karathress_determining_kill_order;

        creators["fathom-lord karathress tanks need to establish aggro"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCTriggerContext 改名 RaidSscTriggerContext
RaidSscTriggerContext::fathom_lord_karathress_tanks_need_to_establish_aggro;

        // Morogrim Tidewalker
        creators["morogrim tidewalker boss engaged by main tank"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCTriggerContext 改名 RaidSscTriggerContext
RaidSscTriggerContext::morogrim_tidewalker_boss_engaged_by_main_tank;

        creators["morogrim tidewalker pulling boss"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCTriggerContext 改名 RaidSscTriggerContext
RaidSscTriggerContext::morogrim_tidewalker_pulling_boss;

        creators["morogrim tidewalker water globules are incoming"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCTriggerContext 改名 RaidSscTriggerContext
RaidSscTriggerContext::morogrim_tidewalker_water_globules_are_incoming;

        // Lady Vashj <Coilfang Matron>
        creators["lady vashj boss engaged by main tank"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCTriggerContext 改名 RaidSscTriggerContext
RaidSscTriggerContext::lady_vashj_boss_engaged_by_main_tank;

        creators["lady vashj boss engaged by ranged in phase 1"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCTriggerContext 改名 RaidSscTriggerContext
RaidSscTriggerContext::lady_vashj_boss_engaged_by_ranged_in_phase_1;

        creators["lady vashj casts shock blast on highest aggro"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCTriggerContext 改名 RaidSscTriggerContext
RaidSscTriggerContext::lady_vashj_casts_shock_blast_on_highest_aggro;

        creators["lady vashj bot has static charge"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCTriggerContext 改名 RaidSscTriggerContext
RaidSscTriggerContext::lady_vashj_bot_has_static_charge;

        creators["lady vashj pulling boss in phase 1 and phase 3"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCTriggerContext 改名 RaidSscTriggerContext
RaidSscTriggerContext::lady_vashj_pulling_boss_in_phase_1_and_phase_3;

        creators["lady vashj adds spawn in phase 2 and phase 3"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCTriggerContext 改名 RaidSscTriggerContext
RaidSscTriggerContext::lady_vashj_adds_spawn_in_phase_2_and_phase_3;

        creators["lady vashj coilfang strider is approaching"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCTriggerContext 改名 RaidSscTriggerContext
RaidSscTriggerContext::lady_vashj_coilfang_strider_is_approaching;

        creators["lady vashj tainted elemental cheat"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCTriggerContext 改名 RaidSscTriggerContext
RaidSscTriggerContext::lady_vashj_tainted_elemental_cheat;

        creators["lady vashj tainted core was looted"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCTriggerContext 改名 RaidSscTriggerContext
RaidSscTriggerContext::lady_vashj_tainted_core_was_looted;

        creators["lady vashj toxic sporebats are spewing poison clouds"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCTriggerContext 改名 RaidSscTriggerContext
RaidSscTriggerContext::lady_vashj_toxic_sporebats_are_spewing_poison_clouds;

        creators["lady vashj bot is entangled in toxic spores or static charge"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCTriggerContext 改名 RaidSscTriggerContext
RaidSscTriggerContext::lady_vashj_bot_is_entangled_in_toxic_spores_or_static_charge;
    }

private:
    // General
    static Trigger* serpent_shrine_cavern_no_encounter_in_progress(PlayerbotAI* botAI) {
        return new SerpentShrineCavernNoEncounterInProgressTrigger(botAI);
    }

    // Trash
    static Trigger* underbog_colossus_spawned_toxic_pool_after_death(PlayerbotAI* botAI) {
        return new UnderbogColossusSpawnedToxicPoolAfterDeathTrigger(botAI);
    }
    static Trigger* greyheart_tidecaller_water_elemental_totem_spawned(PlayerbotAI* botAI) {
        return new GreyheartTidecallerWaterElementalTotemSpawnedTrigger(botAI);
    }

    // Hydross the Unstable <Duke of Currents>
    static Trigger* hydross_the_unstable_bot_is_frost_tank(PlayerbotAI* botAI) {
        return new HydrossTheUnstableBotIsFrostTankTrigger(botAI);
    }
    static Trigger* hydross_the_unstable_bot_is_nature_tank(PlayerbotAI* botAI) {
        return new HydrossTheUnstableBotIsNatureTankTrigger(botAI);
    }
    static Trigger* hydross_the_unstable_elementals_spawned(PlayerbotAI* botAI) {
        return new HydrossTheUnstableElementalsSpawnedTrigger(botAI);
    }
    static Trigger* hydross_the_unstable_danger_from_water_tombs(PlayerbotAI* botAI) {
        return new HydrossTheUnstableDangerFromWaterTombsTrigger(botAI);
    }
    static Trigger* hydross_the_unstable_tank_needs_aggro_upon_phase_change(PlayerbotAI* botAI) {
        return new HydrossTheUnstableTankNeedsAggroUponPhaseChangeTrigger(botAI);
    }
    static Trigger* hydross_the_unstable_aggro_resets_upon_phase_change(PlayerbotAI* botAI) {
        return new HydrossTheUnstableAggroResetsUponPhaseChangeTrigger(botAI);
    }
    static Trigger* hydross_the_unstable_need_to_manage_timers(PlayerbotAI* botAI) {
        return new HydrossTheUnstableNeedToManageTimersTrigger(botAI);
    }

    // The Lurker Below
    static Trigger* the_lurker_below_spout_is_active(PlayerbotAI* botAI) {
        return new TheLurkerBelowSpoutIsActiveTrigger(botAI);
    }
    static Trigger* the_lurker_below_boss_is_active_for_main_tank(PlayerbotAI* botAI) {
        return new TheLurkerBelowBossIsActiveForMainTankTrigger(botAI);
    }
    static Trigger* the_lurker_below_boss_casts_geyser(PlayerbotAI* botAI) {
        return new TheLurkerBelowBossCastsGeyserTrigger(botAI);
    }
    static Trigger* the_lurker_below_boss_is_submerged(PlayerbotAI* botAI) {
        return new TheLurkerBelowBossIsSubmergedTrigger(botAI);
    }
    static Trigger* the_lurker_below_need_to_prepare_timer_for_spout(PlayerbotAI* botAI) {
        return new TheLurkerBelowNeedToPrepareTimerForSpoutTrigger(botAI);
    }

    // Leotheras the Blind
    //By leewheel 2026-09-04: 上游——删除 leotheras_the_blind_boss_is_inactive 工厂(触发器随上游一并移除)
    static Trigger* leotheras_the_blind_boss_transformed_into_demon_form(PlayerbotAI* botAI) {
        return new LeotherasTheBlindBossTransformedIntoDemonFormTrigger(botAI);
    }
    static Trigger* leotheras_the_blind_only_warlock_should_tank_demon_form(PlayerbotAI* botAI) {
        return new LeotherasTheBlindOnlyWarlockShouldTankDemonFormTrigger(botAI);
    }
    static Trigger* leotheras_the_blind_boss_engaged_by_ranged(PlayerbotAI* botAI) {
        return new LeotherasTheBlindBossEngagedByRangedTrigger(botAI);
    }
    static Trigger* leotheras_the_blind_boss_channeling_whirlwind(PlayerbotAI* botAI) {
        return new LeotherasTheBlindBossChannelingWhirlwindTrigger(botAI);
    }
    static Trigger* leotheras_the_blind_bot_has_too_many_chaos_blast_stacks(PlayerbotAI* botAI) {
        return new LeotherasTheBlindBotHasTooManyChaosBlastStacksTrigger(botAI);
    }
    static Trigger* leotheras_the_blind_inner_demon_has_awakened(PlayerbotAI* botAI) {
        return new LeotherasTheBlindInnerDemonHasAwakenedTrigger(botAI);
    }
    static Trigger* leotheras_the_blind_entered_final_phase(PlayerbotAI* botAI) {
        return new LeotherasTheBlindEnteredFinalPhaseTrigger(botAI);
    }
    static Trigger* leotheras_the_blind_demon_form_tank_needs_aggro(PlayerbotAI* botAI) {
        return new LeotherasTheBlindDemonFormTankNeedsAggro(botAI);
    }
    static Trigger* leotheras_the_blind_boss_wipes_aggro_upon_phase_change(PlayerbotAI* botAI) {
        return new LeotherasTheBlindBossWipesAggroUponPhaseChangeTrigger(botAI);
    }

    // Fathom-Lord Karathress
    static Trigger* fathom_lord_karathress_boss_engaged_by_main_tank(PlayerbotAI* botAI) {
        return new FathomLordKarathressBossEngagedByMainTankTrigger(botAI);
    }
    static Trigger* fathom_lord_karathress_caribdis_engaged_by_first_assist_tank(PlayerbotAI* botAI) {
        return new FathomLordKarathressCaribdisEngagedByFirstAssistTankTrigger(botAI);
    }
    static Trigger* fathom_lord_karathress_sharkkis_engaged_by_second_assist_tank(PlayerbotAI* botAI) {
        return new FathomLordKarathressSharkkisEngagedBySecondAssistTankTrigger(botAI);
    }
    static Trigger* fathom_lord_karathress_tidalvess_engaged_by_third_assist_tank(PlayerbotAI* botAI) {
        return new FathomLordKarathressTidalvessEngagedByThirdAssistTankTrigger(botAI);
    }
    static Trigger* fathom_lord_karathress_caribdis_tank_needs_dedicated_healer(PlayerbotAI* botAI) {
        return new FathomLordKarathressCaribdisTankNeedsDedicatedHealerTrigger(botAI);
    }
    static Trigger* fathom_lord_karathress_pulling_bosses(PlayerbotAI* botAI) {
        return new FathomLordKarathressPullingBossesTrigger(botAI);
    }
    static Trigger* fathom_lord_karathress_determining_kill_order(PlayerbotAI* botAI) {
        return new FathomLordKarathressDeterminingKillOrderTrigger(botAI);
    }
    static Trigger* fathom_lord_karathress_tanks_need_to_establish_aggro(PlayerbotAI* botAI) {
        return new FathomLordKarathressTanksNeedToEstablishAggroTrigger(botAI);
    }

    // Morogrim Tidewalker
    static Trigger* morogrim_tidewalker_boss_engaged_by_main_tank(PlayerbotAI* botAI) {
        return new MorogrimTidewalkerBossEngagedByMainTankTrigger(botAI);
    }
    static Trigger* morogrim_tidewalker_pulling_boss(PlayerbotAI* botAI) {
        return new MorogrimTidewalkerPullingBossTrigger(botAI);
    }
    static Trigger* morogrim_tidewalker_water_globules_are_incoming(PlayerbotAI* botAI) {
        return new MorogrimTidewalkerWaterGlobulesAreIncomingTrigger(botAI);
    }

    // Lady Vashj <Coilfang Matron>
    static Trigger* lady_vashj_boss_engaged_by_main_tank(PlayerbotAI* botAI) {
        return new LadyVashjBossEngagedByMainTankTrigger(botAI);
    }
    static Trigger* lady_vashj_boss_engaged_by_ranged_in_phase_1(PlayerbotAI* botAI) {
        return new LadyVashjBossEngagedByRangedInPhase1Trigger(botAI);
    }
    static Trigger* lady_vashj_casts_shock_blast_on_highest_aggro(PlayerbotAI* botAI) {
        return new LadyVashjCastsShockBlastOnHighestAggroTrigger(botAI);
    }
    static Trigger* lady_vashj_bot_has_static_charge(PlayerbotAI* botAI) {
        return new LadyVashjBotHasStaticChargeTrigger(botAI);
    }
    static Trigger* lady_vashj_pulling_boss_in_phase_1_and_phase_3(PlayerbotAI* botAI) {
        return new LadyVashjPullingBossInPhase1AndPhase3Trigger(botAI);
    }
    static Trigger* lady_vashj_adds_spawn_in_phase_2_and_phase_3(PlayerbotAI* botAI) {
        return new LadyVashjAddsSpawnInPhase2AndPhase3Trigger(botAI);
    }
    static Trigger* lady_vashj_coilfang_strider_is_approaching(PlayerbotAI* botAI) {
        return new LadyVashjCoilfangStriderIsApproachingTrigger(botAI);
    }
    static Trigger* lady_vashj_tainted_elemental_cheat(PlayerbotAI* botAI) {
        return new LadyVashjTaintedElementalCheatTrigger(botAI);
    }
    static Trigger* lady_vashj_tainted_core_was_looted(PlayerbotAI* botAI) {
        return new LadyVashjTaintedCoreWasLootedTrigger(botAI);
    }
    static Trigger* lady_vashj_toxic_sporebats_are_spewing_poison_clouds(PlayerbotAI* botAI) {
        return new LadyVashjToxicSporebatsAreSpewingPoisonCloudsTrigger(botAI);
    }
    static Trigger* lady_vashj_bot_is_entangled_in_toxic_spores_or_static_charge(PlayerbotAI* botAI) {
        return new LadyVashjBotIsEntangledInToxicSporesOrStaticChargeTrigger(botAI);
    }
};

#endif
