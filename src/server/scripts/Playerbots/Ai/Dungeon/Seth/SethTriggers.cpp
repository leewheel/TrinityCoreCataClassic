/* 地下城机器人策略 */
#include "SethTriggers.h"
#include "EncounterHelpers.h"
#include "Playerbots.h"
#include "SethShared.h"

using namespace SethShared;
using namespace EncounterHelpers;

bool TimeLostControllerDropsCharmingTotemTrigger::IsActive()
{
    return IsMechanicTrackerBot(bot, SETH_MAP_ID) &&
        AI_VALUE2(Unit*, "find target", "18327");
}

bool SethekkProphetCastsFearTrigger::IsActive()
{
    if (bot->getClass() != CLASS_SHAMAN)
        return false;

    if (!AI_VALUE2(Unit*, "find target", "18325"))
        return false;

    return !AI_VALUE2(bool, "has totem", "tremor totem");
}

bool DarkweaverSythBossSummonsElementalsTrigger::IsActive()
{
    if (!IsMechanicTrackerBot(bot, SETH_MAP_ID))
        return false;

    Unit* syth = AI_VALUE2(Unit*, "find target", "18472");
    return syth && syth->GetHealthPct() > BOSS_BURN_HEALTH_PCT;
}

bool AnzuEncounterHasTwoPhasesTrigger::IsActive()
{
    return AI_VALUE2(Unit*, "find target", "23035");
}

bool AnzuBirdSpiritsProvideBuffsTrigger::IsActive()
{
    return bot->getClass() == CLASS_DRUID && PlayerbotAI::IsHeal(bot) &&
        AI_VALUE2(Unit*, "find target", "23035");
}

bool TalonKingIkissBossEngagedByTankTrigger::IsActive()
{
    if (!PlayerbotAI::IsTank(bot))
        return false;

    Unit* ikiss = AI_VALUE2(Unit*, "find target", "18473");
    return ikiss && ikiss->GetVictim() == bot && bot->IsWithinMeleeRange(ikiss);
}

bool TalonKingIkissRangedPrepareForArcaneExplosionTrigger::IsActive()
{
    if (!PlayerbotAI::IsRanged(bot))
        return false;

    Unit* ikiss = AI_VALUE2(Unit*, "find target", "18473");
    return ikiss && !ikiss->HasAura(Id(SethSpells::SPELL_ARCANE_BUBBLE)) &&
        bot->IsWithinLOSInMap(ikiss);
}

bool TalonKingIkissBossCastingArcaneExplosionTrigger::IsActive()
{
    // Arcane Bubble is put up 1s before casting Arcane Explosion
    Unit* ikiss = AI_VALUE2(Unit*, "find target", "18473");
    return ikiss && ikiss->HasAura(Id(SethSpells::SPELL_ARCANE_BUBBLE));
}

bool TalonKingIkissBossOutOfLosTrigger::IsActive()
{
    Unit* ikiss = AI_VALUE2(Unit*, "find target", "18473");
    return ikiss && !ikiss->HasAura(Id(SethSpells::SPELL_ARCANE_BUBBLE)) &&
        !bot->IsWithinLOSInMap(ikiss);
}
