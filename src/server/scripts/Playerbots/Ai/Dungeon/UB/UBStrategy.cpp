/*
 * 幽暗沼泽 机器人策略
 */

#include "UBStrategy.h"
#include "Playerbots.h"
#include "UBMultipliers.h"

void TbcDungeonUnderbogStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode("ub foul spores", {
        NextAction("ub retreat from foul spores", ACTION_EMERGENCY + 10) }));

    triggers.push_back(new TriggerNode("ub spore cloud danger", {
        NextAction("ub vacate spore cloud", ACTION_EMERGENCY + 2) }));

    //By leewheel 2026-09-04: 补齐上游 HEAD 接线——幽暗蝙蝠鞭击应对(实现类已在,
    //此前仅补了实现未注册触发器, 功能处于休眠状态)
    triggers.push_back(new TriggerNode("ub underbat lash", {
        NextAction("ub clear underbat back", ACTION_EMERGENCY + 1) }));
    //End By leewheel
}

void TbcDungeonUnderbogStrategy::InitMultipliers(std::vector<Multiplier*>& multipliers)
{
    multipliers.push_back(new HungarfenFoulSporesMultiplier(botAI));
    multipliers.push_back(new HungarfenMushroomIgnoreMultiplier(botAI));
    //By leewheel 2026-09-04: 补齐上游 HEAD 接线——鞭击范围内禁 SetBehindTarget(转身背对蝙蝠会挨鞭)
    multipliers.push_back(new UnderbatFacingMultiplier(botAI));
    //End By leewheel
}

void TbcDungeonUnderbogStrategy::AppendTargetExclusions(GuidSet& exclusions, TargetValueExclusionType /*type*/)
{
    AiObjectContext* context = botAI->GetAiObjectContext();
    if (!AI_VALUE2(Unit*, "find target", "17770"))
        return;

    GuidVector const& mushrooms = AI_VALUE_REF(GuidVector, "ub mushrooms");
    exclusions.insert(mushrooms.begin(), mushrooms.end());
}
