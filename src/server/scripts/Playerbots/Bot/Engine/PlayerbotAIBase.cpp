/*
 * 机器人AI基类
 * 从 AzerothCore mod-playerbots 移植到 TrinityCore
 */

#include "PlayerbotAIBase.h"

#include "Playerbots.h"
#include "PerfMonitor.h"

PlayerbotAIBase::PlayerbotAIBase(bool isBotAI) : nextAICheckDelay(0), _isBotAI(isBotAI) {}

void PlayerbotAIBase::UpdateAI(uint32 elapsed, bool minimal)
{
    if (totalPmo)
        totalPmo->finish();

    totalPmo = sPerfMonitor.start(PERF_MON_TOTAL, "PlayerbotAIBase::FullTick");

    if (nextAICheckDelay > elapsed)
        nextAICheckDelay -= elapsed;
    else
        nextAICheckDelay = 0;

    if (!CanUpdateAI())
        return;

    UpdateAIInternal(elapsed, minimal);
    YieldThread(nullptr);
}

void PlayerbotAIBase::SetNextCheckDelay(uint32 const delay)
{
    nextAICheckDelay = delay;
}

void PlayerbotAIBase::IncreaseNextCheckDelay(uint32 delay)
{
    nextAICheckDelay += delay;
}

bool PlayerbotAIBase::CanUpdateAI() { return nextAICheckDelay == 0; }

void PlayerbotAIBase::YieldThread(Player* bot, uint32 delay)
{
    if (nextAICheckDelay < delay)
    {
        // 为每个机器人添加确定性的小偏移（0-200ms），以错开更新并防止 CPU 峰值
        uint32 offset = bot ? (bot->GetGUID().GetCounter() % 201) : 0;
        nextAICheckDelay = delay + offset;
    }
}

bool PlayerbotAIBase::IsActive() { return nextAICheckDelay < sPlayerbotAIConfig.maxWaitForMove; }

bool PlayerbotAIBase::IsBotAI() const { return _isBotAI; }
