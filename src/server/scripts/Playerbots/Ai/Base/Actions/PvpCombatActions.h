/* 副本机器人策略 */
#ifndef PLAYERBOTS_PVPCOMBATACTIONS_H
#define PLAYERBOTS_PVPCOMBATACTIONS_H

#include "Action.h"
#include "MovementActions.h"
#include "UseItemAction.h"

class PlayerbotAI;

// 使用 PVP 徽章解除控制(昏迷/恐惧/定身)
class PvpBreakCrowdControlAction : public Action
{
public:
    PvpBreakCrowdControlAction(PlayerbotAI* botAI)
        : Action(botAI, "pvp break crowd control") {}
    bool Execute(Event event) override;
    bool isUseful() override;
};

// 打断当前目标的施法(各职业专属打断技能由职业策略的打断动作负责,
// 此动作为通用徽章解控后的备选, 不做具体施法)
class PvpInterruptCastAction : public Action
{
public:
    PvpInterruptCastAction(PlayerbotAI* botAI)
        : Action(botAI, "pvp interrupt cast") {}
    bool Execute(Event event) override;
    bool isUseful() override;
};

// 拉开与近战敌人的距离(远程职业风筝)
class PvpKiteAction : public MovementAction
{
public:
    PvpKiteAction(PlayerbotAI* botAI)
        : MovementAction(botAI, "pvp kite") {}
    bool Execute(Event event) override;
    bool isUseful() override;
};

// 血量过低时脱离战斗(逃跑)
class PvpFleeAction : public FleeAction
{
public:
    PvpFleeAction(PlayerbotAI* botAI)
        : FleeAction(botAI) {}
    bool Execute(Event event) override;
    bool isUseful() override;
};

// 打绷带回血: 敌人被控制或距离远时打绷带, 防止被中断
//By leewheel 2026-08-29: 老大需求——低血量PVP时先控制对手再打绷带回血
class PvpBandageAction : public UseItemAction
{
public:
    PvpBandageAction(PlayerbotAI* botAI)
        : UseItemAction(botAI, "pvp bandage") {}
    bool Execute(Event event) override;
    bool isUseful() override;
};
//End By leewheel

#endif