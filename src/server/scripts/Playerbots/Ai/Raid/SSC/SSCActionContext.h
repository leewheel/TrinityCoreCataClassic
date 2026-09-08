/* 毒蛇神殿 机器人策略 */
#ifndef PLAYERBOTS_SSCACTIONCONTEXT_H
#define PLAYERBOTS_SSCACTIONCONTEXT_H

#include "NamedObjectContext.h"
#include "SSCActions.h"

class //By leewheel 2026-09-04: 上游——RaidSSCActionContext 改名 RaidSscActionContext
RaidSscActionContext : public NamedObjectContext<Action>
{
public:
    //By leewheel 2026-09-04: 上游——RaidSSCActionContext 改名 RaidSscActionContext
RaidSscActionContext()
    {
        // General
        creators["serpent shrine cavern reset encounter states"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCActionContext 改名 RaidSscActionContext
RaidSscActionContext::serpent_shrine_cavern_reset_encounter_states;

        // Trash
        creators["underbog colossus escape toxic pool"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCActionContext 改名 RaidSscActionContext
RaidSscActionContext::underbog_colossus_escape_toxic_pool;

        creators["greyheart tidecaller mark water elemental totem"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCActionContext 改名 RaidSscActionContext
RaidSscActionContext::greyheart_tidecaller_mark_water_elemental_totem;

        // Hydross the Unstable <Duke of Currents>
        creators["hydross the unstable position frost tank"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCActionContext 改名 RaidSscActionContext
RaidSscActionContext::hydross_the_unstable_position_frost_tank;

        creators["hydross the unstable position nature tank"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCActionContext 改名 RaidSscActionContext
RaidSscActionContext::hydross_the_unstable_position_nature_tank;

        creators["hydross the unstable prioritize elemental adds"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCActionContext 改名 RaidSscActionContext
RaidSscActionContext::hydross_the_unstable_prioritize_elemental_adds;

        creators["hydross the unstable frost phase spread out"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCActionContext 改名 RaidSscActionContext
RaidSscActionContext::hydross_the_unstable_frost_phase_spread_out;

        creators["hydross the unstable misdirect boss to tank"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCActionContext 改名 RaidSscActionContext
RaidSscActionContext::hydross_the_unstable_misdirect_boss_to_tank;

        creators["hydross the unstable stop dps upon phase change"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCActionContext 改名 RaidSscActionContext
RaidSscActionContext::hydross_the_unstable_stop_dps_upon_phase_change;

        creators["hydross the unstable manage timers"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCActionContext 改名 RaidSscActionContext
RaidSscActionContext::hydross_the_unstable_manage_timers;

        // The Lurker Below
        creators["the lurker below run around behind boss"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCActionContext 改名 RaidSscActionContext
RaidSscActionContext::the_lurker_below_run_around_behind_boss;

        creators["the lurker below position main tank"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCActionContext 改名 RaidSscActionContext
RaidSscActionContext::the_lurker_below_position_main_tank;

        creators["the lurker below spread ranged in arc"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCActionContext 改名 RaidSscActionContext
RaidSscActionContext::the_lurker_below_spread_ranged_in_arc;

        creators["the lurker below tanks pick up adds"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCActionContext 改名 RaidSscActionContext
RaidSscActionContext::the_lurker_below_tanks_pick_up_adds;

        creators["the lurker below manage spout timer"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCActionContext 改名 RaidSscActionContext
RaidSscActionContext::the_lurker_below_manage_spout_timer;

        // Leotheras the Blind
        //By leewheel 2026-09-04: 上游——删除 leotheras the blind target spellbinders(动作随上游一并移除)
        creators["leotheras the blind demon form tank attack boss"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCActionContext 改名 RaidSscActionContext
RaidSscActionContext::leotheras_the_blind_demon_form_tank_attack_boss;

        creators["leotheras the blind melee tanks don't attack demon form"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCActionContext 改名 RaidSscActionContext
RaidSscActionContext::leotheras_the_blind_melee_tanks_dont_attack_demon_form;

        creators["leotheras the blind position ranged"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCActionContext 改名 RaidSscActionContext
RaidSscActionContext::leotheras_the_blind_position_ranged;

        creators["leotheras the blind run away from whirlwind"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCActionContext 改名 RaidSscActionContext
RaidSscActionContext::leotheras_the_blind_run_away_from_whirlwind;

        creators["leotheras the blind melee dps run away from boss"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCActionContext 改名 RaidSscActionContext
RaidSscActionContext::leotheras_the_blind_melee_dps_run_away_from_boss;

        creators["leotheras the blind destroy inner demon"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCActionContext 改名 RaidSscActionContext
RaidSscActionContext::leotheras_the_blind_destroy_inner_demon;

        creators["leotheras the blind final phase assign dps priority"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCActionContext 改名 RaidSscActionContext
RaidSscActionContext::leotheras_the_blind_final_phase_assign_dps_priority;

        creators["leotheras the blind misdirect boss to demon form tank"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCActionContext 改名 RaidSscActionContext
RaidSscActionContext::leotheras_the_blind_misdirect_boss_to_demon_form_tank;

        creators["leotheras the blind manage dps wait timers"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCActionContext 改名 RaidSscActionContext
RaidSscActionContext::leotheras_the_blind_manage_dps_wait_timers;

        // Fathom-Lord Karathress
        creators["fathom-lord karathress main tank position boss"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCActionContext 改名 RaidSscActionContext
RaidSscActionContext::fathom_lord_karathress_main_tank_position_boss;

        creators["fathom-lord karathress first assist tank position caribdis"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCActionContext 改名 RaidSscActionContext
RaidSscActionContext::fathom_lord_karathress_first_assist_tank_position_caribdis;

        creators["fathom-lord karathress second assist tank position sharkkis"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCActionContext 改名 RaidSscActionContext
RaidSscActionContext::fathom_lord_karathress_second_assist_tank_position_sharkkis;

        creators["fathom-lord karathress third assist tank position tidalvess"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCActionContext 改名 RaidSscActionContext
RaidSscActionContext::fathom_lord_karathress_third_assist_tank_position_tidalvess;

        creators["fathom-lord karathress position caribdis tank healer"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCActionContext 改名 RaidSscActionContext
RaidSscActionContext::fathom_lord_karathress_position_caribdis_tank_healer;

        creators["fathom-lord karathress misdirect bosses to tanks"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCActionContext 改名 RaidSscActionContext
RaidSscActionContext::fathom_lord_karathress_misdirect_bosses_to_tanks;

        creators["fathom-lord karathress assign dps priority"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCActionContext 改名 RaidSscActionContext
RaidSscActionContext::fathom_lord_karathress_assign_dps_priority;

        creators["fathom-lord karathress manage dps timer"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCActionContext 改名 RaidSscActionContext
RaidSscActionContext::fathom_lord_karathress_manage_dps_timer;

        // Morogrim Tidewalker
        creators["morogrim tidewalker misdirect boss to main tank"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCActionContext 改名 RaidSscActionContext
RaidSscActionContext::morogrim_tidewalker_misdirect_boss_to_main_tank;

        creators["morogrim tidewalker move boss to tank position"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCActionContext 改名 RaidSscActionContext
RaidSscActionContext::morogrim_tidewalker_move_boss_to_tank_position;

        creators["morogrim tidewalker phase 2 reposition ranged"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCActionContext 改名 RaidSscActionContext
RaidSscActionContext::morogrim_tidewalker_phase_2_reposition_ranged;

        // Lady Vashj <Coilfang Matron>
        creators["lady vashj main tank position boss"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCActionContext 改名 RaidSscActionContext
RaidSscActionContext::lady_vashj_main_tank_position_boss;

        creators["lady vashj phase 1 spread ranged in arc"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCActionContext 改名 RaidSscActionContext
RaidSscActionContext::lady_vashj_phase_1_spread_ranged_in_arc;

        creators["lady vashj set grounding totem in main tank group"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCActionContext 改名 RaidSscActionContext
RaidSscActionContext::lady_vashj_set_grounding_totem_in_main_tank_group;

        creators["lady vashj static charge move away from group"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCActionContext 改名 RaidSscActionContext
RaidSscActionContext::lady_vashj_static_charge_move_away_from_group;

        creators["lady vashj misdirect boss to main tank"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCActionContext 改名 RaidSscActionContext
RaidSscActionContext::lady_vashj_misdirect_boss_to_main_tank;

        creators["lady vashj assign phase 2 and phase 3 dps priority"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCActionContext 改名 RaidSscActionContext
RaidSscActionContext::lady_vashj_assign_phase_2_and_phase_3_dps_priority;

        creators["lady vashj misdirect strider to first assist tank"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCActionContext 改名 RaidSscActionContext
RaidSscActionContext::lady_vashj_misdirect_strider_to_first_assist_tank;

        creators["lady vashj tank attack and move away strider"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCActionContext 改名 RaidSscActionContext
RaidSscActionContext::lady_vashj_tank_attack_and_move_away_strider;

        creators["lady vashj loot tainted core"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCActionContext 改名 RaidSscActionContext
RaidSscActionContext::lady_vashj_loot_tainted_core;

        creators["lady vashj teleport to tainted elemental"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCActionContext 改名 RaidSscActionContext
RaidSscActionContext::lady_vashj_teleport_to_tainted_elemental;

        creators["lady vashj pass the tainted core"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCActionContext 改名 RaidSscActionContext
RaidSscActionContext::lady_vashj_pass_the_tainted_core;

        creators["lady vashj avoid toxic spores"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCActionContext 改名 RaidSscActionContext
RaidSscActionContext::lady_vashj_avoid_toxic_spores;

        creators["lady vashj use free action abilities"] =
            &//By leewheel 2026-09-04: 上游——RaidSSCActionContext 改名 RaidSscActionContext
RaidSscActionContext::lady_vashj_use_free_action_abilities;
    }

private:
    // General
    static Action* serpent_shrine_cavern_reset_encounter_states(PlayerbotAI* botAI) {
        return new SerpentShrineCavernResetEncounterStatesAction(botAI);
    }

    // Trash
    static Action* underbog_colossus_escape_toxic_pool(PlayerbotAI* botAI) {
        return new UnderbogColossusEscapeToxicPoolAction(botAI);
    }
    static Action* greyheart_tidecaller_mark_water_elemental_totem(PlayerbotAI* botAI) {
        return new GreyheartTidecallerMarkWaterElementalTotemAction(botAI);
    }

    // Hydross the Unstable <Duke of Currents>
    static Action* hydross_the_unstable_position_frost_tank(PlayerbotAI* botAI) {
        return new HydrossTheUnstablePositionFrostTankAction(botAI);
    }
    static Action* hydross_the_unstable_position_nature_tank(PlayerbotAI* botAI) {
        return new HydrossTheUnstablePositionNatureTankAction(botAI);
    }
    static Action* hydross_the_unstable_prioritize_elemental_adds(PlayerbotAI* botAI) {
        return new HydrossTheUnstablePrioritizeElementalAddsAction(botAI);
    }
    static Action* hydross_the_unstable_frost_phase_spread_out(PlayerbotAI* botAI) {
        return new HydrossTheUnstableFrostPhaseSpreadOutAction(botAI);
    }
    static Action* hydross_the_unstable_misdirect_boss_to_tank(PlayerbotAI* botAI) {
        return new HydrossTheUnstableMisdirectBossToTankAction(botAI);
    }
    static Action* hydross_the_unstable_stop_dps_upon_phase_change(PlayerbotAI* botAI) {
        return new HydrossTheUnstableStopDpsUponPhaseChangeAction(botAI);
    }
    static Action* hydross_the_unstable_manage_timers(PlayerbotAI* botAI) {
        return new HydrossTheUnstableManageTimersAction(botAI);
    }

    // The Lurker Below
    static Action* the_lurker_below_run_around_behind_boss(PlayerbotAI* botAI) {
        return new TheLurkerBelowRunAroundBehindBossAction(botAI);
    }
    static Action* the_lurker_below_position_main_tank(PlayerbotAI* botAI) {
        return new TheLurkerBelowPositionMainTankAction(botAI);
    }
    static Action* the_lurker_below_spread_ranged_in_arc(PlayerbotAI* botAI) {
        return new TheLurkerBelowSpreadRangedInArcAction(botAI);
    }
    static Action* the_lurker_below_tanks_pick_up_adds(PlayerbotAI* botAI) {
        return new TheLurkerBelowTanksPickUpAddsAction(botAI);
    }
    static Action* the_lurker_below_manage_spout_timer(PlayerbotAI* botAI) {
        return new TheLurkerBelowManageSpoutTimerAction(botAI);
    }

    // Leotheras the Blind
    //By leewheel 2026-09-04: 上游——删除 leotheras_the_blind_target_spellbinders 工厂(动作随上游一并移除)
    static Action* leotheras_the_blind_demon_form_tank_attack_boss(PlayerbotAI* botAI) {
        return new LeotherasTheBlindDemonFormTankAttackBossAction(botAI);
    }
    static Action* leotheras_the_blind_melee_tanks_dont_attack_demon_form(PlayerbotAI* botAI) {
        return new LeotherasTheBlindMeleeTanksDontAttackDemonFormAction(botAI);
    }
    static Action* leotheras_the_blind_position_ranged(PlayerbotAI* botAI) {
        return new LeotherasTheBlindPositionRangedAction(botAI);
    }
    static Action* leotheras_the_blind_run_away_from_whirlwind(PlayerbotAI* botAI) {
        return new LeotherasTheBlindRunAwayFromWhirlwindAction(botAI);
    }
    static Action* leotheras_the_blind_melee_dps_run_away_from_boss(PlayerbotAI* botAI) {
        return new LeotherasTheBlindMeleeDpsRunAwayFromBossAction(botAI);
    }
    static Action* leotheras_the_blind_destroy_inner_demon(PlayerbotAI* botAI) {
        return new LeotherasTheBlindDestroyInnerDemonAction(botAI);
    }
    static Action* leotheras_the_blind_misdirect_boss_to_demon_form_tank(PlayerbotAI* botAI) {
        return new LeotherasTheBlindMisdirectBossToDemonFormTankAction(botAI);
    }
    static Action* leotheras_the_blind_final_phase_assign_dps_priority(PlayerbotAI* botAI) {
        return new LeotherasTheBlindFinalPhaseAssignDpsPriorityAction(botAI);
    }
    static Action* leotheras_the_blind_manage_dps_wait_timers(PlayerbotAI* botAI) {
        return new LeotherasTheBlindManageDpsWaitTimersAction(botAI);
    }

    // Fathom-Lord Karathress
    static Action* fathom_lord_karathress_main_tank_position_boss(PlayerbotAI* botAI) {
        return new FathomLordKarathressMainTankPositionBossAction(botAI);
    }
    static Action* fathom_lord_karathress_first_assist_tank_position_caribdis(PlayerbotAI* botAI) {
        return new FathomLordKarathressFirstAssistTankPositionCaribdisAction(botAI);
    }
    static Action* fathom_lord_karathress_second_assist_tank_position_sharkkis(PlayerbotAI* botAI) {
        return new FathomLordKarathressSecondAssistTankPositionSharkkisAction(botAI);
    }
    static Action* fathom_lord_karathress_third_assist_tank_position_tidalvess(PlayerbotAI* botAI) {
        return new FathomLordKarathressThirdAssistTankPositionTidalvessAction(botAI);
    }
    static Action* fathom_lord_karathress_position_caribdis_tank_healer(PlayerbotAI* botAI) {
        return new FathomLordKarathressPositionCaribdisTankHealerAction(botAI);
    }
    static Action* fathom_lord_karathress_misdirect_bosses_to_tanks(PlayerbotAI* botAI) {
        return new FathomLordKarathressMisdirectBossesToTanksAction(botAI);
    }
    static Action* fathom_lord_karathress_assign_dps_priority(PlayerbotAI* botAI) {
        return new FathomLordKarathressAssignDpsPriorityAction(botAI);
    }
    static Action* fathom_lord_karathress_manage_dps_timer(PlayerbotAI* botAI) {
        return new FathomLordKarathressManageDpsTimerAction(botAI);
    }

    // Morogrim Tidewalker
    static Action* morogrim_tidewalker_misdirect_boss_to_main_tank(PlayerbotAI* botAI) {
        return new MorogrimTidewalkerMisdirectBossToMainTankAction(botAI);
    }
    static Action* morogrim_tidewalker_move_boss_to_tank_position(PlayerbotAI* botAI) {
        return new MorogrimTidewalkerMoveBossToTankPositionAction(botAI);
    }
    static Action* morogrim_tidewalker_phase_2_reposition_ranged(PlayerbotAI* botAI) {
        return new MorogrimTidewalkerPhase2RepositionRangedAction(botAI);
    }

    // Lady Vashj <Coilfang Matron>
    static Action* lady_vashj_main_tank_position_boss(PlayerbotAI* botAI) {
        return new LadyVashjMainTankPositionBossAction(botAI);
    }
    static Action* lady_vashj_phase_1_spread_ranged_in_arc(PlayerbotAI* botAI) {
        return new LadyVashjPhase1SpreadRangedInArcAction(botAI);
    }
    static Action* lady_vashj_set_grounding_totem_in_main_tank_group(PlayerbotAI* botAI) {
        return new LadyVashjSetGroundingTotemInMainTankGroupAction(botAI);
    }
    static Action* lady_vashj_static_charge_move_away_from_group(PlayerbotAI* botAI) {
        return new LadyVashjStaticChargeMoveAwayFromGroupAction(botAI);
    }
    static Action* lady_vashj_misdirect_boss_to_main_tank(PlayerbotAI* botAI) {
        return new LadyVashjMisdirectBossToMainTankAction(botAI);
    }
    static Action* lady_vashj_assign_phase_2_and_phase_3_dps_priority(PlayerbotAI* botAI) {
        return new LadyVashjAssignPhase2AndPhase3DpsPriorityAction(botAI);
    }
    static Action* lady_vashj_misdirect_strider_to_first_assist_tank(PlayerbotAI* botAI) {
        return new LadyVashjMisdirectStriderToFirstAssistTankAction(botAI);
    }
    static Action* lady_vashj_tank_attack_and_move_away_strider(PlayerbotAI* botAI) {
        return new LadyVashjTankAttackAndMoveAwayStriderAction(botAI);
    }
    static Action* lady_vashj_teleport_to_tainted_elemental(PlayerbotAI* botAI) {
        return new LadyVashjTeleportToTaintedElementalAction(botAI);
    }
    static Action* lady_vashj_loot_tainted_core(PlayerbotAI* botAI) {
        return new LadyVashjLootTaintedCoreAction(botAI);
    }
    static Action* lady_vashj_pass_the_tainted_core(PlayerbotAI* botAI) {
        return new LadyVashjPassTheTaintedCoreAction(botAI);
    }
    static Action* lady_vashj_avoid_toxic_spores(PlayerbotAI* botAI) {
        return new LadyVashjAvoidToxicSporesAction(botAI);
    }
    static Action* lady_vashj_use_free_action_abilities(PlayerbotAI* botAI) {
        return new LadyVashjUseFreeActionAbilitiesAction(botAI);
    }
};

#endif
