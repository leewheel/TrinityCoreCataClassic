/*
 * 移植来源: AC azerothcore-wotlk mod-playerbots the-lab 提交 188a7f9b (implement generic rogue strategy)
 * 移植适配 TC 框架
 * 业务对标: AC azerothcore-wotlk mod-playerbots
 * 作者: leewheel
 */

#include "GenericRogueStrategy.h"

class GenericRogueStrategyActionNodeFactory : public NamedObjectFactory<ActionNode>
{
public:
    GenericRogueStrategyActionNodeFactory()
    {
        creators["use deadly poison on off hand"] = &use_deadly_poison_on_off_hand;
    }

private:
    // 副手想上致命毒但包里没有(或等级不够)时，回退到速效毒
    static ActionNode* use_deadly_poison_on_off_hand([[maybe_unused]] PlayerbotAI* botAI)
    {
        return new ActionNode("use deadly poison on off hand",
                            /*P*/ {},
                            /*A*/ { NextAction("use instant poison on off hand") },
                            /*C*/ {});
    }
};

GenericRogueStrategy::GenericRogueStrategy(PlayerbotAI* botAI) : CombatStrategy(botAI)
{
    actionNodeFactories.Add(new GenericRogueStrategyActionNodeFactory());
}

void GenericRogueStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    CombatStrategy::InitTriggers(triggers);

    // 毒药优先级应高于普通攻击但低于任何保命技能。盗贼整体策略以后还要重做，
    // 当前 26 正好低于消失(Cloak of Shadows)和闪避(Evasion)、高于切割(Slice and Dice)。
    triggers.push_back(
        new TriggerNode(
            "main hand weapon no enchant",
            {
                NextAction("use instant poison on main hand", 26.0f)
            }
        )
    );
    triggers.push_back(
        new TriggerNode(
            "off hand weapon no enchant",
            {
                NextAction("use deadly poison on off hand", 25.5f)
            }
        )
    );
}
