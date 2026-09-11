/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "LfgActions.h"

#include "AiFactory.h"
//By leewheel 2026-08-26: 新增包含——ObjectAccessor(提案成员查真人)、Random(frand传送偏移)
#include "ObjectAccessor.h"
#include "Random.h"
//End By leewheel
#include "ItemVisitors.h"
#include "LFGMgr.h"
#include "LfgPackets.h" // By leewheel 2026-08-03: 使用核心DFTeleport包解析
#include "Opcodes.h"
#include "Playerbots.h"
#include "World.h"
#include "WorldPacket.h"
#include "RandomPlayerbotMgr.h"

using namespace lfg;

namespace
{
//By leewheel 2026-09-09: TC-Cata的LFGMgr无GetProposalMembers公开方法
//暂返回nullptr：bot战斗中仍会延迟应答提案，仅缺少"传送到真人身边待命"的优化
static Player* GetRealPlayerInProposal(uint32 /*proposalId*/)
{
    return nullptr;
}
//End By leewheel

//By leewheel 2026-08-26: 把bot传送到真实玩家身边待命——接受含真人的提案后立即执行,
//传送强制脱离野外战斗/清空目标, 死亡bot也会被拽离尸体位置由后续流程复活;
//满组后仍按正常LFG流程(TeleportPlayer)进副本, 不影响原有传送链路
static void TeleportBotToPlayer(Player* bot, Player* target)
{
    if (!bot || !target || bot == target)
        return;

    //目标处于不可跟随状态(正在传送/骑乘飞行/在战场)时不传送, bot留在原地走正常流程
    if (!target->IsInWorld() || target->IsBeingTeleported() ||
        target->HasUnitState(UNIT_STATE_IN_FLIGHT) || target->InBattleground())
        return;

    if (!bot->IsInWorld() || bot->IsDuringRemoveFromWorld() || bot->IsBeingTeleported())
        return;

    float x = target->GetPositionX() + frand(-3.0f, 3.0f);
    float y = target->GetPositionY() + frand(-3.0f, 3.0f);

    bot->TeleportTo(target->GetMapId(), x, y, target->GetPositionZ(), target->GetOrientation());
    TC_LOG_INFO("playerbots", "[随机本] Bot {} 接受含真玩家的LFG提案, 传送至玩家 {} 身边待命",
        bot->GetName(), target->GetName());
}
//End By leewheel

//By leewheel 2026-08-26: 提案统一应答入口(存储id路径与事件包路径共用):
//① 校验提案仍然存在(45秒超时/他人拒绝后被服务器移除, 失败通知包会携带旧id重触发)
//② 提案含真实玩家→无论战斗/死亡一律立即接受(老大需求: 有真人绝不拒绝),
//   随机bot接受后传送到该真人身边待命
//③ 纯bot提案→战斗/死亡时暂不应答保持PENDING, 由触发器每5秒重试, 45秒超时全队自动回队列
static bool AnswerLfgProposal(PlayerbotAI* botAI, uint32 id)
{
    Player* bot = botAI->GetBot();

    if (sLFGMgr->GetState(bot->GetGUID()) != LFG_STATE_PROPOSAL)
    {
        botAI->GetAiObjectContext()->GetValue<uint32>("lfg proposal")->Set(0);
        return false;
    }

    Player* realMember = GetRealPlayerInProposal(id);

    if (!realMember && (bot->IsInCombat() || bot->isDead()))
        return true;

    botAI->GetAiObjectContext()->GetValue<uint32>("lfg proposal")->Set(0);
    bot->ClearUnitState(UNIT_STATE_ALL_STATE);

    sLFGMgr->UpdateProposal(id, bot->GetGUID(), true);

    if (RandomPlayerbotMgr::instance().IsRandomBot(bot))
    {
        //仅未进组的bot执行Refresh: Refresh尾部LeaveOrDisbandGroup会解散队伍,
        //最后同意提案的bot此时已进LFG新组, 绝不能对它Refresh(会把新组拆散)
        if (!bot->GetGroup())
        {
            RandomPlayerbotMgr::instance().Refresh(bot);
            //Refresh内部factory.Refresh可能触发bot登出导致PlayerbotAI被删除, 必须重新获取
            botAI = GET_PLAYERBOT_AI(bot);
            if (!botAI)
                return true;
            botAI->ResetStrategies();
        }

        if (realMember)
            TeleportBotToPlayer(bot, realMember);
    }

    botAI->Reset();
    return true;
}
//End By leewheel
}

bool LfgJoinAction::Execute(Event /*event*/) { return JoinLFG(); }

