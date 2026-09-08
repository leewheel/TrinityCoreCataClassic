/* 副本机器人策略 */
#include "ZAActions.h"
#include "EncounterHelpers.h"
#include "Playerbots.h"
#include "RtiTargetValue.h"
#include "ZAHelpers.h"
//By leewheel 2026-08-22: 随上游(9e8e7718..70b60954)补充标准库头文件
#include <algorithm>
#include <array>
#include <iterator>
#include <vector>

using namespace ZaHelpers;
using namespace EncounterHelpers;

// General

bool ZulAmanResetEncounterStatesAction::Execute(Event /*event*/)
{
    bool reset = false;
    reset |= akilzonStormTimer.erase(bot->GetInstanceId()) > 0;
    //By leewheel 2026-09-04: 上游7a2787f3——仅非战斗时清除标记(战斗中继续用骷髅/月亮标记目标)
    if (!AI_VALUE2(bool, "combat", "self target"))
    {
        reset |= ClearTargetIcon(bot, RtiTargetValue::skullIndex);
        reset |= ClearTargetIcon(bot, RtiTargetValue::moonIndex);
    }
    //End By leewheel

    return reset;
}

bool ZulAmanMisdirectBossToMainTankAction::Execute(Event /*event*/)
{
    Unit* boss = AI_VALUE(Unit*, "boss target");
    if (!boss)
        return false;

    Player* mainTank = GetGroupMainTank(bot);
    if (!mainTank)
        return false;

    if (botAI->CanCastSpell("misdirection", mainTank))
        return botAI->CastSpell("misdirection", mainTank);

    if (!bot->HasAura(Id(ZaSpells::SPELL_MISDIRECTION)))
        return false;

    return botAI->CanCastSpell("steady shot", boss) && botAI->CastSpell("steady shot", boss);
}

bool ZulAmanTanksPositionBossAction::Execute(Event /*event*/)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", _bossName);
    if (!boss)
        return false;

    if (AI_VALUE(Unit*, "current target") != boss && PlayerbotAI::IsMainTank(bot))
        return Attack(boss);

    if (boss->GetVictim() != bot || !bot->IsWithinMeleeRange(boss))
        return false;

    constexpr float arrivalDist = 2.0f;
    float moveX;
    float moveY;
    bool backwards;
    if (!GetStepToPosition(bot, _position, arrivalDist, boss, moveX, moveY, backwards))
        return false;

    return MoveTo(
        ZA_MAP_ID, moveX, moveY, bot->GetPositionZ(), false, false, false, false,
        MovementPriority::MOVEMENT_COMBAT, true, backwards);
}

bool ZulAmanSpreadRangedAction::Execute(Event /*event*/)
{
    float minDistance = _minDistance;
    Player* nearestPlayer = GetNearestPlayerInRadius(bot, minDistance);
    return nearestPlayer && FleePosition(nearestPlayer->GetPosition(), minDistance);
}

bool ZulAmanRunAwayFromWhirlwindAction::Execute(Event /*event*/)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", _bossName);
    if (!boss)
        return false;

    float const currentDistance = bot->GetExactDist2d(boss);
    if (currentDistance >= ZA_WHIRLWIND_SAFE_DISTANCE)
        return false;

    bot->CastStop();
    return MoveAway(boss, ZA_WHIRLWIND_SAFE_DISTANCE - currentDistance);
}

// Trash

//By leewheel 2026-08-22: 随上游(9e8e7718..70b60954)——守护图腾改走FindNearestCreature, 误导后稳固射击判定重构
bool AmanishiMedicineManMarkWardAction::Execute(Event /*event*/)
{
    constexpr float searchRadius = 40.0f;

    Creature* protectiveWard = bot->FindNearestCreature(
        Id(ZaNpcs::NPC_AMANI_PROTECTIVE_WARD), searchRadius); //By leewheel 2026-09-04: 上游89a4c459——第三参默认true
    if (protectiveWard)
        return MarkTargetWithSkull(bot, protectiveWard);

    Creature* healingWard = bot->FindNearestCreature(
        Id(ZaNpcs::NPC_AMANI_HEALING_WARD), searchRadius);
    return healingWard && MarkTargetWithSkull(bot, healingWard);
}
//End By leewheel

// Akil'zon <Eagle Avatar>

bool AkilzonMoveToEyeOfTheStormAction::Execute(Event /*event*/)
{
    Player* target = GetElectricalStormTarget(bot);
    if (!target && !PlayerbotAI::IsMainTank(bot))
        target = GetGroupMainTank(bot);

    if (!target || bot->GetExactDist2d(target) <= 3.0f)
        return false;

    bot->CastStop();
    return MoveTo(
        ZA_MAP_ID, target->GetPositionX(), target->GetPositionY(), bot->GetPositionZ(),
        false, false, false, false, MovementPriority::MOVEMENT_FORCED, true, false);
}

bool AkilzonManageElectricalStormTimerAction::Execute(Event /*event*/)
{
    return akilzonStormTimer.try_emplace(bot->GetInstanceId(), getMSTime()).second;
}

// Nalorakk <Bear Avatar>

