/* 太阳之井高地 机器人策略 */
#include "SWPEncounter_Twins.h"
#include "AiObjectContext.h"
#include "CellImpl.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "NearestGameObjects.h"
#include "Playerbots.h"
#include "ThreatManager.h"
#include <list>

namespace SwpHelpers
{

// Note: Alythess and Sacrolash each have a CombatReach of 2.5f

namespace
{

std::unordered_map<ObjectGuid, ObjectGuid> alythessTankLastBlazeGuid;

//By leewheel 2026-08-21: 移植 brighton-chi cbfc7ea0——一次缓存周期内只做一次网格扫描,
//供艾莉赛斯坦克测试的所有候选站位共用, 避免逐候选重复全图扫描
std::vector<Position> const& GetCachedBlazePositions(PlayerbotAI* botAI)
{
    return botAI->GetAiObjectContext()
        ->GetValue<std::vector<Position>>("eredar twins blaze")->RefGet();
}
//End By leewheel

// Adjusted positions are to address the occasional bug (?) where Alythess moves
Position GetAdjustedPosition(Unit* alythess, Position const& basePosition)
{
    if (!alythess)
        return basePosition;

    Position const& alythessPosition = alythess->GetPosition();
    Position const& startPosition = ALYTHESS_START_POSITION;

    float const offsetX = alythessPosition.GetPositionX() - startPosition.GetPositionX();
    float const offsetY = alythessPosition.GetPositionY() - startPosition.GetPositionY();
    float const offsetZ = alythessPosition.GetPositionZ() - startPosition.GetPositionZ();

    float const baseX = basePosition.GetPositionX();
    float const baseY = basePosition.GetPositionY();
    float const baseZ = basePosition.GetPositionZ();

    return { baseX + offsetX, baseY + offsetY, baseZ + offsetZ };
}

} // end anonymous namespace

std::unordered_map<uint32, EredarTwinsIncomingConflagrationState>
	eredarTwinsIncomingConflagrationStates;

std::unordered_map<uint32, EredarTwinsBlazeTargetState> eredarTwinsBlazeTargetStates;

std::unordered_map<uint32, uint32> eredarTwinsDpsHoldStartMs;

Position GetAlythessTankPosition(Unit* alythess, uint8 index)
{
    if (index >= ALYTHESS_TANK_POSITIONS.size())
        index = 0;

    return GetAdjustedPosition(alythess, ALYTHESS_TANK_POSITIONS[index]);
}

Position GetEredarTwinsP2MeleePosition(Unit* alythess)
{
    return GetAdjustedPosition(alythess, EREDAR_TWINS_P2_MELEE_POSITION);
}

Position GetEredarTwinsP2RangedPosition(Unit* alythess)
{
    return GetAdjustedPosition(alythess, EREDAR_TWINS_P2_RANGED_POSITION);
}

bool IsAnySacrolashTank(Player* bot)
{
    return PlayerbotAI::IsMainTank(bot) || PlayerbotAI::IsAssistTankOfIndex(bot, 1, false);
}

bool IsAlythessTank(Player* bot)
{
    return PlayerbotAI::IsAssistTankOfIndex(bot, 0, false);
}

bool ShouldHoldTwinThreat(
    Player* bot, Unit* boss, float threatHoldRatio, bool (*isTwinTank)(Player*))
{
    if (!boss || PlayerbotAI::IsHeal(bot) || isTwinTank(bot))
        return false;

    float twinTankThreat = 0.0f;
    float botThreat = 0.0f;
    bool foundTwinTankThreat = false;
    bool foundBotThreat = false;

    //By leewheel 2026-08-21: 移植 brighton-chi e9485e70——无需排序, 改用未排序威胁列表
    auto const threatList = boss->GetThreatMgr().GetUnsortedThreatList();
    //End By leewheel
    for (auto itr = threatList.begin(); itr != threatList.end(); ++itr)
    {
        ThreatReference const* threatRef = *itr;
        if (!threatRef || !threatRef->IsAvailable())
            continue;

        Unit* victim = threatRef->GetVictim();
        if (!victim)
            continue;

        Player* threatPlayer = victim->ToPlayer();
        if (!threatPlayer || !threatPlayer->IsAlive())
            continue;

        float const threat = threatRef->GetThreat();

        if (isTwinTank(threatPlayer) &&
            (!foundTwinTankThreat || threat < twinTankThreat))
        {
            twinTankThreat = threat;
            foundTwinTankThreat = true;
        }

        if (threatPlayer == bot)
        {
            botThreat = threat;
            foundBotThreat = true;
        }
    }

    if (!foundTwinTankThreat || !foundBotThreat || twinTankThreat <= 0.0f)
        return false;

    return botThreat >= twinTankThreat * threatHoldRatio;
}

//By leewheel 2026-08-21: 移植 brighton-chi cbfc7ea0——只存坐标不存GameObject指针:
//值可能存活超过产生它的那个tick, 烈焰消失后缓存指针会悬垂。烈焰不会移动, 坐标在对象
//存续期间始终正确
std::vector<Position> FindEredarTwinsBlazePositions(Player* bot)
{
    std::list<GameObject*> nearbyObjects;
    //By leewheel 2026-08-27: TC适配——thelab用Acore命名空间, TC3.4.3为Trinity
    AnyGameObjectInObjectRangeCheck check(bot, BLAZE_SEARCH_RADIUS);
    Trinity::GameObjectListSearcher<AnyGameObjectInObjectRangeCheck> searcher(
        bot, nearbyObjects, check);
    Cell::VisitObjects(bot, searcher, BLAZE_SEARCH_RADIUS);

    std::vector<Position> positions;
    for (GameObject* nearbyObject : nearbyObjects)
    {
        if (nearbyObject && nearbyObject->GetEntry() == Id(SwpObjects::GO_BLAZE))
            positions.push_back(nearbyObject->GetPosition());
    }

    return positions;
}
//End By leewheel

bool IsAlythessTankPositionSafe(PlayerbotAI* botAI, Position const& position)
{
    //By leewheel 2026-08-21: 改走缓存值(一次网格扫描供全部候选共用)
    for (Position const& blaze : GetCachedBlazePositions(botAI))
    {
        if (blaze.GetExactDist2d(position) <= BLAZE_DANGER_RADIUS)
            return false;
    }
    //End By leewheel

    return true;
}

bool ShouldAdvanceAlythessTankPosition(Unit* alythess, Player* bot)
{
    if (!alythess)
        return false;

    ObjectGuid const botGuid = bot->GetGUID();

    GameObject* blazeObject =
        bot->FindNearestGameObject(Id(SwpObjects::GO_BLAZE), BLAZE_DANGER_RADIUS);
    if (!blazeObject)
    {
        alythessTankLastBlazeGuid.erase(botGuid);
        return false;
    }

    ObjectGuid const blazeGuid = blazeObject->GetGUID();
    auto const lastBlaze = alythessTankLastBlazeGuid.find(botGuid);
    if (lastBlaze != alythessTankLastBlazeGuid.end() && lastBlaze->second == blazeGuid)
        return false;

    alythessTankLastBlazeGuid[botGuid] = blazeGuid;
    return true;
}

//By leewheel 2026-08-21: 移植 brighton-chi 3c8d60e0——DPS等待窗口由触发器打开而非multiplier:
//multiplier按注册顺序求值且一旦有乘数把相关性清零就中断, 是否执行取决于动作与前置乘数,
//在那里启动计时会让等待从任意时刻算起
void RecordEredarTwinsDpsHoldStart(Player* bot)
{
    eredarTwinsDpsHoldStartMs.try_emplace(bot->GetInstanceId(), getMSTime());
}
//End By leewheel

void RecordIncomingEredarTwinsConflagrationTarget(Player* target)
{
    if (!target)
        return;

    uint32 const now = getMSTime();
    EredarTwinsIncomingConflagrationState& state =
        eredarTwinsIncomingConflagrationStates[target->GetInstanceId()];

    if (state.targetGuid != target->GetGUID())
        state.delayMs = now + CONFLAGRATION_DELAY_MS;

    state.targetGuid = target->GetGUID();
    state.expireMs = now + CONFLAGRATION_WINDOW_MS;
}

Player* GetEredarTwinsConflagrationTarget(Player* bot)
{
    auto const incomingItr = eredarTwinsIncomingConflagrationStates.find(bot->GetInstanceId());

    if (incomingItr == eredarTwinsIncomingConflagrationStates.end())
        return nullptr;

    EredarTwinsIncomingConflagrationState const& state = incomingItr->second;
    uint32 const now = getMSTime();

    if (state.expireMs <= now)
    {
        eredarTwinsIncomingConflagrationStates.erase(incomingItr);
        return nullptr;
    }

    if (state.delayMs > now)
        return nullptr;

    Group* group = bot->GetGroup();
    if (!group)
        return nullptr;

    for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
    {
        Player* member = ref->GetSource();
        if (member && member->GetGUID() == state.targetGuid)
            return member;
    }

    return nullptr;
}

void RecordEredarTwinsBlazeTarget(Player* target)
{
    if (!target)
        return;

    EredarTwinsBlazeTargetState& state = eredarTwinsBlazeTargetStates[target->GetInstanceId()];
    state.targetGuid = target->GetGUID();
    state.startMs = getMSTime();
}

Player* GetEredarTwinsBlazeTarget(Player* bot)
{
    auto const itr = eredarTwinsBlazeTargetStates.find(bot->GetInstanceId());
    if (itr == eredarTwinsBlazeTargetStates.end())
        return nullptr;

    EredarTwinsBlazeTargetState const& state = itr->second;
    if (GetMSTimeDiffToNow(state.startMs) >= BLAZE_TARGET_WINDOW_MS)
    {
        eredarTwinsBlazeTargetStates.erase(itr);
        return nullptr;
    }

    Group* group = bot->GetGroup();
    if (!group)
        return nullptr;

    for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
    {
        Player* member = ref->GetSource();
        if (member && member->GetGUID() == state.targetGuid)
            return member;
    }

    return nullptr;
}

//By leewheel 2026-08-14: 清理双子Blaze跟踪(匿名namespace的map在此访问)
bool ClearEredarTwinsBlazeState(std::vector<ObjectGuid> const& guids)
{
    bool erased = false;
    for (ObjectGuid const& guid : guids)
    {
        if (alythessTankLastBlazeGuid.erase(guid) > 0)
            erased = true;
    }
    return erased;
}
//End By leewheel

}
