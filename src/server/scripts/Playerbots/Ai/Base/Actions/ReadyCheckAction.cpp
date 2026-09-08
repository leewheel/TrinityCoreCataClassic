/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include <memory>
#include <mutex>
#include <vector>

#include "ReadyCheckAction.h"
#include "Event.h"
#include "Playerbots.h"

//By leewheel 2026-08-23: 合并 the-lab #2571 后修正——AC 原版 SendReadyConfirm 直接调用
// HandleRaidReadyCheckOpcode, TC 无此函数; 按本项目 ReadyCheckAction 的 TC 格式修复发送
// (TC ReadyCheckResponseClient::Read 只读1字节 index: bit7=含PartyIndex, bit6=IsReady)。
//By leewheel 2026-08-27: 修复崩溃——原 MSG_RAID_READY_CHECK(映射SMSG_READY_CHECK_STARTED服务器包) 构造的包
// 在 ReadyCheckResponseClient 的 ClientPacket 构造里 ASSERT(GetOpcode()==CMSG_READY_CHECK_RESPONSE) 失败崩溃。
// 改用 CMSG_READY_CHECK_RESPONSE(客户端回应包)
static void SendReadyConfirm(Player* bot)
{
    WorldPacket packet(CMSG_READY_CHECK_RESPONSE);
    packet << uint8(0xC0);
    WPPCompat::ReadyCheckResponse(bot->GetSession(), packet);
}
//End By leewheel

std::string const formatPercent(std::string const name, uint8 value, float percent)
{
    std::ostringstream out;

    std::string color;
    if (percent > 75)
        color = "|cff00ff00";
    else if (percent > 50)
        color = "|cffffff00";
    else
        color = "|cffff0000";

    out << "|cffffffff[" << name << "]" << color << "x" << (int)value;
    return out.str();
}

class ReadyChecker
{
public:
    virtual ~ReadyChecker() = default;
    virtual bool Check(PlayerbotAI* botAI, AiObjectContext* context) = 0;
    virtual std::string const getName() = 0;
    virtual bool PrintAlways() { return true; }

    static std::vector<std::unique_ptr<ReadyChecker>> checkers;
    static std::once_flag initFlag;
};

std::vector<std::unique_ptr<ReadyChecker>> ReadyChecker::checkers;
std::once_flag ReadyChecker::initFlag;

class HealthChecker : public ReadyChecker
{
public:
    bool Check(PlayerbotAI* /*botAI*/, AiObjectContext* context) override
    {
        return AI_VALUE2(uint8, "health", "self target") > sPlayerbotAIConfig.almostFullHealth;
    }

    std::string const getName() override { return "HP"; }
};

class ManaChecker : public ReadyChecker
{
public:
    bool Check(PlayerbotAI* /*botAI*/, AiObjectContext* context) override
    {
        return !AI_VALUE2(bool, "has mana", "self target") ||
               AI_VALUE2(uint8, "mana", "self target") > sPlayerbotAIConfig.mediumHealth;
    }

    std::string const getName() override { return "MP"; }
};

class DistanceChecker : public ReadyChecker
{
public:
    bool Check(PlayerbotAI* botAI, AiObjectContext* /*context*/) override
    {
        Player* bot = botAI->GetBot();
        if (Player* master = botAI->GetMaster())
        {
            bool distance = bot->GetDistance(master) <= sPlayerbotAIConfig.sightDistance;
            if (!distance)
            {
                return false;
            }
        }

        return true;
    }

    bool PrintAlways() override { return false; }
    std::string const getName() override { return "Far away"; }
};

