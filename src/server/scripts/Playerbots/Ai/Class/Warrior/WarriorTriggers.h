/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#ifndef PLAYERBOTS_WARRIORTRIGGERS_H
#define PLAYERBOTS_WARRIORTRIGGERS_H

#include "GenericTriggers.h"
#include "PlayerbotAI.h"

class BattleShoutTrigger : public BuffTrigger
{
public:
    BattleShoutTrigger(PlayerbotAI* botAI) : BuffTrigger(botAI, "battle shout") {}
    bool IsActive() override;
};

BUFF_TRIGGER(BattleStanceTrigger, "battle stance");
BUFF_TRIGGER(DefensiveStanceTrigger, "defensive stance");
BUFF_TRIGGER(BerserkerStanceTrigger, "berserker stance");
BUFF_TRIGGER(ShieldBlockTrigger, "shield block");
BUFF_TRIGGER(CommandingShoutTrigger, "commanding shout");
DEBUFF_TRIGGER(DisarmDebuffTrigger, "disarm");
DEBUFF_TRIGGER(SunderArmorDebuffTrigger, "sunder armor");
DEBUFF_TRIGGER(MortalStrikeDebuffTrigger, "mortal strike");
// DEBUFF_ENEMY_TRIGGER(RendDebuffOnAttackerTrigger, "rend");

class RendDebuffOnAttackerTrigger : public DebuffOnMeleeAttackerTrigger
{
public:
    RendDebuffOnAttackerTrigger(PlayerbotAI* botAI) : DebuffOnMeleeAttackerTrigger(botAI, "rend") {}
};

CAN_CAST_TRIGGER(RevengeAvailableTrigger, "revenge");
CAN_CAST_TRIGGER(OverpowerAvailableTrigger, "overpower");
BUFF_TRIGGER(RampageAvailableTrigger, "rampage");
BUFF_TRIGGER_A(BloodrageBuffTrigger, "bloodrage");
CAN_CAST_TRIGGER(VictoryRushTrigger, "victory rush");
// By leewheel 2026-08-07: 改为完整类定义, 重写 GetFallbackSpellIds 添加 spell ID fallback
class SwordAndBoardTrigger : public HasAuraTrigger
{
public:
    SwordAndBoardTrigger(PlayerbotAI* botAI) : HasAuraTrigger(botAI, "sword and board") {}
    std::vector<uint32> GetFallbackSpellIds() const override { return { 46951, 46952, 46953, 50227 }; }
};
SNARE_TRIGGER(ConcussionBlowTrigger, "concussion blow");
SNARE_TRIGGER(HamstringTrigger, "hamstring");
SNARE_TRIGGER(MockingBlowTrigger, "mocking blow");
SNARE_TRIGGER(ThunderClapSnareTrigger, "thunder clap");
DEBUFF_TRIGGER(ThunderClapTrigger, "thunder clap");
SNARE_TRIGGER(TauntSnareTrigger, "taunt");
SNARE_TRIGGER(InterceptSnareTrigger, "intercept");
CD_TRIGGER(InterceptCanCastTrigger, "intercept");
SNARE_TRIGGER(ShockwaveSnareTrigger, "shockwave");
DEBUFF_TRIGGER(ShockwaveTrigger, "shockwave");
BOOST_TRIGGER(DeathWishTrigger, "death wish");
BOOST_TRIGGER(RecklessnessTrigger, "recklessness");
BUFF_TRIGGER(BloodthirstBuffTrigger, "bloodthirst");
BUFF_TRIGGER(WhirlwindTrigger, "whirlwind");
BUFF_TRIGGER(BerserkerRageBuffTrigger, "berserker rage");
INTERRUPT_HEALER_TRIGGER(ShieldBashInterruptEnemyHealerSpellTrigger, "shield bash");
INTERRUPT_TRIGGER(ShieldBashInterruptSpellTrigger, "shield bash");
INTERRUPT_HEALER_TRIGGER(PummelInterruptEnemyHealerSpellTrigger, "pummel");
INTERRUPT_TRIGGER(PummelInterruptSpellTrigger, "pummel");
INTERRUPT_HEALER_TRIGGER(InterceptInterruptEnemyHealerSpellTrigger, "intercept");
INTERRUPT_TRIGGER(InterceptInterruptSpellTrigger, "intercept");
DEFLECT_TRIGGER(SpellReflectionTrigger, "spell reflection");
// By leewheel 2026-08-07: 改为完整类定义, 重写 GetFallbackSpellIds 添加 spell ID fallback
class SuddenDeathTrigger : public HasAuraTrigger
{
public:
    SuddenDeathTrigger(PlayerbotAI* botAI) : HasAuraTrigger(botAI, "sudden death") {}
    std::vector<uint32> GetFallbackSpellIds() const override { return { 29723, 29724, 29725, 52437 }; }
};
class SlamInstantTrigger : public HasAuraTrigger
{
public:
    SlamInstantTrigger(PlayerbotAI* botAI) : HasAuraTrigger(botAI, "slam!") {}
    std::vector<uint32> GetFallbackSpellIds() const override { return { 46916 }; }
};
class TasteForBloodTrigger : public HasAuraTrigger
{
public:
    TasteForBloodTrigger(PlayerbotAI* botAI) : HasAuraTrigger(botAI, "taste for blood") {}
    std::vector<uint32> GetFallbackSpellIds() const override { return { 56636, 56637, 56638, 60503 }; }
};

class RendDebuffTrigger : public DebuffTrigger
{
public:
    RendDebuffTrigger(PlayerbotAI* botAI) : DebuffTrigger(botAI, "rend", 1, true) {}
};

class VigilanceTrigger : public BuffOnPartyTrigger
{
public:
    VigilanceTrigger(PlayerbotAI* botAI) : BuffOnPartyTrigger(botAI, "vigilance") {}

    bool IsActive() override;
};

class ShatteringThrowTrigger : public Trigger
{
public:
    ShatteringThrowTrigger(PlayerbotAI* botAI) : Trigger(botAI, "shattering throw trigger") {}

    bool IsActive() override;
};

// class SlamTrigger : public HasAuraTrigger
// {
// public:
//     SlamTrigger(PlayerbotAI* ai) : HasAuraTrigger(ai, "slam!") {}
// };

#endif
