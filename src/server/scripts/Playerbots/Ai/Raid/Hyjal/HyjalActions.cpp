/* 海加尔山 机器人策略 */
/*By leewheel 2026-08-18: 对齐 brighton-chi the-lab HEAD(e92a52db)——
  移植 8df2134e/33f67223/0a3a0367/7f293f95/ab50f6a1/fdefbb16/47abcff2/fde72295/5f34b67d:
  - 误导/主坦站位收敛为通用 HyjalMisdirectBossToMainTankAction/HyjalMainTankPositionBossAction(压缩)
  - Anetheron 误导接战阈值 BOSS_ENGAGED_HEALTH_PCT、地狱火Immersion中心距判定 GetExactDist2d
  - Archimonde 地狱火完整体系: 全字段加权避让/被围扇形逃生(两轮验证)/月神之佑/危险半径收窄(3→2y)
  - 5f34b67d 危害缓存: GetDoomfirePositions 等走 "hyjal doomfire trail" 值
  保留本地改进: ResetEncounterStates 按本实例全部玩家清理 GUID-keyed 状态(GetInstancePlayerGuids)。*/
#include "HyjalActions.h"
#include "HyjalHelpers.h"
#include "EncounterHelpers.h"
#include "Playerbots.h"
#include <algorithm>
#include <cmath>
#include <iterator>
#include <vector>

using namespace HyjalHelpers;
using namespace EncounterHelpers;

// Every mover here walks in short steps rather than handing MoveTo a far destination, and the Z it
// seeds MoveTo with is always the bot's own. That is not a detail: MoveTo seeds SearchForBestPath
// with the Z it is given, and the search resolves a point only a few yards ahead. Passing the
// destination's Z instead asks it to reconcile ground fifty yards away with ground under the bot's
// feet, which on Hyjal's terrain fails outright--MoveTo returns false and the bot simply stands
// there, with no error and nothing suppressing it. The destination's Z belongs in the arrival test,
// never in the step
// General

bool HyjalSummitResetEncounterStatesAction::Execute(Event /*event*/)
{
    bool reset = false;

    Action* winterchillAction = context->GetAction("rage winterchill spread ranged in circle");
    if (winterchillAction && static_cast<RageWinterchillSpreadRangedInCircleAction*>(
            winterchillAction)->ResetWinterchillPositionReached())
    {
        reset = true;
    }

    Action* anetheronAction = context->GetAction("anetheron spread ranged in circle");
    if (anetheronAction && static_cast<AnetheronSpreadRangedInCircleAction*>(
            anetheronAction)->ResetAnetheronPositionReached())
    {
        reset = true;
    }

    reset |= botsBelowManaThreshold.erase(bot->GetGUID()) > 0;
    reset |= archimondeAirBurstTargets.erase(bot->GetInstanceId()) > 0;

    return reset;
}

bool HyjalMisdirectBossToMainTankAction::Execute(Event /*event*/)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", _bossName);
    if (!boss)
        return false;

    Player* mainTank = GetGroupMainTank(bot);
    //By leewheel 2026-08-21: 移植上游——主坦死亡时不误导(尸体无法承接误导)
    //By leewheel 2026-08-23: the-lab 更新为单参数 GetGroupMainTank API
    if (!mainTank || !mainTank->IsAlive())
        return false;
    //End By leewheel

    if (botAI->CanCastSpell("misdirection", mainTank))
        return botAI->CastSpell("misdirection", mainTank);

    if (bot->HasAura(Id(HyjalSpells::SPELL_MISDIRECTION)) &&
        botAI->CanCastSpell("steady shot", boss))
    {
        return botAI->CastSpell("steady shot", boss);
    }

    return false;
}

bool HyjalMainTankPositionBossAction::Execute(Event /*event*/)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", _bossName);
    if (!boss)
        return false;

    if (AI_VALUE(Unit*, "current target") != boss)
        return Attack(boss);

    if (boss->GetVictim() != bot || !bot->IsWithinMeleeRange(boss))
        return false;

    if (bot->GetHealthPct() < _bailBelowHealthPct)
        return false;

    //By leewheel 2026-09-04: 上游4f9815d1/6d5d68cd——步进计算改用共享助手 EncounterHelpers::GetStepToPosition
    //(内部含到达距离判定与后退步判定)
    float moveX;
    float moveY;
    bool backwards;
    if (!GetStepToPosition(bot, _position, 4.0f, boss, moveX, moveY, backwards))
        return false;
    //End By leewheel

    return MoveTo(
        HYJAL_MAP_ID, moveX, moveY, bot->GetPositionZ(), false, false, false, false,
        MovementPriority::MOVEMENT_COMBAT, true, backwards);
}

// Rage Winterchill

// This is essentially a forced "avoid aoe" due to the default AiPlayerbot.MaxAoeAvoidRadius in the
// config being 15 yards; avoid aoe works fine without this strategy if it is set to 20+ yards.
bool RageWinterchillRangedGetOutOfDeathAndDecayAction::Execute(Event /*event*/)
{
    Position pool;
    if (!GetDeathAndDecayPosition(bot, pool))
        return false;

    constexpr uint32 minInterval = 0;
    return FleePosition(pool, DEATH_AND_DECAY_RADIUS, minInterval);
}

