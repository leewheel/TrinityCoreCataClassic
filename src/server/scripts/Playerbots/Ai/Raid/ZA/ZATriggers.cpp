/* 副本机器人策略 */
#include "ZATriggers.h"
#include "EncounterHelpers.h"
#include "InstanceScript.h"
#include "Playerbots.h"
#include "ZAHelpers.h"

using namespace ZaHelpers;
using namespace EncounterHelpers;

// General

bool ZulAmanNoEncounterInProgressTrigger::IsActive()
{
    if (bot->GetMapId() != ZA_MAP_ID)
        return false;

    InstanceScript* instance = bot->GetInstanceScript();
    if (!instance || instance->IsEncounterInProgress())
        return false;

    return IsMechanicTrackerBot(bot, ZA_MAP_ID);
}

// The misdirect on the pull is the same job on every boss, and every Zul'Aman boss - and nothing
// else in the instance - runs a BossAI, so "boss target" resolves whichever one the raid is on.
bool ZulAmanPullingBossTrigger::IsActive()
{
    if (bot->getClass() != CLASS_HUNTER)
        return false;

    Unit* boss = AI_VALUE(Unit*, "boss target");
    return boss && boss->GetHealthPct() > BOSS_ENGAGED_HEALTH_PCT;
}

// Trash

bool AmanishiMedicineManSummonedWardTrigger::IsActive()
{
    //By leewheel 2026-08-22: 随上游(9e8e7718..70b60954)增加机制追踪bot限定
    return IsMechanicTrackerBot(bot, ZA_MAP_ID) &&
        AI_VALUE2(Unit*, "find target", "23581");
    //End By leewheel
}

// Akil'zon <Eagle Avatar>

//By leewheel 2026-08-30: 删除旧版每boss拉怪触发器（AkilzonPullingBossTrigger 等），
//  上游新版已统一为 ZulAmanPullingBossTrigger（使用 "boss target" + BOSS_ENGAGED_HEALTH_PCT），
//  本地 .h 已同步为统一版，.cpp 旧类残留导致 C2653 编译错误
//End By leewheel

// Akil'zon <Eagle Avatar>

bool AkilzonBossEngagedByTanksTrigger::IsActiveInEncounter()
{
    //By leewheel 2026-08-22: 随上游(9e8e7718..70b60954)拆分为早退+静态调用
    if (!PlayerbotAI::IsTank(bot))
        return false;

    if (!AI_VALUE2(Unit*, "find target", "23574"))
        return false;

    return !GetElectricalStormTarget(bot);
    //End By leewheel
}

bool AkilzonBossCastsStaticDisruptionTrigger::IsActiveInEncounter()
{
    if (!PlayerbotAI::IsRanged(bot))
        return false;

    if (!AI_VALUE2(Unit*, "find target", "23574"))
        return false;

    auto it = akilzonStormTimer.find(bot->GetInstanceId());
    if (it == akilzonStormTimer.end())
        return true;

    return !IsInStormWindow(it->second);
}

bool AkilzonElectricalStormIncomingTrigger::IsActiveInEncounter()
{
    if (!AI_VALUE2(Unit*, "find target", "23574"))
        return false;

    auto it = akilzonStormTimer.find(bot->GetInstanceId());
    if (it == akilzonStormTimer.end())
        return false;

    return IsInStormWindow(it->second);
}

bool AkilzonBotsNeedToPrepareForElectricalStormTrigger::IsActiveInEncounter()
{
    if (!IsMechanicTrackerBot(bot, ZA_MAP_ID))
        return false;

    return AI_VALUE2(Unit*, "find target", "23574");
}

// Nalorakk <Bear Avatar>

bool NalorakkBossSwitchesFormsTrigger::IsActiveInEncounter()
{
    //By leewheel 2026-08-22: 随上游(9e8e7718..70b60954)先查目标再判职责
    if (!AI_VALUE2(Unit*, "find target", "23576"))
        return false;

    return PlayerbotAI::IsMainTank(bot) || PlayerbotAI::IsAssistTankOfIndex(bot, 0, true);
    //End By leewheel
}

bool NalorakkBossCastsSurgeTrigger::IsActiveInEncounter()
{
    return PlayerbotAI::IsRanged(bot) &&
           AI_VALUE2(Unit*, "find target", "23576");
}

// Jan'alai <Dragonhawk Avatar>

//By leewheel 2026-08-30: HasFireBombNearby 改 IsJanalaiBombing（对齐上游，本地 ZAHelpers 已有该函数）
bool JanalaiBossEngagedByTanksTrigger::IsActiveInEncounter()
{
    if (!PlayerbotAI::IsTank(bot))
        return false;

    Unit* janalai = AI_VALUE2(Unit*, "find target", "23578");
    return janalai && !IsJanalaiBombing(janalai);
}
//End By leewheel

