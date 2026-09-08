/*
 * 魔导师平台 机器人策略
 */
//By leewheel 2026-09-05: 移植来源 AC mod-playerbots MgTTriggerContext.h 移植适配 TC 框架(上游 feat tbc-mgt #2663)
//业务对标: AC azerothcore-wotlk modules/mod-playerbots src/Ai/Dungeon/MgT/MgTTriggerContext.h
//End By leewheel

#ifndef PLAYERBOTS_MGTTRIGGERCONTEXT_H
#define PLAYERBOTS_MGTTRIGGERCONTEXT_H

#include "AiObjectContext.h"
#include "MgTTriggers.h"
#include "TriggerContext.h"

class TbcDungeonMagistersTerraceTriggerContext : public NamedObjectContext<Trigger>
{
public:
    TbcDungeonMagistersTerraceTriggerContext()
    {
        creators["mgt crystal active"] = &TbcDungeonMagistersTerraceTriggerContext::mgt_crystal_active;
        creators["mgt out of room"] = &TbcDungeonMagistersTerraceTriggerContext::mgt_out_of_room;
        creators["mgt in dampening field"] = &TbcDungeonMagistersTerraceTriggerContext::mgt_in_dampening_field;
        creators["mgt mage guard at range"] = &TbcDungeonMagistersTerraceTriggerContext::mgt_mage_guard_at_range;
        creators["mgt arcane nova range"] = &TbcDungeonMagistersTerraceTriggerContext::mgt_arcane_nova_range;
        creators["mgt priority interrupt"] = &TbcDungeonMagistersTerraceTriggerContext::mgt_priority_interrupt;
        creators["mgt focus target"] = &TbcDungeonMagistersTerraceTriggerContext::mgt_focus_target;
        creators["mgt enraged wretched"] = &TbcDungeonMagistersTerraceTriggerContext::mgt_enraged_wretched;
        creators["mgt delrissa interrupt"] = &TbcDungeonMagistersTerraceTriggerContext::mgt_delrissa_interrupt;
        creators["mgt delrissa focus target"] =
            &TbcDungeonMagistersTerraceTriggerContext::mgt_delrissa_focus_target;
        creators["mgt delrissa tremor totem"] =
            &TbcDungeonMagistersTerraceTriggerContext::mgt_delrissa_tremor_totem;
        creators["mgt flame strike"] = &TbcDungeonMagistersTerraceTriggerContext::mgt_flame_strike;
        creators["mgt phoenix burn"] = &TbcDungeonMagistersTerraceTriggerContext::mgt_phoenix_burn;
        creators["mgt kael focus target"] = &TbcDungeonMagistersTerraceTriggerContext::mgt_kael_focus_target;
        creators["mgt kael interrupt"] = &TbcDungeonMagistersTerraceTriggerContext::mgt_kael_interrupt;
        creators["mgt gravity lapse"] = &TbcDungeonMagistersTerraceTriggerContext::mgt_gravity_lapse;
    }

private:
    static Trigger* mgt_crystal_active(PlayerbotAI* botAI) { return new MgTCrystalActiveTrigger(botAI); }
    static Trigger* mgt_out_of_room(PlayerbotAI* botAI) { return new MgTOutOfRoomTrigger(botAI); }
    static Trigger* mgt_in_dampening_field(PlayerbotAI* botAI) { return new MgTInDampeningFieldTrigger(botAI); }
    static Trigger* mgt_mage_guard_at_range(PlayerbotAI* botAI) { return new MgTMageGuardAtRangeTrigger(botAI); }
    static Trigger* mgt_arcane_nova_range(PlayerbotAI* botAI) { return new MgTArcaneNovaRangeTrigger(botAI); }
    static Trigger* mgt_priority_interrupt(PlayerbotAI* botAI) { return new MgTPriorityInterruptTrigger(botAI); }
    static Trigger* mgt_focus_target(PlayerbotAI* botAI) { return new MgTFocusTargetTrigger(botAI); }
    static Trigger* mgt_enraged_wretched(PlayerbotAI* botAI) { return new MgTEnragedWretchedTrigger(botAI); }
    static Trigger* mgt_delrissa_interrupt(PlayerbotAI* botAI) { return new MgTDelrissaInterruptTrigger(botAI); }
    static Trigger* mgt_delrissa_focus_target(PlayerbotAI* botAI)
    {
        return new MgTDelrissaFocusTargetTrigger(botAI);
    }
    static Trigger* mgt_delrissa_tremor_totem(PlayerbotAI* botAI)
    {
        return new MgTDelrissaTremorTotemTrigger(botAI);
    }
    static Trigger* mgt_flame_strike(PlayerbotAI* botAI) { return new MgTFlameStrikeTrigger(botAI); }
    static Trigger* mgt_phoenix_burn(PlayerbotAI* botAI) { return new MgTPhoenixBurnTrigger(botAI); }
    static Trigger* mgt_kael_focus_target(PlayerbotAI* botAI) { return new MgTKaelFocusTargetTrigger(botAI); }
    static Trigger* mgt_kael_interrupt(PlayerbotAI* botAI) { return new MgTKaelInterruptTrigger(botAI); }
    static Trigger* mgt_gravity_lapse(PlayerbotAI* botAI) { return new MgTGravityLapseTrigger(botAI); }
};

#endif