//By leewheel 2026-08-22: 随上游(9e8e7718..70b60954)——职责判定提前到目标检查之后
bool NalorakkTanksPositionBossAction::Execute(Event /*event*/)
{
    Unit* nalorakk = AI_VALUE2(Unit*, "find target", "23576");
    if (!nalorakk)
        return false;

    // Main tank takes bear, assist tank takes troll
    Player* nalorakkTank = nullptr;
    if (IsNalorakkInBearForm(nalorakk))
        nalorakkTank = GetGroupMainTank(bot);
    else
        nalorakkTank = GetGroupAssistTank(bot, 0);

    if (nalorakkTank && nalorakkTank == bot)
    {
        if (AI_VALUE(Unit*, "current target") != nalorakk)
            return Attack(nalorakk);

        if (nalorakk->GetVictim() != bot)
            return botAI->DoSpecificAction("taunt spell", Event(), true);

        if (!bot->IsWithinMeleeRange(nalorakk))
            return false;
    }

    //By leewheel 2026-09-04: 上游14413ee2——两坦克都要走到位但仅持有纳洛拉克者倒退, 方向判定交给GetStepToPosition
    constexpr float arrivalDist = 2.0f;
    float moveX;
    float moveY;
    bool backwards;
    if (!GetStepToPosition(
            bot, NALORAKK_TANK_POSITION, arrivalDist, nalorakk, moveX, moveY, backwards))
    {
        return false;
    }

    return MoveTo(
        ZA_MAP_ID, moveX, moveY, bot->GetPositionZ(), false, false, false, false,
        MovementPriority::MOVEMENT_COMBAT, true, backwards);
}

// Jan'alai <Dragonhawk Avatar>

bool JanalaiSpreadRangedInCircleAction::Execute(Event /*event*/)
{
    Group* group = bot->GetGroup();
    if (!group)
        return false;

    std::vector<Player*> rangedMembers;
    for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
    {
        Player* member = ref->GetSource();
        if (!member || !PlayerbotAI::IsRanged(member))
            continue;

        rangedMembers.push_back(member);
    }

    //By leewheel 2026-09-04: 上游a8cc34f9——环绕半径用常量(与JanalaiAvoidFireBombs的安全距离共同可调)
    if (rangedMembers.empty())
        return false;

    auto findIt = std::find(rangedMembers.begin(), rangedMembers.end(), bot);
    size_t botIndex =
        (findIt != rangedMembers.end()) ? std::distance(rangedMembers.begin(), findIt) : 0;
    size_t count = rangedMembers.size();
    float angle = (count == 1) ? 0.0f
        : (2.0f * float(M_PI) * static_cast<float>(botIndex) / static_cast<float>(count)); //By leewheel 2026-09-03 修复C4305警告

    float targetX =
        JANALAI_TANK_POSITION.GetPositionX() + JANALAI_RANGED_SPREAD_RADIUS * std::cos(angle);
    float targetY =
        JANALAI_TANK_POSITION.GetPositionY() + JANALAI_RANGED_SPREAD_RADIUS * std::sin(angle);
    //End By leewheel

    if (bot->GetExactDist2d(targetX, targetY) <= 2.0f)
        return false;

    return MoveTo(
        ZA_MAP_ID, targetX, targetY, bot->GetPositionZ(), false, false, false, false,
        MovementPriority::MOVEMENT_COMBAT, true, false);
}

bool JanalaiAvoidFireBombsAction::Execute(Event /*event*/)
{
    auto const& bombs = GetNearbyFireBombs(botAI);

    if (bombs.empty())
        return false;

    //By leewheel 2026-09-04: 上游14413ee2——危险判定改精确距离+安全半径常量化(5.5y), 搜索距离常量化(30y)
    bool inDanger = false;
    for (Unit* bomb : bombs)
    {
        if (bot->GetExactDist2d(bomb) < JANALAI_FIRE_BOMB_SAFE_DISTANCE)
        {
            inDanger = true;
            break;
        }
    }

    if (!inDanger)
        return false;

    constexpr float maxFloorDeviation = 5.0f;
    if (std::fabs(bot->GetPositionZ() - JANALAI_PLATFORM_Z) > maxFloorDeviation)
        return false;

    constexpr float moveDist = 3.5f;

    float stepX, stepY, stepZ;
    if (!FindSafeStepInJanalaiZone(
            bot, bombs, JANALAI_SAFE_ZONE, JANALAI_FIRE_BOMB_MAX_SEARCH_DISTANCE,
            JANALAI_FIRE_BOMB_SAFE_DISTANCE, moveDist, stepX, stepY, stepZ))
    {
        return false;
    }

    bot->CastStop();
    return MoveTo(
        ZA_MAP_ID, stepX, stepY, stepZ, false, false, false, false,
        MovementPriority::MOVEMENT_FORCED, true, false);
    //End By leewheel
}

bool JanalaiMarkAmanishiHatchersAction::Execute(Event /*event*/)
{
    auto [hatcherLow, hatcherHigh] = GetAmanishiHatcherPair(botAI);

    if (!hatcherLow || !hatcherHigh || hatcherHigh == hatcherLow)
        return false;

    return MarkTargetWithMoon(bot, hatcherHigh) || MarkTargetWithSkull(bot, hatcherLow);
}

