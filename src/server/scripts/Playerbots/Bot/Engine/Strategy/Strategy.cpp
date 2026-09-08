/*
 * 策略基类实现
 * 从 AzerothCore mod-playerbots 移植到 TrinityCore
 */

#include "Strategy.h"

#include "Playerbots.h"

class ActionNodeFactoryInternal : public NamedObjectFactory<ActionNode>
{
public:
    ActionNodeFactoryInternal()
    {
        creators["melee"] = &melee;
        creators["healthstone"] = &healthstone;
        creators["be near"] = &follow_master_random;
        creators["attack anything"] = &attack_anything;
        creators["move random"] = &move_random;
        creators["move to loot"] = &move_to_loot;
        creators["food"] = &food;
        creators["drink"] = &drink;
        creators["mana potion"] = &mana_potion;
        creators["healing potion"] = &healing_potion;
        creators["flee"] = &flee;
    }

private:
    static ActionNode* melee([[maybe_unused]] PlayerbotAI* botAI)
    {
        return new ActionNode("melee", {}, {}, {});
    }

    static ActionNode* healthstone([[maybe_unused]] PlayerbotAI* botAI)
    {
        return new ActionNode("healthstone", {}, { NextAction("healing potion") }, {});
    }

    static ActionNode* follow_master_random([[maybe_unused]] PlayerbotAI* botAI)
    {
        return new ActionNode("be near", {}, { NextAction("follow") }, {});
    }

    static ActionNode* attack_anything([[maybe_unused]] PlayerbotAI* botAI)
    {
        return new ActionNode("attack anything", {}, {}, {});
    }

    static ActionNode* move_random([[maybe_unused]] PlayerbotAI* botAI)
    {
        return new ActionNode("move random", {}, { NextAction("stay line") }, {});
    }

    static ActionNode* move_to_loot([[maybe_unused]] PlayerbotAI* botAI)
    {
        return new ActionNode("move to loot", {}, {}, {});
    }

    static ActionNode* food([[maybe_unused]] PlayerbotAI* botAI)
    {
        return new ActionNode("food", {}, {}, {});
    }

    static ActionNode* drink([[maybe_unused]] PlayerbotAI* botAI)
    {
        return new ActionNode("drink", {}, {}, {});
    }

    static ActionNode* mana_potion([[maybe_unused]] PlayerbotAI* botAI)
    {
        return new ActionNode("mana potion", {}, {}, {});
    }

    static ActionNode* healing_potion([[maybe_unused]] PlayerbotAI* botAI)
    {
        return new ActionNode("healing potion", {}, { NextAction("food") }, {});
    }

    static ActionNode* flee([[maybe_unused]] PlayerbotAI* botAI)
    {
        return new ActionNode("flee", {}, {}, {});
    }
};

Strategy::Strategy(PlayerbotAI* botAI) : PlayerbotAIAware(botAI)
{
    actionNodeFactories.Add(new ActionNodeFactoryInternal());
}

ActionNode* Strategy::GetAction(std::string const name) { return actionNodeFactories.GetContextObject(name, botAI); }