uint32 LfgJoinAction::GetRoles()
{
    if (!RandomPlayerbotMgr::instance().IsRandomBot(bot))
    {
        //By leewheel 2026-07-20: 非随机bot(玩家组队bot/FastGroup bot)使用bySpec=true
        //原代码使用默认bySpec=false，依赖AI策略系统判断坦克/治疗
        //但FastGroup bot刚上线时策略可能尚未初始化，导致角色判断错误
        //改用bySpec=true直接按天赋页判断，与FastGroup的GetPlayerRole逻辑一致
        if (botAI->IsTank(bot, true))
            return PLAYER_ROLE_TANK;
        if (botAI->IsHeal(bot, true))
            return PLAYER_ROLE_HEALER;
        else
            return PLAYER_ROLE_DAMAGE;
        //End By leewheel
    }

    uint8 spec = AiFactory::GetPlayerSpecTab(bot);
    switch (bot->getClass())
    {
        case CLASS_DRUID:
            if (spec == 2)
                return PLAYER_ROLE_HEALER;
            else if (spec == 1)
                return PLAYER_ROLE_TANK;
            else
                return PLAYER_ROLE_DAMAGE;
            break;
        case CLASS_PALADIN:
            if (spec == 1)
                return PLAYER_ROLE_TANK;
            else if (!spec)
                return PLAYER_ROLE_HEALER;
            else
                return PLAYER_ROLE_DAMAGE;
            break;
        case CLASS_PRIEST:
            if (spec != 2)
                return PLAYER_ROLE_HEALER;
            else
                return PLAYER_ROLE_DAMAGE;
            break;
        case CLASS_SHAMAN:
            if (spec == 2)
                return PLAYER_ROLE_HEALER;
            else
                return PLAYER_ROLE_DAMAGE;
            break;
        case CLASS_WARRIOR:
            if (spec == 2)
                return PLAYER_ROLE_TANK;
            else
                return PLAYER_ROLE_DAMAGE;
            break;
        case CLASS_DEATH_KNIGHT:
            if (spec == 0)
                return PLAYER_ROLE_TANK;
            else
                return PLAYER_ROLE_DAMAGE;
            break;

        default:
            return PLAYER_ROLE_DAMAGE;
            break;
    }

    return PLAYER_ROLE_DAMAGE;
}

bool LfgJoinAction::JoinLFG()
{
    // check if already in lfg
    LfgState state = sLFGMgr->GetState(bot->GetGUID());
    if (state != LFG_STATE_NONE)
        return false;

    /*ItemCountByQuality visitor;
    IterateItems(&visitor, ITERATE_ITEMS_IN_EQUIP);
    bool random = urand(0, 100) < 20;
    bool heroic = urand(0, 100) < 50 &&
                  (visitor.count[ITEM_QUALITY_EPIC] >= 3 || visitor.count[ITEM_QUALITY_RARE] >= 10) &&
                  bot->GetLevel() >= 70;
    bool rbotAId = !heroic && (urand(0, 100) < 50 && visitor.count[ITEM_QUALITY_EPIC] >= 5 &&
                               (bot->GetLevel() == 60 || bot->GetLevel() == 70 || bot->GetLevel() == 80));*/

    LfgDungeonSet list;
    std::vector<uint32> selected;

    std::vector<uint32> dungeons = RandomPlayerbotMgr::instance().LfgDungeons[bot->GetTeamId()];
    if (!dungeons.size())
        return false;

    for (std::vector<uint32>::iterator i = dungeons.begin(); i != dungeons.end(); ++i)
    {
        LFGDungeonEntry const* dungeon = sLFGDungeonStore.LookupEntry(*i);
        if (!dungeon || (dungeon->TypeID != LFG_TYPE_RANDOM && dungeon->TypeID != LFG_TYPE_DUNGEON &&
                         dungeon->TypeID != LFG_TYPE_HEROIC && dungeon->TypeID != LFG_TYPE_RAID))
            continue;

        auto const& botLevel = bot->GetLevel();

        /*LFG_TYPE_RANDOM on classic is 15-58 so bot over level 25 will never queue*/
        if ((dungeon->MinLevel && (botLevel < dungeon->MinLevel || botLevel > dungeon->MaxLevel)) ||
            (botLevel > dungeon->MinLevel + 10 && dungeon->TypeID == LFG_TYPE_DUNGEON))
            continue;

        //By leewheel 2026-08-02: 随机本资料片门槛硬编码——防止低等级bot排进高等级随机本
        //(如59级bot排TBC随机本259:数据MinLevel=59放行,但随机池内副本61-70级打不动)
        //TBC外域60级进入、LK北极68级进入——不依赖DB2数据,数据缺失/错误时依然生效
        if (dungeon->TypeID == LFG_TYPE_RANDOM)
        {
            uint32 expansionMinLevel = 15;
            if (dungeon->ExpansionLevel >= 1)
                expansionMinLevel = 60;
            if (dungeon->ExpansionLevel >= 2)
                expansionMinLevel = 68;
            if (botLevel < expansionMinLevel)
                continue;
        }
        //End By leewheel

        selected.push_back(dungeon->ID);
        list.insert(dungeon->ID);
    }

    if (!selected.size())
        return false;

    if (list.empty())
        return false;

    bool many = list.size() > 1;
    LFGDungeonEntry const* dungeon = sLFGDungeonStore.LookupEntry(*list.begin());

    // check role for console msg
    std::string _roles = "multiple roles";
    uint32 roleMask = GetRoles();
    if (roleMask & PLAYER_ROLE_TANK)
        _roles = "TANK";

    if (roleMask & PLAYER_ROLE_HEALER)
        _roles = "HEAL";

    if (roleMask & PLAYER_ROLE_DAMAGE)
        _roles = "DPS";

    //By leewheel 2025-07-10
    // TC中LocalizedString的operator[]需要LocaleConstant枚举
    TC_LOG_INFO("playerbots", "Bot {} {}:{} <{}>: queues LFG, Dungeon as {} ({})", bot->GetGUID().ToString(),
             bot->GetTeamId() == TEAM_ALLIANCE ? "A" : "H", bot->GetLevel(), bot->GetName(), _roles,
             many ? "several dungeons" : dungeon->Name[LOCALE_enUS]);
    //End By leewheel 2025-07-10

    //By leewheel 2026-07-20: TC中CMSG_LFG_JOIN被Playerbots.h定义为0(AC opcode不存在于TC)
    //原始WorldPacket(CMSG_LFG_JOIN)的opcode=0，入队后在opcodeTable中无handler被丢弃
    //改为直接调用sLFGMgr->JoinLfg，TC签名为(Player*, uint8, LfgDungeonSet&)无comment参数
    sLFGMgr->JoinLfg(bot, uint8(roleMask), list);
    //End By leewheel

    return true;
}