// Halazzi <Lynx Avatar>

bool HalazziFirstAssistTankAttackSpiritLynxAction::Execute(Event /*event*/)
{
    Unit* lynx = AI_VALUE2(Unit*, "find target", "24143");
    if (lynx)
    {
        if (AI_VALUE(Unit*, "current target") != lynx)
            return Attack(lynx);

        if (lynx->GetVictim() != bot && botAI->DoSpecificAction("taunt spell", Event(), true))
            return true;
    }

    if (lynx && lynx->GetVictim() != bot)
        return false;

    Position const& position = HALAZZI_TANK_POSITION;
    float const distToPosition = bot->GetExactDist2d(position);
    if (distToPosition <= 2.0f)
        return false;

    return MoveTo(
        ZA_MAP_ID, position.GetPositionX(), position.GetPositionY(), bot->GetPositionZ(),
        false, false, false, false, MovementPriority::MOVEMENT_COMBAT, true, false);
}

bool HalazziDpsAttackTotemAndBossAction::Execute(Event /*event*/)
{
    // Target priority 1: Corrupted Lightning Totems
    constexpr float searchRadius = 40.0f;
    if (Creature* totem =
            bot->FindNearestCreature(Id(ZaNpcs::NPC_CORRUPTED_LIGHTNING_TOTEM), searchRadius))
    {
        if (MarkTargetWithSkull(bot, totem))
            return true;

        return AI_VALUE(Unit*, "current target") != totem && Attack(totem);
    }

    // Target priority 2: Halazzi
    if (Unit* halazzi = AI_VALUE2(Unit*, "find target", "23577"))
        return AI_VALUE(Unit*, "current target") != halazzi && Attack(halazzi);

    // 不要攻击山猫之灵
    return false;
}

// Hex Lord Malacrass

bool HexLordMalacrassAssignDpsPriorityAction::Execute(Event /*event*/)
{
    static constexpr std::array hexLordAdds = {
        Id(ZaNpcs::NPC_LORD_RAADAN),
        Id(ZaNpcs::NPC_ALYSON_ANTILLE),
        Id(ZaNpcs::NPC_KORAGG),
        Id(ZaNpcs::NPC_DARKHEART),
        Id(ZaNpcs::NPC_FENSTALKER),
        Id(ZaNpcs::NPC_GAZAKROTH),
        Id(ZaNpcs::NPC_THURG),
        Id(ZaNpcs::NPC_SLITHER),
        Id(ZaNpcs::NPC_HEX_LORD_MALACRASS)
    };

    //By leewheel 2026-09-04: 上游14413ee2——单次遍历按优先级序数选最高目标(替代外层entry循环)
    Unit* priorityTarget = nullptr;
    size_t bestRank = hexLordAdds.size();
    for (auto const& guid : AI_VALUE(GuidVector, "possible targets no los"))
    {
        Unit* unit = botAI->GetUnit(guid);
        if (!unit || !unit->IsAlive())
            continue;

        auto const it = std::find(hexLordAdds.begin(), hexLordAdds.end(), unit->GetEntry());
        size_t const rank = std::distance(hexLordAdds.begin(), it);
        if (rank < bestRank)
        {
            bestRank = rank;
            priorityTarget = unit;
        }
    }

    return priorityTarget && MarkTargetWithSkull(bot, priorityTarget);
    //End By leewheel
}

bool HexLordMalacrassMoveAwayFromFreezingTrapAction::Execute(Event /*event*/)
{
    //By leewheel 2026-09-04: 上游14413ee2——陷阱走200ms缓存值; 安全距离11y常量化; 距离判定改精确2维
    GameObject* trap = GetNearbyFreezingTrap(botAI);
    //End By leewheel

    if (!trap)
        return false;

    float const currentDistance = bot->GetExactDist2d(trap);
    if (currentDistance >= ZA_FREEZING_TRAP_SAFE_DISTANCE)
        return false;

    constexpr uint32 minInterval = 0;
    return FleePosition(trap->GetPosition(), ZA_FREEZING_TRAP_SAFE_DISTANCE, minInterval);
}

// Zul'jin

bool ZuljinSpreadRaidForCyclonesAction::Execute(Event /*event*/)
{
    size_t slotIndex;
    //By leewheel 2026-09-04: 上游14413ee2——改用Zul'jin专用站位索引
    if (!GetZuljinSpreadSlotIndex(bot, ZULJIN_SPREAD_POSITIONS.size(), slotIndex))
        return false;
    //End By leewheel

    Position const& position = ZULJIN_SPREAD_POSITIONS[slotIndex];

    constexpr float arrivalDist = 2.0f;
    float moveX;
    float moveY;
    bool backwards;
    if (!GetStepToPosition(bot, position, arrivalDist, nullptr, moveX, moveY, backwards))
        return false;

    return MoveTo(
        ZA_MAP_ID, moveX, moveY, bot->GetPositionZ(), false, false, false, false,
        MovementPriority::MOVEMENT_COMBAT, true, backwards);
}
