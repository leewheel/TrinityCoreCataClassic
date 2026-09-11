/* 副本机器人策略 */
//移植来源: 玩法创意来源老大需求——暗夜精灵牧师影遁蹲守雷霆崖电梯走道, 心灵控制部落敌人跳崖
//业务对标: TC 心灵控制(605)为引导法术, 被控目标跟随施法者移动
//By leewheel 2026-08-29 新增世界 PVP 趣味玩法: 雷霆崖心灵控制伏击
//End By leewheel
#include "PvpMindControl.h"
#include "CellImpl.h"
#include "GridNotifiersImpl.h"
#include "MotionMaster.h"
#include "Playerbots.h"
#include "Player.h"
#include "Unit.h"
#include "SpellInfo.h"
#include "SpellMgr.h"

// 雷霆崖电梯走道区域(卡利姆多 map 1)
namespace
{
    constexpr float TB_WALKWAY_MIN_X = -1310.0f;
    constexpr float TB_WALKWAY_MAX_X = -1270.0f;
    constexpr float TB_WALKWAY_MIN_Y = 80.0f;
    constexpr float TB_WALKWAY_MAX_Y = 150.0f;
    constexpr float TB_WALKWAY_Z = 131.0f;

    // 走道东南悬崖边缘(被控敌人坠落点)
    constexpr float CLIFF_X = -1298.0f;
    constexpr float CLIFF_Y = 105.0f;
    constexpr float CLIFF_Z = 131.0f;

    // 牧师伏击点: 悬崖边缘附近, 便于心灵控制后把敌人带向边缘
    constexpr float AMBUSH_X = -1286.0f;
    constexpr float AMBUSH_Y = 105.0f;
    constexpr float AMBUSH_Z = 131.0f;

    constexpr uint32 SPELL_SHADOWMELD = 58984;      // 影遁
    constexpr uint32 SPELL_MIND_CONTROL = 605;      // 精神控制
    constexpr uint32 SPELL_MIND_CONTROL_MAJOR = 1098; // 精神控制(强化)

    //By leewheel 2026-08-29: 随机轮换——并非每个暗夜精灵牧师每天都去蹲守,
    //按 (GUID低32位 + 当天日期) 哈希决定今天是否参与, 保证每天换人
    bool ShouldParticipateToday(Player* bot)
    {
        uint32 const today = time(nullptr) / 86400; // 天序号
        uint32 const seed = static_cast<uint32>(bot->GetGUID().GetCounter()) + today * 7919u;
        // 约 1/3 的暗夜精灵牧师当天参与伏击, 其余照常做自己平时的事(正常AI行为)
        return (seed % 3) == 0;
    }
    //End By leewheel
}

bool IsThunderBluffWalkway(Player* bot)
{
    if (bot->GetMapId() != 1)
        return false;

    float x = bot->GetPositionX();
    float y = bot->GetPositionY();
    return x >= TB_WALKWAY_MIN_X && x <= TB_WALKWAY_MAX_X &&
           y >= TB_WALKWAY_MIN_Y && y <= TB_WALKWAY_MAX_Y;
}

// 找到靠近的敌对玩家(部落)
Player* FindNearbyEnemyOnWalkway(Player* bot, float maxDist = 40.0f)
{
    //By leewheel 2026-09-09: TC用AnyPlayerInPositionRangeCheck替代AnyPlayerInObjectRangeCheck
    std::list<Player*> nearby;
    Trinity::AnyPlayerInPositionRangeCheck check(bot, maxDist);
    Trinity::PlayerListSearcher<Trinity::AnyPlayerInPositionRangeCheck> searcher(bot, nearby, check);
    //End By leewheel
    Cell::VisitAllObjects(bot, searcher, maxDist);

    for (Player* player : nearby)
    {
        if (!player || player == bot || !player->IsAlive() || !player->IsInWorld())
            continue;

        if (player->GetTeamId() == bot->GetTeamId())
            continue;

        // 只伏击走道上的目标(即将经过)
        if (IsThunderBluffWalkway(player))
            return player;
    }

    return nullptr;
}

