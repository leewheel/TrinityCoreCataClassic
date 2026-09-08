/* 风暴要塞 机器人策略 */
//By leewheel 2026-09-04: 上游4f9815d1——Boss 触发器继承 TempestKeepEncounterTrigger 中间基类,
//   实现函数改名 IsActiveInEncounter(IsActive 由基类 final 提供, 内含 IsEncounterInProgress 闸门)
//End By leewheel
#include "TKTriggers.h"
#include "EncounterHelpers.h"
#include "InstanceScript.h"
#include "MoveSpline.h"
#include "Playerbots.h"
#include "TKHelpers.h"
//By leewheel 2026-08-21: 移植 brighton-chi 615c78d2——阶段读取改用 GetKaelthasPhase,不再需要 boss_kaelthas 镜像类
//End By leewheel
#include <array>

using namespace TkHelpers;
using namespace EncounterHelpers;

// General

// Read the instance's own encounter state rather than the bot's combat state to determine when
// it is safe to erase encounter maps.
bool TempestKeepNoEncounterInProgressTrigger::IsActive()
{
    if (!IsMechanicTrackerBot(bot, TK_MAP_ID))
        return false;

    InstanceScript* instance = bot->GetInstanceScript();
    return instance && !instance->IsEncounterInProgress();
}

//By leewheel 2026-08-24: 移植 brighton-chi c9a1a088——机器人卡在坠落flag(TC适配: 保留机器人战斗状态自检)
bool TempestKeepBotIsStuckFallingTrigger::IsActive()
{
    return bot->HasUnitMovementFlag(MOVEMENTFLAG_FALLING) && bot->movespline->Finalized() &&
        bot->GetMapId() == TK_MAP_ID && !AI_VALUE2(bool, "combat", "self target");
}
//End By leewheel

// Trash

bool CrimsonHandCenturionCastsArcaneFlurryTrigger::IsActive()
{
    return bot->getClass() == CLASS_MAGE &&
        AI_VALUE2(Unit*, "find target", "20048");
}

// Al'ar <Phoenix God>

bool AlarPullingBossTrigger::IsActive()
{
    if (bot->getClass() != CLASS_HUNTER)
        return false;

    Unit* alar = AI_VALUE2(Unit*, "find target", "19514");
    return alar && alar->GetHealthPct() > BOSS_ENGAGED_HEALTH_PCT;
}

bool AlarBossIsFlyingBetweenPlatformsTrigger::IsActiveInEncounter()
{
    Unit* alar = AI_VALUE2(Unit*, "find target", "19514");
    //By leewheel 2026-08-21: 移植 brighton-chi 78f030ba——GetInstanceId() 取代 GetMap()->GetInstanceId()
    if (!alar || IsAlarInPhase2(alar->GetInstanceId()))
        return false;
    //End By leewheel

    int8 locationIndex = GetAlarCurrentLocationIndex(alar);
    if (locationIndex == LOCATION_NONE)
        locationIndex = GetAlarDestinationLocationIndex(alar);

    return locationIndex != POINT_QUILL_OR_DIVE_IDX && locationIndex != POINT_MIDDLE_IDX;
}

bool AlarEmbersExplodeUponDeathTrigger::IsActiveInEncounter()
{
    return PlayerbotAI::IsTank(bot) && AI_VALUE2(Unit*, "find target", "19551");
}

bool AlarKillingEmbersDamagesBossTrigger::IsActiveInEncounter()
{
    return PlayerbotAI::IsRangedDps(bot) && AI_VALUE2(Unit*, "find target", "19551");
}

bool AlarIncomingFlameQuillsTrigger::IsActiveInEncounter()
{
    Unit* alar = AI_VALUE2(Unit*, "find target", "19514");
    //By leewheel 2026-08-21: 移植 brighton-chi 78f030ba——GetInstanceId() 取代 GetMap()->GetInstanceId()
    if (!alar || IsAlarInPhase2(alar->GetInstanceId()))
        return false;
    //End By leewheel

    return GetAlarCurrentLocationIndex(alar) == POINT_QUILL_OR_DIVE_IDX ||
        GetAlarDestinationLocationIndex(alar) == POINT_QUILL_OR_DIVE_IDX;
}

