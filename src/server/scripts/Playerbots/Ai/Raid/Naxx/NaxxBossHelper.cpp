/*
 * 移植来源: AC azerothcore-wotlk mod-playerbots 提交 a773b8c6 (Naxx Heigan 安全舞 fight clock)
 * 移植适配 TC 框架
 * 业务对标: AC azerothcore-wotlk mod-playerbots
 * 作者: leewheel
 */

#include "NaxxBossHelper.h"
#include "Playerbots.h"
#include "SpellAuras.h"
#include "Timer.h"
#include <mutex>
#include <unordered_map>

namespace
{
    // 超过这个时间未观察到战斗则视为战斗结束（灭团/脱战），下次战斗从头开始
    constexpr uint32 HeiganStateStaleMs = 30000;

    // 战斗时钟，由所有打同一个Heigan的bot共享（以boss guid为键）
    struct HeiganFightRegistry
    {
        std::mutex lock;
        std::unordered_map<ObjectGuid, HeiganBossHelper::FightState> states;
    };

    HeiganFightRegistry& GetHeiganFightRegistry()
    {
        static HeiganFightRegistry registry;
        return registry;
    }
}  // namespace

bool HeiganBossHelper::UpdateBossAI()
{
    if (_unit && (!_unit->IsInWorld() || !_unit->IsAlive() || !_unit->IsInCombat()))
        Reset();

    if (!_unit)
    {
        if (!bot->IsInCombat())
        {
            Reset();
            return false;
        }

        _unit = AI_VALUE2(Unit*, "find target", "heigan the unclean");
        if (!_unit)
            return false;
    }

    uint32 now = getMSTime();

    // 快速阶段检测：Heigan瞬移到平台上，停止攻击（REACT_PASSIVE）后1秒开始引导瘟疫之云
    // 瘟疫之云的光环是权威判断依据；"在平台上且没有目标"只桥接那1秒窗口（以及可能中断的引导）
    Aura* plagueCloud = _unit->GetAura(NaxxSpellIds::PlagueCloud);
    bool idleOnPlatform = !_unit->GetVictim() && _unit->IsWithinDist2d(PlatformX, PlatformY, PlatformRadius);

    HeiganFightRegistry& registry = GetHeiganFightRegistry();
    std::scoped_lock<std::mutex> guard(registry.lock);

    // 清理过期的条目（上次战斗/其他实例中早已结束的战斗）
    for (auto itr = registry.states.begin(); itr != registry.states.end();)
    {
        if (now - itr->second.lastSeenMs > HeiganStateStaleMs)
            itr = registry.states.erase(itr);
        else
            ++itr;
    }

    FightState& state = registry.states[_unit->GetGUID()];
    bool firstObservation = state.lastSeenMs == 0;
    uint32 elapsed = firstObservation ? 0 : now - state.phaseStartMs;

    bool fast = DetectFastPhase(state, plagueCloud != nullptr, idleOnPlatform, firstObservation, elapsed);
    UpdateFightState(state, fast, plagueCloud, firstObservation, now);

    _fastPhase = state.fastPhase;
    _phaseElapsedMs = now - state.phaseStartMs;
    return true;
}

bool HeiganBossHelper::DetectFastPhase(FightState const& state, bool plagueCloudUp, bool idleOnPlatform,
                                       bool firstObservation, uint32 elapsed) const
{
    if (plagueCloudUp)
        return true;

    // 仍在平台上且没有目标：快速阶段持续中（桥接瘟疫之云光环出现前的1秒窗口）
    if (state.fastPhase)
        return idleOnPlatform;

    // 慢速阶段固定持续SlowPhaseDurationMs，在未满此时间前忽略平台上的空闲Boss（例如刚开战时Boss站在平台上）
    return idleOnPlatform && !firstObservation && elapsed + 5000 >= SlowPhaseDurationMs;
}