// Spread ranged DPS in a circle initially. After the initial spread, movement is free.
bool RageWinterchillSpreadRangedInCircleAction::Execute(Event /*event*/)
{
    if (_winterchillPositionReached)
        return false;

    RangedGroups groups = GetRangedGroups(bot);
    auto [botIndex, count] = GetBotCircleIndexAndCount(bot, groups);
    if (count == 0)
        return false;

    float const radius = PlayerbotAI::IsHeal(bot) ? 25.0f : 35.0f;
    constexpr float arcSpan = 2.0f * float(M_PI); //By leewheel 2026-09-03 修复C4305警告
    constexpr float arcCenter = 0.0f;
    constexpr float arcStart = arcCenter - arcSpan / 2.0f;

    float const angle = (count == 1) ? arcCenter :
        (arcStart + arcSpan * static_cast<float>(botIndex) / static_cast<float>(count));

    // The assigned angle only has to be roughly right--all this is doing is keeping ranged apart--
    // so a point that cannot be reached is worth abandoning for its neighbour rather than walking
    // at forever. Ranged are close enough together that swapping arcs with someone costs nothing
    Position const& position = WINTERCHILL_TANK_POSITION;
    constexpr float moveDist = 3.5f;
    float moveX, moveY, moveZ, chosenX, chosenY;
    if (!FindStepToCircle(bot, position, radius, angle, moveDist, moveX, moveY, moveZ, {},
                          &chosenX, &chosenY))
    {
        // Nowhere on the ring can be reached at all, so settle for where the bot stands rather
        // than spend the fight asking again and contending with everything else that wants to
        // move it. Being spread is a preference here, not a requirement
        _winterchillPositionReached = true;
        return false;
    }

    // Measured against the point actually being walked to. A bot that had to settle for a
    // neighbouring angle is finished when it gets there, not left asking forever for one it
    // cannot reach
    if (bot->GetExactDist2d(chosenX, chosenY) <= 2.0f)
    {
        _winterchillPositionReached = true;
        return false;
    }

    return MoveTo(
        HYJAL_MAP_ID, moveX, moveY, moveZ, false, false, false, false,
        MovementPriority::MOVEMENT_COMBAT, true, false);
}

// Melee looks for an open position within the boss's melee range. If one isn't available (likely
// the case if D&D lands on melee, with its 20y radius), then melee takes the shortest path out of
// the hazard and waits it out.
//
// Two jobs, since the suppression that comes with this action reaches past the pool itself: inside
// the pool it is an escape, and from there out to DEATH_AND_DECAY_MELEE_CONTROL_RADIUS it is the
// only thing that can walk the bot back onto Winterchill's ring
bool RageWinterchillMeleeManeuverThroughDeathAndDecayAction::Execute(Event /*event*/)
{
    Unit* winterchill = AI_VALUE2(Unit*, "find target", "17767");
    if (!winterchill)
        return false;

    Position pool;
    if (!GetDeathAndDecayPosition(bot, pool))
        return false;

    constexpr float moveDist = 10.0f;
    float moveX, moveY, moveZ;

    float const meleeRadius = bot->GetMeleeRange(winterchill) - MELEE_RANGE_INSET;

    std::vector<BlockedArc> blocked;
    BlockedArc poolArc;
    if (GetHazardBlockedArc(
            winterchill->GetPosition(), meleeRadius, pool, DEATH_AND_DECAY_RADIUS, poolArc))
    {
        blocked.push_back(poolArc);
    }

    float const bossX = winterchill->GetPositionX();
    float const bossY = winterchill->GetPositionY();
    float const botHeading = std::atan2(bot->GetPositionY() - bossY, bot->GetPositionX() - bossX);

    float standAngle;
    if (FindNearestUnblockedAngle(blocked, botHeading, standAngle))
    {
        float const targetX = bossX + std::cos(standAngle) * meleeRadius;
        float const targetY = bossY + std::sin(standAngle) * meleeRadius;
        float const distToTarget = bot->GetExactDist2d(targetX, targetY);

        constexpr float minStepDistance = 0.5f;
        if (distToTarget < minStepDistance)
            return false;

        float const stepDist = std::min(moveDist, distToTarget);
        float const botX = bot->GetPositionX();
        float const botY = bot->GetPositionY();

        return MoveTo(
            HYJAL_MAP_ID, botX + ((targetX - botX) / distToTarget) * stepDist,
            botY + ((targetY - botY) / distToTarget) * stepDist, bot->GetPositionZ(),
            false, false, false, false, MovementPriority::MOVEMENT_COMBAT, true, false);
    }

    // No heading on the ring is open. Fleeing is the answer only while the bot is actually in the
    // pool: the escape aims at a point on a circle drawn round it, so a bot that has already
    // cleared that circle would be walked back inward toward it. Standing still is better--the
    // ring reopens on its own as Winterchill is dragged or the pool expires
    if (!IsInDeathAndDecay(bot))
        return false;

    constexpr float escapeMargin = 2.0f;
    if (!GetHazardEscapeStep(
            bot, pool, DEATH_AND_DECAY_RADIUS + escapeMargin, moveDist, moveX, moveY, moveZ))
    {
        return false;
    }

    return MoveTo(
        HYJAL_MAP_ID, moveX, moveY, moveZ, false, false, false, false,
        MovementPriority::MOVEMENT_COMBAT, true, false);
}

// Anetheron

bool AnetheronMisdirectBossAndInfernalsToTanksAction::Execute(Event /*event*/)
{
    Unit* anetheron = AI_VALUE2(Unit*, "find target", "17808");
    if (!anetheron)
        return false;

    Player* tankTarget = nullptr;
    Unit* enemyTarget = nullptr;
    if (anetheron->GetHealthPct() > BOSS_ENGAGED_HEALTH_PCT)
    {
        tankTarget = GetGroupMainTank(bot);
        enemyTarget = anetheron;
    }
    else if (Unit* infernal = GetLooseInfernal(bot))
    {
        tankTarget = GetInfernalTank(bot);
        enemyTarget = infernal;
    }

    //By leewheel 2026-08-21: 移植上游——误导目标(主坦/地狱火坦)死亡时不误导
    if (!tankTarget || !enemyTarget || !tankTarget->IsAlive())
        return false;
    //End By leewheel

    if (botAI->CanCastSpell("misdirection", tankTarget))
        return botAI->CastSpell("misdirection", tankTarget);

    if (bot->HasAura(Id(HyjalSpells::SPELL_MISDIRECTION)) &&
        botAI->CanCastSpell("steady shot", enemyTarget))
    {
        return botAI->CastSpell("steady shot", enemyTarget);
    }

    return false;
}

