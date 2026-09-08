/* 祖阿曼 机器人策略 */
/* 移植来源: brighton-chi mod-playerbots the-lab ZAHelpers.h 移植适配 TC 框架 */
#ifndef PLAYERBOTS_ZAHELPERS_H
#define PLAYERBOTS_ZAHELPERS_H

#include "Common.h"
#include "ObjectGuid.h"
#include "Position.h"
//By leewheel 2026-09-04: 上游979da939——Unit.h不再直接包含, 改前置声明GameObject
#include <array>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

class GameObject;
class Player;
class PlayerbotAI;
class Unit;
//End By leewheel

namespace ZaHelpers
{

template <typename T, std::enable_if_t<std::is_enum_v<T>, int> = 0>
constexpr uint32 Id(T value)
{
    return static_cast<uint32>(value);
}

enum class ZaSpells : uint32
{
    // Akil'zon <Eagle Avatar>
    SPELL_ELECTRICAL_STORM          = 43648,

    // Nalorakk <Bear Avatar>
    SPELL_BEARFORM                  = 42377,

    // Jan'alai <Dragonhawk Avatar>
    SPELL_FIRE_BOMB_CHANNEL         = 42621,

    // Hex Lord Malacrass
    SPELL_HEX_LORD_WHIRLWIND        = 43442,
    SPELL_HEX_LORD_SPELL_REFLECTION = 43443,
    SPELL_UNSTABLE_AFFLICTION       = 43522,

    // Zul'jin
    SPELL_ZULJIN_WHIRLWIND          = 17207,
    SPELL_SHAPE_OF_THE_BEAR         = 42594,
    SPELL_SHAPE_OF_THE_EAGLE        = 42606,
    SPELL_SHAPE_OF_THE_LYNX         = 42607,
    SPELL_SHAPE_OF_THE_DRAGONHAWK   = 42608,
    //By leewheel 2026-09-05: 上游88c08483——注释改写: 43149是6秒光环,祖尔金在迅猛龙阶段自我施放,
    //每秒跳2次,每次对随机目标施放43150;可能是他最危险的技能,但不确定如何不通过法术钩子或
    //重声明其bossai就能获取目标,两者为此都不值得,记录在此供日后实现
    // SPELL_CLAW_RAGE              = 43149,
    //End By leewheel

    // Hunter
    SPELL_MISDIRECTION              = 35079,
};

enum class ZaNpcs : uint32
{
    // Trash
    NPC_AMANI_HEALING_WARD          = 23757,
    NPC_AMANI_PROTECTIVE_WARD       = 23822,

    // Jan'alai <Dragonhawk Avatar>
    NPC_JANALAI                     = 23578,
    NPC_AMANI_DRAGONHAWK_HATCHLING  = 23598,
    NPC_AMANISHI_HATCHER            = 23818,
    NPC_FIRE_BOMB                   = 23920,

    // Halazzi <Lynx Avatar>
    NPC_HALAZZI                     = 23577,
    //By leewheel 2026-09-04: 上游——山猫之灵(辅助坦拾取目标, 主坦禁止嘲讽)
    NPC_SPIRIT_OF_THE_LYNX          = 24143,
    //End By leewheel
    NPC_CORRUPTED_LIGHTNING_TOTEM   = 24224,

    // Hex Lord Malacrass
    NPC_HEX_LORD_MALACRASS          = 24239,
    NPC_ALYSON_ANTILLE              = 24240,
    NPC_THURG                       = 24241,
    NPC_SLITHER                     = 24242,
    NPC_LORD_RAADAN                 = 24243,
    NPC_GAZAKROTH                   = 24244,
    NPC_FENSTALKER                  = 24245,
    NPC_DARKHEART                   = 24246,
    NPC_KORAGG                      = 24247,

