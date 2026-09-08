/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "BloodDKStrategy.h"

#include "Playerbots.h"

class BloodDKStrategyActionNodeFactory : public NamedObjectFactory<ActionNode>
{
public:
    BloodDKStrategyActionNodeFactory()
    {
        creators["rune strike"] = &rune_strike;
        creators["heart strike"] = &heart_strike;
        creators["death strike"] = &death_strike;
        creators["icy touch"] = &icy_touch;
        creators["dark command"] = &dark_command;
        creators["taunt spell"] = &dark_command;
    }

private:
    static ActionNode* rune_strike([[maybe_unused]] PlayerbotAI* botAI)
    {
        return new ActionNode(
            "rune strike",
            {
                //By leewheel 2026-07-30: 血魄DK坦克必须血魄姿态(1.8x仇恨),原写frost presence错配FrostDK,导致仇恨严重不足
                NextAction("blood presence")
                //End By leewheel
            },
            /*A*/ {},
            /*C*/ {}
        );
    }
    static ActionNode* icy_touch([[maybe_unused]] PlayerbotAI* botAI)
    {
        return new ActionNode(
            "icy touch",
            {
                //By leewheel 2026-07-30: 同上
                NextAction("blood presence")
                //End By leewheel
            },
            /*A*/ {},
            /*C*/ {}
        );
    }
    static ActionNode* heart_strike([[maybe_unused]] PlayerbotAI* botAI)
    {
        return new ActionNode(
            "heart strike",
            {
                //By leewheel 2026-07-30: 同上
                NextAction("blood presence")
                //End By leewheel
            },
            /*A*/ {
                NextAction("blood strike")
            },
            /*C*/ {}
        );
    }

    static ActionNode* death_strike([[maybe_unused]] PlayerbotAI* botAI)
    {
        return new ActionNode(
            "death strike",
            {
                //By leewheel 2026-07-30: 同上
                NextAction("blood presence")
                //End By leewheel
            },
            /*A*/ {},
            /*C*/ {}
        );
    }
    static ActionNode* dark_command([[maybe_unused]] PlayerbotAI* botAI)
    {
        return new ActionNode(
            "dark command",
            {
                //By leewheel 2026-07-30: 同上
                NextAction("blood presence")
                //End By leewheel
            },
            /*A*/ {
                NextAction("death grip")
            },
            /*C*/ {}
        );
    }
};

BloodDKStrategy::BloodDKStrategy(PlayerbotAI* botAI) : GenericDKStrategy(botAI)
{
    actionNodeFactories.Add(new BloodDKStrategyActionNodeFactory());
}

std::vector<NextAction> BloodDKStrategy::getDefaultActions()
{
    //By leewheel 2026-07-30: 血魄DK坦克仇恨循环优先级重排
    //核心思路: Rune Strike 副手攻击每 5s 触发一次是血魄DK第一仇恨来源, 必须最高优先级
    //Heart Strike 需要双疾病, 优先级次之; Icy Touch 远程开疾病但仇恨一般, 放在后面
    //Dark Command taunt 单独 trigger 触发(在 lose aggro 时)
    return {
        NextAction("rune strike", ACTION_DEFAULT + 0.7f),
        NextAction("heart strike", ACTION_DEFAULT + 0.6f),
        NextAction("death strike", ACTION_DEFAULT + 0.5f),
        NextAction("icy touch", ACTION_DEFAULT + 0.4f),
        NextAction("plague strike", ACTION_DEFAULT + 0.3f),
        NextAction("dancing rune weapon", ACTION_DEFAULT + 0.2f),
        NextAction("horn of winter", ACTION_DEFAULT + 0.1f),
        NextAction("melee", ACTION_DEFAULT)
    };
}

void BloodDKStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    GenericDKStrategy::InitTriggers(triggers);

    triggers.push_back(
        new TriggerNode(
            "hysteria no cd",
            {
                NextAction("hysteria", ACTION_NORMAL + 4)
            }
        )
    );
    //By leewheel 2026-07-30: 强制血魄姿态,坦克 DK 失去血魄时立即切回
    triggers.push_back(
        new TriggerNode(
            "no blood presence",
            {
                NextAction("blood presence", ACTION_HIGH + 10)
            }
        )
    );
    //End By leewheel
    triggers.push_back(
        new TriggerNode(
            "rune strike",
            {
                NextAction("rune strike", ACTION_NORMAL + 3)
            }
        )
    );
    triggers.push_back(
        new TriggerNode(
            "blood tap",
            {
                NextAction("blood tap", ACTION_HIGH + 5)
            }
        )
    );
    triggers.push_back(
        new TriggerNode(
            "lose aggro",
            {
                //By leewheel 2026-07-30: 坦克失仇恨第一时间嘲讽 + 死亡之握强制拉回, 优先级提到最高
                NextAction("dark command", ACTION_HIGH + 6),
                NextAction("death grip", ACTION_HIGH + 5)
                //End By leewheel
            }
        )
    );
    //By leewheel 2026-07-30: 目标脱离近战距离时, 死亡之握拉回, 防止坦克空挥仇恨大降
    triggers.push_back(
        new TriggerNode(
            "enemy out of melee range",
            {
                NextAction("death grip", ACTION_HIGH + 4)
            }
        )
    );
    //End By leewheel
    triggers.push_back(
        new TriggerNode(
            "low health",
            {
                NextAction("army of the dead", ACTION_HIGH + 4),
                NextAction("death strike", ACTION_HIGH + 3)
            }
        )
    );
    triggers.push_back(
        new TriggerNode(
            "critical health",
            {
                NextAction("vampiric blood", ACTION_HIGH + 5)
            }
        )
    );
    triggers.push_back(
        new TriggerNode(
            "icy touch",
            {
                NextAction("icy touch", ACTION_HIGH + 2)
            }
        )
    );
    triggers.push_back(
        new TriggerNode(
            "plague strike",
            {
                NextAction("plague strike", ACTION_HIGH + 2)
            }
        )
    );
    triggers.push_back(
        new TriggerNode(
            "high unholy rune",
            {
                NextAction("death strike", ACTION_HIGH + 1)
            }
        )
    );
}
