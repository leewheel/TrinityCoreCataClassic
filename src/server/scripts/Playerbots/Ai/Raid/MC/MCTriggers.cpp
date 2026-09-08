/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "MCTriggers.h"
#include "MCHelpers.h"
//By leewheel 2026-08-23: McInLavaTrigger 改用 Map::GetLiquidStatus(TC, 需要完整 Map/LiquidData 符号)
#include "Map.h"
//End By leewheel
#include "SharedDefines.h"
#include "SpellAuras.h"

using namespace MoltenCoreHelpers;

bool McLivingBombDebuffTrigger::IsActive()
{
    // No check for Baron Geddon, because bots may have the bomb even after Geddon died.
    return bot->HasAura(SPELL_LIVING_BOMB);
}

bool McBaronGeddonInfernoTrigger::IsActive()
{
    if (Unit* boss = AI_VALUE2(Unit*, "find target", "12056")) // #12056 = Baron Geddon
        return boss->HasAura(SPELL_INFERNO);
    return false;
}

bool McShazzrahRangedTrigger::IsActive()
{
    // Only fire inside Arcane Explosion range. The move-away action no-ops
    // beyond it, so an unconditional trigger makes every already-safe ranged
    // bot attempt a failing move each tick.
    if (!PlayerbotAI::IsRanged(bot))
        return false;

    Unit* boss = AI_VALUE2(Unit*, "find target", "12264"); // #12264 = Shazzrah
    return boss && bot->GetDistance2d(boss) < ARCANE_EXPLOSION_DISTANCE;
}

bool McInLavaTrigger::IsActive()
{
    //By leewheel 2026-08-23: AC 版用 LiquidData.Flags/Status(LIQUID_MAP_*), TC 的 LiquidData 仅 type_flags/entry/level;
    // 改用 Map::GetLiquidStatus 请求 Magma 类型液体, 存在(水位在角色身上)即判定身处岩浆
    LiquidData liquid;
    ZLiquidStatus status = bot->GetMap()->GetLiquidStatus(
        bot->GetPhaseShift(), bot->GetPositionX(), bot->GetPositionY(), bot->GetPositionZ(),
        map_liquidHeaderTypeFlags::Magma, &liquid);
    return status != LIQUID_MAP_NO_WATER;
    //End By leewheel
}

bool McGolemaggMagmaSplashTrigger::IsActive()
{
    if (PlayerbotAI::IsTank(bot))
        return false;

    Unit* boss = AI_VALUE2(Unit*, "find target", "11988"); // #11988 = Golemagg the Incinerator
    if (!boss)
        return false;

    // Below 10% the fight is a burn race (Earthquake pulses, Core Ragers
    // converge): stack management stops mattering, melee return to dps.
    if (boss->GetHealthPct() <= 10.0f)
        return false;

    Aura* splash = bot->GetAura(SPELL_MAGMA_SPLASH);
    if (!splash || splash->GetStackAmount() < MAGMA_SPLASH_BACK_OFF_STACKS)
        return false;

    // Only fire while still inside swing range; once the bot has backed off,
    // the multiplier keeps it from re-engaging until the stack expires.
    return bot->GetDistance2d(boss) < MAGMA_SPLASH_BACK_OFF_DISTANCE;
}

bool McGolemaggMarkBossTrigger::IsActive()
{
    // any tank may mark the boss
    return PlayerbotAI::IsTank(bot) && AI_VALUE2(Unit*, "find target", "11988"); // Golemagg
}

bool McGolemaggIsMainTankTrigger::IsActive()
{
    return PlayerbotAI::IsMainTank(bot) && AI_VALUE2(Unit*, "find target", "11988"); // Golemagg
}

bool McGolemaggIsAssistTankTrigger::IsActive()
{
    return PlayerbotAI::IsAssistTank(bot) && AI_VALUE2(Unit*, "find target", "11988"); // Golemagg
}

bool McGolemaggIsHealerTrigger::IsActive()
{
    return PlayerbotAI::IsHeal(bot) && AI_VALUE2(Unit*, "find target", "11988"); // Golemagg
}

bool McCoreHoundMarkTrigger::IsActive()
{
    // #11671 = 熔核犬(Core Hound)
    return PlayerbotAI::IsMainTank(bot) && AI_VALUE2(Unit*, "find target", "11671");
}