bool AnetheronSpreadRangedInCircleAction::Execute(Event /*event*/)
{
    if (_anetheronPositionReached)
    {
        constexpr float safeDistFromPlayer = 6.0f;
        constexpr uint32 minInterval = 2000;
        if (Player* nearestPlayer = GetNearestPlayerInRadius(bot, safeDistFromPlayer))
            return FleePosition(nearestPlayer->GetPosition(), safeDistFromPlayer, minInterval);

        return false;
    }

    RangedGroups groups = GetRangedGroups(bot);
    auto [botIndex, count] = GetBotCircleIndexAndCount(bot, groups);
    if (count == 0)
        return false;

    float const radius = PlayerbotAI::IsHeal(bot) ? 27.0f : 34.0f;
    constexpr float arcSpan = float(M_PI) * 2.0f; //By leewheel 2026-09-03 修复C4305警告
    constexpr float arcCenter = 0.0f;
    constexpr float arcStart = arcCenter - arcSpan / 2.0f;

    float const angle = (count == 1) ? arcCenter :
        (arcStart + arcSpan * static_cast<float>(botIndex) / static_cast<float>(count));

    Position const& position = ANETHERON_TANK_POSITION;

    // The circle was laid out with sin for X and cos for Y here, mirroring Winterchill's
    // convention. Over a full circle of evenly spaced points that maps the set onto itself, so the
    // ring is unchanged and only which bot stands where differs
    constexpr float moveDist = 3.5f;
    float moveX, moveY, moveZ, chosenX, chosenY;
    if (!FindStepToCircle(bot, position, radius, angle, moveDist, moveX, moveY, moveZ, {},
                          &chosenX, &chosenY))
    {
        // As at Winterchill: no reachable angle at all means settle for where the bot stands
        _anetheronPositionReached = true;
        return false;
    }

    if (bot->GetExactDist2d(chosenX, chosenY) <= 2.0f)
    {
        _anetheronPositionReached = true;
        return false;
    }

    return MoveTo(
        HYJAL_MAP_ID, moveX, moveY, moveZ, false, false, false, false,
        MovementPriority::MOVEMENT_COMBAT, true, false);
}

// Everyone standing near whoever Inferno is aimed at is about to be stunned, since the Infernal
// lands on that player's feet. The 3.5s cast is the whole window, and stepping out of it also
// starts the bot clear of the immolation aura the Infernal carries afterwards
bool AnetheronMoveAwayFromInfernoTargetAction::Execute(Event /*event*/)
{
    Unit* anetheron = AI_VALUE2(Unit*, "find target", "17808");
    if (!anetheron)
        return false;

    Player* infernoTarget = GetInfernoTarget(anetheron);
    if (!infernoTarget || infernoTarget == bot)
        return false;

    constexpr uint32 minInterval = 0;
    return FleePosition(infernoTarget->GetPosition(), INFERNAL_ESCAPE_DISTANCE, minInterval);
}

// Infernals cannot be taunted, so nothing the tank does will pull one off its victim. What moves
// an Infernal is its victim walking, and the summon itself lands wherever its target stands when
// the 3.5s cast ends. Both cases are the same job: carry it to the gathering spot
bool AnetheronBringInfernalToInfernalTankAction::Execute(Event /*event*/)
{
    Position const& position = GetInfernalTankPosition(bot);
    float const distToPosition = bot->GetExactDist2d(position);

    if (distToPosition <= 2.0f)
        return false;

    float const botX = bot->GetPositionX();
    float const botY = bot->GetPositionY();
    constexpr float maxMoveDist = 3.5f;
    float const moveDist = std::min(maxMoveDist, distToPosition);
    float const moveX = botX + ((position.GetPositionX() - botX) / distToPosition) * moveDist;
    float const moveY = botY + ((position.GetPositionY() - botY) / distToPosition) * moveDist;

    return MoveTo(
        HYJAL_MAP_ID, moveX, moveY, bot->GetPositionZ(), false, false,
        false, false, MovementPriority::MOVEMENT_FORCED, true, false);
}

// Stand where the Infernals are being gathered and let stock tank assist work through whatever has
// arrived. There is no pick-up to perform: Infernals are immune to taunt, so holding them is plain
// threat work, and the exclusions in the strategy are what keep this bot on them and off Anetheron
bool AnetheronInfernalTankTakePositionAction::Execute(Event /*event*/)
{
    Position const& position = GetInfernalTankPosition(bot);
    float const distToPosition = bot->GetExactDist2d(position);

    if (distToPosition <= 3.0f)
        return false;

    //By leewheel 2026-09-04: 上游4f9815d1/6d5d68cd——步进计算改用共享助手 EncounterHelpers::GetStepToPosition
    //(后退步判定: 背离当前持有地狱火的方向, 使地狱火保持在面前跟随)
    float moveX;
    float moveY;
    bool backwards;
    if (!GetStepToPosition(bot, position, 3.0f, GetInfernalTargetingBot(bot), moveX, moveY, backwards))
        return false;
    //End By leewheel

    return MoveTo(
        HYJAL_MAP_ID, moveX, moveY, bot->GetPositionZ(), false, false,
        false, false, MovementPriority::MOVEMENT_COMBAT, true, backwards);
}

//By leewheel 2026-08-30: 移植上游——AnetheronGetOutOfImmolationAction 实现（活地狱火对10码内持续灼烧，非持有者离开）
bool AnetheronGetOutOfImmolationAction::Execute(Event /*event*/)
{
    Unit* infernal = GetNearestInfernal(bot);
    if (!infernal || infernal->GetVictim() == bot)
        return false;

    constexpr uint32 minInterval = 0;
    return FleePosition(infernal->GetPosition(), INFERNAL_DANGER_RADIUS, minInterval);
}
//End By leewheel

