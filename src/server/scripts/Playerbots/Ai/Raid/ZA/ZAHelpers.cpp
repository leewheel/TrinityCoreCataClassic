/* 副本机器人策略 */
//By leewheel 2026-09-04: 上游14413ee2/979da939/04612b0b/7a2787f3/783e1eac/4f2d155b/a8cc34f9/7df92554——
//  通用安全区改为 Jan'alai 专用多边形搜索 + Zul'jin 专用站位索引 + 冰霜陷阱缓存值 + 命名空间本地助手
//End By leewheel
#include "ZAHelpers.h"
#include "EncounterHelpers.h"
#include "Playerbots.h"
//By leewheel 2026-08-22: 随上游(9e8e7718..70b60954)补充标准库头文件
#include <algorithm>
#include <cmath>
#include <limits>
#include <list>
//End By leewheel

//By leewheel 2026-09-04: 上游783e1eac——本地命名空间助手用匿名namespace, ZaHelpers本体后置
using namespace EncounterHelpers;

namespace
{

// 点在凸多边形内部当且仅当它在每条边的同侧
bool IsInsideSafeZone(std::vector<Position> const& corners, float x, float y)
{
    // The point is inside a convex polygon when it falls on the same side of every edge.
    size_t const count = corners.size();
    int8 insideSign = 0;
    for (size_t i = 0; i < count; ++i)
    {
        Position const& edgeStart = corners[i];
        Position const& edgeEnd = corners[(i + 1) % count];

        float cross =
            (edgeEnd.GetPositionX() - edgeStart.GetPositionX()) * (y - edgeStart.GetPositionY()) -
            (edgeEnd.GetPositionY() - edgeStart.GetPositionY()) * (x - edgeStart.GetPositionX());

        if (cross == 0.0f)
            continue;  // 恰好在边上，不排除任何情况

        int8 const sign = cross > 0.0f ? 1 : -1;
        if (insideSign == 0)
            insideSign = sign;
        else if (insideSign != sign)
            return false;
    }

    return true;
}

}  // end anonymous namespace

