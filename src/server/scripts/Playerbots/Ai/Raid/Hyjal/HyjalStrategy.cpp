/* 海加尔山 机器人策略 */
/*By leewheel 2026-08-18: 对齐 brighton-chi the-lab HEAD(e92a52db)——
  移植 7f293f95/33f67223(触发/动作命名)、ab50f6a1(新增 ArchimondeControlDoomfireAvoidanceMultiplier)。*/
#include "HyjalStrategy.h"
#include "AiObjectContext.h"
#include "HyjalHelpers.h"
#include "HyjalMultipliers.h"
#include "Playerbots.h"

using namespace HyjalHelpers;

//By leewheel 2026-09-04: 上游——RaidHyjalSummitStrategy 改名 RaidHyjalStrategy
void RaidHyjalStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
//End By leewheel
{
    // General
    triggers.push_back(new TriggerNode("hyjal summit no encounter in progress", {
        NextAction("hyjal summit reset encounter states", ACTION_EMERGENCY + 10) }));

    // Rage Winterchill
    triggers.push_back(new TriggerNode("rage winterchill pulling boss", {
        NextAction("rage winterchill misdirect boss to main tank", ACTION_RAID + 1) }));

    triggers.push_back(new TriggerNode("rage winterchill should be tanked", {
        NextAction("rage winterchill main tank position boss", ACTION_RAID) }));

    triggers.push_back(new TriggerNode("rage winterchill ranged should spread", {
        NextAction("rage winterchill spread ranged in circle", ACTION_RAID) }));

    triggers.push_back(new TriggerNode("rage winterchill melee near death and decay", {
        NextAction(
            "rage winterchill melee maneuver through death and decay", ACTION_EMERGENCY + 1) }));

    triggers.push_back(new TriggerNode("rage winterchill ranged in death and decay", {
        NextAction("rage winterchill ranged get out of death and decay", ACTION_EMERGENCY + 1) }));

    // Anetheron
    triggers.push_back(new TriggerNode("anetheron pulling boss or infernal", {
        NextAction("anetheron misdirect boss and infernals to tanks", ACTION_RAID + 2) }));

    triggers.push_back(new TriggerNode("anetheron should be tanked", {
        NextAction("anetheron main tank position boss", ACTION_RAID) }));

    triggers.push_back(new TriggerNode("anetheron ranged should spread", {
        NextAction("anetheron spread ranged in circle", ACTION_RAID + 1) }));

    triggers.push_back(new TriggerNode("anetheron bot is targeted by infernal", {
        NextAction("anetheron bring infernal to infernal tank", ACTION_EMERGENCY + 7) }));

    triggers.push_back(new TriggerNode("anetheron bot is near inferno target", {
        NextAction("anetheron move away from inferno target", ACTION_EMERGENCY + 6) }));

    triggers.push_back(new TriggerNode("anetheron infernals pulse immolation", {
        NextAction("anetheron get out of immolation", ACTION_EMERGENCY + 1) }));

    triggers.push_back(new TriggerNode("anetheron infernals should be tanked away", {
        NextAction("anetheron infernal tank take position", ACTION_EMERGENCY + 1) }));

    triggers.push_back(new TriggerNode("anetheron should divide dps", {
        NextAction("anetheron assign dps priority", ACTION_RAID) }));

    // Kaz'rogal
    triggers.push_back(new TriggerNode("kaz'rogal pulling boss", {
        NextAction("kaz'rogal misdirect boss to main tank", ACTION_RAID + 1) }));

    triggers.push_back(new TriggerNode("kaz'rogal should be tanked", {
        NextAction("kaz'rogal main tank position boss", ACTION_RAID) }));

    triggers.push_back(new TriggerNode("kaz'rogal can split malevolent cleave damage", {
        NextAction("kaz'rogal assist tanks move in front", ACTION_RAID) }));

    triggers.push_back(new TriggerNode("kaz'rogal ranged should avoid war stomp", {
        NextAction("kaz'rogal spread ranged in arc", ACTION_RAID) }));

    triggers.push_back(new TriggerNode("kaz'rogal bot is low on mana", {
        NextAction("kaz'rogal move away from group", ACTION_EMERGENCY + 2) }));

    triggers.push_back(new TriggerNode("kaz'rogal hunter should preserve mana", {
        NextAction("kaz'rogal activate aspect of the viper", ACTION_EMERGENCY + 6) }));

    triggers.push_back(new TriggerNode("kaz'rogal mark on mage or paladin", {
        NextAction("kaz'rogal cancel mark", ACTION_EMERGENCY + 6) }));

    triggers.push_back(new TriggerNode("kaz'rogal immunity no longer needed", {
        NextAction("kaz'rogal cancel immunity", ACTION_RAID + 1) }));

    triggers.push_back(new TriggerNode("kaz'rogal warlock should manage mana", {
        NextAction("kaz'rogal warlock manage mana", ACTION_EMERGENCY + 6) }));

    // Azgalor
    triggers.push_back(new TriggerNode("azgalor pulling boss", {
        NextAction("azgalor misdirect boss to main tank", ACTION_RAID + 2) }));

    triggers.push_back(new TriggerNode("azgalor should be tanked", {
        NextAction("azgalor main tank position boss", ACTION_RAID) }));

    triggers.push_back(new TriggerNode("azgalor ranged should spread", {
        NextAction("azgalor disperse ranged", ACTION_RAID + 1) }));

    triggers.push_back(new TriggerNode("azgalor melee near rain of fire", {
        NextAction("azgalor melee maneuver through fire", ACTION_EMERGENCY + 1) }));

    triggers.push_back(new TriggerNode("azgalor ranged in rain of fire", {
        NextAction("azgalor ranged get out of rain of fire", ACTION_EMERGENCY + 1) }));

    triggers.push_back(new TriggerNode("azgalor bot is doomed", {
        NextAction("azgalor move to doomguard tank", ACTION_EMERGENCY + 2) }));

    triggers.push_back(new TriggerNode("azgalor should control doomguards", {
        NextAction("azgalor first assist tank position doomguard", ACTION_RAID) }));

    triggers.push_back(new TriggerNode("azgalor should divide dps", {
        NextAction("azgalor determine dps priority", ACTION_RAID) }));

    // Archimonde
    triggers.push_back(new TriggerNode("archimonde pulling boss", {
        NextAction("archimonde misdirect boss to main tank", ACTION_RAID + 1) }));

    triggers.push_back(new TriggerNode("archimonde should be tanked", {
        NextAction("archimonde move boss to initial position", ACTION_RAID) }));

    triggers.push_back(new TriggerNode("archimonde boss casts fear", {
        //By leewheel 2026-09-04: 对齐上游b8304144——动作改名，只对萨满放战栗图腾(牧师fear ward走通用策略)
        NextAction("archimonde set tremor totem", ACTION_RAID + 1) }));

    triggers.push_back(new TriggerNode("archimonde boss casting air burst", {
        NextAction("archimonde keep air burst away from tank", ACTION_EMERGENCY + 8) }));

    triggers.push_back(new TriggerNode("archimonde ranged should spread", {
        NextAction("archimonde spread ranged", ACTION_RAID) }));

    triggers.push_back(new TriggerNode("archimonde bot is near doomfire", {
        NextAction("archimonde avoid doomfire", ACTION_EMERGENCY + 6) }));

    triggers.push_back(new TriggerNode("archimonde bot stood in doomfire", {
        NextAction("archimonde remove doomfire dot", ACTION_EMERGENCY + 7) }));
}