// Melee stay on Anetheron throughout. Ranged attack Infernals if they are reasonably nearby.
bool AnetheronAssignDpsPriorityAction::Execute(Event /*event*/)
{
    Unit* anetheron = AI_VALUE2(Unit*, "find target", "17808");
    if (!anetheron)
        return false;

    // Centre to centre, as the Immolation itself measures: it is cast by the creature, which is the
    // case the post-#26967 area check adds no combat reach for. GetDistance would subtract both
    // object sizes, and a Towering Infernal's is not small--the bot would be fleeing from well
    // outside the aura, and since FleePosition reports success on any tick it moves, this would
    // return before reaching the targeting below and leave the bot without a target while it ran
    if (Unit* nearest = GetNearestInfernal(bot))
    {
        constexpr uint32 minInterval = 0;
        if (nearest->GetVictim() != bot &&
            bot->GetExactDist2d(nearest) < INFERNAL_DANGER_RADIUS)
        {
            return FleePosition(nearest->GetPosition(), INFERNAL_DANGER_RADIUS, minInterval);
        }
    }

    if (PlayerbotAI::IsMelee(bot) || PlayerbotAI::IsHeal(bot))
    {
        if (AI_VALUE(Unit*, "current target") != anetheron)
            return Attack(anetheron);

        return false;
    }

    Unit* infernal = GetFocusedInfernal(botAI);
    if (infernal && anetheron->GetHealthPct() > 10.0f &&
        bot->GetDistance2d(infernal) < 50.0f)
    {
        // Wait for the tank to pick up the Infernal before attacking directly
        Player* infernalTank = GetInfernalTank(bot);
        if (!infernalTank || infernal->GetVictim() == infernalTank)
        {
            if (AI_VALUE(Unit*, "current target") != infernal)
                return Attack(infernal);

            return false;
        }
    }

    if (AI_VALUE(Unit*, "current target") != anetheron)
        return Attack(anetheron);

    return false;
}

// Kaz'rogal
// CombatReach is 7.875 yards

bool KazrogalAssistTanksMoveInFrontAction::Execute(Event /*event*/)
{
    Player* mainTank = GetGroupMainTank(bot);
    if (!mainTank)
        return false;

    constexpr float arrivalDist = 4.0f;
    float moveX;
    float moveY;
    bool backwards;
    if (!GetStepToPosition(
            bot, mainTank->GetPosition(), arrivalDist, nullptr, moveX, moveY, backwards))
    {
        return false;
    }

    return MoveTo(
        HYJAL_MAP_ID, moveX, moveY, bot->GetPositionZ(), false, false,
        false, false, MovementPriority::MOVEMENT_COMBAT, true, false);
}

bool KazrogalSpreadRangedInArcAction::Execute(Event /*event*/)
{
    Unit* kazrogal = AI_VALUE2(Unit*, "find target", "17888");
    if (!kazrogal)
        return false;

    std::vector<Player*> const rangedMembers = GetRangedMembers(bot);
    if (rangedMembers.empty())
        return false;

    //By leewheel 2026-08-21: 移植 brighton-chi d0f388e0——不在名单的bot拒绝占位(避免错误归到0号位)
    auto findIt = std::find(rangedMembers.begin(), rangedMembers.end(), bot);
    if (findIt == rangedMembers.end())
        return false;

    size_t const count = rangedMembers.size();
    size_t const botIndex = std::distance(rangedMembers.begin(), findIt);
    //End By leewheel

    float const arcRadius = GetKazrogalRangedArcRadius(kazrogal);
    float const arcSpan = GetKazrogalRangedArcSpan(arcRadius);
    float const arcStart = KAZROGAL_RANGED_ARC_CENTER - arcSpan / 2.0f;

    float angle = (count == 1) ? KAZROGAL_RANGED_ARC_CENTER :
        (arcStart + arcSpan * static_cast<float>(botIndex) / static_cast<float>(count - 1));

    float const targetX = kazrogal->GetPositionX() + arcRadius * std::cos(angle);
    float const targetY = kazrogal->GetPositionY() + arcRadius * std::sin(angle);

    float const distToTarget = bot->GetExactDist2d(targetX, targetY);
    if (distToTarget <= 0.5f)
        return false;

    float const botX = bot->GetPositionX();
    float const botY = bot->GetPositionY();
    constexpr float maxMoveDist = 3.5f;
    float const moveDist = std::min(maxMoveDist, distToTarget);
    float const moveX = botX + ((targetX - botX) / distToTarget) * moveDist;
    float const moveY = botY + ((targetY - botY) / distToTarget) * moveDist;

    return MoveTo(
        HYJAL_MAP_ID, moveX, moveY, bot->GetPositionZ(), false, false,
        false, false, MovementPriority::MOVEMENT_COMBAT, true, false);
}

bool KazrogalMoveAwayFromGroupAction::Execute(Event /*event*/)
{
    if (bot->GetPower(POWER_MANA) > MARK_REJOIN_MANA)
    {
        botsBelowManaThreshold.erase(bot->GetGUID());
        return false;
    }

    Player* nearestPlayer = GetNearestPlayerInRadius(bot, MARK_ESCAPE_DISTANCE);
    if (!nearestPlayer)
        return false;

    float const step = MARK_ESCAPE_DISTANCE - bot->GetExactDist2d(nearestPlayer);

    // 远离离自己最近的玩家。当弧线上并列两个bot时基本是切向，这是分离一对bot最快的方向。
    // 改为直接从卡兹洛加径向逃出几乎无法分离：径向逃生让间隙随半径放大，相距1.5码的相邻
    // bot要跑到营地远端才能拉开到16码。
    //
    // 例外是当该玩家比bot更靠外时——远离它反而会指向raid内部。这时改用卡兹洛加作参照：
    // 径向向外总是有效进度，绝不会向内。两个分支都用MoveAway，它会扫出9个朝向做碰撞检测，
    // 在路障和战争机器常伴两侧的路径上很有必要
    Unit* kazrogal = AI_VALUE2(Unit*, "find target", "17888");
    if (kazrogal && nearestPlayer->GetExactDist2d(kazrogal) > bot->GetExactDist2d(kazrogal))
        return MoveAway(kazrogal, step);

    return MoveAway(nearestPlayer, step);
}

bool KazrogalActivateAspectOfTheViperAction::Execute(Event /*event*/)
{
    return botAI->CanCastSpell(Id(HyjalSpells::SPELL_ASPECT_OF_THE_VIPER), bot) &&
        botAI->CastSpell(Id(HyjalSpells::SPELL_ASPECT_OF_THE_VIPER), bot);
}