bool LfgRoleCheckAction::Execute(Event /*event*/)
{
    if (Group* group = bot->GetGroup())
    {
        uint32 newRoles = GetRoles();

        //By leewheel 2026-09-03 修复C4189警告：诊断变量在TC_LOG_INFO恢复后才有意义，按AC原版恢复诊断日志(log中文)
        uint8 specTab = AiFactory::GetPlayerSpecTab(bot);
        bool isTank = botAI->IsTank(bot, true);
        bool isHeal = botAI->IsHeal(bot, true);
        bool isRandom = RandomPlayerbotMgr::instance().IsRandomBot(bot);
        TC_LOG_INFO("playerbots", "[LFG角色诊断] Bot {} (职业={}, 天赋页={}, 随机bot={}) IsTank={}, IsHeal={} => 选取角色={}",
            bot->GetName(), uint32(bot->getClass()), uint32(specTab), isRandom ? "是" : "否",
            isTank ? "true" : "false", isHeal ? "true" : "false",
            (newRoles & PLAYER_ROLE_TANK) ? "TANK" : (newRoles & PLAYER_ROLE_HEALER) ? "HEALER" : "DPS");
        //End By leewheel

        //By leewheel 2026-07-20: TC中CMSG_LFG_SET_ROLES被定义为0，直接调用sLFGMgr API
        sLFGMgr->SetRoles(bot->GetGUID(), newRoles);
        sLFGMgr->UpdateRoleCheck(group->GetGUID(), bot->GetGUID(), newRoles);
        //End By leewheel

        // TC_LOG_INFO("playerbots", "Bot {} {}:{} <{}>: LFG roles checked", bot->GetGUID().ToString().c_str(),
        //          bot->GetTeamId() == TEAM_ALLIANCE ? "A" : "H", bot->GetLevel(), bot->GetName().c_str());

        return true;
    }

    return false;
}

