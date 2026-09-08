/* 太阳之井高地 机器人策略 */
#ifndef PLAYERBOTS_SWPENCOUNTERKJ_H
#define PLAYERBOTS_SWPENCOUNTERKJ_H

#include "ObjectGuid.h"
#include "Position.h"
#include "SWPSharedConstants.h"
#include <array>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class Player;
class PlayerbotAI;
class Unit;

namespace SwpHelpers
{

struct KiljaedenRangedBotAssignment
{
    ObjectGuid guid;
    uint8 slotIndex = 0;
};

struct KiljaedenArmageddon
{
    Position destination;
    uint32 expireMs = 0;
    float safeDistance = 0.0f;
};

struct KiljaedenDarknessShieldState
{
    bool inDarkness = false;
    bool shieldCastThisDarkness = false;
    uint32 darknessStartMs = 0;
    uint32 lastDarknessCastMsLeft = 0;
};

struct KiljaedenEncounterState
{
    std::vector<KiljaedenArmageddon> armageddons;
    std::unordered_map<ObjectGuid, uint8> rangedAssignments;
    std::unordered_map<ObjectGuid, uint8> rangedArmageddonAssignments;
    uint32 dragonOrbAnnouncementMs = 0;
    uint32 rangedAssignmentRebuildMs = 0;
    uint32 rangedArmageddonRebuildMs = 0;
};

inline std::array const KILJAEDEN_DRAGON_ORB_ENTRIES = {
    Id(SwpObjects::GO_DRAGON_ORB_1),
    Id(SwpObjects::GO_DRAGON_ORB_2),
    Id(SwpObjects::GO_DRAGON_ORB_3),
    Id(SwpObjects::GO_DRAGON_ORB_4),
};

inline constexpr uint32 KILJAEDEN_ARMAGEDDON_HAZARD_DURATION_MS = 10000;
inline constexpr float KILJAEDEN_ARMAGEDDON_SAFE_DISTANCE = 11.0f;
//By leewheel 2026-08-21: 移植 brighton-chi 096512a0——基尔加丹各阶段HP阈值常量
inline constexpr float KILJAEDEN_PHASE3_HP_THRESHOLD = 85.0f;
inline constexpr float KILJAEDEN_PHASE4_HP_THRESHOLD = 55.0f;
inline constexpr float KILJAEDEN_PHASE5_HP_THRESHOLD = 25.0f;
//End By leewheel

//By leewheel 2026-09-04: 上游1e110a5f——KJ常量重排+改名(HOLY_PALADIN_STUN_STANDOFF→HAND_HOLY_PALADIN_STANDOFF、
//  RANGED_ASSIGNMENT_REBUILD_INTERVAL_MS→KILJAEDEN_前缀、DRAGON_*→KILJAEDEN_DRAGON_*前缀、
//  删KILJAEDEN_REFLECTION_*触及常量(改用SWPSharedConstants.h全局触及常量)、删HAND_SELF_AOE_RACIAL_RADIUS/HAND_SHOCKWAVE_RADIUS(同上))
// Feeds the "kiljaeden hands" value.
inline constexpr uint32 HAND_CACHE_INTERVAL_MS = 200;
// The presence of Dragon Orbs is cached, but GO_FLAG_IN_USE and GO_FLAG_NOT_SELECTABLE are not.
inline constexpr uint32 DRAGON_ORB_CACHE_INTERVAL_MS = 200;

// Position check to gate trying to find Hands.
inline constexpr float SUNWELL_CENTER_RADIUS = 100.0f;
inline constexpr float HAND_SEARCH_RADIUS = 75.0f;
// Hammer of Justice is a single-target spell that counts both CombatReaches so its actual range is
// its tooltip range of 10y + 1.5y (player) + 2.5y (Hand) = 14y. Holding a little inside that
// threshold leaves room for the Hand to shift between the range check and the cast landing.
inline constexpr float HAND_HOLY_PALADIN_STANDOFF = 12.0f;
// Timing between stuns for bots to coordinate them. A gate based purely on UNIT_STATE_STUNNED
// still results in spam stuns due to the delay between a spell casting and resolving.
inline constexpr uint32 HAND_CONTROL_CLAIM_MS = 1500;
// Hands cast Shadow Infusion (45772) at or below 20% HP, which makes them permanently immune
// to both stun and silence.
inline constexpr float HAND_CC_IMMUNE_HP_PERCENT = 20.0f;

// Throttle assigned ranged position rebuilds since they should be stable during the encounter.
inline constexpr uint32 KILJAEDEN_RANGED_ASSIGNMENT_REBUILD_INTERVAL_MS = 1000;
inline constexpr uint32 ARMAGEDDON_ASSIGNMENT_REBUILD_INTERVAL_MS = 250;
inline constexpr float KILJAEDEN_RANGED_ARC_ORIENTATION = 0.8f;
inline constexpr float KILJAEDEN_INNER_RANGED_RADIUS = 23.0f;
inline constexpr float KILJAEDEN_OUTER_RANGED_RADIUS = 36.0f;
inline constexpr uint8 KILJAEDEN_INNER_RANGED_SLOT_COUNT = 7;
inline constexpr uint8 KILJAEDEN_OUTER_RANGED_SLOT_COUNT = 11;
inline constexpr uint8 KILJAEDEN_TOTAL_RANGED_SLOT_COUNT =
    KILJAEDEN_INNER_RANGED_SLOT_COUNT + KILJAEDEN_OUTER_RANGED_SLOT_COUNT;
// Only up to two ranged bots may share a slot when an Armageddon forces a reshuffle.
inline constexpr uint8 KILJAEDEN_MAX_BOTS_PER_RANGED_SLOT = 2;
//By leewheel 2026-09-04 收尾: 旧名 ARMAGEDDON_HAZARD_DURATION_MS/ARMAGEDDON_SAFE_DISTANCE 已迁移到
//58-59 行的 KILJAEDEN_ARMAGEDDON_* 并让 SWPScripts.cpp 改用新名, 删除并存旧名避免双份常量

inline constexpr float KILJAEDEN_REFLECTION_SEARCH_RADIUS = 100.0f;

inline constexpr float DRAGON_ORB_SEARCH_RADIUS = 200.0f;
inline constexpr float DRAGON_ORB_IN_USE_HOLD_DISTANCE = 15.0f;
// Grace after using an Orb before a lingering root is considered stale and is cleared.
inline constexpr uint32 DRAGON_ORB_USE_GRACE_MS = 2000;
inline constexpr uint32 DRAGON_ORB_ANNOUNCEMENT_RESET_MS = 10000;
//By leewheel 2026-09-04: 上游70808114——补 SHIELD_OF_THE_BLUE_CAST_WINDOW_MS 常量
//Shield of the Blue (45848) 持续 5 秒, Darkness of a Thousand Souls (46605) 为 8 秒引导,
//所以护盾需要在黑暗引导剩余时间低于此窗口时才放出(留 0.5 秒余量)
inline constexpr int32 SHIELD_OF_THE_BLUE_CAST_WINDOW_MS = 4500;
//End By leewheel
// Bots with Fire Bloom hold this far off the Darkness stack until the Shield casts.
inline constexpr float FIRE_BLOOM_STANDOFF = 15.0f;

// Breath: Haste and Breath: Revitalize are 13y cones on allies, so the dragon stops a little
// under half that from its target and looks for a cluster of roughly the same to cover at once.
inline constexpr float KILJAEDEN_DRAGON_BREATH_STANDOFF = 6.0f;
inline constexpr float KILJAEDEN_DRAGON_STANDOFF_TOLERANCE = 1.0f;
inline constexpr float KILJAEDEN_DRAGON_CLUSTER_RADIUS = 6.0f;
inline constexpr uint8 KILJAEDEN_DRAGON_MIN_CLUSTER_SIZE = 3;
//End By leewheel

//By leewheel 2026-08-29: 引入 mod-playerbots 新提交——新增 KJ 手部控制常量和范围搜索常量
//By leewheel 2026-09-04: 旧 HAND_SELF_AOE_RACIAL_RADIUS/HAND_SHOCKWAVE_RADIUS 改用SWPSharedConstants.h的
//   SELF_AOE_RACIAL_RADIUS/SHOCKWAVE_RADIUS 全局常量(上游1e110a5f); 旧 HOLY_PALADIN_STUN_STANDOFF 改名 HAND_HOLY_PALADIN_STANDOFF(见上)
//End By leewheel

//By leewheel 2026-09-05: 上游1e110a5f——改名 KILJAEDEN_CENTER_POSITION→SUNWELL_CENTER_POSITION
//(该中心点同时供非KJ的触发/乘数使用, 上游统一命名)
inline Position const SUNWELL_CENTER_POSITION =   { 1698.450f, 628.030f, 28.199f };
//End By leewheel
inline Position const KILJAEDEN_TANK_POSITION =     { 1704.729f, 634.891f, 27.787f };
inline Position const KILJAEDEN_S_MELEE_POSITION =  { 1689.487f, 632.119f, 27.823f };
inline Position const KILJAEDEN_E_MELEE_POSITION =  { 1700.542f, 619.589f, 27.786f };
inline Position const KILJAEDEN_DARKNESS_POSITION = { 1709.768f, 642.241f, 27.706f };

extern std::unordered_set<ObjectGuid> kiljaedenTrackedArmageddonTargets;
extern std::unordered_map<uint32, KiljaedenEncounterState> kiljaedenEncounterStates;
extern std::unordered_map<uint32, std::unordered_map<ObjectGuid, uint32>> kiljaedenHandControlClaims;
extern std::unordered_map<ObjectGuid::LowType, uint32> kiljaedenDragonOrbUseTimes;

GuidVector FindKiljaedenHandGuids(Player* bot);
std::vector<Unit*> GetKiljaedenHands(PlayerbotAI* botAI);
bool IsKiljaedenHandControlClaimed(Unit* hand);
void ClaimKiljaedenHandControl(Unit* hand);
void AddKiljaedenArmageddon(
    uint32 instanceId, Position const& destination, uint32 durationMs, float safeDistance);
bool TryGetKiljaedenNearestArmageddon(Player* bot, KiljaedenArmageddon& armageddon);
void PruneExpiredKiljaedenArmageddons(uint32 instanceId);
bool TryGetKiljaedenRangedSlotPosition(uint8 slotIndex, Position& position);
void EnsureKiljaedenRangedAssignments(Player* bot);
void EnsureKiljaedenRangedArmageddonAssignments(Player* bot);
bool IsKiljaedenCastingDarknessOfAThousandSouls(Unit* kiljaeden);
GuidVector FindKiljaedenDragonOrbGuids(Player* bot);
Player* GetKiljaedenDragonOrbUser(Player* bot);
bool ResetKiljaedenDragonOrbUserAnnouncement(uint32 instanceId);
bool HasRecentKiljaedenDragonOrbUse(Player* bot, uint32 recentMs);
bool HasKiljaedenDragonAura(Player* bot);
Unit* GetKiljaedenControlledDragon(Player* bot);
bool CastKiljaedenDragonSpell(Unit* dragon, uint32 spellId);
Player* FindBestKiljaedenDragonClusterTarget(Player* bot, Unit* dragon, uint32 spellId);
Player* FindClosestKiljaedenDragonTarget(Player* bot, Unit* dragon, uint32 spellId = 0);
//By leewheel 2026-08-27: 对齐 the-lab 19196110——移除 HasAtLeastThreeBotTanks(不再要求3个机器人坦克), 改用 GetGroupAssistTank/GetGroupMainTank
//End By leewheel

}

#endif