class HunterChecker : public ReadyChecker
{
public:
    bool Check(PlayerbotAI* botAI, AiObjectContext* /*context*/) override
    {
        Player* bot = botAI->GetBot();
        if (bot->getClass() == CLASS_HUNTER)
        {
            if (!bot->GetUInt32Value(PLAYER_AMMO_ID))
            {
                //By leewheel 2026-08-01: 玩家可见文本中文化
                botAI->TellError("弹药耗尽！");
                return false;
            }

            if (!bot->GetPet())
            {
                //By leewheel 2026-08-01: 玩家可见文本中文化
                botAI->TellError("没有宠物！");
                return false;
            }

            if (bot->GetPet()->GetHappinessState() == UNHAPPY)
            {
                //By leewheel 2026-08-01: 玩家可见文本中文化
                botAI->TellError("宠物不开心！");
                return false;
            }
        }

        return true;
    }

    bool PrintAlways() override { return false; }
    std::string const getName() override { return "Far away"; }
};

class ItemCountChecker : public ReadyChecker
{
public:
    ItemCountChecker(std::string const item, std::string const name) : item(item), name(name) {}

    bool Check(PlayerbotAI* /*botAI*/, AiObjectContext* context) override
    {
        return AI_VALUE2(uint32, "item count", item) > 0;
    }

    std::string const getName() override { return name; }

private:
    std::string const item;
    std::string const name;
};

class ManaPotionChecker : public ItemCountChecker
{
public:
    ManaPotionChecker(std::string const item, std::string const name) : ItemCountChecker(item, name) {}

    bool Check(PlayerbotAI* botAI, AiObjectContext* context) override
    {
        return !AI_VALUE2(bool, "has mana", "self target") || ItemCountChecker::Check(botAI, context);
    }
};

bool ReadyCheckAction::Execute(Event event)
{
    WorldPacket& p = event.getPacket();
    //By leewheel 2026-08-04: 修复——TC的SMSG_READY_CHECK_STARTED(=MSG_RAID_READY_CHECK)格式与AC不同
    //AC格式: ObjectGuid player (发起者GUID)
    //TC格式(ReadyCheckStarted): uint8 PartyIndex + ObjectGuid PartyGUID + ObjectGuid InitiatorGUID + uint32 Duration
    //原代码 p >> player 把PartyIndex(uint8)当作ObjectGuid的lowMask读,导致player读到垃圾值
    //修复: 跳过PartyIndex和PartyGUID,读InitiatorGUID
    //包体最小: 1 + 2(packed GUID最小) + 2(packed GUID最小) + 4 = 9字节
    if (p.wpos() < p.rpos() || (p.wpos() - p.rpos()) < 9)
        return false;

    uint8 partyIndex;
    ObjectGuid partyGuid, player;
    p >> partyIndex >> partyGuid >> player;
    //End By leewheel
    if (player == bot->GetGUID())
        return false;

    //By leewheel 2026-08-23: 合并 the-lab #2571(Force rebuff on ready check)——
    // 延迟就绪回复直到增益补满(保留上方 leewheel 2026-08-04 的 TC 包格式修复)。
    if (sPlayerbotAIConfig.forceRebuffOnReadyCheck && !bot->IsInCombat() &&
        botAI->HasStrategy("force rebuff", BOT_STATE_NON_COMBAT))
    {
        ReportReadinessToMaster();
        botAI->forceRebuff.Begin(true);
        return true;
    }
    //End By leewheel

    return ReadyCheck();
}

