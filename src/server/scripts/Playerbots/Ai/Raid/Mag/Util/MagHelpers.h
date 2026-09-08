/* 玛瑟里顿的巢穴 机器人策略 */
#ifndef PLAYERBOTS_MAGHELPERS_H
#define PLAYERBOTS_MAGHELPERS_H

#include "Common.h"
#include "ObjectGuid.h"
#include "Position.h"
#include <ctime>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class Creature;
class GameObject;
class Map;
class Player;
class PlayerbotAI;
class Unit;

namespace MagHelpers
{

template <typename T, std::enable_if_t<std::is_enum_v<T>, int> = 0>
constexpr uint32 Id(T value)
{
    return static_cast<uint32>(value);
}

enum class MagSpells : uint32
{
    // Magtheridon
    SPELL_SHADOW_CAGE   = 30205,
    SPELL_SHADOW_GRASP  = 30410,
    SPELL_BLAST_NOVA    = 30616,
    SPELL_DEBRIS_SPAWN  = 30630,
    SPELL_QUAKE         = 30657,

    // Hunter
    SPELL_MISDIRECTION  = 35079,
};

enum class MagNpcs : uint32
{
    NPC_BURNING_ABYSSAL = 17454,
};

enum class MagObjs : uint32
{
    GO_BLAZE            = 181832,
};

struct CubeInfo
{
    ObjectGuid guid;
    float x, y, z;
};

inline constexpr uint32 MAG_MAP_ID                = 544;
inline constexpr uint32 SOUTH_CHANNELER           = 90978;
inline constexpr uint32 WEST_CHANNELER            = 90979;
inline constexpr uint32 NORTHWEST_CHANNELER       = 90980;
inline constexpr uint32 EAST_CHANNELER            = 90982;
inline constexpr uint32 NORTHEAST_CHANNELER       = 90981;
inline constexpr uint8 BLAST_NOVA_INTERIM_SECONDS = 45;
inline constexpr uint32 BLAST_NOVA_INTERIM_MS = BLAST_NOVA_INTERIM_SECONDS * IN_MILLISECONDS;

inline Position const WAITING_FOR_MAGTHERIDON_POSITION = { -31.962f,  -8.514f, -0.304f, 0.657f };
inline Position const MAGTHERIDON_TANK_POSITION =        {  -6.147f, -37.812f, -0.411f,   0.0f };
inline Position const NW_CHANNELER_TANK_POSITION =       { -11.764f,  30.818f, -0.411f,   0.0f };
inline Position const NE_CHANNELER_TANK_POSITION =       { -12.490f, -26.211f, -0.411f,   0.0f };
inline Position const RANGED_SPREAD_POSITION =           { -14.890f,   1.995f, -0.406f,   0.0f };
inline Position const HEALER_SPREAD_POSITION =           {  -2.265f,   1.874f, -0.404f,   0.0f };

extern std::unordered_map<uint32, uint32> dpsWaitTimer;
extern std::unordered_map<uint32, uint32> blastNovaTimer;
//By leewheel 2026-08-21: 移植 brighton-chi e92a52db..2eaa83f3——顶部落石标记改为集合(仅记录实例Id,不再用map<bool>)
extern std::unordered_set<uint32> ceilingCollapseApplied;
//End By leewheel
extern std::unordered_map<uint32, bool> lastBlastNovaState;
extern std::unordered_map<uint32, std::unordered_map<ObjectGuid, CubeInfo>>
    botToCubeAssignments;

extern std::vector<uint32> const MANTICRON_CUBE_DB_GUIDS;
std::vector<CubeInfo> GetAllCubeInfosByDbGuids(
    Map* map, std::vector<uint32> const& cubeDbGuids);
Creature* GetChanneler(Player* bot, uint32 dbGuid);
bool IsMagtheridonActive(Unit* magtheridon);
bool IsBlastNovaCasting(Unit* magtheridon);
bool IsCubeClicker(Player* bot);
//By leewheel 2026-08-21: 移植 brighton-chi 49c3de92——碎石检测改为动态对象查询(不再依赖位置记录容器)；
//e92a52db..2eaa83f3——进一步拆分出"取碎石位置/取烈焰列表"的一次性查询助手(供行动内循环复用)
inline constexpr float DEBRIS_HAZARD_RADIUS = 10.0f;
inline constexpr float CONFLAGRATION_HAZARD_RADIUS = 5.0f;
bool GetActiveDebrisPosition(Player* bot, Position& debris);
std::vector<GameObject*> GetActiveConflagrations(PlayerbotAI* botAI);
bool IsPositionInConflagration(std::vector<GameObject*> const& blazes, float x, float y);
bool IsPositionInActiveDebris(Player* bot, float x, float y, float radius = DEBRIS_HAZARD_RADIUS);
bool IsPositionInActiveConflagration(PlayerbotAI* botAI, float x, float y);
//End By leewheel

}

#endif
