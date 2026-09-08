/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#ifndef PLAYERBOTS_LFGTRIGGERS_H
#define PLAYERBOTS_LFGTRIGGERS_H

#include "Trigger.h"

class PlayerbotAI;

class LfgProposalActiveTrigger : public Trigger
{
public:
    //By leewheel 2026-08-26: 重试间隔40秒改为5秒——提案有效期仅45秒(LFG_TIME_PROPOSAL),
    //原40秒间隔导致战斗中的bot基本只有一次应答机会;
    //LfgAcceptAction改为战斗/死亡时暂不应答后, 必须靠本触发器高频重试才能脱战后及时接受。
    //Trigger构造函数对<100的参数自动乘1000(毫秒), 传5即5000ms
    LfgProposalActiveTrigger(PlayerbotAI* botAI) : Trigger(botAI, "lfg proposal active", 5) {}

    bool IsActive() override;
};

class UnknownDungeonTrigger : public Trigger
{
public:
    UnknownDungeonTrigger(PlayerbotAI* botAI) : Trigger(botAI, "unknown dungeon", 20 * 2000) {}

    bool IsActive() override;
};

// By leewheel 2026-08-09: 高频率 LFG 排队触发器——原 "random" 触发器仅 7% 概率
// 且受 bot AI 更新频率(分片+MapUpdateInterval)拖累, 响应随机本队列要好几分钟才几个 bot。
// 改为每次 AI 更新都尝试排队(JoinLFG 内部先检查 LFG 状态, 已排队直接返回, 无重复入队开销)
class LfgJoinTrigger : public Trigger
{
public:
    LfgJoinTrigger(PlayerbotAI* botAI) : Trigger(botAI, "lfg join") {}

    bool IsActive() override;
};
// End By leewheel

#endif