void ReadyCheckAction::ReportReadinessToMaster()
{
    std::call_once(
        ReadyChecker::initFlag,
        []()
        {
            ReadyChecker::checkers.reserve(8);

            ReadyChecker::checkers.emplace_back(std::make_unique<HealthChecker>());
            ReadyChecker::checkers.emplace_back(std::make_unique<ManaChecker>());
            ReadyChecker::checkers.emplace_back(std::make_unique<DistanceChecker>());
            ReadyChecker::checkers.emplace_back(std::make_unique<HunterChecker>());

            ReadyChecker::checkers.emplace_back(std::make_unique<ItemCountChecker>("food", "Food"));
            ReadyChecker::checkers.emplace_back(std::make_unique<ManaPotionChecker>("drink", "Water"));
            ReadyChecker::checkers.emplace_back(std::make_unique<ItemCountChecker>("healing potion", "Hpot"));
            ReadyChecker::checkers.emplace_back(std::make_unique<ManaPotionChecker>("mana potion", "Mpot"));
        });

    bool result = true;
    for (auto const& checkerPtr : ReadyChecker::checkers)
    {
        if (!checkerPtr)
            continue;

        bool ok = checkerPtr->Check(botAI, context);
        result = result && ok;
    }

    std::ostringstream out;

    uint32 hp = AI_VALUE2(uint32, "item count", "healing potion");
    //By leewheel 2026-08-01: 玩家可见文本中文化
    out << formatPercent("血药", hp, 100.0 * hp / 5);

    out << ", ";
    uint32 food = AI_VALUE2(uint32, "item count", "food");
    //By leewheel 2026-08-01: 玩家可见文本中文化
    out << formatPercent("食物", food, 100.0 * food / 20);

    if (AI_VALUE2(bool, "has mana", "self target"))
    {
        out << ", ";
        uint32 mp = AI_VALUE2(uint32, "item count", "mana potion");
        //By leewheel 2026-08-01: 玩家可见文本中文化
        out << formatPercent("蓝药", mp, 100.0 * mp / 5);

        out << ", ";
        uint32 water = AI_VALUE2(uint32, "item count", "water");
        //By leewheel 2026-08-01: 玩家可见文本中文化
        out << formatPercent("饮水", water, 100.0 * water / 20);
    }
    //End By leewheel

    botAI->TellMaster(out);
}

bool ReadyCheckAction::ReadyCheck()
{
    ReportReadinessToMaster();

    //By leewheel 2026-08-27: 修复崩溃——同 SendReadyConfirm，改用 CMSG_READY_CHECK_RESPONSE
    WorldPacket packet(CMSG_READY_CHECK_RESPONSE);
    // By leewheel 2026-08-07: 修复——TC的ReadyCheckResponseClient::Read()只读1字节index：
    // bit7=是否含PartyIndex, bit6=IsReady(就绪)。原代码写 bot->GetGUID()+uint8(1)，
    // Read() 会把 GUID 低字节当 index 读，就绪状态随机错误。现直接写 0xC0(bit7+bit6)=已就绪。
    packet << uint8(0xC0);
    // By leewheel 2026-07-09: 使用WPPCompat兼容层
    WPPCompat::ReadyCheckResponse(bot->GetSession(), packet);
    // End By leewheel

    //By leewheel 2026-08-23: 合并 the-lab #2571——补满增益后结束就绪延迟状态
    botAI->forceRebuff.End();
    //End By leewheel

    botAI->ChangeStrategy("-ready check", BOT_STATE_NON_COMBAT);

    return true;
}

bool FinishReadyCheckAction::Execute(Event /*event*/) { return ReadyCheck(); }

// Manual "rebuff" command: opens a rebuff window with no ready check to answer.
bool ForceRebuffAction::Execute(Event /*event*/)
{
    if (bot->IsInCombat() || !botAI->HasStrategy("force rebuff", BOT_STATE_NON_COMBAT))
        return false;

    botAI->forceRebuff.Begin(false);
    return true;
}

bool ReadyReplyAction::isUseful()
{
    ForceRebuffState const& rebuff = botAI->forceRebuff;
    return rebuff.IsPending() && !rebuff.IsOnGlobalCooldown() &&
           !bot->GetCurrentSpell(CURRENT_GENERIC_SPELL) && !bot->GetCurrentSpell(CURRENT_CHANNELED_SPELL) &&
           !rebuff.IsBuffPendingThisCycle();
}

bool ReadyReplyAction::Execute(Event /*event*/)
{
    if (botAI->forceRebuff.ShouldReplyToReadyCheck())
        SendReadyConfirm(bot);

    botAI->forceRebuff.End();
    return true;
}
