#include "ACMultipliers.h"
#include "ACActions.h"
#include "ACTriggers.h"
#include "HunterActions.h" //By leewheel 2026-08-24: 对齐the-lab 885c0997——施法怪克制(CastDisengage)
#include "MageActions.h"   //By leewheel 2026-08-24: 对齐the-lab 885c0997——施法怪克制(CastBlinkBack)
#include "MovementActions.h"
#include "ReachTargetActions.h"
#include "AiObjectContext.h"
#include "Playerbots.h"
//By leewheel 2026-08-24: 对齐the-lab 885c0997/54937cb5——Shirrak苦难迷宫策略重构:
//FleeFocusFire简化为"非移动/非位移法术直接放行; 只有位移法术和移动类动作受限", 新增闪现/后跳克制
float ShirrakFleeFocusFireMultiplier::GetValue(Action* action)
{
    if (dynamic_cast<AttackAction*>(action))
        return 1.0f;

    bool const isMovementSpell = dynamic_cast<CastReachTargetSpellAction*>(action) ||
        dynamic_cast<CastBlinkBackAction*>(action) || dynamic_cast<CastDisengageAction*>(action);

    if (!isMovementSpell && !dynamic_cast<MovementAction*>(action))
        return 1.0f;

    if (!AI_VALUE2(Unit*, "find target", "18371"))
        return 1.0f;

    if (dynamic_cast<ShirrakFleeFocusFireAction*>(action))
        return 1.0f;

    //By leewheel 2026-09-05: 上游9ed41a81——网格列表改为FindNearestCreature查最近火焰(避免建列表),
    //位移法术(闪烁/后跳/够施法距离)在搜索半径内一律压制, 普通移动只在危险距离内压制
    Creature* flare = bot->FindNearestCreature(NPC_FOCUS_FIRE, FLARE_SEARCH_RADIUS);
    if (!flare)
        return 1.0f;

    if (isMovementSpell)
        return 0.0f;

    float currentDistance = bot->GetExactDist2d(flare);
    constexpr float safeDistance = 12.0f;
    constexpr float buffer = 2.0f;
    return currentDistance < safeDistance + buffer ? 0.0f : 1.0f;
}
//End By leewheel