bool LfgAcceptAction::Execute(Event event)
{
    //By leewheel 2026-08-26: 重构——两个事件来源(存储id/提案包)统一走AnswerLfgProposal,
    //核心策略: 提案含真实玩家→无条件接受并传送至玩家身边待命(修复"机器人拒绝入队次数太多");
    //纯bot提案战斗/死亡时暂不应答等待重试; 失效提案直接清理
    uint32 id = AI_VALUE(uint32, "lfg proposal");

    // Try accept if already stored
    if (id)
        return AnswerLfgProposal(botAI, id);
    //End By leewheel

    //By leewheel 2026-07-20: TC的SMSG_LFG_PROPOSAL_UPDATE包格式与AC完全不同
    //AC格式: uint32 dungeonId, uint8 state, uint32 proposalId
    //TC格式: RideTicket(ObjectGuid+uint32 Id+uint32 Type+int64 Time+bit), uint64 InstanceID, uint32 ProposalID, uint32 Slot, int8 State
    //原AC解析 p >> dungeonId >> state >> id 在TC下读出垃圾值
    //修正为按TC格式解析，并用sLFGMgr->UpdateProposal替代opcode=0的原始包
    if (!event.getPacket().empty())
    {
        WorldPacket p(event.getPacket());
        //By leewheel 2026-08-04: 修复rpos(0)把opcode当包体读 + 长度检查
        //SMSG_LFG_PROPOSAL_UPDATE包体: requesterGuid(8)+ticketId(4)+rideType(4)+ticketTime(8)
        //+1bit+instanceId(8)+proposalId(4)=37字节最小
        if (p.wpos() < p.rpos() || (p.wpos() - p.rpos()) < 37)
            return false;
        //End By leewheel

        // 跳过RideTicket
        ObjectGuid requesterGuid;
        p >> requesterGuid;
        uint32 ticketId;
        p >> ticketId;
        //By leewheel 2026-07-21: 服务端RideTicket写包为uint32 Type + int64 Time(Timestamp<>默认int64)，
        //原按uint8+uint32读取会错位7字节，id读进恒0的InstanceID区域导致永不接受提议。
        uint32 rideType;
        p >> rideType;
        int64 ticketTime;
        p >> ticketTime;
        //End By leewheel
        p.ReadBit();  // Unknown925
        p.ResetBitPos();
        // 跳过InstanceID
        uint64 instanceId;
        p >> instanceId;
        // 读取ProposalID
        p >> id;

        if (id)
            return AnswerLfgProposal(botAI, id);
    }
    //End By leewheel

    return false;
}

bool LfgLeaveAction::Execute(Event /*event*/)
{
    // Don't leave if lfg strategy enabled
    // if (botAI->HasStrategy("lfg", BOT_STATE_NON_COMBAT))
    //    return false;

    // Don't leave if already invited / in dungeon
    if (sLFGMgr->GetState(bot->GetGUID()) > LFG_STATE_QUEUED)
        return false;

    //By leewheel 2026-08-18: 移植 brighton-chi the-lab 3285622c(修复随机机器人留在随机本队列#2612)
    //不要退出刻意加入的队列:否则"seldom"随机触发(~300秒)会把随机机器人直接拽出队列,
    //导致LFGQueue::CheckCompatibility永远看不到能稳定组成队伍的满角色池。
    //关闭 RandomBotJoinLfg 后,仍在队列中的机器人仍可正常退出。配置开关先判断,特性关闭时跳过 IsRandomBot 扫描。
    if (sPlayerbotAIConfig.randomBotJoinLfg && RandomPlayerbotMgr::instance().IsRandomBot(bot))
        return false;
    //End By leewheel

    //By leewheel 2026-07-20: TC中CMSG_LFG_LEAVE被定义为0，直接调用sLFGMgr API
    sLFGMgr->LeaveLfg(bot->GetGUID());
    //End By leewheel
    return true;
}

bool LfgLeaveAction::isUseful() { return true; }

bool LfgTeleportAction::Execute(Event event)
{
    bool out = false;

    WorldPacket p(event.getPacket());
    if (!p.empty())
    {
        //By leewheel 2026-08-03: 修复——客户端包rpos=2(opcode已消费)，且CMSG_DF_TELEPORT是bit-packed(ReadBit)，
        //原 rpos(0)+p>>out(1字节) 既错位又格式不符，改用核心包解析
        WorldPackets::LFG::DFTeleport packet(std::move(p));
        packet.Read();
        out = packet.TeleportOut;
    }

    bot->ClearUnitState(UNIT_STATE_ALL_STATE);

    //By leewheel 2026-07-20: TC中CMSG_LFG_TELEPORT被定义为0，直接调用sLFGMgr API
    sLFGMgr->TeleportPlayer(bot, out);
    //End By leewheel

    return true;
}

bool LfgJoinAction::isUseful()
{
    if (!sPlayerbotAIConfig.randomBotJoinLfg)
    {
        // botAI->ChangeStrategy("-lfg", BOT_STATE_NON_COMBAT);
        return false;
    }

    if (bot->GetLevel() < 15)
        return false;

    // don't use if active player master
    if (GET_PLAYERBOT_AI(bot)->IsRealPlayer())
        return false;

    if (bot->GetGroup() && bot->GetGroup()->GetLeaderGUID() != bot->GetGUID())
    {
        // botAI->ChangeStrategy("-lfg", BOT_STATE_NON_COMBAT);
        return false;
    }

    if (bot->IsBeingTeleported())
        return false;

    if (bot->InBattleground())
        return false;

    if (bot->InBattlegroundQueue())
        return false;

    if (bot->isDead())
        return false;

    if (!RandomPlayerbotMgr::instance().IsRandomBot(bot))
        return false;

    Map* map = bot->GetMap();
    if (map && map->Instanceable())
        return false;

    LfgState state = sLFGMgr->GetState(bot->GetGUID());
    if (state != LFG_STATE_NONE)
        return false;

    return true;
}
