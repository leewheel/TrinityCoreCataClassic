/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "LfgTriggers.h"

#include "LFGMgr.h" // By leewheel 2026-08-09: sLFGMgr 引用
#include "Playerbots.h"

bool LfgProposalActiveTrigger::IsActive() { return AI_VALUE(uint32, "lfg proposal"); }

bool UnknownDungeonTrigger::IsActive()
{
    return botAI->HasActivePlayerMaster() && botAI->GetMaster() && botAI->GetMaster()->IsInWorld() &&
           botAI->GetMaster()->GetMap()->IsDungeon() && bot->GetMapId() == botAI->GetMaster()->GetMapId();
}

// By leewheel 2026-08-09: 每次 AI 更新都尝试 LFG 排队(配置开关 + 未排队)
// JoinLFG 内部先检查 LFG 状态, 已排队直接返回, 不会重复入队
bool LfgJoinTrigger::IsActive()
{
    return sPlayerbotAIConfig.randomBotJoinLfg && !sLFGMgr->GetState(bot->GetGUID());
}
// End By leewheel
