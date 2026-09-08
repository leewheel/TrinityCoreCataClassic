/*
 * 构建共享策略上下文
 * 从 AzerothCore mod-playerbots 移植到 TrinityCore
 */

#include "AiObjectContext.h"
#include "StrategyContext.h"
#include "Ai/Dungeon/DungeonStrategyContext.h"
#include "Ai/Raid/RaidStrategyContext.h"

void AiObjectContext::BuildSharedStrategyContexts(SharedNamedObjectContextList<Strategy>& strategyContexts)
{
    strategyContexts.Add(new StrategyContext());
    strategyContexts.Add(new MovementStrategyContext());
    strategyContexts.Add(new AssistStrategyContext());
    strategyContexts.Add(new QuestStrategyContext());
    strategyContexts.Add(new DungeonStrategyContext());
    strategyContexts.Add(new RaidStrategyContext());
}
