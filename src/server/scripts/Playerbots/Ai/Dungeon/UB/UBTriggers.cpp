/*
 * 幽暗沼泽 机器人策略
 */

#include "UBTriggers.h"
#include "Playerbots.h"
#include "UBShared.h"

using namespace UnderbogHungarfen;

bool UBFoulSporesTrigger::IsActive()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "17770");
    return boss && boss->HasAura(SPELL_FOUL_SPORES);
}

bool UBSporeCloudDangerTrigger::IsActive()
{
    //By leewheel 2026-09-04 核对修正: 删除原17770 boss门控——上游按"最近危险蘑菇"独立判定,
    //Hungarfen死后残留毒云仍需规避(EMERGENCY级安全动作), 原门控会让bot站在毒云里
    GuidVector const& mushrooms = AI_VALUE_REF(GuidVector, "ub mushrooms");
    return GetNearestDangerousMushroom(bot, mushrooms, MushroomDangerRange(bot)) != nullptr;
}

//By leewheel 2026-08-30: 移植上游——UBUnderbatLashTrigger 实现（幽暗蝙蝠进入鞭击范围，本地缺失实现导致链接错误）
bool UBUnderbatLashTrigger::IsActive()
{
    if (PlayerbotAI::IsTank(bot))
        return false;

    auto const& attackers = AI_VALUE_REF(GuidVector, "attackers");
    return GetNearestUnderbatInLashRange(bot, attackers) != nullptr;
}
//End By leewheel
