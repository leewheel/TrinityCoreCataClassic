/* 副本机器人策略 */
#ifndef PLAYERBOTS_PVPCOMBATTRIGGERS_H
#define PLAYERBOTS_PVPCOMBATTRIGGERS_H

#include "Trigger.h"

class PlayerbotAI;

// 自己被控制(定身/昏迷/恐惧/迷惑) → 需要用徽章或种族技能解控
class PvpBotCcedTrigger : public Trigger
{
public:
    PvpBotCcedTrigger(PlayerbotAI* botAI) : Trigger(botAI, "pvp bot cced") {}
    bool IsActive() override;
};

// 敌方目标正在施法 → 应该打断
class PvpTargetCastingTrigger : public Trigger
{
public:
    PvpTargetCastingTrigger(PlayerbotAI* botAI) : Trigger(botAI, "pvp target casting") {}
    bool IsActive() override;
};

// 敌方目标是治疗者(根据职业/正在施法判断) → 优先集火打断
class PvpTargetIsHealerTrigger : public Trigger
{
public:
    PvpTargetIsHealerTrigger(PlayerbotAI* botAI) : Trigger(botAI, "pvp target is healer") {}
    bool IsActive() override;
};

// 自己血量过低(≤30%) → 开保命/逃跑
class PvpBotLowHealthTrigger : public Trigger
{
public:
    PvpBotLowHealthTrigger(PlayerbotAI* botAI) : Trigger(botAI, "pvp bot low health") {}
    bool IsActive() override;
};

// 低血量(≤40%)PVP战斗 → 控制对手争取时间, 打绷带回血, 控制CD则逃跑
class PvpLowHealthCCTrigger : public Trigger
{
public:
    PvpLowHealthCCTrigger(PlayerbotAI* botAI) : Trigger(botAI, "pvp low health cc") {}
    bool IsActive() override;
};

// 自己正在被多名敌方攻击(被集火) → 走位/保命
class PvpBotFocusedTrigger : public Trigger
{
public:
    PvpBotFocusedTrigger(PlayerbotAI* botAI) : Trigger(botAI, "pvp bot focused") {}
    bool IsActive() override;
};

// 近战敌人贴脸(法师/术士等远程职业) → 闪现/拉开
class PvpKiteTargetTrigger : public Trigger
{
public:
    PvpKiteTargetTrigger(PlayerbotAI* botAI) : Trigger(botAI, "pvp kite target") {}
    bool IsActive() override;
};

// 敌方玩家在视野内 → 发现敌对玩家
class PvpEnemyPlayerSeenTrigger : public Trigger
{
public:
    PvpEnemyPlayerSeenTrigger(PlayerbotAI* botAI) : Trigger(botAI, "pvp enemy player seen") {}
    bool IsActive() override;
};

// 爆发技能可用(爆发型长CD技能冷却完毕) → 判断局势使用
class PvpBurstAvailableTrigger : public Trigger
{
public:
    PvpBurstAvailableTrigger(PlayerbotAI* botAI) : Trigger(botAI, "pvp burst available") {}
    bool IsActive() override;
};

// 需要治疗自己(HP<50%且不在战斗或治疗者空闲) → 自疗/喝药
class PvpNeedHealTrigger : public Trigger
{
public:
    PvpNeedHealTrigger(PlayerbotAI* botAI) : Trigger(botAI, "pvp need heal") {}
    bool IsActive() override;
};

// 需要逃跑(血量极低且被追) → 脱离战斗
class PvpFleeTrigger : public Trigger
{
public:
    PvpFleeTrigger(PlayerbotAI* botAI) : Trigger(botAI, "pvp flee") {}
    bool IsActive() override;
};

//By leewheel 2026-08-30: 强化控场循环——PVP 脱战恢复后, 控制技能CD就绪, 重新投入战斗
//  实现老大要求的"控制→远遁→绷带→等CD→再来一轮"完整闭环。
//  原版无此循环(无绷带代码, 脱战仅坐下吃喝), 此为超越原版的优化。
class PvpReengageTrigger : public Trigger
{
public:
    PvpReengageTrigger(PlayerbotAI* botAI) : Trigger(botAI, "pvp cc ready reengage") {}
    bool IsActive() override;
};
//End By leewheel

#endif