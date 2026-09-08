/* 太阳之井高地 机器人策略 */
#ifndef PLAYERBOTS_SWPENCOUNTERMURU_H
#define PLAYERBOTS_SWPENCOUNTERMURU_H

#include "ObjectGuid.h"
#include "Position.h"
#include "SWPSharedConstants.h"
#include <unordered_map>
#include <vector>

class Creature;
class Player;
class PlayerbotAI;
class Unit;

namespace SwpHelpers
{

struct MuruEncounterTargets
{
    Unit* muru = nullptr;
    Unit* entropius = nullptr;
    std::vector<Unit*> voidSentinels;
    std::vector<Unit*> voidSpawns;
    std::vector<Unit*> furyMages;
    std::vector<Unit*> berserkers;
};

// What the "muru encounter targets" value stores
struct MuruEncounterGuids
{
    ObjectGuid muru;
    ObjectGuid entropius;
    GuidVector voidSentinels;
    GuidVector voidSpawns;
    GuidVector furyMages;
    GuidVector berserkers;
};
//End By leewheel

struct MuruDarknessState
{
    uint32 startMs = 0;
    uint32 expireMs = 0;
};

inline Position const MURU_ENTRANCE_POSITION =             { 1840.567f, 605.769f, 71.250f };
inline Position const MURU_CENTER_POSITION =               { 1816.250f, 625.484f, 69.604f };
inline Position const MURU_STACK_POSITION =                { 1836.532f, 608.957f, 71.222f };
inline Position const MURU_VOID_SENTINEL_N_TANK_POSITION = { 1840.448f, 630.605f, 70.567f };
inline Position const MURU_VOID_SENTINEL_E_TANK_POSITION = { 1814.960f, 601.646f, 70.547f };

//By leewheel 2026-09-04: 上游1e110a5f——MURU_前缀触及常量上移到SWPSharedConstants.h全局常量族
//   (MELEE_ABILITY_REACH/HAMMER_OF_JUSTICE_REACH/RANGED_ABILITY_REACH/SELF_AOE_RACIAL_RADIUS/
//    WIND_SHEAR_REACH/SILENCING_SHOT_REACH), Berserker/Fury Mage 筛选按这些距离过滤候选
// For the "muru encounter targets" value. Only list membership is cached, not states read (like
// auras, casting, health).
inline constexpr uint32 MURU_ENCOUNTER_TARGETS_CACHE_INTERVAL_MS = 200;
// Feeds the "muru void zones" value.
inline constexpr uint32 VOID_ZONE_CACHE_INTERVAL_MS = 200;
// Feeds the "muru singularity" value. Only one exists at a time: Entropius casts Black Hole every
// 29s, and Singularities despawn after 18s.
inline constexpr uint32 SINGULARITY_CACHE_INTERVAL_MS = 200;
//End By leewheel

// Darkness cycle: 45998 ticks every 45s and triggers the 3s pre-effect 45999, whose own tick casts
// 45996, a 15y zone doing 3k a second. 45996 is also applied to M'uru itself (via a separate
// effect), so once it is applied, the Darkness window is read off that aura and these two are only
// estimates used before the aura is applied.
//By leewheel 2026-09-04: 上游1e110a5f——DARKNESS_* 常量统一加 MURU_ 前缀
inline constexpr uint32 MURU_DARKNESS_PRE_EFFECT_MS = 3000;
inline constexpr uint32 MURU_DARKNESS_AURA_MS = 20000;
// This is an arbitrary window to allow tanks a bit more time to get positioned after Darkness.
inline constexpr uint32 MURU_DARKNESS_EARLY_WINDOW_MS = 10000;

// By leewheel 2026-08-27: 对齐 the-lab cc4219a3/1e0caf61——移除 DARKNESS_RUN_BACK_ALLOWANCE_MS(黑暗预放行时间窗),
// TryGetMuruDarknessActiveState 改为严格的 expireMs>now 判定
//End By leewheel

// Darkness damages within 15 yards of M'uru; the rest is avoidance padding.
//By leewheel 2026-09-04: 上游1e110a5f——改名 MURU_DARKNESS_SAFE_DISTANCE(避免跨副本撞名)
inline constexpr float MURU_DARKNESS_SAFE_DISTANCE = 20.0f;
//End By leewheel

// The maximum distance from the melee dps holding spot that they wander to attack during Darkness.
inline constexpr float MURU_HOLDING_POSITION_RADIUS = 20.0f;

// Targeting is based on the nearest mob; this buffer is to keep targets sticky.
inline constexpr float MURU_TARGET_SWITCH_MARGIN = 10.0f;

// Radius of Shadow Bolt Volley (46082), which is centred on the enslaved Void Spawn.
inline constexpr float MURU_SHADOW_BOLT_VOLLEY_RADIUS = 20.0f;

// Void Zones (25879) have aura 46262, ticking 46264 for 3k in a 3y radius, and spawn Dark Fiends.
// The wide safe distance is in anticipation of the Dark Fiend spawn. Search is measured by
// IsWithinDist, which adds both CombatReaches for a total of 14.5y.
// By leewheel 2026-08-27: 对齐 the-lab 1e0caf61——常量整理+新增暗鬼搜索半径(search radius 区分击杀驱散/规避)
//By leewheel 2026-09-04: 上游1e110a5f——VOID_ZONE_SAFE_DISTANCE 8→10(检索按IsWithinDist计双触及共14.5y)
inline constexpr float VOID_ZONE_SEARCH_RADIUS = 12.0f;
inline constexpr float VOID_ZONE_SAFE_DISTANCE = 10.0f;
// Dark Fiend search radii for killing (dispelling) and avoiding, respectively
// 暗鬼搜索半径: 前者用于击杀/驱散, 后者用于规避
inline constexpr float DARK_FIEND_DISPEL_SEARCH_RADIUS = 50.0f;
inline constexpr float DARK_FIEND_AVOID_SEARCH_RADIUS = 15.0f;
// A Dark Fiend detonates within 2y of whoever it is chasing. The safe distance is deliberately
// wide as touching a single Dark Fiend is almost a guaranteed wipe.
//By leewheel 2026-09-04: 上游1e110a5f——DARK_FIEND_SAFE_DISTANCE 10→12
inline constexpr float DARK_FIEND_SAFE_DISTANCE = 12.0f;
//End By leewheel

inline constexpr float SINGULARITY_SEARCH_RADIUS = 30.0f;

// Tanks drag nothing further than this from the ranged stack.
inline constexpr float MURU_MAX_TARGET_DIST_FROM_STACK = 25.0f;

// 误导只在目标几乎满血时才值得消耗冷却
inline constexpr float MURU_MISDIRECT_MIN_TARGET_HP_PERCENT = 80.0f;

// Dps cooldowns are held until 97% to allow for initial positioning.
inline constexpr float MURU_MAX_DPS_HP_PERCENT = 97.0f;
//End By leewheel

extern std::unordered_map<uint32, MuruDarknessState> muruDarknessStates;
extern std::unordered_map<uint32, std::unordered_map<ObjectGuid, uint8>>
    muruVoidSentinelTankAssignments;

bool IsMuruPhaseActive(Unit* muru);
bool TryGetMuruDarknessActiveState(Player* bot, Unit* muru);
//By leewheel 2026-09-04: 上游1e110a5f——默认参数跟随改名 MURU_DARKNESS_EARLY_WINDOW_MS
bool TryGetMuruDarknessEarlyState(
    Player* bot, Unit* muru, uint32 earlyWindowMs = MURU_DARKNESS_EARLY_WINDOW_MS);
//End By leewheel
MuruEncounterGuids FindMuruEncounterGuids(PlayerbotAI* botAI);
void GatherMuruEncounterTargets(PlayerbotAI* botAI, MuruEncounterTargets& targets);
Unit* FindMuruBerserkerToStun(PlayerbotAI* botAI);
Unit* FindMuruFuryMageToInterrupt(PlayerbotAI* botAI);
Unit* FindMuruFuryMageToSpellsteal(PlayerbotAI* botAI);
bool IsTankingMuruVoidSentinel(PlayerbotAI* botAI);
GuidVector FindMuruVoidZoneGuids(Player* bot);
ObjectGuid FindMuruSingularityGuid(Player* bot);
Creature* FindMuruVoidZoneToAvoid(PlayerbotAI* botAI);
Creature* FindAvailableVoidSpawnForEnslave(PlayerbotAI* botAI);
//End By leewheel

}

#endif