    // Zul'jin
    NPC_ZULJIN                      = 23863,
    NPC_FEATHER_VORTEX              = 24136,
};

enum class ZaObjects : uint32
{
    GO_FREEZING_TRAP                = 186669,
};

// General
inline constexpr uint32 ZA_MAP_ID = 568;
//By leewheel 2026-09-04: 上游14413ee2/04612b0b——旋风安全距离改为12(半径8y+4y避让), 新增HOLD距离(15=12+3y)供"别急着跑回来"乘数使用
// For Hex Lord and Zul'jin. Whirlwind radius is 8y. Safe distance has 4y of MoveAway padding;
// hold distance is for the don't run back in multiplier and adds another 3y of padding.
inline constexpr float ZA_WHIRLWIND_SAFE_DISTANCE = 12.0f;
inline constexpr float ZA_WHIRLWIND_HOLD_DISTANCE = 15.0f;
//End By leewheel

//By leewheel 2026-09-04: 上游14413ee2——删除通用 SafeZoneQuad/IsOnFlatFloor/FindSafeStepInZone/GetSpreadSlotIndex/
//  CountAttackersByEntry 声明, 改为 Jan'alai 专用多边形安全区 + Zul'jin 专用站位索引 + 幼龙计数(见下方各段)
//End By leewheel
//By leewheel 2026-08-30: 补回 IsPathSafeFromHazards 声明——本地 FindSafeStepInZone 调用但 .h 未声明（本地改进函数）
//End By leewheel
bool IsPathSafeFromHazards(
    Position const& start, Position const& end,
    std::vector<Unit*> const& hazards, float hazardRadius);
//By leewheel 2026-09-04: 上游183b6d34→HEAD——危害判定改为精确2维距离(与调用方危险判定一致)
bool IsPositionSafeFromHazards(
    float x, float y, std::vector<Unit*> const& hazards, float hazardRadius);
//End By leewheel

// Akil'zon <Eagle Avatar>
inline Position const AKILZON_TANK_POSITION = { 378.369f, 1407.718f, 74.797f };
// Electrical Storm runs on a fixed 60s cycle once the encounter starts. Bots react from
// AKILZON_STORM_LEAD_MS before each cast until the storm itself has expired.
inline constexpr uint32 AKILZON_STORM_PERIOD_MS = 60000;
inline constexpr uint32 AKILZON_STORM_LEAD_MS = 5000;
inline constexpr uint32 AKILZON_STORM_DURATION_MS = 10000;
// Instance id -> getMSTime() stamp of the first storm cycle.
extern std::unordered_map<uint32, uint32> akilzonStormTimer;
bool IsInStormWindow(uint32 startMs);
Player* GetElectricalStormTarget(Player* bot);

// Nalorakk <Bear Avatar>
inline Position const NALORAKK_TANK_POSITION = { -80.208f, 1324.530f, 40.942f };
bool IsNalorakkInBearForm(Unit* nalorakk);

// Jan'alai <Dragonhawk Avatar>
inline Position const JANALAI_TANK_POSITION = { -33.873f, 1149.571f, 19.146f };
//By leewheel 2026-09-04: 上游14413ee2——安全区从四角矩形+地面探测改为实测五边形(贴火焰墙), 删除FLOOR_TOLERANCE
inline constexpr float JANALAI_PLATFORM_Z = 19.146f;
// The safe zone is about the entire reachable platform, minus the destroyed, fenced-off corner and
// the parts that jut out on each side. The measured area is right up against the Fire Wall, but
// that's ok because the Fire Wall's damage actually comes from the same Fire Bomb trigger NPCs so
// the Fire Bomb avoidance will keep away from the wall too.
inline std::vector<Position> const JANALAI_SAFE_ZONE = {
    Position(-12.576924f, 1131.9163f, JANALAI_PLATFORM_Z),
    Position(-52.068108f, 1132.8198f, JANALAI_PLATFORM_Z),
    Position(-51.508590f, 1158.4890f, JANALAI_PLATFORM_Z),
    Position(-44.008713f, 1168.6125f, JANALAI_PLATFORM_Z),
    Position(-12.115170f, 1167.6681f, JANALAI_PLATFORM_Z)
};
//End By leewheel
// Hatchers open eggs in a ramp - 1 on the first tick, then 2, then 3, every 5s - across 40 eggs,
// 20 per side. Bloodlust waits for this many Hatchlings to be up.
inline constexpr uint32 JANALAI_BLOODLUST_HATCHLING_COUNT = 6;
//By leewheel 2026-09-04: 上游a8cc34f9/04612b0b——远程环绕半径/搜索距离/安全距离常量化
// Ranged spread evenly around the tank position to reduce players hit by Flame Breath.
inline constexpr float JANALAI_RANGED_SPREAD_RADIUS = 15.0f;
// How far bots search for a safe spot from Fire Bombs.
inline constexpr float JANALAI_FIRE_BOMB_MAX_SEARCH_DISTANCE = 30.0f;
// How far bots search for the Fire Bombs themselves, which obviously needs to be farther.
inline constexpr float JANALAI_FIRE_BOMB_SEARCH_RADIUS = 40.0f;
// Fire Bomb (42630) has a 4y radius. It does not include CombatReach so the 1.5y is just extra
// padding. It's not needed strategically since bots are standing in place, but I don't think humans
// would stand closer than this because it looks very dangerous, Thus, 5.5y is a good compromise to
// allow enough safe spots without looking too unrealistic.
inline constexpr float JANALAI_FIRE_BOMB_SAFE_DISTANCE = 5.5f;
//End By leewheel
// Feeds the "jan'alai fire bombs" value. Longer than the 200ms the other raids use for hazards,
// because bombs are unusually well behaved: they all spawn at once at their final positions, never
// move, and detonate on a fixed 11s timer (StartBombing() in boss_janalai.cpp). Staleness costs
// only detection latency against that 11s, and the value holds GUIDs rather than pointers, so a
// despawned bomb resolves to nullptr instead of dangling however far behind the cache runs.
inline constexpr uint32 FIRE_BOMB_CACHE_INTERVAL_MS = 1000;
// Jan'alai hatches every remaining egg at once at 35% HP so that opens the door for Bloodlust in
// any case. Using 33% to account for some delay for the event to actually complete.
inline constexpr float JANALAI_HATCH_ALL_HEALTH_PCT = 33.0f;
//By leewheel 2026-09-04: 上游14413ee2——孵化者对/幼龙计数/轰炸判定声明前移(供ZAMultipliers直接使用)
std::pair<Unit*, Unit*> GetAmanishiHatcherPair(PlayerbotAI* botAI);
uint32 CountJanalaiHatchlingsByEntry(PlayerbotAI* botAI);
// Jan'alai casts Fire Bomb Channel at the beginning of each Fire Bomb event until the bombs
// detonate. This is the best check for the event being active because the bomb trigger NPCs stick
// around for 4s after the detonation.
bool IsJanalaiBombing(Unit* janalai);
//End By leewheel
// The grid search, run once per cache interval behind the "jan'alai fire bombs" value.
GuidVector FindNearbyFireBombGuids(Player* bot);
// GetNearbyFireBombs() resolves that value for the avoidance search, which is the only caller that
// needs the bombs themselves. Everything else asks IsJanalaiBombing() instead.
std::vector<Unit*> GetNearbyFireBombs(PlayerbotAI* botAI);
//By leewheel 2026-09-04: 上游14413ee2——Jan'alai专用安全步搜索(凸多边形, 无地面探测)
bool FindSafeStepInJanalaiZone(
    Player* bot, std::vector<Unit*> const& hazards, std::vector<Position> const& safeZone,
    float maxSearchDistance, float hazardRadius, float moveDist,
    float& stepX, float& stepY, float& stepZ);
//End By leewheel

// Halazzi <Lynx Avatar>
inline Position const HALAZZI_TANK_POSITION = { 370.733f, 1131.202f, 6.516f };

//By leewheel 2026-09-04: 上游14413ee2——冰霜陷阱安全距离11(眩晕半径10y+1y), 搜索半径16(乘数共用), 新增200ms缓存值
// Hex Lord Malacrass
// Freezing Trap (43448) stuns for 10s across a 10y radius, even though its trigger radius is much
// shorter at 2.5y. Padded by 1y instead of 2y because without a dedicated tank spot, the boss tends
// to be toward the back of the room, and bots can run through the door to Zul'jin if sent too far.
inline constexpr float ZA_FREEZING_TRAP_SAFE_DISTANCE = 11.0f;
// Also used for HexLordMalacrassStayAwayFromFreezingTrapMultiplier.
inline constexpr float ZA_FREEZING_TRAP_SEARCH_RADIUS = 16.0f;
// Feeds the "hex lord malacrass freezing trap" value.
inline constexpr uint32 FREEZING_TRAP_CACHE_INTERVAL_MS = 200;
ObjectGuid FindNearbyFreezingTrapGuid(Player* bot);
GameObject* GetNearbyFreezingTrap(PlayerbotAI* botAI);
//End By leewheel

// Zul'jin
inline Position const ZULJIN_TANK_POSITION = { 120.210f, 705.564f, 45.111f };
//By leewheel 2026-09-04: 上游14413ee2——旋风分散注释改写(4旋风7y/s追人+4y光环, 不可闪避→远程散开拉开行程)
// 4 Cyclones (Feather Vortex NPCs) chase random raid members at 7y/s (player run speed) and have
// a 4y radius. They're not really dodgeable as a result, so the approach is just to spread ranged
// out for the Cyclones to have to travel farther and not be able to hit as many targets at once.
inline constexpr float ZULJIN_SPREAD_Z = 45.111f;
inline std::array<Position, 8> const ZULJIN_SPREAD_POSITIONS = {{
    // Healer slots.
    Position(120.462f, 728.502f, ZULJIN_SPREAD_Z),
    Position(119.984f, 693.188f, ZULJIN_SPREAD_Z),
    // Ranged dps slots.
    Position(94.939f, 713.698f, ZULJIN_SPREAD_Z),
    Position(145.286f, 713.034f, ZULJIN_SPREAD_Z),
    Position(103.869f, 728.703f, ZULJIN_SPREAD_Z),
    Position(135.847f, 728.816f, ZULJIN_SPREAD_Z),
    Position(143.862f, 697.302f, ZULJIN_SPREAD_Z),
    Position(95.618f, 698.439f, ZULJIN_SPREAD_Z)
}};
//End By leewheel

//By leewheel 2026-09-04: 上游14413ee2——Zul'jin专用站位索引(治疗优先, 前两槽位于祖尔金楼梯下/上)
// Ranged are ordered by healers first. The first two positions are centered at the bottom of the
// stairs leading to Zul'jin's starting spot and the top of the stairs where his battle area begins.
// Presumably, players will bring 2 healers, and the intent is to allow each healer to reach every
// other bot's position (accordingly, no two spots are more than 39y apart).
bool GetZuljinSpreadSlotIndex(Player* bot, size_t slotCount, size_t& slotIndex);
//End By leewheel

}

#endif