bool AlarRisingFromTheAshesTrigger::IsActiveInEncounter()
{
    Unit* alar = AI_VALUE2(Unit*, "find target", "19514");
    if (!alar || alar->GetHealthPct() > 5.0f)
        return false;

    //By leewheel 2026-08-21: 移植 brighton-chi 78f030ba——GetInstanceId() 取代 GetMap()->GetInstanceId()
    if (IsAlarInPhase2(alar->GetInstanceId()))
        return false;
    //End By leewheel

    return GetAlarCurrentLocationIndex(alar) != POINT_QUILL_OR_DIVE_IDX &&
        GetAlarDestinationLocationIndex(alar) != POINT_QUILL_OR_DIVE_IDX;
}

bool AlarIsInPhase2Trigger::IsActiveInEncounter()
{
    Unit* alar = AI_VALUE2(Unit*, "find target", "19514");
    //By leewheel 2026-08-21: 移植 brighton-chi 78f030ba——GetInstanceId() 取代 GetMap()->GetInstanceId()
    return alar && IsAlarInPhase2(alar->GetInstanceId());
    //End By leewheel
}

bool AlarShouldManagePhaseTrackerTrigger::IsActiveInEncounter()
{
    return IsMechanicTrackerBot(bot, TK_MAP_ID) && AI_VALUE2(Unit*, "find target", "19514");
}

// Void Reaver

bool VoidReaverShouldBeTankedTrigger::IsActiveInEncounter()
{
    return PlayerbotAI::IsTank(bot) && AI_VALUE2(Unit*, "find target", "19516");
}

bool VoidReaverKnockAwayPullsAggroToNonTanksTrigger::IsActiveInEncounter()
{
    if (bot->getClass() == CLASS_DEATH_KNIGHT || bot->getClass() == CLASS_DRUID ||
        bot->getClass() == CLASS_SHAMAN || bot->getClass() == CLASS_WARRIOR)
    {
        return false;
    }

    if (PlayerbotAI::IsTank(bot))
        return false;

    Unit* voidReaver = AI_VALUE2(Unit*, "find target", "19516");
    return voidReaver && voidReaver->GetVictim() == bot;
}

bool VoidReaverRangedShouldStandBackTrigger::IsActiveInEncounter()
{
    if (!PlayerbotAI::IsRanged(bot))
        return false;

    Unit* voidReaver = AI_VALUE2(Unit*, "find target", "19516");
    if (!voidReaver || voidReaver->GetVictim() == bot)
        return false;

    return !IsNearActiveArcaneOrb(bot, ARCANE_ORB_BUFFER_DISTANCE);
}

bool VoidReaverArcaneOrbIsIncomingTrigger::IsActiveInEncounter()
{
    if (PlayerbotAI::IsTank(bot))
        return false;

    Unit* voidReaver = AI_VALUE2(Unit*, "find target", "19516");
    if (!voidReaver || voidReaver->GetVictim() == bot)
        return false;

    return IsNearActiveArcaneOrb(bot, ARCANE_ORB_SAFE_DISTANCE);
}

// High Astromancer Solarian

bool HighAstromancerSolarianShouldBeTankedTrigger::IsActiveInEncounter()
{
    if (!PlayerbotAI::IsMainTank(bot))
        return false;

    Unit* astromancer = AI_VALUE2(Unit*, "find target", "18805");
    if (!astromancer)
        return false;

    Creature* astromancerCreature = astromancer->ToCreature();
    return astromancerCreature && astromancerCreature->GetReactState() != REACT_PASSIVE;
}

bool HighAstromancerSolarianBotHasWrathOfTheAstromancerTrigger::IsActiveInEncounter()
{
    return HasWrathOfTheAstromancer(bot);
}

