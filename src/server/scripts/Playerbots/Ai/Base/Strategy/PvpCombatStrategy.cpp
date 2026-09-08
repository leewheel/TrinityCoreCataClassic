/* 副本机器人策略 */
//移植来源: 业务逻辑参考 Trickerer NPCBots (bot_ai.cpp PvP 战斗循环) 移植适配 Playerbots Strategy 框架
//业务对标: D:\WoWSourcedCode\AcoreSource\TrickererNbversion\AzerothCore-wotlk-with-NPCBots bot_ai.cpp
//By leewheel 2026-08-29 引入 NPCBots PVP 策略原理——控制/打断/爆发/走位/保命/逃跑触发链,
//                       并按职业接入各自 PVP 专属技能(打断/控制/保命/爆发)
//End By leewheel
#include "PvpCombatStrategy.h"
#include "PvpCombatTriggers.h"
#include "PvpCombatActions.h"
#include "Playerbots.h"
#include "Player.h"
#include "AiFactory.h"

PvpCombatStrategy::PvpCombatStrategy(PlayerbotAI* botAI) : Strategy(botAI) {}

void PvpCombatStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    // ---------- 通用 PVP 触发链 ----------
    // 自己被控制 → 用徽章/种族技能解控
    triggers.push_back(new TriggerNode(
        "pvp bot cced", { NextAction("pvp break crowd control", ACTION_EMERGENCY) }));

    // 目标在读条且是玩家 → 打断(各职业打断技能见下方职业专属链)
    triggers.push_back(new TriggerNode(
        "pvp target casting", { NextAction("pvp interrupt cast", ACTION_RAID + 5) }));

    // 自己血量过低且是 PVP 战斗 → 保命(职业专属)/逃跑
    triggers.push_back(new TriggerNode(
        "pvp bot low health", { NextAction("pvp flee", ACTION_HIGH + 1) }));

    // 低血量PVP → 控制对手(职业专属技能见下方各职业case) → 打绷带回血 → 控制CD则逃跑
    //By leewheel 2026-08-29: 老大需求——低血量时先控制对手争取时间, 打绷带恢复, 控制技能CD中只能跑
    triggers.push_back(new TriggerNode(
        "pvp low health cc", {
            NextAction("pvp bandage", ACTION_EMERGENCY),
            NextAction("pvp flee", ACTION_HIGH + 2) }));

    // 血量极低仍被攻击 → 逃跑脱离
    triggers.push_back(new TriggerNode(
        "pvp flee", { NextAction("pvp flee", ACTION_EMERGENCY + 2) }));

    // 远程职业被近战贴脸 → 风筝拉开
    triggers.push_back(new TriggerNode(
        "pvp kite target", { NextAction("pvp kite", ACTION_RAID + 3) }));

    // 爆发可用且处于 PVP 战斗 → 职业专属爆发技能
    triggers.push_back(new TriggerNode(
        "pvp burst available", { NextAction("pvp burst available", ACTION_RAID) }));

    //By leewheel 2026-08-30: 控场循环闭环——脱战恢复+控制CD就绪后重新进攻最近敌方玩家
    //  完成老大要求的"控制→远遁→绷带→等CD→再来一轮"完整循环
    triggers.push_back(new TriggerNode(
        "pvp cc ready reengage", { NextAction("attack enemy player", ACTION_HIGH) }));
    //End By leewheel

    // ---------- 职业专属 PVP 触发链 ----------
    Player* bot = botAI->GetBot();

    switch (bot->getClass())
    {
        case CLASS_MAGE:
            // 控制: 变羊 / 深结 / 冰环 / 冲击波
            triggers.push_back(new TriggerNode("pvp target is healer", { NextAction("counterspell", ACTION_RAID + 6) }));
            triggers.push_back(new TriggerNode("pvp target casting", { NextAction("counterspell", ACTION_RAID + 5) }));
            // 低血量: 变羊/冰环控制对手后打绷带
            //By leewheel 2026-08-29: 老大需求——低血量时控制对手争取打绷带时间
            triggers.push_back(new TriggerNode("pvp low health cc", { NextAction("polymorph", ACTION_EMERGENCY + 1) }));
            triggers.push_back(new TriggerNode("pvp low health cc", { NextAction("frost nova", ACTION_EMERGENCY + 1) }));
            //End By leewheel
            // 保命: 冰箱 / 闪现
            triggers.push_back(new TriggerNode("pvp bot low health", { NextAction("ice block", ACTION_EMERGENCY + 4) }));
            triggers.push_back(new TriggerNode("pvp bot focused", { NextAction("blink back", ACTION_EMERGENCY + 3) }));
            //By leewheel 2026-08-30: 对齐 NPCBots 法师循环——被近战贴脸时冰环定身+闪现拉开(标准风筝套路 agent3确认)
            triggers.push_back(new TriggerNode("pvp kite target", { NextAction("frost nova", ACTION_EMERGENCY + 2) }));
            triggers.push_back(new TriggerNode("pvp kite target", { NextAction("blink back", ACTION_EMERGENCY + 1) }));
            //End By leewheel
            // 爆发: 冰冷血脉 / 奥术强化 / 燃烧
            triggers.push_back(new TriggerNode("pvp burst available", { NextAction("icy veins", ACTION_RAID + 2) }));
            triggers.push_back(new TriggerNode("pvp burst available", { NextAction("arcane power", ACTION_RAID + 1) }));
            triggers.push_back(new TriggerNode("pvp burst available", { NextAction("combustion", ACTION_RAID) }));
            break;

        case CLASS_ROGUE:
            // 打断: 脚踢 / 肾击
            triggers.push_back(new TriggerNode("pvp target casting", { NextAction("kick", ACTION_RAID + 5) }));
            triggers.push_back(new TriggerNode("pvp target is healer", { NextAction("kick", ACTION_RAID + 6) }));
            // 低血量: 肾击/凿击/致盲控制对手后打绷带
            //By leewheel 2026-08-29: 老大需求——低血量时控制对手争取打绷带时间
            triggers.push_back(new TriggerNode("pvp low health cc", { NextAction("kidney shot", ACTION_EMERGENCY + 1) }));
            triggers.push_back(new TriggerNode("pvp low health cc", { NextAction("gouge", ACTION_EMERGENCY + 1) }));
            triggers.push_back(new TriggerNode("pvp low health cc", { NextAction("blind", ACTION_EMERGENCY + 1) }));
            //End By leewheel
            // 保命: 消失 / 斗篷 / 闪避
            triggers.push_back(new TriggerNode("pvp bot low health", { NextAction("vanish", ACTION_EMERGENCY + 4) }));
            triggers.push_back(new TriggerNode("pvp bot focused", { NextAction("cloak of shadows", ACTION_EMERGENCY + 3) }));
            triggers.push_back(new TriggerNode("pvp bot focused", { NextAction("evasion", ACTION_EMERGENCY + 2) }));
            //By leewheel 2026-08-30: 对齐 NPCBots 盗贼循环——消失后立即潜行, 配合 PvpReengageTrigger 实现
            //  "打不过→消失→潜行→等CD→再起手" 的完整重置循环(agent3确认NPCBots唯一真正的重置再打循环)
            triggers.push_back(new TriggerNode("pvp bot low health", { NextAction("stealth", ACTION_EMERGENCY + 3) }));
            //End By leewheel
            // 爆发: 肾击控制后爆发
            triggers.push_back(new TriggerNode("pvp burst available", { NextAction("kidney shot", ACTION_RAID + 1) }));
            break;

        case CLASS_WARRIOR:
            // 打断: 拳击
            triggers.push_back(new TriggerNode("pvp target casting", { NextAction("pummel", ACTION_RAID + 5) }));
            triggers.push_back(new TriggerNode("pvp target is healer", { NextAction("pummel", ACTION_RAID + 6) }));
            // 低血量: 挫志怒吼/破胆怒吼恐惧控制对手后打绷带
            //By leewheel 2026-08-29: 老大需求——低血量时控制对手争取打绷带时间
            triggers.push_back(new TriggerNode("pvp low health cc", { NextAction("intimidating shout", ACTION_EMERGENCY + 1) }));
            //End By leewheel
            // 保命: 盾墙 / 缴械
            triggers.push_back(new TriggerNode("pvp bot low health", { NextAction("shield wall", ACTION_EMERGENCY + 4) }));
            triggers.push_back(new TriggerNode("pvp bot focused", { NextAction("disarm", ACTION_EMERGENCY + 3) }));
            // 追击/风筝: 冲锋 / 拦截 / 断筋
            triggers.push_back(new TriggerNode("pvp kite target", { NextAction("intercept", ACTION_RAID + 3) }));
            triggers.push_back(new TriggerNode("pvp target casting", { NextAction("hamstring", ACTION_RAID + 4) }));
            break;

        case CLASS_PRIEST:
            // 打断: 沉默
            triggers.push_back(new TriggerNode("pvp target casting", { NextAction("silence", ACTION_RAID + 5) }));
            triggers.push_back(new TriggerNode("pvp target is healer", { NextAction("silence", ACTION_RAID + 6) }));
            // 低血量: 心灵尖啸恐惧控制对手后打绷带
            //By leewheel 2026-08-29: 老大需求——低血量时控制对手争取打绷带时间
            triggers.push_back(new TriggerNode("pvp low health cc", { NextAction("psychic scream", ACTION_EMERGENCY + 1) }));
            //End By leewheel
            // 保命: 渐隐
            triggers.push_back(new TriggerNode("pvp bot focused", { NextAction("fade", ACTION_EMERGENCY + 3) }));
            // 控制: 心灵尖啸
            triggers.push_back(new TriggerNode("pvp bot focused", { NextAction("psychic scream", ACTION_EMERGENCY + 2) }));
            // 雷霆崖心灵控制伏击(暗夜精灵牧师玩法): 影遁蹲守→心灵控制→命令敌人跳崖
            //By leewheel 2026-08-29: 老大需求——暗夜精灵牧师影遁躲雷霆崖电梯走道, 心灵控制部落跳崖, 按日期随机轮换
            triggers.push_back(new TriggerNode("thunder bluff mind control", {
                NextAction("pvp shadowmeld", ACTION_NORMAL),
                NextAction("pvp mind control", ACTION_RAID + 1),
                NextAction("pvp mind control to cliff", ACTION_RAID + 2) }));
            //End By leewheel
            break;

        case CLASS_PALADIN:
            // 控制: 制裁之锤 / 忏悔
            triggers.push_back(new TriggerNode("pvp target casting", { NextAction("hammer of justice", ACTION_RAID + 5) }));
            triggers.push_back(new TriggerNode("pvp target is healer", { NextAction("hammer of justice", ACTION_RAID + 6) }));
            // 低血量: 制裁之锤眩晕控制对手后打绷带
            //By leewheel 2026-08-29: 老大需求——低血量时控制对手争取打绷带时间
            triggers.push_back(new TriggerNode("pvp low health cc", { NextAction("hammer of justice", ACTION_EMERGENCY + 1) }));
            //End By leewheel
            // 保命: 圣盾术
            triggers.push_back(new TriggerNode("pvp bot low health", { NextAction("divine shield", ACTION_EMERGENCY + 4) }));
            break;

        case CLASS_WARLOCK:
            // 打断: 语言诅咒已在 debuff 链, 用恐惧控制
            triggers.push_back(new TriggerNode("pvp bot focused", { NextAction("fear", ACTION_EMERGENCY + 2) }));
            // 低血量: 恐惧/暗影之怒控制对手后打绷带
            //By leewheel 2026-08-29: 老大需求——低血量时控制对手争取打绷带时间
            triggers.push_back(new TriggerNode("pvp low health cc", { NextAction("fear", ACTION_EMERGENCY + 1) }));
            triggers.push_back(new TriggerNode("pvp low health cc", { NextAction("shadowfury", ACTION_EMERGENCY + 1) }));
            //End By leewheel
            // 保命: 暗影之怒(群体眩晕)
            triggers.push_back(new TriggerNode("pvp bot focused", { NextAction("shadowfury", ACTION_EMERGENCY + 3) }));
            break;

        case CLASS_HUNTER:
            // 打断: 驱散射击 / 毒蛇钉刺(减速)
            triggers.push_back(new TriggerNode("pvp target casting", { NextAction("wyvern sting", ACTION_RAID + 5) }));
            // 低血量: 翼龙钉刺/冰冻陷阱控制对手后打绷带
            //By leewheel 2026-08-29: 老大需求——低血量时控制对手争取打绷带时间
            triggers.push_back(new TriggerNode("pvp low health cc", { NextAction("wyvern sting", ACTION_EMERGENCY + 1) }));
            triggers.push_back(new TriggerNode("pvp low health cc", { NextAction("freezing trap", ACTION_EMERGENCY + 1) }));
            //End By leewheel
            // 保命: 假死 / 逃脱
            triggers.push_back(new TriggerNode("pvp bot low health", { NextAction("feign death", ACTION_EMERGENCY + 4) }));
            triggers.push_back(new TriggerNode("pvp kite target", { NextAction("disengage", ACTION_RAID + 3) }));
            // 控制: 冰冻陷阱
            triggers.push_back(new TriggerNode("pvp kite target", { NextAction("freezing trap", ACTION_RAID + 2) }));
            //By leewheel 2026-08-30: 对齐 NPCBots 猎人循环——被近战贴脸时散射→后跳→重拉距(agent3确认)
            triggers.push_back(new TriggerNode("pvp kite target", { NextAction("scatter shot", ACTION_EMERGENCY + 1) }));
            //End By leewheel
            break;

        case CLASS_SHAMAN:
            // 打断: 风剪
            triggers.push_back(new TriggerNode("pvp target casting", { NextAction("wind shear", ACTION_RAID + 5) }));
            triggers.push_back(new TriggerNode("pvp target is healer", { NextAction("wind shear", ACTION_RAID + 6) }));
            // 低血量: 冰霜震击减速/定身控制对手后打绷带
            //By leewheel 2026-08-29: 老大需求——低血量时控制对手争取打绷带时间
            triggers.push_back(new TriggerNode("pvp low health cc", { NextAction("frost shock", ACTION_EMERGENCY + 1) }));
            //End By leewheel
            // 保命: 地缚图腾(减速) / 根基图腾
            triggers.push_back(new TriggerNode("pvp kite target", { NextAction("earthbind", ACTION_RAID + 3) }));
            // 驱散: 净化
            triggers.push_back(new TriggerNode("pvp target is healer", { NextAction("purge", ACTION_RAID + 4) }));
            break;

        case CLASS_DRUID:
            // 控制: 纠缠根须 / 休眠 / 旋风
            triggers.push_back(new TriggerNode("pvp target casting", { NextAction("entangling roots", ACTION_RAID + 5) }));
            triggers.push_back(new TriggerNode("pvp target is healer", { NextAction("hibernate", ACTION_RAID + 6) }));
            triggers.push_back(new TriggerNode("pvp bot focused", { NextAction("cyclone", ACTION_EMERGENCY + 2) }));
            // 低血量: 旋风/纠缠根须/休眠控制对手后打绷带
            //By leewheel 2026-08-29: 老大需求——低血量时控制对手争取打绷带时间
            triggers.push_back(new TriggerNode("pvp low health cc", { NextAction("cyclone", ACTION_EMERGENCY + 1) }));
            triggers.push_back(new TriggerNode("pvp low health cc", { NextAction("entangling roots", ACTION_EMERGENCY + 1) }));
            triggers.push_back(new TriggerNode("pvp low health cc", { NextAction("hibernate", ACTION_EMERGENCY + 1) }));
            //End By leewheel
            // 打断: 猛击
            triggers.push_back(new TriggerNode("pvp target casting", { NextAction("bash", ACTION_RAID + 5) }));
            //By leewheel 2026-08-30: 对齐 NPCBots 德鲁伊循环——被变羊/定身/减速时切换形态解除(agent3确认
            //  druid BreakCC:1151-1184 变形解控), 狂暴解除恐惧
            triggers.push_back(new TriggerNode("pvp bot cced", { NextAction("bear form", ACTION_EMERGENCY + 3) }));
            triggers.push_back(new TriggerNode("pvp bot cced", { NextAction("cat form", ACTION_EMERGENCY + 3) }));
            //End By leewheel
            break;

        case CLASS_DEATH_KNIGHT:
            // 打断: 心智冰封 / 绞袭
            triggers.push_back(new TriggerNode("pvp target casting", { NextAction("mind freeze", ACTION_RAID + 5) }));
            triggers.push_back(new TriggerNode("pvp target is healer", { NextAction("strangulate", ACTION_RAID + 6) }));
            // 低血量: 绞袭/冰链控制对手后打绷带
            //By leewheel 2026-08-29: 老大需求——低血量时控制对手争取打绷带时间
            triggers.push_back(new TriggerNode("pvp low health cc", { NextAction("strangulate", ACTION_EMERGENCY + 1) }));
            triggers.push_back(new TriggerNode("pvp low health cc", { NextAction("chains of ice", ACTION_EMERGENCY + 1) }));
            //End By leewheel
            // 控制: 死亡之握(拉近) / 冰链(减速)
            triggers.push_back(new TriggerNode("pvp kite target", { NextAction("chains of ice", ACTION_RAID + 3) }));
            triggers.push_back(new TriggerNode("pvp kite target", { NextAction("death grip", ACTION_RAID + 2) }));
            // 保命: 冰封之韧
            triggers.push_back(new TriggerNode("pvp bot low health", { NextAction("icebound fortitude", ACTION_EMERGENCY + 4) }));
            break;

        default:
            break;
    }
}

void PvpCombatStrategy::InitMultipliers(std::vector<Multiplier*>& /*multipliers*/)
{
    // PVP 乘数器由各职业策略根据局势细化
}