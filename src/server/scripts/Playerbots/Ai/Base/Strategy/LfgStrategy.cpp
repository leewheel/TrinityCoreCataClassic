/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "LfgStrategy.h"

#include "Playerbots.h"

void LfgStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    //By leewheel 2026-08-09: "random" 7%概率+AI更新慢导致响应随机本队列要好几分钟,
    //改用高频率 "lfg queue" 触发器, 每次 AI 更新都尝试排队(JoinLFG 内部已检查状态)
    triggers.push_back(new TriggerNode("lfg queue", { NextAction("lfg join", relevance) }));
    //End By leewheel
    triggers.push_back(
        new TriggerNode("seldom", { NextAction("lfg leave", relevance) }));
    triggers.push_back(new TriggerNode(
        "unknown dungeon", { NextAction("give leader in dungeon", relevance) }));
}

LfgStrategy::LfgStrategy(PlayerbotAI* botAI) : PassThroughStrategy(botAI) {}