bool HighAstromancerSolarianSolariumPriestsSpawnedTrigger::IsActiveInEncounter()
{
    if (!PlayerbotAI::IsMelee(bot) || PlayerbotAI::IsMainTank(bot))
        return false;

    return AI_VALUE2(Unit*, "find target", "18806");
}

//By leewheel 2026-09-04: 对齐上游b8304144——fear ward改由通用牧师策略处理，删除尖叫触发
//End By leewheel

// Kael'thas Sunstrider <Lord of the Blood Elves>

bool KaelthasSunstriderThaladredIsFixatedOnBotTrigger::IsActiveInEncounter()
{
    Unit* thaladred = AI_VALUE2(Unit*, "find target", "20064");
    if (!thaladred || thaladred->GetVictim() != bot)
        return false;

    Unit* kaelthas = AI_VALUE2(Unit*, "find target", "19622");
    if (!kaelthas)
        return false;

    //By leewheel 2026-08-21: 移植 brighton-chi 615c78d2——阶段读取改用 GetKaelthasPhase
    uint32 const phase = GetKaelthasPhase(kaelthas);
    if (PlayerbotAI::IsTank(bot) && phase == PHASE_ALL_ADVISORS)
        return false;

    return phase != PHASE_NONE;
}

bool KaelthasSunstriderPullingTankableAdvisorsTrigger::IsActiveInEncounter()
{
    if (bot->getClass() != CLASS_HUNTER)
        return false;

    Unit* kaelthas = AI_VALUE2(Unit*, "find target", "19622");
    if (!kaelthas)
        return false;

    uint32 const phase = GetKaelthasPhase(kaelthas);
    return phase == PHASE_SINGLE_ADVISOR || phase == PHASE_ALL_ADVISORS;
    //End By leewheel
}

bool KaelthasSunstriderSanguinarOrTelonicusShouldBeTankedTrigger::IsActiveInEncounter()
{
    if (!PlayerbotAI::IsTank(bot))
        return false;

    if (PlayerbotAI::IsMainTank(bot))
        return IsAdvisorActive(AI_VALUE2(Unit*, "find target", "20060"));

    if (PlayerbotAI::IsAssistTankOfIndex(bot, 0, true))
        return IsAdvisorActive(AI_VALUE2(Unit*, "find target", "20063"));

    return false;
}
//End By leewheel

//By leewheel 2026-09-04: 对齐上游b8304144——fear ward改由通用牧师策略处理，删除咆哮触发
//End By leewheel

bool KaelthasSunstriderCapernianShouldBeTankedByWarlockTrigger::IsActiveInEncounter()
{
    if (bot->getClass() != CLASS_WARLOCK || GetCapernianTank(bot) != bot)
        return false;

    return IsAdvisorActive(AI_VALUE2(Unit*, "find target", "20062"));
}

bool KaelthasSunstriderCapernianBlowsUpNearAndFarTrigger::IsActiveInEncounter()
{
    if (!IsAdvisorActive(AI_VALUE2(Unit*, "find target", "20062")))
        return false;

    if (bot->getClass() == CLASS_WARLOCK && GetCapernianTank(bot) == bot)
        return false;

    return true;
}

bool KaelthasSunstriderBotsShouldHoldPhase3PositionsTrigger::IsActiveInEncounter()
{
    Unit* kaelthas = AI_VALUE2(Unit*, "find target", "19622");
    if (!kaelthas)
        return false;

    //By leewheel 2026-08-21: 移植 brighton-chi 615c78d2——阶段读取改用 GetKaelthasPhase
    if (GetKaelthasPhase(kaelthas) != PHASE_ALL_ADVISORS)
        return false;
    //End By leewheel

    Unit* sanguinar = AI_VALUE2(Unit*, "find target", "20060");
    // The healer holds its spot from the start of the revival until Sanguinar dies, since that
    // spot is what keeps both melee tanks in range.
    if (PlayerbotAI::IsAssistHealOfIndex(bot, 0, true))
        return sanguinar && sanguinar->IsAlive();

    // The Sanguinar check is a proxy for the revival/Kael talk phase (any non-selectable advisor
    // would do, since all four revive together, but Sanguinar is already needed for the healer).
    if (!sanguinar || !sanguinar->HasUnitFlag(UNIT_FLAG_NOT_SELECTABLE))
        return false;

    return PlayerbotAI::IsMainTank(bot) ||
        PlayerbotAI::IsAssistTankOfIndex(bot, 0, true) ||
        (bot->getClass() == CLASS_WARLOCK && GetCapernianTank(bot) == bot);
}
//End By leewheel