namespace ZaHelpers
{

//By leewheel 2026-09-04: 上游183b6d34→HEAD——IsPositionSafeFromHazards 移入 ZaHelpers 命名空间
//(此前误落匿名namespace导致链接错误 LNK2019)
bool IsPositionSafeFromHazards(
    float x, float y, std::vector<Unit*> const& hazards, float hazardRadius)
{
    // 精确2维距离，与调用方的危险判定(ExactDist2d)保持一致
    for (Unit* hazard : hazards)
    {
        if (hazard->GetExactDist2d(x, y) < hazardRadius)
            return false;
    }

    return true;
}
//End By leewheel

//By leewheel 2026-08-30: 本地保留的独立安全函数（BT/TK/SSC 的 FindSafestNearbyPosition 仍在使用）
//End By leewheel
// General
Position FindSafestNearbyPosition(Player* bot,
    std::vector<Unit*> const& hazards, const Position& safeZoneCenter,
    float safeZoneRadius, float hazardRadius, bool requireSafePath)
{
    constexpr float searchStep = float(M_PI) / 8.0f; //By leewheel 2026-09-03 修复C4305警告
    constexpr float distanceStep = 1.0f;

    Position bestPos;
    float minMoveDistance = std::numeric_limits<float>::max();
    bool foundSafe = false;

    for (float distance = 0.0f;
            distance <= safeZoneRadius; distance += distanceStep)
    {
        for (float angle = 0.0f; angle < 2 * float(M_PI); angle += searchStep) //By leewheel 2026-09-03 修复C4305警告
        {
            float x = bot->GetPositionX() + distance * std::cos(angle);
            float y = bot->GetPositionY() + distance * std::sin(angle);

            if (safeZoneCenter.GetExactDist2d(x, y) > safeZoneRadius)
                continue;

            if (!IsPositionSafeFromHazards(x, y, hazards, hazardRadius))
                continue;

            Position testPos(x, y, bot->GetPositionZ());

            bool pathSafe = true;
            if (requireSafePath)
            {
                pathSafe =
                    IsPathSafeFromHazards(bot->GetPosition(), testPos, hazards, hazardRadius);
                if (!pathSafe)
                    continue;
            }

            float moveDistance = bot->GetExactDist2d(x, y);
            if (!foundSafe || moveDistance < minMoveDistance)
            {
                bestPos = testPos;
                minMoveDistance = moveDistance;
                foundSafe = pathSafe;
            }
        }

        if (foundSafe)
            break;
    }

    return bestPos;
}

bool IsPathSafeFromHazards(Position const& start, Position const& end,
    std::vector<Unit*> const& hazards, float hazardRadius)
{
    constexpr uint8 numChecks = 10;
    float dx = end.GetPositionX() - start.GetPositionX();
    float dy = end.GetPositionY() - start.GetPositionY();

    for (uint8 i = 1; i <= numChecks; ++i)
    {
        float ratio = static_cast<float>(i) / numChecks;
        float checkX = start.GetPositionX() + dx * ratio;
        float checkY = start.GetPositionY() + dy * ratio;

        if (!IsPositionSafeFromHazards(checkX, checkY, hazards, hazardRadius))
            return false;
    }

    return true;
}

std::vector<Unit*> GetAllHazardTriggers(Player* bot, uint32 entry, float searchRadius)
{
    std::vector<Unit*> triggers;
    std::list<Creature*> creatureList;
    bot->GetCreatureListWithEntryInGrid(creatureList, entry, searchRadius);

    for (Creature* creature : creatureList)
    {
        if (creature && creature->IsAlive())
            triggers.push_back(creature);
    }

    return triggers;
}

// Akil'zon <Eagle Avatar>

std::unordered_map<uint32, uint32> akilzonStormTimer;

bool IsInStormWindow(uint32 startMs)
{
    uint32 const elapsed = GetMSTimeDiffToNow(startMs);
    if (elapsed < AKILZON_STORM_PERIOD_MS - AKILZON_STORM_LEAD_MS)
        return false;

    uint32 const phase = (elapsed + AKILZON_STORM_LEAD_MS) % AKILZON_STORM_PERIOD_MS;
    return phase < AKILZON_STORM_LEAD_MS + AKILZON_STORM_DURATION_MS;
}

Player* GetElectricalStormTarget(Player* bot)
{
    Group* group = bot->GetGroup();
    if (!group)
        return nullptr;

    for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
    {
        Player* member = ref->GetSource();
        if (member && member->HasAura(Id(ZaSpells::SPELL_ELECTRICAL_STORM)))
            return member;
    }

    return nullptr;
}

// Nalorakk <Bear Avatar>

bool IsNalorakkInBearForm(Unit* nalorakk)
{
    return nalorakk && nalorakk->HasAura(Id(ZaSpells::SPELL_BEARFORM));
}

// Jan'alai <Dragonhawk Avatar>

//By leewheel 2026-09-04: 上游14413ee2——孵化者对/幼龙计数/轰炸判定/专用安全步搜索(按上游顺序前移至此)
std::pair<Unit*, Unit*> GetAmanishiHatcherPair(PlayerbotAI* botAI)
{
    Unit* lowest = nullptr;
    Unit* highest = nullptr;

    AiObjectContext* context = botAI->GetAiObjectContext();
    for (auto const& targetGuid : AI_VALUE(GuidVector, "possible targets no los"))
    {
        Unit* unit = botAI->GetUnit(targetGuid);
        if (unit && unit->GetEntry() == Id(ZaNpcs::NPC_AMANISHI_HATCHER))
        {
            if (!lowest || unit->GetGUID().GetCounter() < lowest->GetGUID().GetCounter())
                lowest = unit;

            if (!highest || unit->GetGUID().GetCounter() > highest->GetGUID().GetCounter())
                highest = unit;
        }
    }

    return {lowest, highest};
}

uint32 CountJanalaiHatchlingsByEntry(PlayerbotAI* botAI)
{
    uint32 count = 0;

    AiObjectContext* context = botAI->GetAiObjectContext();
    for (auto const& targetGuid : AI_VALUE(GuidVector, "attackers"))
    {
        Unit* unit = botAI->GetUnit(targetGuid);
        if (unit && unit->IsAlive() &&
            unit->GetEntry() == Id(ZaNpcs::NPC_AMANI_DRAGONHAWK_HATCHLING))
        {
            ++count;
        }
    }

    return count;
}

bool IsJanalaiBombing(Unit* janalai)
{
    return janalai && janalai->HasAura(Id(ZaSpells::SPELL_FIRE_BOMB_CHANNEL));
}
//End By leewheel

GuidVector FindNearbyFireBombGuids(Player* bot)
{
    std::list<Creature*> creatureList;
    bot->GetCreatureListWithEntryInGrid(
        creatureList, Id(ZaNpcs::NPC_FIRE_BOMB), JANALAI_FIRE_BOMB_SEARCH_RADIUS);

    GuidVector guids;
    guids.reserve(creatureList.size());
    for (Creature* creature : creatureList)
    {
        if (creature && creature->IsAlive())
            guids.push_back(creature->GetGUID());
    }

    return guids;
}

std::vector<Unit*> GetNearbyFireBombs(PlayerbotAI* botAI)
{
    GuidVector const& guids =
        botAI->GetAiObjectContext()->GetValue<GuidVector>("jan'alai fire bombs")->RefGet();

    std::vector<Unit*> bombs;
    bombs.reserve(guids.size());
    for (ObjectGuid const& guid : guids)
    {
        Unit* unit = botAI->GetUnit(guid);
        if (unit && unit->IsAlive())
            bombs.push_back(unit);
    }

    return bombs;
}

//By leewheel 2026-09-04: 上游14413ee2——Jan'alai专用安全步搜索(凸多边形边界+危害精确距离, 步点经CanTakeStepTowards校验)
// Shortest move to a spot that is safe from Fire Bombs, within the measured area
bool FindSafeStepInJanalaiZone(
    Player* bot, std::vector<Unit*> const& hazards, std::vector<Position> const& safeZone,
    float maxSearchDistance, float hazardRadius, float moveDist,
    float& stepX, float& stepY, float& stepZ)
{
    //By leewheel 2026-09-04: 对齐上游d5ad6b79——浮点累加循环改为整数圈/角度计数，避免浮点误差导致漏扫或多扫
    constexpr uint8 angleCount = 16;
    constexpr float angleStep = 2.0f * float(M_PI) / angleCount;
    constexpr float distanceStep = 1.0f;

    uint32 const ringCount = static_cast<uint32>(maxSearchDistance / distanceStep);

    for (uint32 ring = 1; ring <= ringCount; ++ring)
    {
        float const distance = ring * distanceStep;
        for (uint8 i = 0; i < angleCount; ++i)
        {
            float const angle = i * angleStep;
            float const x = bot->GetPositionX() + distance * std::cos(angle);
            float const y = bot->GetPositionY() + distance * std::sin(angle);

            if (!IsInsideSafeZone(safeZone, x, y))
                continue;

            if (!IsPositionSafeFromHazards(x, y, hazards, hazardRadius))
                continue;

            if (!CanTakeStepTowards(bot, x, y, moveDist, stepX, stepY, stepZ))
                continue;

            return true;
        }
    }

    return false;
}
//End By leewheel

// Halazzi <Lynx Avatar>
// N/A

// Hex Lord Malacrass

//By leewheel 2026-09-04: 上游14413ee2——冰霜陷阱GUID缓存值(触发器与动作每tick各查一次, 一次网格搜索服务两处)
ObjectGuid FindNearbyFreezingTrapGuid(Player* bot)
{
    if (bot->GetMapId() != ZA_MAP_ID)
        return ObjectGuid::Empty;

    GameObject* trap = bot->FindNearestGameObject(
        Id(ZaObjects::GO_FREEZING_TRAP), ZA_FREEZING_TRAP_SEARCH_RADIUS, true);

    return trap ? trap->GetGUID() : ObjectGuid::Empty;
}

GameObject* GetNearbyFreezingTrap(PlayerbotAI* botAI)
{
    ObjectGuid const guid = botAI->GetAiObjectContext()
        ->GetValue<ObjectGuid>("hex lord malacrass freezing trap")->Get();

    return guid.IsEmpty() ? nullptr : botAI->GetGameObject(guid);
}
//End By leewheel

// Zul'jin

//By leewheel 2026-09-04: 上游14413ee2——Zul'jin专用站位索引(治疗优先占前两槽, 死亡成员保留槽位)
bool GetZuljinSpreadSlotIndex(Player* bot, size_t slotCount, size_t& slotIndex)
{
    if (slotCount == 0)
        return false;

    Group* group = bot->GetGroup();
    if (!group)
        return false;

    std::vector<Player*> healers;
    std::vector<Player*> rangedDps;
    for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
    {
        Player* member = ref->GetSource();
        if (!member || member->GetMapId() != ZA_MAP_ID || !PlayerbotAI::IsRanged(member))
            continue;

        if (PlayerbotAI::IsHeal(member))
            healers.push_back(member);
        else
            rangedDps.push_back(member);
    }

    auto const healerIt = std::find(healers.begin(), healers.end(), bot);
    if (healerIt != healers.end())
    {
        slotIndex = static_cast<size_t>(std::distance(healers.begin(), healerIt)) % slotCount;
        return true;
    }

    auto const dpsIt = std::find(rangedDps.begin(), rangedDps.end(), bot);
    if (dpsIt == rangedDps.end())
        return false;

    // 治疗占据列表头部，dps 序号接着治疗往后排
    size_t const ordinal =
        healers.size() + static_cast<size_t>(std::distance(rangedDps.begin(), dpsIt));
    slotIndex = ordinal % slotCount;
    return true;
}
//End By leewheel

}