void HeiganBossHelper::UpdateFightState(FightState& state, bool fast, Aura const* plagueCloud,
                                        bool firstObservation, uint32 now)
{
    if (fast)
    {
        if (plagueCloud)
        {
            // 精确计时：瘟疫之云光环在阶段开始后PlagueCloudDelayMs时施加，持续FastPhaseDurationMs
            int32 auraElapsed = plagueCloud->GetMaxDuration() - plagueCloud->GetDuration();
            if (auraElapsed < 0)
                auraElapsed = 0;
            state.phaseStartMs = now - PlagueCloudDelayMs - uint32(auraElapsed);
            state.exact = true;
        }
        else if (!state.fastPhase)
        {
            // 刚瞬移，瘟疫之云尚未出现
            state.phaseStartMs = now;
            state.exact = false;
        }
        state.fastPhase = true;
    }
    else
    {
        if (state.fastPhase)
        {
            // 快速→慢速转换：快速阶段持续正好FastPhaseDurationMs
            state.phaseStartMs = state.exact ? state.phaseStartMs + FastPhaseDurationMs : now;
        }
        else if (firstObservation)
        {
            // 拉怪（或长时间无人观察此战斗）
            state.phaseStartMs = now;
            state.exact = false;
        }
        state.fastPhase = false;
    }
    state.lastSeenMs = now;
}

uint32 HeiganBossHelper::GetEruptionIndex() const
{
    uint32 first = _fastPhase ? FastFirstEruptionMs : SlowFirstEruptionMs;
    uint32 interval = _fastPhase ? FastEruptionIntervalMs : SlowEruptionIntervalMs;
    if (_phaseElapsedMs < first + EruptionSafetyMarginMs)
        return 0;
    return (_phaseElapsedMs - EruptionSafetyMarginMs - first) / interval + 1;
}

uint32 HeiganBossHelper::GetSafeWaypoint() const
{
    uint32 index = GetEruptionIndex();
    // 当前阶段最后一次喷发后，下一个安全点指向下一阶段的第一个喷发位置
    if (index >= (_fastPhase ? FastEruptionCount : SlowEruptionCount))
        return 0;
    uint32 r = index % (2 * (WaypointCount - 1));
    return r < WaypointCount ? r : 2 * (WaypointCount - 1) - r;
}

uint32 HeiganBossHelper::GetMsUntilNextEruption() const
{
    uint32 first = _fastPhase ? FastFirstEruptionMs : SlowFirstEruptionMs;
    uint32 interval = _fastPhase ? FastEruptionIntervalMs : SlowEruptionIntervalMs;
    uint32 count = _fastPhase ? FastEruptionCount : SlowEruptionCount;
    uint32 happened = _phaseElapsedMs < first ? 0 : (_phaseElapsedMs - first) / interval + 1;
    uint32 next;
    if (happened >= count)
    {
        // 当前阶段已无喷发；下一次喷发是下一阶段的第一次
        uint32 phaseLength = _fastPhase ? FastPhaseDurationMs : SlowPhaseDurationMs;
        uint32 nextFirst = _fastPhase ? SlowFirstEruptionMs : FastFirstEruptionMs;
        next = phaseLength + nextFirst;
    }
    else
        next = first + happened * interval;

    return next > _phaseElapsedMs ? next - _phaseElapsedMs : 0;
}

bool HeiganBossHelper::IsAtWaypoint(uint32 index, float tolerance) const
{
    if (index >= WaypointCount)
        return false;
    return bot->GetDistance2d(WaypointX[index], WaypointY[index]) <= tolerance;
}

bool HeiganBossHelper::ShouldDance()
{
    if (_fastPhase)
        return true;
    if (PlayerbotAI::IsRanged(bot))
        return false;
    if (PlayerbotAI::IsMainTank(bot) && !AI_VALUE2(bool, "has aggro", "boss target"))
        return false;
    return true;
}

bool HeiganBossHelper::CanStandStillFor(uint32 ms)
{
    if (!ShouldDance())
        return true;
    return IsAtWaypoint(GetSafeWaypoint()) && GetMsUntilNextEruption() > ms;
}
//End By leewheel 2026-08-30