//By leewheel 2026-09-04: 上游——RaidHyjalSummitStrategy 改名 RaidHyjalStrategy
void RaidHyjalStrategy::InitMultipliers(std::vector<Multiplier*>& multipliers)
//End By leewheel
{
    // Trash
    multipliers.push_back(new HyjalSummitDelayDpsCooldownsMultiplier(botAI));

    // Rage Winterchill
    multipliers.push_back(new RageWinterchillDisableCombatFormationMoveMultiplier(botAI));
    multipliers.push_back(new RageWinterchillMeleeControlAvoidanceMultiplier(botAI));
    multipliers.push_back(new RageWinterchillRangedControlAvoidanceMultiplier(botAI));

    // Anetheron
    multipliers.push_back(new AnetheronDisableAssistTargetingMultiplier(botAI));
    multipliers.push_back(new AnetheronAvoidAccidentalInfernalAggroMultiplier(botAI));
    multipliers.push_back(new AnetheronInfernalTargetRunToPositionMultiplier(botAI));
    multipliers.push_back(new AnetheronControlMovementMultiplier(botAI));
    multipliers.push_back(new AnetheronControlMisdirectionMultiplier(botAI));

    // Kaz'rogal
    multipliers.push_back(new KazrogalDisableDisperseAndTankFaceMultiplier(botAI));
    multipliers.push_back(new KazrogalControlLowManaMovementMultiplier(botAI));
    multipliers.push_back(new KazrogalKeepAspectOfTheViperActiveMultiplier(botAI));

    // Azgalor
    multipliers.push_back(new AzgalorDisableAutoTargetingAndPositioningMultiplier(botAI));
    multipliers.push_back(new AzgalorDoomedBotPrioritizePositioningMultiplier(botAI));
    multipliers.push_back(new AzgalorMeleeDpsControlAvoidanceMultiplier(botAI));
    multipliers.push_back(new AzgalorRangedControlAvoidanceMultiplier(botAI));

    // Archimonde
    multipliers.push_back(new ArchimondeDisableCombatFormationMoveMultiplier(botAI));
    multipliers.push_back(new ArchimondeControlDoomfireAvoidanceMultiplier(botAI));
    multipliers.push_back(new ArchimondeSetTremorTotemMultiplier(botAI));
}

//By leewheel 2026-08-21: 移植上游——移除目标排除机制 AppendTargetExclusions。
// 上游改为让地狱火坦走通用坦克辅助接手地狱火, 配合 AnetheronAvoidAccidentalInfernalAggroMultiplier
// (按距离过滤)防止其它坦克AOE误拉; 不再按"是否地狱火坦"在目标值里排除安纳塞隆/地狱火