bool KazrogalCancelMarkAction::Execute(Event /*event*/)
{
    //By leewheel 2026-09-04 修复: 恢复上游语义——本动作只负责施法开免疫, 删除非治疗先RemoveAura
    //的自剥逻辑(原写法每tick触发时会把刚开出的冰块/圣盾立即剥掉, 而法术5分钟CD无法再开,
    //DPS法师/圣骑士整个Mark期间裸吃蓝烧); 免疫移除职责由"kaz'rogal immunity no longer needed"
    //触发器绑定的KazrogalCancelImmunityAction承担, 并改用GetKazrogalImmunitySpell统一选法术
    uint32 const spellId = GetKazrogalImmunitySpell(bot);
    return spellId && botAI->CanCastSpell(spellId, bot) && botAI->CastSpell(spellId, bot);
}

//By leewheel 2026-08-30: 移植上游——KazrogalCancelImmunityAction 实现（移除冰/圣盾免疫，本地仅有声明无实现导致链接错误）
bool KazrogalCancelImmunityAction::Execute(Event /*event*/)
{
    uint32 const spellId = GetKazrogalImmunitySpell(bot);
    if (!spellId || !bot->HasAura(spellId))
        return false;

    bot->RemoveAura(spellId);
    return true;
}
//End By leewheel

// Life Tap first, because it is the only one of the two that removes the problem rather than
// softening it: mana bought back above the danger line keeps the warlock out of the escape
// entirely. Shadow Ward is what is left once health is too low to trade
bool KazrogalWarlockManageManaAction::Execute(Event /*event*/)
{
    if (bot->GetPower(POWER_MANA) <= MARK_LIFE_TAP_MANA &&
        bot->GetHealthPct() > sPlayerbotAIConfig.lowHealth &&
        botAI->CanCastSpell("life tap", bot))
    {
        return botAI->CastSpell("life tap", bot);
    }

    if (!HasMarkOfKazrogal(bot))
        return false;

    return botAI->CanCastSpell("shadow ward", bot) && botAI->CastSpell("shadow ward", bot);
}

// Azgalor
// CombatReach is 8.8 yards
// Doomguard CombatReach is 3.75 yards

bool AzgalorDisperseRangedAction::Execute(Event /*event*/)
{
    Unit* azgalor = AI_VALUE2(Unit*, "find target", "17842");
    if (!azgalor)
        return false;

    float const safeDistFromBoss = 30.0f; // ~20 yards + boss and bot CombatReaches
    constexpr uint32 minInterval = 0;

    if (bot->GetExactDist2d(azgalor) < safeDistFromBoss &&
        FleePosition(azgalor->GetPosition(), safeDistFromBoss, minInterval))
    {
        return true;
    }

    Unit* doomguard = AI_VALUE2(Unit*, "find target", "17864");
    constexpr float safeDistFromDoomguard = 10.0f; // War Stomp is 10 yards center-to-center

    if (doomguard && bot->GetExactDist2d(doomguard) < safeDistFromDoomguard)
        return FleePosition(doomguard->GetPosition(), safeDistFromDoomguard);

    if (doomguard && AI_VALUE(Unit*, "current target") == doomguard)
        return false;

    constexpr float safeDistFromPlayer = 5.0f;
    Player* nearestPlayer = GetNearestPlayerInRadius(bot, safeDistFromPlayer);
    return nearestPlayer && FleePosition(nearestPlayer->GetPosition(), safeDistFromPlayer);
}

// The same question as at Winterchill, with two differences: Azgalor can have more than one pool
// up at a time, and his frontal arc is taken away by the cleave chain whatever the fire is doing.
// Both are just further blocked arcs on the same ring. Cleave safety is never traded against
// standing in fire--fire ticks, cleave kills
bool AzgalorMeleeManeuverThroughFireAction::Execute(Event /*event*/)
{
    Unit* azgalor = AI_VALUE2(Unit*, "find target", "17842");
    if (!azgalor)
        return false;

    std::vector<Position> const& pools = GetRainOfFirePositions(bot);
    if (pools.empty())
        return false;

    constexpr float moveDist = 10.0f;
    float moveX;
    float moveY;
    float moveZ;
    float const meleeRadius = bot->GetMeleeRange(azgalor) - MELEE_RANGE_INSET;

    std::vector<BlockedArc> blocked;
    blocked.reserve(pools.size() + 1);

    for (Position const& pool : pools)
    {
        BlockedArc poolArc;
        if (GetHazardBlockedArc(
                azgalor->GetPosition(), meleeRadius, pool, RAIN_OF_FIRE_RADIUS, poolArc))
        {
            blocked.push_back(poolArc);
        }
    }

    // Every ring point sits inside the chain radius of whoever he is hitting, so on this ring the
    // range half of the cleave rule never saves anyone and his frontal arc is simply unavailable
    blocked.push_back({ azgalor->GetOrientation(), CLEAVE_DANGER_ARC / 2.0f });

    float const bossX = azgalor->GetPositionX();
    float const bossY = azgalor->GetPositionY();
    float const botHeading =
        std::atan2(bot->GetPositionY() - bossY, bot->GetPositionX() - bossX);

    float standAngle;
    if (FindNearestUnblockedAngle(blocked, botHeading, standAngle))
    {
        // Level ground, as at Winterchill, so the step needs no validating--only the unblocked
        // angle decides whether the ring is worth standing on
        float const targetX = bossX + std::cos(standAngle) * meleeRadius;
        float const targetY = bossY + std::sin(standAngle) * meleeRadius;
        float const distToTarget = bot->GetExactDist2d(targetX, targetY);

        // Already standing on the open heading, so hold rather than divide by nothing below
        constexpr float minStepDistance = 0.5f;
        if (distToTarget < minStepDistance)
            return false;

        float const stepDist = std::min(moveDist, distToTarget);
        float const botX = bot->GetPositionX();
        float const botY = bot->GetPositionY();

        return MoveTo(
            HYJAL_MAP_ID, botX + ((targetX - botX) / distToTarget) * stepDist,
            botY + ((targetY - botY) / distToTarget) * stepDist, bot->GetPositionZ(),
            false, false, false, false, MovementPriority::MOVEMENT_COMBAT, true, false);
    }

    // No heading on the ring is open. Fleeing is the answer only while the bot is actually in fire:
    // the escape aims at a point on a circle drawn round the pool, so a bot that has already
    // cleared that circle would be walked back inward toward it. Standing still is better--the ring
    // reopens on its own as Azgalor is dragged or the pool expires
    if (!IsInRainOfFire(bot))
        return false;

    // Leave the nearest pool, still refusing any heading that would cross into the cleave
    Position const* nearest = nullptr;
    float nearestDistance = 0.0f;
    for (Position const& pool : pools)
    {
        float const distance = bot->GetExactDist2d(pool);
        if (!nearest || distance < nearestDistance)
        {
            nearest = &pool;
            nearestDistance = distance;
        }
    }

    constexpr float escapeMargin = 2.0f;
    auto cleaveSafe = [azgalor](float x, float y)
    {
        return IsSafeFromAzgalorCleave(azgalor, x, y);
    };

    if (!GetHazardEscapeStep(
            bot, *nearest, RAIN_OF_FIRE_RADIUS + escapeMargin, moveDist,
            moveX, moveY, moveZ, cleaveSafe))
    {
        return false;
    }

    return MoveTo(
        HYJAL_MAP_ID, moveX, moveY, moveZ, false, false, false, false,
        MovementPriority::MOVEMENT_COMBAT, true, false);
}

