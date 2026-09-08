/* 副本机器人策略 */
//By leewheel 2026-09-04: 上游8baf63da——BOSS触发器实现改 IsActiveInEncounter(中间基类已闸门 IsEncounterInProgress);
//  NoEncounterInProgress 触发器类名对齐上游(GruulsLairNoEncounterInProgressTrigger)
//End By leewheel
#include "GruulTriggers.h"
#include "GruulHelpers.h"
#include "InstanceScript.h"
#include "Playerbots.h"

using namespace GruulHelpers;

// General

bool GruulsLairNoEncounterInProgressTrigger::IsActive()
{
    if (bot->GetMapId() != GRUUL_MAP_ID)
        return false;

    InstanceScript* instance = bot->GetInstanceScript();
    return instance && !instance->IsEncounterInProgress();
}

// High King Maulgar <Lord of the Ogres>

bool HighKingMaulgarThreeOgresNeedMeleeTanksTrigger::IsActiveInEncounter()
{
    if (IsBlindeyeTank(bot))
        return AI_VALUE2(Unit*, "find target", "18836");

    if (IsOlmTank(bot))
        return AI_VALUE2(Unit*, "find target", "18834");

    return IsMaulgarTank(bot) && AI_VALUE2(Unit*, "find target", "18831");
}

bool HighKingMaulgarKroshNeedsMageTankTrigger::IsActiveInEncounter()
{
    return IsKroshMageTank(bot) && AI_VALUE2(Unit*, "find target", "18832");
}

bool HighKingMaulgarKigglerNeedsMoonkinTankTrigger::IsActiveInEncounter()
{
    return IsKigglerMoonkinTank(bot) && AI_VALUE2(Unit*, "find target", "18835");
}

bool HighKingMaulgarDeterminingKillOrderTrigger::IsActiveInEncounter()
{
    if (!AI_VALUE2(Unit*, "find target", "18831"))
        return false;

    if (IsMaulgarTank(bot))
        return false;

    if (IsOlmTank(bot))
        return !AI_VALUE2(Unit*, "find target", "18834");

    if (IsBlindeyeTank(bot))
        return !AI_VALUE2(Unit*, "find target", "18836");

    if (IsKroshMageTank(bot))
        return !AI_VALUE2(Unit*, "find target", "18832");

    if (IsKigglerMoonkinTank(bot))
        return !AI_VALUE2(Unit*, "find target", "18835");

    return true;
}

bool HighKingMaulgarBossChannelingWhirlwindTrigger::IsActiveInEncounter()
{
    Unit* maulgar = AI_VALUE2(Unit*, "find target", "18831");
    if (!maulgar || !maulgar->HasAura(Id(GruulSpells::SPELL_WHIRLWIND)))
        return false;

    return !IsMaulgarTank(bot);
}

bool HighKingMaulgarKroshCastsBlastWaveTrigger::IsActiveInEncounter()
{
    if (PlayerbotAI::IsTank(bot) || IsKroshMageTank(bot))
        return false;

    return AI_VALUE2(Unit*, "find target", "18832");
}

bool HighKingMaulgarWildFelStalkerSpawnedTrigger::IsActiveInEncounter()
{
    //By leewheel 2026-09-04 对齐上游语义: 改网格搜索缓存值判空——
    //原 find target 依赖魔犬进入仇恨列表, 未被拉到时术士永远不放逐;
    //网格搜索即使魔犬未进战也能发现(上游 52e295d2/3b0ee7f7 同义)
    return bot->getClass() == CLASS_WARLOCK && !GetNearbyWildFelStalkers(botAI).empty();
}

//By leewheel 2026-09-04 对齐上游: IsActive→IsActiveInEncounter(配合 .h 的基类恢复,
//套上 IsEncounterInProgress 闸门, 开怪前/灭团重整不再白耗误导 CD)
bool HighKingMaulgarPullingOgreCouncilTrigger::IsActiveInEncounter()
{
    if (bot->getClass() != CLASS_HUNTER)
        return false;

    Unit* blindeye = AI_VALUE2(Unit*, "find target", "18836");
    return blindeye && blindeye->GetHealthPct() > BLINDEYE_ENGAGED_HEALTH_PCT;
}

//By leewheel 2026-09-04: 对齐上游b8304144——fear ward改由通用牧师策略处理，删除恐吓咆哮触发
//End By leewheel

// Gruul the Dragonkiller

bool GruulTheDragonkillerShouldBeTankedTrigger::IsActiveInEncounter()
{
    return PlayerbotAI::IsTank(bot) && AI_VALUE2(Unit*, "find target", "19044");
}

bool GruulTheDragonkillerRangedShouldSpreadTrigger::IsActiveInEncounter()
{
    return PlayerbotAI::IsRanged(bot) && AI_VALUE2(Unit*, "find target", "19044");
}

bool GruulTheDragonkillerIncomingShatterTrigger::IsActiveInEncounter()
{
    return HasGroundSlam(bot);
}
