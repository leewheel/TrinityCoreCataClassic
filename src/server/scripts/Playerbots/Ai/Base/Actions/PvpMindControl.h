/* 副本机器人策略 */
#ifndef PLAYERBOTS_PVPMINDCONTROL_H
#define PLAYERBOTS_PVPMINDCONTROL_H

#include "Trigger.h"
#include "MovementActions.h"
#include "Action.h"

class PlayerbotAI;

// 伏击触发: 暗夜精灵牧师蹲守雷霆崖电梯走道, 有部落敌人靠近
class ThunderBluffMindControlTrigger : public Trigger
{
public:
    ThunderBluffMindControlTrigger(PlayerbotAI* botAI)
        : Trigger(botAI, "thunder bluff mind control") {}
    bool IsActive() override;
};

// 影遁隐身: 蹲守时先走到悬崖边缘伏击点, 再隐身等敌人靠近
class PvpShadowmeldAction : public MovementAction
{
public:
    PvpShadowmeldAction(PlayerbotAI* botAI)
        : MovementAction(botAI, "pvp shadowmeld") {}
    bool Execute(Event event) override;
    bool isUseful() override;
};

// 心灵控制: 控制靠近的部落敌人
class PvpMindControlAction : public Action
{
public:
    PvpMindControlAction(PlayerbotAI* botAI)
        : Action(botAI, "pvp mind control") {}
    bool Execute(Event event) override;
    bool isUseful() override;
};

// 引导心灵控制走向悬崖: 命令被控目标走到悬崖边缘坠落
class PvpMindControlToCliffAction : public MovementAction
{
public:
    PvpMindControlToCliffAction(PlayerbotAI* botAI)
        : MovementAction(botAI, "pvp mind control to cliff") {}
    bool Execute(Event event) override;
    bool isUseful() override;
};

#endif