// 触发: 暗夜精灵牧师 + 当天轮到蹲守 + 在雷霆崖电梯走道 + 有部落靠近
bool ThunderBluffMindControlTrigger::IsActive()
{
    // 只暗夜精灵牧师参与(会影遁隐身)
    if (bot->getRace() != RACE_NIGHTELF || bot->getClass() != CLASS_PRIEST)
        return false;

    // 随机轮换: 当天没轮到蹲守的牧师照常做自己平时的事(打本/任务/逛街),
    // 伏击触发完全不介入其正常AI行为
    if (!ShouldParticipateToday(bot))
        return false;

    if (!bot->HasSpell(SPELL_MIND_CONTROL) && !bot->HasSpell(SPELL_MIND_CONTROL_MAJOR))
        return false;

    if (!IsThunderBluffWalkway(bot))
        return false;

    return FindNearbyEnemyOnWalkway(bot) != nullptr;
}

// 影遁隐身蹲守
bool PvpShadowmeldAction::Execute(Event /*event*/)
{
    // 先走到悬崖边缘伏击点蹲守, 到位后再影遁隐身
    if (bot->GetExactDist2d(AMBUSH_X, AMBUSH_Y) > 3.0f)
    {
        return MoveTo(1, AMBUSH_X, AMBUSH_Y, AMBUSH_Z, false, false, false, false,
            MovementPriority::MOVEMENT_COMBAT, true, false);
    }

    if (botAI->CanCastSpell(SPELL_SHADOWMELD, bot))
        return botAI->CastSpell(SPELL_SHADOWMELD, bot);

    return false;
}

bool PvpShadowmeldAction::isUseful()
{
    // 未隐身且目标不在身边时隐身
    if (bot->HasAura(SPELL_SHADOWMELD) || bot->IsInCombat())
        return false;

    // 目标距离较远时蹲守隐身, 目标贴近时不隐身(方便直接控制)
    Player* enemy = FindNearbyEnemyOnWalkway(bot, 25.0f);
    return !enemy;
}

// 心灵控制: 控制靠近的部落敌人
bool PvpMindControlAction::Execute(Event /*event*/)
{
    Player* enemy = FindNearbyEnemyOnWalkway(bot);
    if (!enemy)
        return false;

    // 目标已被他人控制则跳过
    if (enemy->IsCharmed())
        return false;

    uint32 mindControl = bot->HasSpell(SPELL_MIND_CONTROL_MAJOR) ?
        SPELL_MIND_CONTROL_MAJOR : SPELL_MIND_CONTROL;

    if (botAI->CanCastSpell(mindControl, enemy))
        return botAI->CastSpell(mindControl, enemy);

    return false;
}

bool PvpMindControlAction::isUseful()
{
    if (!IsThunderBluffWalkway(bot) || bot->IsInCombat())
        return false;

    Player* enemy = FindNearbyEnemyOnWalkway(bot, 20.0f);
    if (!enemy)
        return false;

    // 目标未处于控制状态才施放
    return !enemy->IsCharmed();
}

// 引导心灵控制走向悬崖: 命令被控目标自己走向悬崖边缘坠落
//By leewheel 2026-08-29: 修正实现——心灵控制(605)是引导法术, 牧师本体移动会打断引导,
//                        正确做法是命令被控目标(GetCharmed)走向悬崖, 牧师原地保持引导
bool PvpMindControlToCliffAction::Execute(Event /*event*/)
{
    // 检查自己是否正在引导心灵控制
    Unit* charmed = bot->GetCharmed();
    if (!charmed || !charmed->IsAlive() || !charmed->IsPlayer())
        return false;

    float distToCliff = charmed->GetExactDist2d(CLIFF_X, CLIFF_Y);
    if (distToCliff > 2.0f)
    {
        // 命令被控目标走向悬崖边缘(牧师本体原地引导不动)
        charmed->GetMotionMaster()->MovePoint(1, CLIFF_X, CLIFF_Y, CLIFF_Z);
        return true;
    }

    // 被控目标已到悬崖边缘: 命令其再向前一步, 越过边缘坠落
    charmed->GetMotionMaster()->MovePoint(2, CLIFF_X - 1.0f, CLIFF_Y, CLIFF_Z - 8.0f);
    return true;
}

bool PvpMindControlToCliffAction::isUseful()
{
    // 自己正在引导心灵控制且被控目标存活
    return bot->GetCharmed() && bot->GetCharmed()->IsAlive() && bot->GetCharmed()->IsPlayer();
}
//End By leewheel