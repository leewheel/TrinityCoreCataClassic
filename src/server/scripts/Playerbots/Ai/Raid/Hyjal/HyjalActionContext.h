/* 海加尔山 机器人策略 */
/*By leewheel 2026-08-18: 对齐 brighton-chi the-lab HEAD(e92a52db)——
  移植 fde72295/47abcff2(压缩): 误导/主坦站位统一走通用
  HyjalMisdirectBossToMainTankAction / HyjalMainTankPositionBossAction 注册。*/
#ifndef PLAYERBOTS_HYJALACTIONCONTEXT_H
#define PLAYERBOTS_HYJALACTIONCONTEXT_H

#include "HyjalActions.h"
#include "HyjalHelpers.h"
#include "NamedObjectContext.h"

class //By leewheel 2026-09-04: 上游——RaidHyjalSummitActionContext 改名 RaidHyjalActionContext
RaidHyjalActionContext : public NamedObjectContext<Action>
{
public:
    //By leewheel 2026-09-04: 上游——RaidHyjalSummitActionContext 改名 RaidHyjalActionContext
RaidHyjalActionContext()
    {
        // General
        creators["hyjal summit reset encounter states"] =
            &//By leewheel 2026-09-04: 上游——RaidHyjalSummitActionContext 改名 RaidHyjalActionContext
RaidHyjalActionContext::hyjal_summit_reset_encounter_states;

        // Rage Winterchill
        creators["rage winterchill misdirect boss to main tank"] =
            &//By leewheel 2026-09-04: 上游——RaidHyjalSummitActionContext 改名 RaidHyjalActionContext
RaidHyjalActionContext::rage_winterchill_misdirect_boss_to_main_tank;

        creators["rage winterchill main tank position boss"] =
            &//By leewheel 2026-09-04: 上游——RaidHyjalSummitActionContext 改名 RaidHyjalActionContext
RaidHyjalActionContext::rage_winterchill_main_tank_position_boss;

        creators["rage winterchill spread ranged in circle"] =
            &//By leewheel 2026-09-04: 上游——RaidHyjalSummitActionContext 改名 RaidHyjalActionContext
RaidHyjalActionContext::rage_winterchill_spread_ranged_in_circle;

        creators["rage winterchill ranged get out of death and decay"] =
            &//By leewheel 2026-09-04: 上游——RaidHyjalSummitActionContext 改名 RaidHyjalActionContext
RaidHyjalActionContext::rage_winterchill_ranged_get_out_of_death_and_decay;

        creators["rage winterchill melee maneuver through death and decay"] =
            &//By leewheel 2026-09-04: 上游——RaidHyjalSummitActionContext 改名 RaidHyjalActionContext
RaidHyjalActionContext::rage_winterchill_melee_maneuver_through_death_and_decay;

        // Anetheron
        creators["anetheron misdirect boss and infernals to tanks"] =
            &//By leewheel 2026-09-04: 上游——RaidHyjalSummitActionContext 改名 RaidHyjalActionContext
RaidHyjalActionContext::anetheron_misdirect_boss_and_infernals_to_tanks;

        creators["anetheron main tank position boss"] =
            &//By leewheel 2026-09-04: 上游——RaidHyjalSummitActionContext 改名 RaidHyjalActionContext
RaidHyjalActionContext::anetheron_main_tank_position_boss;

        creators["anetheron spread ranged in circle"] =
            &//By leewheel 2026-09-04: 上游——RaidHyjalSummitActionContext 改名 RaidHyjalActionContext
RaidHyjalActionContext::anetheron_spread_ranged_in_circle;

        creators["anetheron move away from inferno target"] =
            &//By leewheel 2026-09-04: 上游——RaidHyjalSummitActionContext 改名 RaidHyjalActionContext
RaidHyjalActionContext::anetheron_move_away_from_inferno_target;

        creators["anetheron bring infernal to infernal tank"] =
            &//By leewheel 2026-09-04: 上游——RaidHyjalSummitActionContext 改名 RaidHyjalActionContext
RaidHyjalActionContext::anetheron_bring_infernal_to_infernal_tank;

        creators["anetheron infernal tank take position"] =
            &//By leewheel 2026-09-04: 上游——RaidHyjalSummitActionContext 改名 RaidHyjalActionContext
RaidHyjalActionContext::anetheron_infernal_tank_take_position;

        creators["anetheron get out of immolation"] =
            &//By leewheel 2026-09-04: 上游——RaidHyjalSummitActionContext 改名 RaidHyjalActionContext
RaidHyjalActionContext::anetheron_get_out_of_immolation;
        creators["anetheron assign dps priority"] =
            &//By leewheel 2026-09-04: 上游——RaidHyjalSummitActionContext 改名 RaidHyjalActionContext
RaidHyjalActionContext::anetheron_assign_dps_priority;

        // Kaz'rogal
        creators["kaz'rogal misdirect boss to main tank"] =
            &//By leewheel 2026-09-04: 上游——RaidHyjalSummitActionContext 改名 RaidHyjalActionContext
RaidHyjalActionContext::kazrogal_misdirect_boss_to_main_tank;

        creators["kaz'rogal main tank position boss"] =
            &//By leewheel 2026-09-04: 上游——RaidHyjalSummitActionContext 改名 RaidHyjalActionContext
RaidHyjalActionContext::kazrogal_main_tank_position_boss;

        creators["kaz'rogal assist tanks move in front"] =
            &//By leewheel 2026-09-04: 上游——RaidHyjalSummitActionContext 改名 RaidHyjalActionContext
RaidHyjalActionContext::kazrogal_assist_tanks_move_in_front;

        creators["kaz'rogal spread ranged in arc"] =
            &//By leewheel 2026-09-04: 上游——RaidHyjalSummitActionContext 改名 RaidHyjalActionContext
RaidHyjalActionContext::kazrogal_spread_ranged_in_arc;

        creators["kaz'rogal move away from group"] =
            &//By leewheel 2026-09-04: 上游——RaidHyjalSummitActionContext 改名 RaidHyjalActionContext
RaidHyjalActionContext::kazrogal_move_away_from_group;

        creators["kaz'rogal activate aspect of the viper"] =
            &//By leewheel 2026-09-04: 上游——RaidHyjalSummitActionContext 改名 RaidHyjalActionContext
RaidHyjalActionContext::kazrogal_activate_aspect_of_the_viper;

        creators["kaz'rogal cancel mark"] =
            &//By leewheel 2026-09-04: 上游——RaidHyjalSummitActionContext 改名 RaidHyjalActionContext
RaidHyjalActionContext::kazrogal_cancel_mark;

        creators["kaz'rogal cancel immunity"] =
            &//By leewheel 2026-09-04: 上游——RaidHyjalSummitActionContext 改名 RaidHyjalActionContext
RaidHyjalActionContext::kazrogal_cancel_immunity;
        creators["kaz'rogal warlock manage mana"] =
            &//By leewheel 2026-09-04: 上游——RaidHyjalSummitActionContext 改名 RaidHyjalActionContext
RaidHyjalActionContext::kazrogal_warlock_manage_mana;

        // Azgalor
        creators["azgalor misdirect boss to main tank"] =
            &//By leewheel 2026-09-04: 上游——RaidHyjalSummitActionContext 改名 RaidHyjalActionContext
RaidHyjalActionContext::azgalor_misdirect_boss_to_main_tank;

        creators["azgalor main tank position boss"] =
            &//By leewheel 2026-09-04: 上游——RaidHyjalSummitActionContext 改名 RaidHyjalActionContext
RaidHyjalActionContext::azgalor_main_tank_position_boss;

        creators["azgalor disperse ranged"] =
            &//By leewheel 2026-09-04: 上游——RaidHyjalSummitActionContext 改名 RaidHyjalActionContext
RaidHyjalActionContext::azgalor_disperse_ranged;

        creators["azgalor melee maneuver through fire"] =
            &//By leewheel 2026-09-04: 上游——RaidHyjalSummitActionContext 改名 RaidHyjalActionContext
RaidHyjalActionContext::azgalor_melee_maneuver_through_fire;

        creators["azgalor ranged get out of rain of fire"] =
            &//By leewheel 2026-09-04: 上游——RaidHyjalSummitActionContext 改名 RaidHyjalActionContext
RaidHyjalActionContext::azgalor_ranged_get_out_of_rain_of_fire;

        creators["azgalor move to doomguard tank"] =
            &//By leewheel 2026-09-04: 上游——RaidHyjalSummitActionContext 改名 RaidHyjalActionContext
RaidHyjalActionContext::azgalor_move_to_doomguard_tank;

        creators["azgalor first assist tank position doomguard"] =
            &//By leewheel 2026-09-04: 上游——RaidHyjalSummitActionContext 改名 RaidHyjalActionContext
RaidHyjalActionContext::azgalor_first_assist_tank_position_doomguard;

        creators["azgalor determine dps priority"] =
            &//By leewheel 2026-09-04: 上游——RaidHyjalSummitActionContext 改名 RaidHyjalActionContext
RaidHyjalActionContext::azgalor_determine_dps_priority;

        // Archimonde
        creators["archimonde misdirect boss to main tank"] =
            &//By leewheel 2026-09-04: 上游——RaidHyjalSummitActionContext 改名 RaidHyjalActionContext
RaidHyjalActionContext::archimonde_misdirect_boss_to_main_tank;

        creators["archimonde move boss to initial position"] =
            &//By leewheel 2026-09-04: 上游——RaidHyjalSummitActionContext 改名 RaidHyjalActionContext
RaidHyjalActionContext::archimonde_move_boss_to_initial_position;

        //By leewheel 2026-09-04: 对齐上游b8304144——牧师fear ward改由通用牧师策略处理，动作改为只放萨满战栗图腾
        creators["archimonde set tremor totem"] =
            &//By leewheel 2026-09-04: 上游——RaidHyjalSummitActionContext 改名 RaidHyjalActionContext
RaidHyjalActionContext::archimonde_set_tremor_totem;

        creators["archimonde keep air burst away from tank"] =
            &//By leewheel 2026-09-04: 上游——RaidHyjalSummitActionContext 改名 RaidHyjalActionContext
RaidHyjalActionContext::archimonde_keep_air_burst_away_from_tank;

        creators["archimonde spread ranged"] =
            &//By leewheel 2026-09-04: 上游——RaidHyjalSummitActionContext 改名 RaidHyjalActionContext
RaidHyjalActionContext::archimonde_spread_ranged;

        creators["archimonde avoid doomfire"] =
            &//By leewheel 2026-09-04: 上游——RaidHyjalSummitActionContext 改名 RaidHyjalActionContext
RaidHyjalActionContext::archimonde_avoid_doomfire;

        creators["archimonde remove doomfire dot"] =
            &//By leewheel 2026-09-04: 上游——RaidHyjalSummitActionContext 改名 RaidHyjalActionContext
RaidHyjalActionContext::archimonde_remove_doomfire_dot;
    }

private:
    // General
    static Action* hyjal_summit_reset_encounter_states(PlayerbotAI* botAI) {
        return new HyjalSummitResetEncounterStatesAction(botAI);
    }

    // Rage Winterchill
    static Action* rage_winterchill_misdirect_boss_to_main_tank(PlayerbotAI* botAI) {
        return new HyjalMisdirectBossToMainTankAction(
            botAI, "rage winterchill misdirect boss to main tank", "rage winterchill");
    }
    static Action* rage_winterchill_main_tank_position_boss(PlayerbotAI* botAI) {
        return new HyjalMainTankPositionBossAction(
            botAI, "rage winterchill main tank position boss", "rage winterchill",
            HyjalHelpers::WINTERCHILL_TANK_POSITION);
    }
    static Action* rage_winterchill_spread_ranged_in_circle(PlayerbotAI* botAI) {
        return new RageWinterchillSpreadRangedInCircleAction(botAI);
    }
    static Action* rage_winterchill_ranged_get_out_of_death_and_decay(PlayerbotAI* botAI) {
        return new RageWinterchillRangedGetOutOfDeathAndDecayAction(botAI);
    }
    static Action* rage_winterchill_melee_maneuver_through_death_and_decay(PlayerbotAI* botAI) {
        return new RageWinterchillMeleeManeuverThroughDeathAndDecayAction(botAI);
    }

    // Anetheron
    static Action* anetheron_misdirect_boss_and_infernals_to_tanks(PlayerbotAI* botAI) {
        return new AnetheronMisdirectBossAndInfernalsToTanksAction(botAI);
    }
    static Action* anetheron_main_tank_position_boss(PlayerbotAI* botAI) {
        return new HyjalMainTankPositionBossAction(
            botAI, "anetheron main tank position boss", "anetheron",
            HyjalHelpers::ANETHERON_TANK_POSITION);
    }
    static Action* anetheron_spread_ranged_in_circle(PlayerbotAI* botAI) {
        return new AnetheronSpreadRangedInCircleAction(botAI);
    }
    static Action* anetheron_move_away_from_inferno_target(PlayerbotAI* botAI) {
        return new AnetheronMoveAwayFromInfernoTargetAction(botAI);
    }
    static Action* anetheron_bring_infernal_to_infernal_tank(PlayerbotAI* botAI) {
        return new AnetheronBringInfernalToInfernalTankAction(botAI);
    }
    static Action* anetheron_infernal_tank_take_position(PlayerbotAI* botAI) {
        return new AnetheronInfernalTankTakePositionAction(botAI);
    }
    static Action* anetheron_get_out_of_immolation(PlayerbotAI* botAI) {
        return new AnetheronGetOutOfImmolationAction(botAI);
    }
    static Action* anetheron_assign_dps_priority(PlayerbotAI* botAI) {
        return new AnetheronAssignDpsPriorityAction(botAI);
    }

    // Kaz'rogal
    static Action* kazrogal_misdirect_boss_to_main_tank(PlayerbotAI* botAI) {
        return new HyjalMisdirectBossToMainTankAction(
            botAI, "kaz'rogal misdirect boss to main tank", "kaz'rogal");
    }
    static Action* kazrogal_main_tank_position_boss(PlayerbotAI* botAI) {
        return new HyjalMainTankPositionBossAction(
            botAI, "kaz'rogal main tank position boss", "kaz'rogal",
            HyjalHelpers::KAZROGAL_TANK_POSITION);
    }
    static Action* kazrogal_assist_tanks_move_in_front(PlayerbotAI* botAI) {
        return new KazrogalAssistTanksMoveInFrontAction(botAI);
    }
    static Action* kazrogal_spread_ranged_in_arc(PlayerbotAI* botAI) {
        return new KazrogalSpreadRangedInArcAction(botAI);
    }
    static Action* kazrogal_move_away_from_group(PlayerbotAI* botAI) {
        return new KazrogalMoveAwayFromGroupAction(botAI);
    }
    static Action* kazrogal_activate_aspect_of_the_viper(PlayerbotAI* botAI) {
        return new KazrogalActivateAspectOfTheViperAction(botAI);
    }
    static Action* kazrogal_cancel_mark(PlayerbotAI* botAI) {
        return new KazrogalCancelMarkAction(botAI);
    }
    static Action* kazrogal_cancel_immunity(PlayerbotAI* botAI) {
        return new KazrogalCancelImmunityAction(botAI);
    }
    static Action* kazrogal_warlock_manage_mana(PlayerbotAI* botAI) {
        return new KazrogalWarlockManageManaAction(botAI);
    }

    // Azgalor
    static Action* azgalor_misdirect_boss_to_main_tank(PlayerbotAI* botAI) {
        return new HyjalMisdirectBossToMainTankAction(
            botAI, "azgalor misdirect boss to main tank", "azgalor");
    }
    static Action* azgalor_main_tank_position_boss(PlayerbotAI* botAI) {
        return new HyjalMainTankPositionBossAction(
            botAI, "azgalor main tank position boss", "azgalor",
            HyjalHelpers::AZGALOR_TANK_POSITION, 60.0f);
    }
    static Action* azgalor_disperse_ranged(PlayerbotAI* botAI) {
        return new AzgalorDisperseRangedAction(botAI);
    }
    static Action* azgalor_melee_maneuver_through_fire(PlayerbotAI* botAI) {
        return new AzgalorMeleeManeuverThroughFireAction(botAI);
    }
    static Action* azgalor_ranged_get_out_of_rain_of_fire(PlayerbotAI* botAI) {
        return new AzgalorRangedGetOutOfRainOfFireAction(botAI);
    }
    static Action* azgalor_move_to_doomguard_tank(PlayerbotAI* botAI) {
        return new AzgalorMoveToDoomguardTankAction(botAI);
    }
    static Action* azgalor_first_assist_tank_position_doomguard(PlayerbotAI* botAI) {
        return new AzgalorFirstAssistTankPositionDoomguardAction(botAI);
    }
    static Action* azgalor_determine_dps_priority(PlayerbotAI* botAI) {
        return new AzgalorDetermineDpsPriorityAction(botAI);
    }

    // Archimonde
    static Action* archimonde_misdirect_boss_to_main_tank(PlayerbotAI* botAI) {
        return new HyjalMisdirectBossToMainTankAction(
            botAI, "archimonde misdirect boss to main tank", "archimonde");
    }
    static Action* archimonde_move_boss_to_initial_position(PlayerbotAI* botAI) {
        return new HyjalMainTankPositionBossAction(
            botAI, "archimonde move boss to initial position", "archimonde",
            HyjalHelpers::ARCHIMONDE_INITIAL_POSITION, 60.0f);
    }
    static Action* archimonde_set_tremor_totem(PlayerbotAI* botAI) {
        return new ArchimondeSetTremorTotemAction(botAI);
    }
    static Action* archimonde_keep_air_burst_away_from_tank(PlayerbotAI* botAI) {
        return new ArchimondeKeepAirBurstAwayFromTankAction(botAI);
    }
    static Action* archimonde_spread_ranged(PlayerbotAI* botAI) {
        return new ArchimondeSpreadRangedAction(botAI);
    }
    static Action* archimonde_avoid_doomfire(PlayerbotAI* botAI) {
        return new ArchimondeAvoidDoomfireAction(botAI);
    }
    static Action* archimonde_remove_doomfire_dot(PlayerbotAI* botAI) {
        return new ArchimondeRemoveDoomfireDotAction(botAI);
    }
};

#endif