bool KaelthasSunstriderDeterminingAdvisorKillOrderTrigger::IsActiveInEncounter()
{
    if (PlayerbotAI::IsMainTank(bot) || PlayerbotAI::IsAssistTankOfIndex(bot, 0, true))
        return false;

    Unit* kaelthas = AI_VALUE2(Unit*, "find target", "19622");
    if (!kaelthas)
        return false;

    //By leewheel 2026-08-21: 移植 brighton-chi 615c78d2——阶段读取改用 GetKaelthasPhase
    uint32 const phase = GetKaelthasPhase(kaelthas);
    return phase == PHASE_SINGLE_ADVISOR || phase == PHASE_ALL_ADVISORS;
    //End By leewheel
}

bool KaelthasSunstriderShouldManageAdvisorDpsTimerTrigger::IsActiveInEncounter()
{
    if (!IsMechanicTrackerBot(bot, TK_MAP_ID))
        return false;

    Unit* kaelthas = AI_VALUE2(Unit*, "find target", "19622");
    if (!kaelthas)
        return false;

    //By leewheel 2026-08-21: 移植 brighton-chi 615c78d2——阶段读取改用 GetKaelthasPhase
    return GetKaelthasPhase(kaelthas) == PHASE_SINGLE_ADVISOR;
    //End By leewheel
}

bool KaelthasSunstriderLegendaryWeaponsAreAliveTrigger::IsActiveInEncounter()
{
    Unit* kaelthas = AI_VALUE2(Unit*, "find target", "19622");
    if (!kaelthas)
        return false;

    //By leewheel 2026-08-21: 移植 brighton-chi 615c78d2——阶段读取改用 GetKaelthasPhase
    if (GetKaelthasPhase(kaelthas) != PHASE_WEAPONS)
        return false;
    //End By leewheel

    if (PlayerbotAI::IsMainTank(bot))
        return false;

    return true;
}

bool KaelthasSunstriderLegendaryAxeCastsWhirlwindTrigger::IsActiveInEncounter()
{
    return PlayerbotAI::IsMainTank(bot) &&
        GetLegendaryWeapon(bot, Id(TkNpcs::NPC_DEVASTATION)) != nullptr;
}

bool KaelthasSunstriderLegendaryWeaponsAreDeadTrigger::IsActiveInEncounter()
{
    Unit* kaelthas = AI_VALUE2(Unit*, "find target", "19622");
    if (!kaelthas)
        return false;

    //By leewheel 2026-08-21: 移植 brighton-chi 615c78d2——阶段读取改用 GetKaelthasPhase
    uint32 const phase = GetKaelthasPhase(kaelthas);
    if (phase < PHASE_WEAPONS || phase > PHASE_ALL_ADVISORS)
        return false;
    //End By leewheel

    Unit* axe = GetLegendaryWeapon(bot, Id(TkNpcs::NPC_DEVASTATION));
    if (axe && axe->GetVictim() == bot)
        return false;

    return !GetDeadLegendaryWeaponGuids(botAI).empty();
}

bool KaelthasSunstriderLegendaryWeaponsAreEquippedTrigger::IsActiveInEncounter()
{
    if (PlayerbotAI::IsHeal(bot))
        return false;

    if (PlayerbotAI::IsMelee(bot) && PlayerbotAI::IsDps(bot))
        return false;

    if (!AI_VALUE2(Unit*, "find target", "19622"))
        return false;

    return GetEquippedItemInSlot(
               bot, EQUIPMENT_SLOT_MAINHAND, Id(TkItems::ITEM_STAFF_OF_DISINTEGRATION)) ||
        GetEquippedItemInSlot(
               bot, EQUIPMENT_SLOT_RANGED, Id(TkItems::ITEM_NETHERSTRAND_LONGBOW)) ||
        GetEquippedItemInSlot(
               bot, EQUIPMENT_SLOT_OFFHAND, Id(TkItems::ITEM_PHASESHIFT_BULWARK));
}