// As at Winterchill, except Azgalor can have more than one pool up, so the nearest is the one to
// leave. Stepping out of it and into another is handled by simply doing this again next tick
bool AzgalorRangedGetOutOfRainOfFireAction::Execute(Event /*event*/)
{
    Position pool;
    if (!GetNearestRainOfFirePosition(bot, pool))
        return false;

    constexpr uint32 minInterval = 0;
    return FleePosition(pool, RAIN_OF_FIRE_RADIUS, minInterval);
}

// The spot is between the paths leading from Thrall's keep
bool AzgalorMoveToDoomguardTankAction::Execute(Event /*event*/)
{
    Position const& position = AZGALOR_DOOMGUARD_POSITION;
    float const distToPosition = bot->GetExactDist2d(position);

    if (distToPosition <= 5.0f)
        return false;

    float const botX = bot->GetPositionX();
    float const botY = bot->GetPositionY();
    constexpr float maxMoveDist = 3.5f;
    float const moveDist = std::min(maxMoveDist, distToPosition);
    float const moveX = botX + ((position.GetPositionX() - botX) / distToPosition) * moveDist;
    float const moveY = botY + ((position.GetPositionY() - botY) / distToPosition) * moveDist;

    return MoveTo(
        HYJAL_MAP_ID, moveX, moveY, bot->GetPositionZ(), false, false,
        false, false, MovementPriority::MOVEMENT_FORCED, true, false);
}

bool AzgalorFirstAssistTankPositionDoomguardAction::Execute(Event /*event*/)
{
    Position const& position = AZGALOR_DOOMGUARD_POSITION;
    float const distToPosition = bot->GetExactDist2d(position);

    bool shouldMove = false;
    bool backwards = false;

    if (Unit* doomguard = AI_VALUE2(Unit*, "find target", "17864"))
    {
        if (AI_VALUE(Unit*, "current target") != doomguard)
            return Attack(doomguard);

        if (doomguard->GetVictim() != bot || !bot->IsWithinMeleeRange(doomguard))
            return false;

        if (distToPosition <= 3.0f)
            return false;

        //By leewheel 2026-09-04: 上游4f9815d1/6d5d68cd——步进计算改用共享助手
        //EncounterHelpers::GetStepToPosition(后退步判定: 背离末日守卫方向)
        float moveX;
        float moveY;
        if (!GetStepToPosition(bot, position, 3.0f, doomguard, moveX, moveY, backwards))
            return false;
        //End By leewheel

        return MoveTo(
            HYJAL_MAP_ID, moveX, moveY, bot->GetPositionZ(), false, false,
            false, false, MovementPriority::MOVEMENT_COMBAT, true, backwards);
    }
    else if (distToPosition > 3.0f)
    {
        // If no Doomguard yet, move to position to wait for it to spawn
        shouldMove = true;
    }
    else
    {
        // If at position and no Doomguard, just wait
        return true;
    }

    if (!shouldMove)
        return false;

    //By leewheel 2026-09-04: 上游4f9815d1/6d5d68cd——无末日守卫等待期: 前向步进同用助手
    float moveX;
    float moveY;
    bool waitBackwards = false;
    if (!GetStepToPosition(bot, position, 3.0f, nullptr, moveX, moveY, waitBackwards))
        return false;
    //End By leewheel

    return MoveTo(
        HYJAL_MAP_ID, moveX, moveY, bot->GetPositionZ(), false, false,
        false, false, MovementPriority::MOVEMENT_COMBAT, true, waitBackwards);
}

// Only nearbyish ranged DPS should attack Doomguards; 65 yards should get to the
// side of Azgalor but not bring in any ranged standing in front
bool AzgalorDetermineDpsPriorityAction::Execute(Event /*event*/)
{
    Unit* azgalor = AI_VALUE2(Unit*, "find target", "17842");
    if (!azgalor)
        return false;

    if (PlayerbotAI::IsMelee(bot))
    {
        if (AI_VALUE(Unit*, "current target") != azgalor)
            return Attack (azgalor);
        return false;
    }

    Unit* target = nullptr;
    if (azgalor->GetHealthPct() < BOSS_BURN_HEALTH_PCT)
    {
        target = azgalor;
    }
    else
    {
        Unit* doomguard = AI_VALUE2(Unit*, "find target", "17864");
        if (doomguard && bot->GetExactDist2d(doomguard) < 70.0f)
            target = doomguard;
        else
            target = azgalor;
    }

    if (!target || AI_VALUE(Unit*, "current target") == target)
        return false;

    return Attack(target);
}

// Archimonde