//By leewheel 2026-08-30: HasFireBombNearby 改 IsJanalaiBombing
bool JanalaiBossCastsFlameBreathTrigger::IsActiveInEncounter()
{
    if (!PlayerbotAI::IsRanged(bot))
        return false;

    Unit* janalai = AI_VALUE2(Unit*, "find target", "23578");
    if (!janalai)
        return false;

    if (AI_VALUE2(Unit*, "find target", "23598"))
        return false;

    return !IsJanalaiBombing(janalai);
}
//End By leewheel

bool JanalaiBossSummoningFireBombsTrigger::IsActiveInEncounter()
{
    return IsJanalaiBombing(AI_VALUE2(Unit*, "find target", "23578"));
}

//By leewheel 2026-09-04: 上游14413ee2——孵化者触发器加血量门槛(35%全孵化后不再分配远程打孵化者)
bool JanalaiAmanishiHatchersSpawnedTrigger::IsActiveInEncounter()
{
    if (!PlayerbotAI::IsRangedDps(bot))
        return false;

    Unit* janalai = AI_VALUE2(Unit*, "find target", "23578");
    if (!janalai || janalai->GetHealthPct() <= JANALAI_HATCH_ALL_HEALTH_PCT)
        return false;

    // Just need to find one Hatcher to fire the trigger
    constexpr float searchRadius = 40.0f;
    return bot->FindNearestCreature(
               Id(ZaNpcs::NPC_AMANISHI_HATCHER), searchRadius);
}
//End By leewheel

// Halazzi <Lynx Avatar>

bool HalazziShouldBeTankedTrigger::IsActiveInEncounter()
{
    return PlayerbotAI::IsMainTank(bot) &&
           AI_VALUE2(Unit*, "find target", "23577");
}

bool HalazziSpiritLynxHasAppearedTrigger::IsActiveInEncounter()
{
    return PlayerbotAI::IsAssistTankOfIndex(bot, 0, true) &&
           AI_VALUE2(Unit*, "find target", "23577");
}

bool HalazziShouldFocusDpsTrigger::IsActiveInEncounter()
{
    return PlayerbotAI::IsDps(bot) &&
           AI_VALUE2(Unit*, "find target", "23577");
}

// Hex Lord Malacrass

bool HexLordMalacrassShouldPrioritizeAddsTrigger::IsActiveInEncounter()
{
    return PlayerbotAI::IsDps(bot) &&
           AI_VALUE2(Unit*, "find target", "24239");
}

bool HexLordMalacrassBossIsChannelingWhirlwindTrigger::IsActiveInEncounter()
{
    Unit* malacrass = AI_VALUE2(Unit*, "find target", "24239");
    if (!malacrass || malacrass->GetVictim() == bot)
        return false;

    return malacrass->HasAura(Id(ZaSpells::SPELL_HEX_LORD_WHIRLWIND));
}

bool HexLordMalacrassBossPlacedFreezingTrapTrigger::IsActiveInEncounter()
{
    //By leewheel 2026-09-04: 上游14413ee2——改走200ms缓存值(与逃跑动作共用一次网格搜索)
    return AI_VALUE2(Unit*, "find target", "24239") && GetNearbyFreezingTrap(botAI) != nullptr;
    //End By leewheel
}

// Zul'jin

bool ZuljinBossEngagedByTanksTrigger::IsActiveInEncounter()
{
    if (!PlayerbotAI::IsTank(bot))
        return false;

    Unit* zuljin = AI_VALUE2(Unit*, "find target", "23863");
    return zuljin &&
           !zuljin->HasAura(Id(ZaSpells::SPELL_SHAPE_OF_THE_EAGLE)) &&
           !zuljin->HasAura(Id(ZaSpells::SPELL_SHAPE_OF_THE_DRAGONHAWK));
}

bool ZuljinBossIsChannelingWhirlwindInTrollFormTrigger::IsActiveInEncounter()
{
    Unit* zuljin = AI_VALUE2(Unit*, "find target", "23863");
    if (!zuljin ||
        !zuljin->HasAura(Id(ZaSpells::SPELL_ZULJIN_WHIRLWIND)))
        return false;

    return !(PlayerbotAI::IsTank(bot) && zuljin->GetVictim() == bot);
}

bool ZuljinBossIsSummoningCyclonesInEagleFormTrigger::IsActiveInEncounter()
{
    Unit* zuljin = AI_VALUE2(Unit*, "find target", "23863");
    return zuljin &&
           zuljin->HasAura(Id(ZaSpells::SPELL_SHAPE_OF_THE_EAGLE));
}

bool ZuljinBossCastsAoeAbilitiesInDragonhawkFormTrigger::IsActiveInEncounter()
{
    if (!PlayerbotAI::IsRanged(bot))
        return false;

    Unit* zuljin = AI_VALUE2(Unit*, "find target", "23863");
    return zuljin &&
           zuljin->HasAura(Id(ZaSpells::SPELL_SHAPE_OF_THE_DRAGONHAWK));
}