bool KaelthasSunstriderLegendaryWeaponsWereLostTrigger::IsActiveInEncounter()
{
    if (bot->GetMapId() != TK_MAP_ID)
        return false;

    if (AI_VALUE2(bool, "combat", "self target"))
        return false;

    constexpr uint32 kaelthasDbGuid = 158218;
    auto const& creatureStore = bot->GetMap()->GetCreatureBySpawnIdStore();
    auto it = creatureStore.find(kaelthasDbGuid);
    if (it == creatureStore.end())
        return false;

    Creature* kaelthas = it->second;
    //By leewheel 2026-08-23: 常量化——KAELTHAS_ROOM_SEARCH_DISTANCE(125)
    if (!kaelthas || bot->GetExactDist2d(kaelthas) > KAELTHAS_ROOM_SEARCH_DISTANCE)
        return false;

    static constexpr std::array weaponSlots = {
        EQUIPMENT_SLOT_MAINHAND, EQUIPMENT_SLOT_OFFHAND, EQUIPMENT_SLOT_RANGED, };

    for (uint8 slot : weaponSlots)
    {
        if (!bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot) && HasEquippableItemForSlot(bot, slot))
            return true;
    }

    return false;
}

bool KaelthasSunstriderBossHasEnteredTheFightTrigger::IsActiveInEncounter()
{
    Unit* kaelthas = AI_VALUE2(Unit*, "find target", "19622");
    if (!kaelthas)
        return false;

    //By leewheel 2026-08-21: 移植 brighton-chi 615c78d2——阶段读取改用 GetKaelthasPhase
    return GetKaelthasPhase(kaelthas) == PHASE_FINAL;
    //End By leewheel
}

bool KaelthasSunstriderPhoenixesAndEggsAreSpawningTrigger::IsActiveInEncounter()
{
    Unit* kaelthas = AI_VALUE2(Unit*, "find target", "19622");
    if (!kaelthas || kaelthas->GetVictim() == bot)
        return false;

    //By leewheel 2026-08-21: 移植 brighton-chi 615c78d2——阶段读取改用 GetKaelthasPhase
    if (GetKaelthasPhase(kaelthas) != PHASE_FINAL)
        return false;
    //End By leewheel

    if (PlayerbotAI::IsMainTank(bot))
        return false;

    //By leewheel 2026-08-21: 移植 brighton-chi 50bdd1c8——凤凰蛋改走 GetPhoenixEgg() 网格查找
    if (AI_VALUE2(Unit*, "find target", "21362"))
        return true;

    return GetPhoenixEgg(bot);
    //End By leewheel
}

bool KaelthasSunstriderRaidMemberIsMindControlledTrigger::IsActiveInEncounter()
{
    if (PlayerbotAI::IsCaster(bot))
        return false;

    Unit* kaelthas = AI_VALUE2(Unit*, "find target", "19622");
    if (!kaelthas)
        return false;

    if (PlayerbotAI::IsTank(bot) && kaelthas->GetVictim() == bot)
        return false;

    if (!bot->HasItemCount(Id(TkItems::ITEM_INFINITY_BLADE), 1, true))
        return false;

    Group* group = bot->GetGroup();
    if (!group)
        return false;

    for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
    {
        Player* member = ref->GetSource();
        if (member && member->HasAura(Id(TkSpells::SPELL_KAELTHAS_MIND_CONTROL)))
            return true;
    }

    return false;
}

bool KaelthasSunstriderBossIsManipulatingGravityTrigger::IsActiveInEncounter()
{
    Unit* kaelthas = AI_VALUE2(Unit*, "find target", "19622");
    return kaelthas && kaelthas->GetHealthPct() <= 50.0f;
}