//By leewheel 2026-09-04: 对齐上游b8304144——牧师fear ward改由通用牧师策略处理，此处只保留萨满战栗图腾；类名同步改 ArchimondeSetTremorTotemAction
bool ArchimondeSetTremorTotemAction::Execute(Event /*event*/)
{
    if (AI_VALUE2(bool, "has totem", "tremor totem"))
        return false;

    if (!botAI->CanCastSpell(Id(HyjalSpells::SPELL_TREMOR_TOTEM), bot))
        return false;

    return botAI->CastSpell(Id(HyjalSpells::SPELL_TREMOR_TOTEM), bot);
}
//End By leewheel

// Air Burst knocks everyone around its target into the air. Losing the whole melee group at once
// is what has to be avoided, since Archimonde turns to a ranged one-shot when nobody is left in
// melee range. Thus, the avoidance is to get away from the tank.
//By leewheel 2026-08-21: 移植上游——参照对象从"主坦"改为阿克蒙德当前目标(activeTank):
// 谁在扛就远离谁, 覆盖换坦场景; GetPendingAirBurstCast 改为 bool+出参
//By leewheel 2026-08-30: 对齐上游类名——ArchimondeSpreadToAvoidAirBurstAction 改 ArchimondeKeepAirBurstAwayFromTankAction
//  (与 HyjalActions.h 声明一致，patch半应用导致旧类名残留)
bool ArchimondeKeepAirBurstAwayFromTankAction::Execute(Event /*event*/)
{
    Unit* archimonde = AI_VALUE2(Unit*, "find target", "17968");
    if (!archimonde)
        return false;

    Unit* activeTank = archimonde->GetVictim();
    if (!activeTank)
        return false;

    // 读条开始时记录, 只在施放期间应答
    AirBurstData airBurst;
    if (!GetPendingAirBurstCast(bot->GetInstanceId(), airBurst))
        return false;

    // 只有以当前坦克或本bot为中心的气爆才可能同时罩住两者
    if (airBurst.targetGuid != activeTank->GetGUID() && airBurst.targetGuid != bot->GetGUID())
        return false;

    float const distanceToActiveTank = bot->GetExactDist2d(activeTank);
    if (distanceToActiveTank >= AIR_BURST_SAFE_DISTANCE)
        return false;

    return MoveAway(activeTank, AIR_BURST_SAFE_DISTANCE - distanceToActiveTank);
}
//End By leewheel

// Runs all fight, not just off the pull. Ranged start stacked from the run in and drift back
// together afterwards, and a clump is what turns one Air Burst into a raid-wide one. The interval
// keeps it a periodic nudge rather than something contending for every tick, and the Doomfire
// multiplier takes it out entirely near a trail so it never argues with the avoidance
bool ArchimondeSpreadRangedAction::Execute(Event /*event*/)
{
    Player* nearestPlayer = GetNearestPlayerInRadius(bot, ARCHIMONDE_RANGED_SPREAD_DISTANCE);
    if (!nearestPlayer)
        return false;

    return FleePosition(
        nearestPlayer->GetPosition(), ARCHIMONDE_RANGED_SPREAD_DISTANCE,
        ARCHIMONDE_RANGED_SPREAD_INTERVAL);
}

// Two jobs, because the suppression that comes with this action reaches further than its push does.
// Inside DOOMFIRE_DANGER_RADIUS it shoves the bot clear; from there out to DOOMFIRE_CONTROL_RADIUS
// the push has faded to nothing but no other movement is allowed yet, so this has to be what walks
// the bot back to Archimonde. Without that second half a bot that dodged a trail stands exactly
// where it was pushed while the tank drags him out of reach, and only starts chasing once the trail
// burns out
bool ArchimondeAvoidDoomfireAction::Execute(Event /*event*/)
{
    Unit* archimonde = AI_VALUE2(Unit*, "find target", "17968");
    if (!archimonde)
        return false;

    // Each trail patch is its own dynamic object that expires on its own after 18s, so the live set
    // of them is the trail. The cached set spans DOOMFIRE_SEARCH_RADIUS; the field loop below narrows
    // it to DOOMFIRE_FIELD_RADIUS so that patches the bot has not reached yet still get a say in
    // which way it goes, while the trapped sweep reads it whole--a bearing is only worth taking if
    // nothing sits near where it lands, and that includes patches beyond the field
    std::vector<Position> const& trail = GetDoomfirePositions(bot);

    float const botX = bot->GetPositionX();
    float const botY = bot->GetPositionY();

    Position const* nearest = nullptr;
    float nearestDistance = 0.0f;
    float totalDx = 0.0f;
    float totalDy = 0.0f;

    for (Position const& patch : trail)
    {
        float const d = bot->GetExactDist2d(patch);
        if (d >= DOOMFIRE_FIELD_RADIUS)
            continue;

        if (!nearest || d < nearestDistance)
        {
            nearest = &patch;
            nearestDistance = d;
        }

        if (d > 0.0f)
        {
            float const weight = (DOOMFIRE_FIELD_RADIUS - d) / DOOMFIRE_FIELD_RADIUS;
            totalDx += (botX - patch.GetPositionX()) / d * weight;
            totalDy += (botY - patch.GetPositionY()) / d * weight;
        }
    }

    // Direction comes from the whole field, distance only from what is actually dangerous. Keeping
    // them apart is what holds the two properties the suppression band rests on: the push still
    // fades to exactly nothing at the danger radius, and a dense cluster can no longer sum to a
    // shove longer than the bot needs and carry it clean past the band
    float norm = std::sqrt(totalDx * totalDx + totalDy * totalDy);
    float moveDist = (nearest && nearestDistance < DOOMFIRE_DANGER_RADIUS) ?
        DOOMFIRE_DANGER_RADIUS - nearestDistance : 0.0f;

    // Boxed in: patches on opposite sides cancel and the field has no direction left to give. That
    // is precisely when holding is worst, because the bot is standing in fire taking damage. Sweep
    // for the bearing whose landing point sits furthest from anything and take it, crossing a patch
    // on the way if that is what it costs. Picking "away from the nearest" instead would only trade
    // one patch for its neighbour and walk back again next tick
    constexpr float minFieldStrength = 0.05f;
    if (nearest && nearestDistance < DOOMFIRE_BURN_RADIUS && norm < minFieldStrength)
    {
        // Two passes, as FindStepToCircle does. Archimonde is fought on a wooded hill, and trees
        // and fallen logs sit in the navmesh--a bearing that is open on the fire alone may not be
        // walkable at all, and MoveTo does not say so: an incomplete path is accepted with the
        // destination quietly replaced by wherever the ray stopped, which can be back in the fire.
        // So prefer a bearing the bot can actually walk, and only when none can be walked take the
        // best of the rest, because moving badly still beats standing here burning
        constexpr uint8 fanSteps = 12;
        constexpr float escapeStep = 10.0f;
        bool found = false;

        for (uint8 pass = 0; pass < 2 && !found; ++pass)
        {
            bool const validate = (pass == 0);
            float bestClearance = -1.0f;

            for (uint8 i = 0; i < fanSteps; ++i)
            {
                float const angle = 2.0f * static_cast<float>(M_PI) * i / fanSteps;
                float const testX = botX + std::cos(angle) * DOOMFIRE_DANGER_RADIUS;
                float const testY = botY + std::sin(angle) * DOOMFIRE_DANGER_RADIUS;

                // Ranked rather than vetoed. Boxed in is exactly the case where every bearing fails
                // an outright test, so the question has to be which is least bad, not which passes
                float clearance = DOOMFIRE_FIELD_RADIUS;
                for (Position const& patch : trail)
                    clearance = std::min(clearance, patch.GetExactDist2d(testX, testY));

                if (clearance <= bestClearance)
                    continue;

                float stepX = testX;
                float stepY = testY;
                float stepZ = bot->GetPositionZ();
                if (validate &&
                    !CanTakeStepTowards(bot, testX, testY, escapeStep, stepX, stepY, stepZ))
                {
                    continue;
                }

                bestClearance = clearance;
                totalDx = stepX - botX;
                totalDy = stepY - botY;
                found = true;
            }
        }

        norm = std::sqrt(totalDx * totalDx + totalDy * totalDy);
        moveDist = norm;
    }

    if (norm > 0.0f && moveDist >= 0.5f)
    {
        float const targetX = botX + (totalDx / norm) * moveDist;
        float const targetY = botY + (totalDy / norm) * moveDist;

        MovementPriority const priority = PlayerbotAI::IsHeal(bot) ?
            MovementPriority::MOVEMENT_COMBAT : MovementPriority::MOVEMENT_FORCED;

        bool const backwards = archimonde->GetVictim() == bot;

        return MoveTo(
            HYJAL_MAP_ID, targetX, targetY, bot->GetPositionZ(), false, false,
            false, false, priority, true, backwards);
    }

    // Nothing is pushing the bot, so take over the chase. That this only happens where the action's
    // own suppression would otherwise hold the bot is the trigger's doing--it fires on the same
    // DOOMFIRE_CONTROL_RADIUS the multiplier suppresses at, so past that ordinary movement is free
    // again and there is nothing here to take over from
    //
    // Each side has to be asked in its own units. GetRange hands back the edge-to-edge figure
    // ReachSpellAction is built with, and IsWithinCombatRange is what consumes it--adding both
    // combat reaches. Measured instead against a raw centre-to-centre distance it would walk ranged
    // in by the whole of Archimonde's hitbox, toward the very trail they just dodged. GetMeleeRange
    // already carries both reaches, so the melee side compares centre to centre directly
    bool const inPosition = PlayerbotAI::IsRanged(bot) ?
        bot->IsWithinCombatRange(archimonde, botAI->GetRange("spell")) :
        bot->GetExactDist2d(archimonde) <= bot->GetMeleeRange(archimonde) - MELEE_RANGE_INSET;

    if (inPosition)
        return false;

    float const distToBoss = bot->GetExactDist2d(archimonde);
    if (distToBoss < 0.5f)
        return false;

    // A whole step every time, letting the test above stop it. Trimming the last step to land
    // exactly on the range would need that range back in centre-to-centre terms, which is the
    // conversion this is avoiding
    constexpr float maxMoveDist = 3.5f;
    float const moveX = botX + ((archimonde->GetPositionX() - botX) / distToBoss) * maxMoveDist;
    float const moveY = botY + ((archimonde->GetPositionY() - botY) / distToBoss) * maxMoveDist;

    // A trail lying between the bot and Archimonde is the ordinary case for ranged, not a corner
    // one: it walks the floor they stand off. Stepping into it only to be shoved straight back out
    // is the bounce this whole arrangement exists to prevent, so a step that would land inside the
    // danger radius is simply not taken. Asked of the destination rather than of the direction,
    // which is what makes it exact--the trail blocks the path or it does not
    if (IsPositionNearDoomfire(bot, moveX, moveY, DOOMFIRE_DANGER_RADIUS))
        return false;

    return MoveTo(
        HYJAL_MAP_ID, moveX, moveY, bot->GetPositionZ(), false, false,
        false, false, MovementPriority::MOVEMENT_COMBAT, true, false);
}

bool ArchimondeRemoveDoomfireDotAction::Execute(Event /*event*/)
{
    switch (bot->getClass())
    {
        case CLASS_MAGE:
            return botAI->CanCastSpell(Id(HyjalSpells::SPELL_ICE_BLOCK), bot) &&
                botAI->CastSpell(Id(HyjalSpells::SPELL_ICE_BLOCK), bot);

        case CLASS_PALADIN:
            return botAI->CanCastSpell(Id(HyjalSpells::SPELL_DIVINE_SHIELD), bot) &&
                botAI->CastSpell(Id(HyjalSpells::SPELL_DIVINE_SHIELD), bot);

        case CLASS_ROGUE:
            return botAI->CanCastSpell(Id(HyjalSpells::SPELL_CLOAK_OF_SHADOWS), bot) &&
                botAI->CastSpell(Id(HyjalSpells::SPELL_CLOAK_OF_SHADOWS), bot);

        default:
            return false;
    }
}
