//By leewheel 2026-07-11
// �?AzerothCore mod-playerbots 移植�?TrinityCore
//End By leewheel

//By leewheel 2026-08-01: 日志清理——注释掉[BotCast]/[BotGreet]施法调试INFO与施法失败DEBUG，仅保留错误
//End By leewheel

#include "PlayerbotAI.h"

#include <cmath>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <string>

#include "AiFactory.h"
#include "BudgetValues.h"
#include "ChannelMgr.h"
//By leewheel 2026-07-11: TC需要单独包含Channel.h
#include "Channel.h"
//End By leewheel
#include "CharacterPackets.h"
//By leewheel 2026-07-11: 需要ChatPackets.h中的CTextEmote
#include "ChatPackets.h"
//End By leewheel
#include "ChatHelper.h"
#include "CheckMountStateAction.h"
#include "Common.h"
#include "CreatureData.h"
#include "DBCStores.h"
#include "EmoteAction.h"
#include "Engine.h"
#include "ExternalEventHelper.h"
#include "GameObjectData.h"
#include "GameTime.h"
#include "GuildMgr.h"
#include "LFGMgr.h"
#include "LastMovementValue.h"
#include "LastSpellCastValue.h"
#include "LogLevelAction.h"
#include "LootObjectStack.h"
//By leewheel 2026-08-15: 渡船/飞艇搭乘检测——Map::GetTransportForPos声明
#include "Map.h"
//End By leewheel
#include "MapMgr.h"
#include "MotionMaster.h"
#include "MoveSplineInit.h"
#include "NewRpgStrategy.h"
#include "ObjectGuid.h"
#include "ObjectMgr.h"
#include "PerfMonitor.h"
#include "Player.h"
#include "PlayerbotTextMgr.h"
#include "PlayerbotAIConfig.h"
#include "PlayerbotMgr.h"
#include "PlayerbotGuildMgr.h"
#include "Playerbots.h"
#include "PositionValue.h"
#include "RBAC.h"
#include "RandomPlayerbotMgr.h"
#include "SayAction.h"
#include "ScriptMgr.h"
#include "ServerFacade.h"
#include "SharedDefines.h"
#include "SocialMgr.h"
#include "SpellAuraEffects.h"
#include "SpellInfo.h"
//By leewheel 2026-07-14: 需要SpellPackets.h中的WorldPackets::Spells::UseItem
#include "SpellPackets.h"
//End By leewheel
#include "Transport.h"
#include "Unit.h"
#include "UpdateTime.h"
#include "Vehicle.h"

//By leewheel 2026-08-16: 对齐the-lab 8e3c12b(#2639)——重力吸引(Gravity Lapse)期间bot悬空
//(IsFlying()=true)仍应可施法，与客户端玩家一致。TK=39432(风暴要塞凯尔萨斯)、MgT=44226(魔导师平台)
constexpr uint32 SPELL_GRAVITY_LAPSE_TK = 39432;
constexpr uint32 SPELL_GRAVITY_LAPSE_MGT = 44226;
//End By leewheel

constexpr uint32 SPELL_TITAN_GRIP = 49152;
constexpr uint32 SPELL_DK_FROST_PRESENCE = 48263;

//By leewheel 2026-07-11: 静态成员定�?
std::vector<std::string> PlayerbotAI::dispel_whitelist = {
    "mutating injection",
    "frostbolt",
};

std::set<std::string> PlayerbotAI::unsecuredCommands;
//End By leewheel

std::vector<std::string>& split(std::string const s, char delim, std::vector<std::string>& elems);
std::vector<std::string> split(std::string const s, char delim);
char* strstri(char const* str1, char const* str2);
std::string& trim(std::string& s);

//By leewheel 2026-07-11: PlayerbotChatHandler方法已在头文件中内联定义，此处不需要重复定�?

void PacketHandlingHelper::AddHandler(uint16 opcode, std::string const handler) { handlers[opcode] = handler; }

void PacketHandlingHelper::Handle(ExternalEventHelper& helper)
{
    while (!queue.empty())
    {
        WorldPacket packet = queue.top();
        queue.pop(); // remove first so handling can't modify the queue while we're using it

        helper.HandlePacket(handlers, packet);
    }
}

void PacketHandlingHelper::AddPacket(WorldPacket const& packet)
{
    if (packet.empty())
        return;
    // assert(handlers);
    // assert(packet);
    // assert(packet.GetOpcode());
    if (handlers.find(packet.GetOpcode()) != handlers.end())
        queue.push(WorldPacket(packet));
}

//By leewheel 2026-07-11: 适配TC头文件，chatHelper为指针类型，添加engine初始�?
PlayerbotAI::PlayerbotAI(Player* bot)
    : PlayerbotAIBase(true),
      chatHelper(nullptr),
      bot(bot),
      master(nullptr),
      accountId(0),
      aiObjectContext(nullptr),
      currentEngine(nullptr),
      engine(nullptr),
      currentState(BOT_STATE_NON_COMBAT),
      chatFilter(this),
      security(bot)
      //By leewheel 2026-08-23: 合并 the-lab(#2571) —— ForceRebuff(补增益)状态
      , forceRebuff(bot)
      //End By leewheel
{
    for (uint8 i = 0; i < BOT_STATE_MAX; i++)
        engines[i] = nullptr;

    for (uint8 i = 0; i < MAX_ACTIVITY_TYPE; i++)
    {
        allowActiveCheckTimer[i] = 0;
        allowActive[i] = false;
    }

    chatHelper = new ChatHelper(this);
    if (!bot->isTaxiCheater() && HasCheat((BotCheatMask::taxi)))
        bot->SetTaxiCheater(true);

    accountId = bot->GetSession()->GetAccountId();
    aiObjectContext = AiFactory::createAiObjectContext(bot, this);

    engines[BOT_STATE_COMBAT] = AiFactory::createCombatEngine(bot, this, aiObjectContext);
    engines[BOT_STATE_NON_COMBAT] = AiFactory::createNonCombatEngine(bot, this, aiObjectContext);
    engines[BOT_STATE_DEAD] = AiFactory::createDeadEngine(bot, this, aiObjectContext);

    currentEngine = engines[BOT_STATE_NON_COMBAT];
    currentState = BOT_STATE_NON_COMBAT;
//End By leewheel

    masterIncomingPacketHandlers.AddHandler(CMSG_GAMEOBJ_USE, "use game object");
    masterIncomingPacketHandlers.AddHandler(CMSG_AREATRIGGER, "area trigger");
    // masterIncomingPacketHandlers.AddHandler(CMSG_GAMEOBJ_USE, "use game object");
    // masterIncomingPacketHandlers.AddHandler(CMSG_LOOT_ROLL, "loot roll");
    masterIncomingPacketHandlers.AddHandler(CMSG_GOSSIP_HELLO, "gossip hello");
    masterIncomingPacketHandlers.AddHandler(CMSG_QUESTGIVER_HELLO, "gossip hello");
    masterIncomingPacketHandlers.AddHandler(CMSG_ACTIVATETAXI, "activate taxi");
    masterIncomingPacketHandlers.AddHandler(CMSG_ACTIVATETAXIEXPRESS, "activate taxi");
    masterIncomingPacketHandlers.AddHandler(CMSG_TAXICLEARALLNODES, "taxi done");
    masterIncomingPacketHandlers.AddHandler(CMSG_TAXICLEARNODE, "taxi done");
    masterIncomingPacketHandlers.AddHandler(CMSG_GROUP_UNINVITE, "uninvite");
    masterIncomingPacketHandlers.AddHandler(CMSG_GROUP_UNINVITE_GUID, "uninvite guid");
    //By leewheel 2026-07-20: TC使用CMSG_DF_TELEPORT，CMSG_LFG_TELEPORT被Playerbots.h定义为0
    masterIncomingPacketHandlers.AddHandler(CMSG_DF_TELEPORT, "lfg teleport");
    //End By leewheel
    masterIncomingPacketHandlers.AddHandler(CMSG_CAST_SPELL, "see spell");
    masterIncomingPacketHandlers.AddHandler(CMSG_REPOP_REQUEST, "release spirit");
    masterIncomingPacketHandlers.AddHandler(CMSG_RECLAIM_CORPSE, "revive from corpse");

    botOutgoingPacketHandlers.AddHandler(SMSG_PETITION_SHOW_SIGNATURES, "petition offer");
    botOutgoingPacketHandlers.AddHandler(SMSG_GROUP_INVITE, "group invite");
    botOutgoingPacketHandlers.AddHandler(SMSG_GUILD_INVITE, "guild invite");
    botOutgoingPacketHandlers.AddHandler(BUY_ERR_NOT_ENOUGHT_MONEY, "not enough money");
    botOutgoingPacketHandlers.AddHandler(BUY_ERR_REPUTATION_REQUIRE, "not enough reputation");
    botOutgoingPacketHandlers.AddHandler(SMSG_GROUP_SET_LEADER, "group set leader");
    botOutgoingPacketHandlers.AddHandler(SMSG_FORCE_RUN_SPEED_CHANGE, "check mount state");
    botOutgoingPacketHandlers.AddHandler(SMSG_RESURRECT_REQUEST, "resurrect request");
    botOutgoingPacketHandlers.AddHandler(SMSG_INVENTORY_CHANGE_FAILURE, "cannot equip");
    botOutgoingPacketHandlers.AddHandler(SMSG_TRADE_STATUS, "trade status");
    botOutgoingPacketHandlers.AddHandler(SMSG_TRADE_STATUS_EXTENDED, "trade status extended");
    botOutgoingPacketHandlers.AddHandler(SMSG_LOOT_RESPONSE, "loot response");
    botOutgoingPacketHandlers.AddHandler(SMSG_ITEM_PUSH_RESULT, "item push result");
    botOutgoingPacketHandlers.AddHandler(SMSG_LOOT_ROLL_WON, "loot roll won");
    botOutgoingPacketHandlers.AddHandler(SMSG_PARTY_COMMAND_RESULT, "party command");
    botOutgoingPacketHandlers.AddHandler(SMSG_LEVELUP_INFO, "levelup");
    botOutgoingPacketHandlers.AddHandler(SMSG_LOG_XPGAIN, "xpgain");
    botOutgoingPacketHandlers.AddHandler(SMSG_CAST_FAILED, "cast failed");
    botOutgoingPacketHandlers.AddHandler(SMSG_DUEL_REQUESTED, "duel requested");
    botOutgoingPacketHandlers.AddHandler(SMSG_INVENTORY_CHANGE_FAILURE, "inventory change failure");
    botOutgoingPacketHandlers.AddHandler(SMSG_BATTLEFIELD_STATUS, "bg status");
    botOutgoingPacketHandlers.AddHandler(SMSG_LFG_ROLE_CHECK_UPDATE, "lfg role check");
    botOutgoingPacketHandlers.AddHandler(SMSG_LFG_PROPOSAL_UPDATE, "lfg proposal");
    botOutgoingPacketHandlers.AddHandler(SMSG_TEXT_EMOTE, "receive text emote");
    botOutgoingPacketHandlers.AddHandler(SMSG_EMOTE, "receive emote");
    botOutgoingPacketHandlers.AddHandler(SMSG_LOOT_START_ROLL, "loot roll"); //By leewheel 2026-08-24: 原为"master loot roll"(仅后台bot), 改"loot roll"事件驱动修复真实玩家组队bot Roll慢; End By leewheel
    botOutgoingPacketHandlers.AddHandler(SMSG_ARENA_TEAM_INVITE, "arena team invite");
    botOutgoingPacketHandlers.AddHandler(SMSG_GROUP_DESTROYED, "group destroyed");
    botOutgoingPacketHandlers.AddHandler(SMSG_GROUP_LIST, "group list");

    masterOutgoingPacketHandlers.AddHandler(SMSG_PARTY_COMMAND_RESULT, "party command");
    masterOutgoingPacketHandlers.AddHandler(MSG_RAID_READY_CHECK, "ready check");
    masterOutgoingPacketHandlers.AddHandler(MSG_RAID_READY_CHECK_FINISHED, "ready check finished");
    masterOutgoingPacketHandlers.AddHandler(SMSG_QUESTGIVER_OFFER_REWARD, "questgiver quest details");

    // quest packet
    masterIncomingPacketHandlers.AddHandler(CMSG_QUESTGIVER_COMPLETE_QUEST, "complete quest");
    masterIncomingPacketHandlers.AddHandler(CMSG_QUESTGIVER_ACCEPT_QUEST, "accept quest");
    masterIncomingPacketHandlers.AddHandler(CMSG_QUEST_CONFIRM_ACCEPT, "confirm quest");
    masterIncomingPacketHandlers.AddHandler(CMSG_PUSHQUESTTOPARTY, "quest share");
    botOutgoingPacketHandlers.AddHandler(SMSG_QUESTUPDATE_COMPLETE, "quest update complete");
    botOutgoingPacketHandlers.AddHandler(SMSG_QUESTUPDATE_ADD_KILL, "quest update add kill");
    //By leewheel 2026-07-24: 注册任务失败包处理器 - 原代码缺失导致定时任务超时后bot不会自动放弃
    botOutgoingPacketHandlers.AddHandler(SMSG_QUESTUPDATE_FAILED, "quest update failed");
    botOutgoingPacketHandlers.AddHandler(SMSG_QUESTUPDATE_FAILED_TIMER, "quest update failed timer");
    //End By leewheel
    // SMSG_QUESTUPDATE_ADD_ITEM no longer used
    // botOutgoingPacketHandlers.AddHandler(SMSG_QUESTUPDATE_ADD_ITEM, "quest update add item");
    botOutgoingPacketHandlers.AddHandler(SMSG_QUEST_CONFIRM_ACCEPT, "confirm quest");
}

//By leewheel 2026-07-11: 添加chatHelper释放
PlayerbotAI::~PlayerbotAI()
{
    for (uint8 i = 0; i < BOT_STATE_MAX; i++)
    {
        if (engines[i])
            delete engines[i];
    }

    if (aiObjectContext)
        delete aiObjectContext;

    //By leewheel 2026-07-12: 添加chatHelper空指针保�?
    if (chatHelper)
        delete chatHelper;
    //End By leewheel

    if (bot)
        PlayerbotsMgr::instance().RemovePlayerBotData(bot->GetGUID(), true);
}
//End By leewheel

void PlayerbotAI::UpdateAI(uint32 elapsed, bool minimal)
{
    // Handle the AI check delay
    if (nextAICheckDelay > elapsed)
        nextAICheckDelay -= elapsed;
    else
        nextAICheckDelay = 0;

    //By leewheel 2026-07-14: 诊断日志 - UpdateAI入口点（无条件输出）
    //By leewheel 2026-07-14: UpdateAI诊断日志已清理
    //End By leewheel

    // Early return if bot is in invalid state
    if (!bot || !bot->GetSession() || !bot->IsInWorld() || bot->IsBeingTeleported() ||
        bot->GetSession()->isLogingOut() || bot->IsDuringRemoveFromWorld())
        return;

    // Handle cheat options (set bot health and power if cheats are enabled)
    if (bot->IsAlive() &&
        (static_cast<uint32>(GetCheat()) > 0 || static_cast<uint32>(sPlayerbotAIConfig.botCheatMask) > 0))
    {
        if (HasCheat(BotCheatMask::health))
            bot->SetFullHealth();

        if (HasCheat(BotCheatMask::mana) && bot->getPowerType() == POWER_MANA)
            bot->SetPower(POWER_MANA, bot->GetMaxPower(POWER_MANA));

        if (HasCheat(BotCheatMask::power) && bot->getPowerType() != POWER_MANA)
            bot->SetPower(bot->getPowerType(), bot->GetMaxPower(bot->getPowerType()));
    }

    AllowActivity();

    if (!CanUpdateAI())
        return;

    // Handle a spell that is still in its preparing phase (including channeled spells).
    Spell* currentSpell = bot->GetCurrentSpell(CURRENT_GENERIC_SPELL);
    if (!currentSpell)
        currentSpell = bot->GetCurrentSpell(CURRENT_CHANNELED_SPELL);

    if (currentSpell)
    {
        if (currentSpell->getState() == SPELL_STATE_PREPARING)
        {
            // Allow external scripts to interrupt a cast in progress
            if (spellInterruptRequested)
            {
                spellInterruptRequested = false;
                bot->InterruptSpell(currentSpell->GetCurrentContainer());
                YieldThread(bot, GetReactDelay());
                return;
            }

            const SpellInfo* spellInfo = currentSpell->GetSpellInfo();
            if (spellInfo)
            {
                Unit* spellTarget = currentSpell->m_targets.GetUnitTarget();
                // Interrupt if target is dead or spell can't target dead units
                if (spellTarget && !spellTarget->IsAlive() && !spellInfo->IsAllowingDeadTarget())
                {
                    bot->InterruptSpell(currentSpell->GetCurrentContainer());
                    YieldThread(bot, GetReactDelay());
                    return;
                }

                GameObject* goSpellTarget = currentSpell->m_targets.GetGOTarget();

                if (goSpellTarget && !goSpellTarget->isSpawned())
                {
                    bot->InterruptSpell(currentSpell->GetCurrentContainer());
                    YieldThread(bot, GetReactDelay());
                    return;
                }

                bool isHeal = false;
                bool isSingleTarget = true;

                for (uint8 i = 0; i < 3; ++i)
                {
                    if (!spellInfo->GetEffects()[i].Effect)
                        continue;

                    // Check if spell is a heal
                    if (spellInfo->GetEffects()[i].Effect == SPELL_EFFECT_HEAL ||
                        spellInfo->GetEffects()[i].Effect == SPELL_EFFECT_HEAL_MAX_HEALTH ||
                        spellInfo->GetEffects()[i].Effect == SPELL_EFFECT_HEAL_MECHANICAL)
                        isHeal = true;

                    // Check if spell is single-target
                    if ((spellInfo->GetEffects()[i].TargetA.GetTarget() &&
                         spellInfo->GetEffects()[i].TargetA.GetTarget() != TARGET_UNIT_TARGET_ALLY) ||
                        (spellInfo->GetEffects()[i].TargetB.GetTarget() &&
                         spellInfo->GetEffects()[i].TargetB.GetTarget() != TARGET_UNIT_TARGET_ALLY))
                    {
                        isSingleTarget = false;
                    }
                }

                // Interrupt if target ally has full health (heal by other member)
                if (isHeal && isSingleTarget && spellTarget && spellTarget->IsFullHealth())
                {
                    bot->InterruptSpell(currentSpell->GetCurrentContainer());
                    YieldThread(bot, GetReactDelay());
                    return;
                }

                // Ensure bot is facing target if necessary
                if (spellTarget && !bot->HasInArc(CAST_ANGLE_IN_FRONT, spellTarget) &&
                    (spellInfo->FacingCasterFlags & SPELL_FACING_FLAG_INFRONT))
                {
                    ServerFacade::instance().SetFacingTo(bot, spellTarget);
                }

                // Wait for spell cast
                YieldThread(bot, GetReactDelay());
                return;
            }
        }
    }

    if (spellInterruptRequested)
    {
        // At this point the preparing-cast branch above did not consume the request.
        // Interrupt a current channel if one still exists; otherwise, clear the stale request.
        if (bot->GetCurrentSpell(CURRENT_CHANNELED_SPELL))
        {
            spellInterruptRequested = false;
            bot->InterruptSpell(CURRENT_CHANNELED_SPELL);
            YieldThread(bot, GetReactDelay());
            return;
        }

        spellInterruptRequested = false;
    }

    // Handle transport check delay
    if (nextTransportCheck > elapsed)
        nextTransportCheck -= elapsed;
    else
        nextTransportCheck = 0;

    if (!nextTransportCheck)
    {
        nextTransportCheck = 1000;
        //By leewheel 2026-09-09: TC-Cata的Map类无GetTransportForPos，Transport::AddPassenger需要2个参数
        //暂时禁用渡船/飞艇搭乘检测功能，TODO: 后续移植Transport相关API
        // Transport* newTransport = bot->GetMap()->GetTransportForPos(
        //     bot->GetPhaseShift(), bot->GetPositionX(), bot->GetPositionY(), bot->GetPositionZ(), bot);
        // if (newTransport != bot->GetTransport())
        // {
        //     if (bot->GetTransport())
        //         bot->GetTransport()->RemovePassenger(bot);
        //     if (newTransport)
        //         newTransport->AddPassenger(bot, Position());
        //     bot->StopMoving();
        // }
        //End By leewheel
    }

    // Update the bot's group status (moved to helper function)
    UpdateAIGroupMaster();

    // Update internal AI
    UpdateAIInternal(elapsed, minimal);
    YieldThread(bot, GetReactDelay());
}

// Helper function for UpdateAI to check group membership and handle removal if necessary
void PlayerbotAI::UpdateAIGroupMaster()
{
    if (!bot)
        return;

    PlayerbotAI* botAI = GET_PLAYERBOT_AI(bot);
    if (!botAI)
        return;

    Group* group = bot->GetGroup();

    bool IsRandomBot = sRandomPlayerbotMgr.IsRandomBot(bot);

    // If bot is not in group verify that for is RandomBot before clearing  master and resetting.
    if (!group)
    {
        if (master && IsRandomBot)
        {
            SetMaster(nullptr);
            Reset(true);
            ResetStrategies();
        }
        return;
    }

    // Bot in BG, but master no longer part of a group: release master
    // Exclude alt and addclass bots as they rely on current (real player) master, security-wise.
    if (bot->InBattleground() && IsRandomBot && master && !master->GetGroup())
        SetMaster(nullptr);

    PlayerbotAI* masterBotAI = nullptr;
    if (master)
        masterBotAI = GET_PLAYERBOT_AI(master);

    if (!master || (masterBotAI && !masterBotAI->IsRealPlayer()))
    {
        Player* newMaster = FindNewMaster();
        if (newMaster)
        {
            master = newMaster;
            botAI->SetMaster(newMaster);
            botAI->ResetStrategies();

            if (!bot->InBattleground())
            {
                botAI->ChangeStrategy("+follow", BOT_STATE_NON_COMBAT);

                if (botAI->GetMaster() == botAI->GetGroupLeader())
                    botAI->TellMaster(PlayerbotTextMgr::instance().GetBotTextOrDefault(
                        "hello_follow", "Hello, I follow you!", {}));
                else
                    botAI->TellMaster(PlayerbotTextMgr::instance().GetBotTextOrDefault(
                        "hello", "Hello!", {}));
            }
            else
            {
                // we're in a battleground, stay with the pack and focus on objective
                botAI->ChangeStrategy("-follow", BOT_STATE_NON_COMBAT);
            }
        }
    }
}

void PlayerbotAI::UpdateAIInternal([[maybe_unused]] uint32 elapsed, bool minimal)
{

    if (!bot || !bot->GetSession())
        return;

    if (!bot->IsInWorld() || bot->IsBeingTeleported() || bot->IsDuringRemoveFromWorld())
        return;

    if (!bot->GetMap())
        return; // instances are created and destroyed on demand

    //By leewheel 2026-07-20: 用perfMonEnabled守卫，避免每次UpdateAIInternal都构造WorldPosition+拼接字符串
    PerfMonitorOperation* pmo = nullptr;
    if (sPlayerbotAIConfig.perfMonEnabled)
    {
        std::string const mapString = WorldPosition(bot).isOverworld() ? std::to_string(bot->GetMapId()) : "I";
        pmo = sPerfMonitor.start(PERF_MON_TOTAL, "PlayerbotAI::UpdateAIInternal " + mapString);
    }
    //End By leewheel

    ExternalEventHelper helper(aiObjectContext);

    // chat replies
    for (auto it = chatReplies.begin(); it != chatReplies.end();)
    {
        time_t checkTime = it->m_time;
        if (checkTime && time(0) < checkTime)
        {
            ++it;
            continue;
        }

        ChatReplyAction::ChatReplyDo(bot, it->m_type, it->m_guid1, it->m_msg, it->m_chanName, it->m_name);
        it = chatReplies.erase(it);
    }

    HandleCommands();

    // logout if logout timer is ready or if instant logout is possible
    if (bot->GetSession()->isLogingOut())
    {
        WorldSession* botWorldSessionPtr = bot->GetSession();
        bool logout = botWorldSessionPtr->ShouldLogOut(time(nullptr));
        if (!master || !master->GetSession()->GetPlayer())
            logout = true;

        //By leewheel 2026-07-11: TC使用HasPlayerFlag替代HasFlag(PLAYER_FLAGS, ...)
        if (bot->HasPlayerFlag(PLAYER_FLAGS_RESTING) || bot->HasUnitState(UNIT_STATE_IN_FLIGHT) ||
            botWorldSessionPtr->HasPermission(rbac::RBAC_PERM_INSTANT_LOGOUT))
        //End By leewheel
        {
            logout = true;
        }

        if (master &&
            //By leewheel 2026-07-11: TC使用HasPlayerFlag
            (master->HasPlayerFlag(PLAYER_FLAGS_RESTING) || master->HasUnitState(UNIT_STATE_IN_FLIGHT) ||
            //End By leewheel
             (master->GetSession() &&
              master->GetSession()->HasPermission(rbac::RBAC_PERM_INSTANT_LOGOUT))))
        {
            logout = true;
        }

        if (logout)
        {
            PlayerbotMgr* masterBotMgr = nullptr;
            if (master)
                masterBotMgr = GET_PLAYERBOT_MGR(master);
            if (masterBotMgr)
            {
                masterBotMgr->LogoutPlayerBot(bot->GetGUID());
            }
            else
            {
                sRandomPlayerbotMgr.LogoutPlayerBot(bot->GetGUID());
            }
            return;
        }

        SetNextCheckDelay(sPlayerbotAIConfig.reactDelay);
        return;
    }

    botOutgoingPacketHandlers.Handle(helper);
    masterIncomingPacketHandlers.Handle(helper);
    masterOutgoingPacketHandlers.Handle(helper);

    DoNextAction(minimal);

    if (pmo)
        pmo->finish();
}

void PlayerbotAI::HandleCommands()
{
    ExternalEventHelper helper(aiObjectContext);

    for (auto it = chatCommands.begin(); it != chatCommands.end();)
    {
        time_t& checkTime = it->GetTime();
        if (checkTime && time(nullptr) < checkTime)
        {
            ++it;
            continue;
        }

        Player* owner = it->GetOwner();
        if (!owner)
        {
            it = chatCommands.erase(it);
            continue;
        }

        const std::string& command = it->GetCommand();
        if (command.empty())
        {
            it = chatCommands.erase(it);
            continue;
        }

        if (!helper.ParseChatCommand(command, owner) && it->GetType() == CHAT_MSG_WHISPER)
        {
            // ostringstream out; out << "Unknown command " << command;
            // TellPlayer(out);
            // helper.ParseChatCommand("help");
        }

        it = chatCommands.erase(it);
    }
}

std::map<std::string, ChatMsg> chatMap;

//By leewheel 2026-07-11: 移除TC头文件中不存在的私有HandleCommand方法

void PlayerbotAI::HandleTeleportAck()
{
    if (!bot || !bot->GetSession())
        return;

    // only for bots
    if (IsRealPlayer())
        return;

    /*
     * FAR TELEPORT (worldport / map change)
     * Player may NOT be in world or grid here.
     * Handle this FIRST.
     */
    if (bot->IsBeingTeleportedFar())
    {
        bot->GetSession()->HandleMoveWorldportAck();

        // after worldport ACK the player should be in a valid map
        if (!bot->GetMap())
        {
            LOG_ERROR("playerbot", "Bot {} has no map after worldport ACK", bot->GetGUID().ToString());
            return;
        }

        // apply instance-related strategies after map attach
        if (sPlayerbotAIConfig.applyInstanceStrategies)
            ApplyInstanceStrategies(bot->GetMapId(), true);

        if (sPlayerbotAIConfig.restrictHealerDPS)
            EvaluateHealerDpsStrategy();

        // reset AI state after teleport
        Reset(true);

        // clear movement only AFTER teleport is finalized and bot is in world. 飞行换地图时跳过(上游a1252c6d)
        if (bot->IsInWorld() && bot->GetMotionMaster() && !bot->IsInFlight())
        {
            //By leewheel 2026-07-11: TC MotionMaster::Clear()不接受bool参数
            bot->GetMotionMaster()->Clear();
            //End By leewheel
            bot->StopMoving();
        }

        // simulate far teleport latency (cmangos-style)
        SetNextCheckDelay(urand(2000, 5000));
        return;
    }

    /*
     * NEAR TELEPORT (same map / instance)
     * Player MUST be in world (and in grid).
     */
    if (bot->IsBeingTeleportedNear())
    {
        if (!bot->IsInWorld())
            return;

        //By leewheel 2026-07-11: TC没有m_mover，bot自身即为mover；HandleMoveTeleportAck需要WorldPackets格式
        Player* plMover = bot;
        if (!plMover)
            return;

        //By leewheel 2026-07-11: TC的MoveTeleportAck没有默认构造函数，需用WorldPacket构�?
        WorldPacket data(CMSG_MOVE_TELEPORT_ACK);
        WorldPackets::Movement::MoveTeleportAck ack(std::move(data));
        ack.MoverGUID = plMover->GetGUID();
        ack.AckIndex = 0;
        ack.MoveTime = 0;
        bot->GetSession()->HandleMoveTeleportAck(ack);
        //End By leewheel
        //End By leewheel

        // clear movement after successful relocation
        if (bot->GetMotionMaster())
        {
            //By leewheel 2026-07-11: TC MotionMaster::Clear()不接受bool参数
            bot->GetMotionMaster()->Clear();
            //End By leewheel
            bot->StopMoving();
        }

        // simulate near teleport latency
        SetNextCheckDelay(urand(1000, 2000));
        return;
    }
}

void PlayerbotAI::Reset(bool full)
{
    //By leewheel 2026-07-12: 添加全面的空指针保护
    //防止bot、aiObjectContext或session为null/dangling时崩�?
    if (!bot || !aiObjectContext)
        return;

    if (!bot->IsInWorld() || bot->IsDuringRemoveFromWorld())
        return;
    //End By leewheel

    if (bot->HasUnitState(UNIT_STATE_IN_FLIGHT))
        return;

    WorldSession* botWorldSessionPtr = bot->GetSession();
    if (!botWorldSessionPtr)
        return;
    bool logout = botWorldSessionPtr->ShouldLogOut(time(nullptr));

    // cancel logout
    if (!logout && bot->GetSession()->isLogingOut())
    {
        WorldPackets::Character::LogoutCancel data = WorldPacket(CMSG_LOGOUT_CANCEL);
        bot->GetSession()->HandleLogoutCancelOpcode(data);
        TellMaster(PlayerbotTextMgr::instance().GetBotTextOrDefault(
            "logout_cancel", "Logout cancelled!", {}));
    }

    currentEngine = engines[BOT_STATE_NON_COMBAT];
    currentState = BOT_STATE_NON_COMBAT;
    nextAICheckDelay = 0;
    whispers.clear();

    //By leewheel 2026-07-12: 为所有GetValue调用添加空指针保�?
    //GetValue可能返回null（值未注册或context已损坏）
    if (auto* v = aiObjectContext->GetValue<Unit*>("old target")) v->Set(nullptr);
    if (auto* v = aiObjectContext->GetValue<Unit*>("current target")) v->Set(nullptr);
    if (auto* v = aiObjectContext->GetValue<GuidVector>("prioritized targets")) v->Reset();
    if (auto* v = aiObjectContext->GetValue<ObjectGuid>("pull target")) v->Set(ObjectGuid::Empty);
    if (auto* v = aiObjectContext->GetValue<ObjectGuid>("pull strategy target")) v->Set(ObjectGuid::Empty);
    if (auto* v = aiObjectContext->GetValue<GuidPosition>("rpg target")) v->Set(GuidPosition());
    if (auto* v = aiObjectContext->GetValue<LootObject>("loot target")) v->Set(LootObject());
    if (auto* v = aiObjectContext->GetValue<uint32>("lfg proposal")) v->Set(0);
    //End By leewheel
    //By leewheel 2026-07-11: TC的SetTarget需要ObjectGuid参数
    bot->SetTarget(ObjectGuid::Empty);
    //End By leewheel

    //By leewheel 2026-07-12: 为full模式下的GetValue调用添加空指针保�?
    if (auto* v = aiObjectContext->GetValue<LastSpellCast&>("last spell cast"))
    {
        LastSpellCast& lastSpell = v->Get();
        lastSpell.Reset();
    }

    if (full)
    {
        if (auto* v = aiObjectContext->GetValue<LastMovement&>("last movement")) v->Get().Set(nullptr);
        if (auto* v = aiObjectContext->GetValue<LastMovement&>("last area trigger")) v->Get().Set(nullptr);
        if (auto* v = aiObjectContext->GetValue<LastMovement&>("last taxi")) v->Get().Set(nullptr);
        if (auto* v = aiObjectContext->GetValue<TravelTarget*>("travel target"))
        {
            if (auto* t = v->Get())
            {
                t->setTarget(TravelMgr::instance().nullTravelDestination, TravelMgr::instance().nullWorldPosition, true);
                t->setStatus(TRAVEL_STATUS_EXPIRED);
                t->setExpireIn(1000);
            }
        }
        rpgInfo = NewRpgInfo();
    }

    if (auto* v = aiObjectContext->GetValue<GuidSet&>("ignore rpg target")) v->Get().clear();
    //End By leewheel

    bot->GetMotionMaster()->Clear();
    bot->CastStop();

    if (full)
    {
        for (uint8 i = 0; i < BOT_STATE_MAX; i++)
        {
            //By leewheel 2026-07-12: 添加engine空指针保�?
            if (engines[i])
                engines[i]->Init();
            //End By leewheel
        }
    }
}

void PlayerbotAI::LeaveOrDisbandGroup()
{
    if (!bot || !bot->GetGroup() || IsRealPlayer())
        return;

    //By leewheel 2026-07-11: TC没有CMSG_GROUP_DISBAND/LEAVE opcode，直接使用Group API
    if (Group* group = bot->GetGroup())
        group->RemoveMember(bot->GetGUID());
    //End By leewheel
}

bool PlayerbotAI::IsAllowedCommand(std::string const text)
{
    if (unsecuredCommands.empty())
    {
        unsecuredCommands.insert("who");
        unsecuredCommands.insert("wts");
        unsecuredCommands.insert("sendmail");
        unsecuredCommands.insert("invite");
        unsecuredCommands.insert("leave");
        unsecuredCommands.insert("lfg");
        unsecuredCommands.insert("pvp stats");
        unsecuredCommands.insert("rpg status");
        //By leewheel 2026-07-24: 添加summon/follow/stay到免检命令列表
        //同组非master玩家安全等级为INVITE(2)，第二道检查需ALLOW_ALL(3)才能发非免检命令
        //summon是安全的：SummonAction只传送到bot的master身边，不会传送到命令发送者
        unsecuredCommands.insert("summon");
        unsecuredCommands.insert("follow");
        unsecuredCommands.insert("stay");
        //End By leewheel
    }

    for (std::set<std::string>::iterator i = unsecuredCommands.begin(); i != unsecuredCommands.end(); ++i)
    {
        if (text.find(*i) == 0)
        {
            return true;
        }
    }

    return false;
}

void PlayerbotAI::HandleCommand(uint32 type, std::string const text, Player* fromPlayer)
{
    if (!GetSecurity()->CheckLevelFor(PLAYERBOT_SECURITY_INVITE, type != CHAT_MSG_WHISPER, fromPlayer))
        return;

    if (type == CHAT_MSG_ADDON)
        return;

    if (type == CHAT_MSG_SYSTEM)
        return;

    if (text.find(sPlayerbotAIConfig.commandSeparator) != std::string::npos)
    {
        std::vector<std::string> commands;
        split(commands, text, sPlayerbotAIConfig.commandSeparator.c_str());
        for (std::vector<std::string>::iterator i = commands.begin(); i != commands.end(); ++i)
        {
            HandleCommand(type, *i, fromPlayer);
        }

        return;
    }

    std::string filtered = text;
    if (!sPlayerbotAIConfig.commandPrefix.empty())
    {
        if (filtered.find(sPlayerbotAIConfig.commandPrefix) != 0)
            return;

        filtered = filtered.substr(sPlayerbotAIConfig.commandPrefix.size());
    }

    if (chatMap.empty())
    {
        chatMap["#w "] = CHAT_MSG_WHISPER;
        chatMap["#p "] = CHAT_MSG_PARTY;
        chatMap["#r "] = CHAT_MSG_RAID;
        chatMap["#a "] = CHAT_MSG_ADDON;
        chatMap["#g "] = CHAT_MSG_GUILD;
    }

    currentChat = std::pair<ChatMsg, time_t>(CHAT_MSG_WHISPER, 0);
    for (std::map<std::string, ChatMsg>::iterator i = chatMap.begin(); i != chatMap.end(); ++i)
    {
        if (filtered.find(i->first) == 0)
        {
            filtered = filtered.substr(3);
            currentChat = std::pair<ChatMsg, time_t>(i->second, time(nullptr) + 2);
            break;
        }
    }

    filtered = chatFilter.Filter(trim(filtered));
    if (filtered.empty())
        return;

    //By leewheel 2026-07-22: 中文命令别名解析，将中文密语命令映射为英文触发器名
    filtered = ChatHelper::ResolveChatCommandAlias(filtered);
    //End By leewheel

    if (filtered.substr(0, 6) == "debug ")
    {
        std::string const response = HandleRemoteCommand(filtered.substr(6));
        WorldPacket data;
        BuildChatPacket(data, (ChatMsg)type, type == CHAT_MSG_ADDON ? LANG_ADDON : LANG_UNIVERSAL, bot,
                                     nullptr, response.c_str());
        fromPlayer->SendDirectMessage(&data);
        return;
    }
    if (!IsAllowedCommand(filtered) &&
        (!GetSecurity()->CheckLevelFor(PLAYERBOT_SECURITY_ALLOW_ALL, type != CHAT_MSG_WHISPER, fromPlayer)))
        return;

    if (type == CHAT_MSG_RAID_WARNING && filtered.find(bot->GetName()) != std::string::npos &&
        filtered.find("award") == std::string::npos)
    {
        chatCommands.push_back(ChatCommandHolder("warning", fromPlayer, type));
        return;
    }

    if ((filtered.size() > 2 && filtered.substr(0, 2) == "d ") ||
        (filtered.size() > 3 && filtered.substr(0, 3) == "do "))
    {
        std::string const action = filtered.substr(filtered.find(" ") + 1);
        DoSpecificAction(action);
    }
    else if (type != CHAT_MSG_WHISPER && filtered.size() > 6 && filtered.substr(0, 6) == "queue ")
    {
        std::string const remaining = filtered.substr(filtered.find(" ") + 1);
        uint32 index = 1;

        if (Group* group = bot->GetGroup())
        {
            for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
            {
                if (ref->GetSource() == master)
                    continue;

                if (ref->GetSource() == bot)
                    break;

                ++index;
            }
        }

        chatCommands.push_back(ChatCommandHolder(remaining, fromPlayer, type, time(nullptr) + index));
    }
    else if (filtered == "reset")
    {
        Reset(true);
    }
    else if (filtered == "logout")
    {
        if (bot->GetSession()->isLogingOut())
            return;

        // Verify the command came from this bot's master. Also handles nullptr
        if (fromPlayer != master)
        {
            if (type == CHAT_MSG_WHISPER)
            {
                std::string message = PlayerbotTextMgr::instance().GetBotTextOrDefault(
                    "bot_not_your_master", "You are not my master!", {});
                bot->Whisper(message, LANG_UNIVERSAL, fromPlayer);
            }
            return;
        }

        PlayerbotMgr* masterBotMgr = GET_PLAYERBOT_MGR(master);
        if (!masterBotMgr)
            return;

        // Only respond if this bot is in master's collection (alt/addclass)
        if (masterBotMgr->GetPlayerBot(bot->GetGUID()))
        {
            if (type == CHAT_MSG_WHISPER)
            {
                std::string message = PlayerbotTextMgr::instance().GetBotTextOrDefault(
                    "logout_start", "I'm logging out!", {});
                TellMaster(message);
            }

            masterBotMgr->LogoutPlayerBot(bot->GetGUID());
        }
        else if (type == CHAT_MSG_WHISPER)
        {
            std::string message = PlayerbotTextMgr::instance().GetBotTextOrDefault(
                "bot_rndbot_no_logout", "You can't command me to logout!", {});
            TellMaster(message);
        }
    }
    else if (filtered == "logout cancel")
    {
        if (!bot->GetSession()->isLogingOut())
            return;

        if (type == CHAT_MSG_WHISPER)
        {
            std::string message = PlayerbotTextMgr::instance().GetBotTextOrDefault(
                "logout_cancel", "Logout cancelled!", {});
            TellMaster(message);
        }

        WorldPackets::Character::LogoutCancel data = WorldPacket(CMSG_LOGOUT_CANCEL);
        bot->GetSession()->HandleLogoutCancelOpcode(data);
    }
    else
    {
        chatCommands.push_back(ChatCommandHolder(filtered, fromPlayer, type));
    }
}

void PlayerbotAI::HandleBotOutgoingPacket(WorldPacket const& packet)
{
    if (packet.empty())
        return;

    if (!bot || !bot->IsInWorld() || bot->IsDuringRemoveFromWorld())
        return;

    //By leewheel 2026-07-14: 添加logout状态检�?
    //在bot正在登出时不要处理包，否则会导致崩溃
    //(GObjectEditor的ClearEditorSession会在OnPlayerLogout时发送addon�?
    if (bot->GetSession() && bot->GetSession()->isLogingOut())
        return;
    //End By leewheel

    //By leewheel 2026-07-14: 添加try-catch保护
    //TC和AC的packet格式可能有差异，解析时可能抛出ByteBuffer异常
    try
    {
    switch (packet.GetOpcode())
    {
        case SMSG_SPELL_FAILURE:
        {
            WorldPacket p(packet);
            //By leewheel 2026-08-04: 修复——TC的SMSG_SPELL_FAILURE格式与AC完全不同
            //AC格式: casterGuid(packed)+count(1)+spellId(4)+result(1)
            //TC格式(SpellFailure packet): CasterUnit(packed)+CastID(packed)+SpellID(4)+Visual(SpellCastVisual)+Reason(2)
            //原代码按AC格式在casterGuid后读uint8 count,但TC下是CastID(ObjectGuid packed)的lowMask字节
            //导致spellId读到CastID中间字节,SpellInterrupted用错误spellId调用
            //修复: 按TC格式在CasterUnit后读CastID(ObjectGuid),再读SpellID
            //注意: ObjectGuid是PackedUInt64编码(2~18字节可变),长度检查用最小值估算
            //最小: CasterUnit(2,全0极端情况)+CastID(2)+SpellID(4)=8字节,实际通常>=22字节
            if (p.wpos() < p.rpos() || (p.wpos() - p.rpos()) < 8)
                return;
            //End By leewheel
            ObjectGuid casterGuid;
            p >> casterGuid;
            if (casterGuid != bot->GetGUID())
                return;
            //By leewheel 2026-08-04: 跳过CastID(ObjectGuid packed),读SpellID
            ObjectGuid castId;
            p >> castId;  // CastID, 业务不需要但必须读取以推进rpos
            uint32 spellId;
            p >> spellId;
            //End By leewheel
            SpellInterrupted(spellId);
            return;
        }
        case SMSG_SPELL_DELAYED:
        {
            WorldPacket p(packet);
            //By leewheel 2026-08-04: SMSG包rpos已是0,移除多余rpos(0); 加长度检查
            //TC的SMSG_SPELL_DELAYED格式: casterGuid(8)+delaytime(4)=12字节
            if (p.wpos() < p.rpos() || (p.wpos() - p.rpos()) < 12)
                return;
            //End By leewheel
            ObjectGuid casterGuid;
            p >> casterGuid;
            if (casterGuid != bot->GetGUID())
                return;

            uint32 delaytime;
            p >> delaytime;
            if (delaytime <= 1000)
                IncreaseNextCheckDelay(delaytime);
            return;
        }
        case SMSG_EMOTE:  // do not react to NPC emotes
        {
            WorldPacket p(packet);
            //By leewheel 2026-08-04: 修复——TC的SMSG_EMOTE格式与AC字段顺序相反
            //AC格式: emoteId(4) + source(8) = 12字节
            //TC格式(Emote packet): Guid(8) + EmoteID(4) + SpellVisualKitIDs.size(4) + SequenceVariation(4) + ...
            //原代码 p >> emoteId >> source 把 Guid 的前4字节当 emoteId, 后4字节+EmoteID当 source, 完全错位
            //导致 source.IsPlayer() 判断错误, 玩家表情被误判为NPC表情不转发, bot互相做表情无反应
            //修复: 按TC格式 Guid(8) + EmoteID(4) 读取, SMSG包rpos已是0无需rpos(0)
            if (p.wpos() < p.rpos() || (p.wpos() - p.rpos()) < 12)
                return;

            ObjectGuid source;
            uint32 emoteId;
            p >> source >> emoteId;
            //End By leewheel
            if (source.IsPlayer())
                botOutgoingPacketHandlers.AddPacket(packet);

            return;
        }
        case SMSG_MESSAGECHAT:  // do not react to self or if not ready to reply
        {
            if (!sPlayerbotAIConfig.randomBotTalk)
                return;

            if (!AllowActivity())
                return;

            WorldPacket p(packet);
            if (!p.empty() && (p.GetOpcode() == SMSG_MESSAGECHAT || p.GetOpcode() == SMSG_GM_MESSAGECHAT))
            {
                //By leewheel 2026-08-04: 修复——TC的SMSG_CHAT(=SMSG_MESSAGECHAT)格式与AC完全不同
                //AC格式: msgtype(1)+lang(4)+guid1(8)+unused(4)+[textLen(4)+name]+[chanName]+guid2(8)+textLen(4)+message+chatTag(1)
                //TC格式(Chat packet): SlashCmd(1)+_Language(4)+SenderGUID(8)+SenderGuildGUID(8)+SenderAccountGUID(8)
                //  +TargetGUID(8)+TargetVirtualAddress(4)+SenderVirtualAddress(4)+AchievementID(4)+DisplayTime(4)+SpellID(4)
                //  +bits[SenderName(11)+TargetName(11)+Prefix(5)+Channel(7)+ChatText(12)+ChatFlags(15)+HideChatLog(1)
                //  +FakeSenderName(1)+Unused_801(1)+ChannelGUID(1)]+FlushBits
                //  +strings[SenderName+TargetName+Prefix+Channel+ChatText]+[Optional Unused_801]+[Optional ChannelGUID]
                //原代码按AC格式用 >> 读null-terminated string, TC是bit-packed(WriteBits长度+WriteString), 完全无法解析
                //导致bot之间的randomBotTalk功能在TC下完全失效(读到垃圾值或抛ByteBufferException被try-catch吞掉)
                //修复: 按TC格式解析, 提取msgtype/lang/guid1/guid2/chanName/message用于后续业务逻辑
                //包体最小: 1+4+8*4+4*5 = 57字节(扁平部分) + 65bits(对齐9字节) = 66字节 + 5个空字符串
                if (p.wpos() < p.rpos() || (p.wpos() - p.rpos()) < 57)
                    return;

                uint8 msgtype;
                uint32 lang;
                ObjectGuid guid1, guid2;  // guid1=SenderGUID, guid2=TargetGUID
                p >> msgtype >> lang;
                p >> guid1;  // SenderGUID
                // 跳过 SenderGuildGUID(8) + SenderAccountGUID(8)
                ObjectGuid senderGuildGuid, senderAccountGuid;
                p >> senderGuildGuid >> senderAccountGuid;
                p >> guid2;  // TargetGUID
                // 跳过 TargetVirtualAddress(4) + SenderVirtualAddress(4) + AchievementID(4) + DisplayTime(4) + SpellID(4) = 20字节
                uint32 targetVirtualAddr, senderVirtualAddr, achievementId;
                float displayTime;
                int32 spellId;
                p >> targetVirtualAddr >> senderVirtualAddr >> achievementId >> displayTime >> spellId;

                if (guid1.IsEmpty())
                    return;

                if (lang == LANG_ADDON)
                    return;

                // 读 bit-packed 字符串长度
                uint32 senderNameLen = p.ReadBits(11);
                uint32 targetNameLen = p.ReadBits(11);
                uint32 prefixLen = p.ReadBits(5);
                uint32 channelLen = p.ReadBits(7);
                uint32 textLen = p.ReadBits(12);
                //By leewheel 2026-09-03 修复C4189警告：chatFlags仅用于AC的chatTag计算(已删)，位必须按序
                //读出保持位流对齐，读出后弃用
                p.ReadBits(15);  // chatFlags
                //End By leewheel
                p.ReadBit();  // HideChatLog
                p.ReadBit();  // FakeSenderName
                //By leewheel 2026-09-03 修复C4189警告：hasUnused801/hasChannelGuid/chatTag为AC扁平包时代遗留，
                //TC的bit必须按序读出(ReadBit不可省略，否则位流错位)，读出后弃用即可，chatTag无引用一并移除
                p.ReadBit();  // hasUnused801 (TC包中未使用的位，按序读取保持位流对齐)
                p.ReadBit();  // hasChannelGuid (同上)
                //End By leewheel
                p.ResetBitPos();

                // 读字符串
                std::string name(p.ReadString(senderNameLen));
                std::string targetName(p.ReadString(targetNameLen));
                std::string prefix(p.ReadString(prefixLen));
                std::string chanName(p.ReadString(channelLen));
                std::string message(p.ReadString(textLen));
                //End By leewheel

                // 仅处理感兴趣的聊天类型
                switch (msgtype)
                {
                    case CHAT_MSG_CHANNEL:
                    case CHAT_MSG_SAY:
                    case CHAT_MSG_PARTY:
                    case CHAT_MSG_YELL:
                    case CHAT_MSG_WHISPER:
                    case CHAT_MSG_GUILD:
                        break;
                    default:
                        return;
                }

                if (chanName == "World")
                    return;

                // do not reply to self but always try to reply to real player
                if (guid1 != bot->GetGUID())
                {
                    time_t lastChat = GetAiObjectContext()->GetValue<time_t>("last said", "chat")->Get();
                    bool isPaused = time(0) < lastChat;
                    bool isFromFreeBot = false;
                    sCharacterCache->GetCharacterNameByGuid(guid1, name);
                    uint32 accountId = sCharacterCache->GetCharacterAccountIdByGuid(guid1);
                    isFromFreeBot = sPlayerbotAIConfig.IsInRandomAccountList(accountId);
                    bool isMentioned = message.find(bot->GetName()) != std::string::npos;

                    // ChatChannelSource chatChannelSource = GetChatChannelSource(bot, msgtype, chanName);

                    // random bot speaks, chat CD
                    if (isFromFreeBot && isPaused)
                        return;

                    // BG: react only if mentioned or if not channel and real player spoke
                    if (bot->InBattleground() && !(isMentioned || (msgtype != CHAT_MSG_CHANNEL && !isFromFreeBot)))
                        return;

                    if (HasRealPlayerMaster() && guid1 != GetMaster()->GetGUID())
                        return;

                    auto itemIds = GetChatHelper()->ExtractAllItemIds(message);
                    if (message.starts_with(sPlayerbotAIConfig.toxicLinksPrefix) &&
                        (itemIds.size() > 0 || GetChatHelper()->ExtractAllQuestIds(message).size() > 0) &&
                        sPlayerbotAIConfig.toxicLinksRepliesChance)
                    {
                        if (urand(0, 50) > 0 || urand(1, 100) > sPlayerbotAIConfig.toxicLinksRepliesChance)
                            return;
                    }
                    else if (itemIds.count(19019) && sPlayerbotAIConfig.thunderfuryRepliesChance)
                    {
                        if (urand(0, 60) > 0 || urand(1, 100) > sPlayerbotAIConfig.thunderfuryRepliesChance)
                            return;
                    }
                    else
                    {
                        if (isFromFreeBot && urand(0, 20))
                            return;

                        // if (msgtype == CHAT_MSG_GUILD && (!sPlayerbotAIConfig.guildRepliesRate || urand(1, 100) >=
                        // sPlayerbotAIConfig.guildRepliesRate)) return;

                        if (!isFromFreeBot)
                        {
                            if (!isMentioned && urand(0, 4))
                                return;
                        }
                        else
                        {
                            if (urand(0, 20 + 10 * isMentioned))
                                return;
                        }
                    }

                    QueueChatResponse(ChatQueuedReply{static_cast<uint32>(msgtype), static_cast<uint32>(guid1.GetCounter()), static_cast<uint32>(guid2.GetCounter()), message,
                                                      chanName, name,
                                                      time(nullptr) + urand(inCombat ? 10 : 5, inCombat ? 25 : 15)});
                    GetAiObjectContext()->GetValue<time_t>("last said", "chat")->Set(time(0) + urand(5, 25));
                    return;
                }
            }

            return;
        }
        case SMSG_FORCE_MOVE_ROOT:      // CMSG_FORCE_MOVE_ROOT_ACK
        case SMSG_FORCE_MOVE_UNROOT:    // CMSG_FORCE_MOVE_UNROOT_ACK
        {
            // Quick fix for CMSG_FORCE_MOVE_ROOT_ACK and CMSG_FORCE_MOVE_UNROOT_ACK:
            // this should resolve issues with MOVEMENTFLAG_ROOT being permanently set
            // when rooted during lost client control (charm + root effects)
            // @see https://github.com/azerothcore/azerothcore-wotlk/pull/23147
            bool forceRoot = (packet.GetOpcode() == SMSG_FORCE_MOVE_ROOT);
            if (forceRoot)
            {
                bot->m_movementInfo.RemoveMovementFlag(MOVEMENTFLAG_MASK_MOVING_FLY);
                bot->m_movementInfo.AddMovementFlag(MOVEMENTFLAG_ROOT);
                bot->StopMoving();
            }
            else
                bot->m_movementInfo.RemoveMovementFlag(MOVEMENTFLAG_ROOT);

            return;
        }
        case SMSG_MOVE_KNOCK_BACK:      // CMSG_MOVE_KNOCK_BACK_ACK
        {
            WorldPacket p(packet);
            //By leewheel 2026-08-04: SMSG包rpos已是0,移除多余rpos(0); 加长度检查
            //格式: guid(8)+counter(4)+vcos(4)+vsin(4)+horizontalSpeed(4)+verticalSpeed(4)=28字节
            if (p.wpos() < p.rpos() || (p.wpos() - p.rpos()) < 28)
                return;
            //End By leewheel

            ObjectGuid guid;
            uint32 counter;
            float vcos, vsin, horizontalSpeed, verticalSpeed = 0.f;

            p >> guid >> counter >> vcos >> vsin >> horizontalSpeed >> verticalSpeed;
            if (horizontalSpeed <= 0.1f)
                horizontalSpeed = 0.11f;
            verticalSpeed = -verticalSpeed;

            bot->CastStop();
            bot->StopMoving();
            bot->GetMotionMaster()->Clear();

            // Unit* currentTarget = GetAiObjectContext()->GetValue<Unit*>("current target")->Get();
            //By leewheel 2026-07-11: TC使用MoveKnockbackFrom(参数为Position引用)
            bot->GetMotionMaster()->MoveKnockbackFrom(Position(bot->GetPositionX() - vcos, bot->GetPositionY() - vsin, bot->GetPositionZ()), horizontalSpeed, verticalSpeed);
            //End By leewheel

            // bot->AddUnitMovementFlag(MOVEMENTFLAG_FALLING);
            // bot->AddUnitMovementFlag(MOVEMENTFLAG_FORWARD);
            // bot->m_movementInfo.AddMovementFlag(MOVEMENTFLAG_PENDING_STOP);
            // if (bot->m_movementInfo.HasMovementFlag(MOVEMENTFLAG_SPLINE_ELEVATION))
            //     bot->m_movementInfo.RemoveMovementFlag(MOVEMENTFLAG_SPLINE_ELEVATION);
            // bot->GetMotionMaster()->MoveIdle();
            // Position dest = bot->GetPosition();
            // float moveTimeHalf = verticalSpeed / Movement::gravity;
            // float dist = 2 * moveTimeHalf * horizontalSpeed;
            // float max_height = -Movement::computeFallElevation(moveTimeHalf, false, -verticalSpeed);

            // Use a mmap raycast to get a valid destination.
            // bot->GetMotionMaster()->MoveKnockbackFrom(fx, fy, horizontalSpeed, verticalSpeed);

            // // set delay based on actual distance
            // float newdis = sqrt(ServerFacade::instance().GetDistance2d(bot, fx, fy));
            // SetNextCheckDelay((uint32)((newdis / dis) * moveTimeHalf * 4 * IN_MILLISECONDS));

            // // add moveflags

            // // copy MovementInfo
            // MovementInfo movementInfo = bot->m_movementInfo;

            // // send ack
            // WorldPacket ack(CMSG_MOVE_KNOCK_BACK_ACK);
            // // movementInfo.jump.cosAngle = vcos;
            // // movementInfo.jump.sinAngle = vsin;
            // // movementInfo.jump.zspeed = -verticalSpeed;
            // // movementInfo.jump.xyspeed = horizontalSpeed;
            // ack << bot->GetGUID().WriteAsPacked();
            // // bot->m_mover->BuildMovementPacket(&ack);
            // ack << (uint32)0;
            // bot->BuildMovementPacket(&ack);
            // // ack << movementInfo.jump.sinAngle;
            // // ack << movementInfo.jump.cosAngle;
            // // ack << movementInfo.jump.xyspeed;
            // // ack << movementInfo.jump.zspeed;
            // bot->GetSession()->HandleMoveKnockBackAck(ack);

            // // // set jump destination for MSG_LAND packet
            // SetJumpDestination(Position(x, y, z, bot->GetOrientation()));

            // bot->Heart();

            // */
            return;
        }
        case SMSG_DISMOUNT:
        {
            WorldPacket p(packet);
            //By leewheel 2026-08-04: SMSG包rpos已是0,移除多余rpos(0); 加长度检查
            //格式: guid(8) = 8字节
            if (p.wpos() < p.rpos() || (p.wpos() - p.rpos()) < 8)
                return;
            //End By leewheel
            ObjectGuid guid;
            p >> guid;
            if (guid != bot->GetGUID())
                return;
            CheckMountStateAction::CompleteDismount(bot);
            return;
        }
        //By leewheel 2026-07-20: TC的SMSG_LFG_PROPOSAL_UPDATE包格式与AC不同
        //TC格式: RideTicket(ObjectGuid+uint32 Id+uint32 Type+int64 Time+bit), uint64 InstanceID, uint32 ProposalID, ...
        //需要在触发"lfg proposal" trigger前解析ProposalID并存入AI值
        //这样LfgAcceptAction的AI_VALUE路径能正确获取proposal ID
        case SMSG_LFG_PROPOSAL_UPDATE:
        {
            WorldPacket p(packet);
            //By leewheel 2026-08-04: SMSG包rpos已是0,移除多余rpos(0); 加长度检查
            //格式: requesterGuid(8)+ticketId(4)+rideType(4)+ticketTime(8)+1bit+instanceId(8)+proposalId(4)=37字节
            if (p.wpos() < p.rpos() || (p.wpos() - p.rpos()) < 37)
                return;
            //End By leewheel
            // 跳过RideTicket
            ObjectGuid requesterGuid;
            p >> requesterGuid;
            uint32 ticketId;
            p >> ticketId;
            //By leewheel 2026-07-21: 服务端RideTicket写包为uint32 Type + int64 Time(Timestamp<>默认int64)，
            //原按uint8+uint32读取会错位7字节，proposalId读进恒0的InstanceID区域导致永不接受提议。
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
            uint32 proposalId;
            p >> proposalId;

            if (proposalId)
            {
                if (auto* v = aiObjectContext->GetValue<uint32>("lfg proposal"))
                    v->Set(proposalId);
            }
            botOutgoingPacketHandlers.AddPacket(packet);
            return;
        }
        //End By leewheel
        default:
            botOutgoingPacketHandlers.AddPacket(packet);
    }
    }
    //By leewheel 2026-07-14: catch ByteBuffer异常
    //By leewheel 2026-09-03 修复C4101警告：异常变量e在调试日志注释状态下未引用，省略变量名保留异常类型捕获
    catch (ByteBufferException const&)
    {
        // TC_LOG_DEBUG("playerbots", "HandleBotOutgoingPacket: 包解析异常 opcode={}, 错误={}",
        //     packet.GetOpcode(), e.what());
    }
    catch (std::exception const&)
    {
        // TC_LOG_DEBUG("playerbots", "HandleBotOutgoingPacket: 未知异常 opcode={}, 错误={}",
        //     packet.GetOpcode(), e.what());
    }
    //End By leewheel
}

void PlayerbotAI::SpellInterrupted(uint32 spellid)
{
    for (uint8 type = CURRENT_MELEE_SPELL; type <= CURRENT_CHANNELED_SPELL; type++)
    {
        Spell* spell = bot->GetCurrentSpell((CurrentSpellTypes)type);
        if (!spell)
            continue;

        SpellInfo const* spellInfo = spell->GetSpellInfo();
        if (!spellInfo)
            continue;

        if (spellInfo->Id == spellid)
            bot->InterruptSpell((CurrentSpellTypes)type);
    }
    // LastSpellCast& lastSpell = aiObjectContext->GetValue<LastSpellCast&>("last spell cast")->Get();
    // lastSpell.id = 0;
}

int32 PlayerbotAI::CalculateGlobalCooldown(uint32 spellid)
{
    if (!spellid)
        return 0;

    if (bot->HasSpellCooldown(spellid))
        return sPlayerbotAIConfig.globalCoolDown;

    return sPlayerbotAIConfig.reactDelay;
}

void PlayerbotAI::HandleMasterIncomingPacket(WorldPacket const& packet)
{
    masterIncomingPacketHandlers.AddPacket(packet);
}

void PlayerbotAI::HandleMasterOutgoingPacket(WorldPacket const& packet)
{
    masterOutgoingPacketHandlers.AddPacket(packet);
}

void PlayerbotAI::ChangeEngine(BotState type)
{
    Engine* engine = engines[type];

    if (currentEngine != engine)
    {
        currentEngine = engine;
        currentState = type;
        ReInitCurrentEngine();

        switch (type)
        {
            case BOT_STATE_COMBAT:
                ChangeEngineOnCombat();
                break;
            case BOT_STATE_NON_COMBAT:
                ChangeEngineOnNonCombat();
                break;
            case BOT_STATE_DEAD:
                // LOG_DEBUG("playerbots",  "=== {} DEAD ===", bot->GetName().c_str());
                break;
            default:
                break;
        }
    }
}

void PlayerbotAI::ChangeEngineOnCombat()
{
    //By leewheel 2026-08-18: 移植 brighton-chi the-lab c6a7d104(修复等待攻击战斗状态检测#2650)
    //进入战斗时记录战斗开始时间,供"等待攻击"策略计算等待时长
    if (HasStrategy("wait for attack", BOT_STATE_COMBAT))
        aiObjectContext->GetValue<time_t>("combat start time")->Set(time(nullptr));
    //End By leewheel

    if (HasStrategy("stay", BOT_STATE_COMBAT))
    {
        aiObjectContext->GetValue<PositionInfo>("pos", "stay")
            ->Set(PositionInfo(bot->GetPositionX(), bot->GetPositionY(), bot->GetPositionZ(), bot->GetMapId()));
    }
}

void PlayerbotAI::ChangeEngineOnNonCombat()
{
    //By leewheel 2026-08-18: 移植 brighton-chi the-lab c6a7d104(修复等待攻击战斗状态检测#2650)
    //脱离战斗时清零战斗开始时间
    if (HasStrategy("wait for attack", BOT_STATE_COMBAT))
        aiObjectContext->GetValue<time_t>("combat start time")->Set(0);
    //End By leewheel

    if (HasStrategy("stay", BOT_STATE_NON_COMBAT))
    {
        aiObjectContext->GetValue<PositionInfo>("pos", "stay")->Reset();
    }
}

void PlayerbotAI::DoNextAction(bool min)
{
    //By leewheel 2026-07-17: DoNextAction调用日志已禁用（刷屏严重�?
    //End By leewheel

    if (!bot->IsInWorld() || bot->IsBeingTeleported() || (GetMaster() && GetMaster()->IsBeingTeleported()))
    {
        //By leewheel 2026-07-17: 提前返回日志已禁�?
        //End By leewheel
        SetNextCheckDelay(sPlayerbotAIConfig.globalCoolDown);
        return;
    }

    // Change engine if just died
    bool isBotAlive = bot->IsAlive();
    if (currentEngine != engines[BOT_STATE_DEAD] && !isBotAlive)
    {
        // Death Count to prevent skeleton piles
        // Player* master = GetMaster();  // warning here - whipowill
        //By leewheel 2026-07-14: 为DoNextAction中的GetValue调用添加空指针保护，防止ACCESS_VIOLATION崩溃
        if (!HasActivePlayerMaster() && !bot->InBattleground())
        {
            if (auto* v = aiObjectContext->GetValue<uint32>("death count"))
            {
                uint32 dCount = v->Get();
                v->Set(++dCount);
            }
        }

        if (auto* v = aiObjectContext->GetValue<Unit*>("current target")) v->Set(nullptr);
        if (auto* v = aiObjectContext->GetValue<Unit*>("enemy player target")) v->Set(nullptr);
        if (auto* v = aiObjectContext->GetValue<ObjectGuid>("pull target")) v->Set(ObjectGuid::Empty);
        if (auto* v = aiObjectContext->GetValue<ObjectGuid>("pull strategy target")) v->Set(ObjectGuid::Empty);
        if (auto* v = aiObjectContext->GetValue<LootObject>("loot target")) v->Set(LootObject());
        //End By leewheel

        ChangeEngine(BOT_STATE_DEAD);
        return;
    }

    // Change engine if just ressed (no movement update when rooted)
    if (currentEngine == engines[BOT_STATE_DEAD] && isBotAlive && !bot->IsRooted())
    {
        bot->SendMovementFlagUpdate();

        ChangeEngine(BOT_STATE_NON_COMBAT);
        return;
    }

    // Clear targets if in combat but sticking with old data
    //By leewheel 2026-07-14: 添加空指针保护防止崩�?
    if (currentEngine == engines[BOT_STATE_NON_COMBAT] && bot->IsInCombat())
    {
        auto targetValue = aiObjectContext->GetValue<Unit*>("current target");
        if (targetValue)
        {
            Unit* currentTarget = targetValue->Get();
            if (currentTarget != nullptr)
            {
                targetValue->Set(nullptr);
            }
        }
    }
    //End By leewheel

    bool minimal = !this->AllowActivity();

    currentEngine->DoNextAction(nullptr, 0, (minimal || min));

    if (minimal)
    {
        if (!bot->isAFK() && !bot->InBattleground() && !HasRealPlayerMaster())
            bot->ToggleAFK();

        SetNextCheckDelay(sPlayerbotAIConfig.passiveDelay);
        return;
    }
    //By leewheel 2026-09-05: 上游6704d553——selfbot(玩家自身挂机时保持在线)不自动切AFK
    else if (bot->isAFK() && !IsSelfBot(bot))
        bot->ToggleAFK();
    //End By leewheel

    if (master && master->IsInWorld())
    {
        float distance = ServerFacade::instance().GetDistance2d(bot, master);

        if (master->m_movementInfo.HasMovementFlag(MOVEMENTFLAG_WALKING) && distance < 20.0f)
            bot->m_movementInfo.AddMovementFlag(MOVEMENTFLAG_WALKING);
        else
            bot->m_movementInfo.RemoveMovementFlag(MOVEMENTFLAG_WALKING);

        if (master->IsSitState() && nextAICheckDelay < 1000)
        {
            if (!bot->isMoving() && distance < 10.0f)
                bot->SetStandState(UNIT_STAND_STATE_SIT);
        }
        else if (nextAICheckDelay < 1000)
            bot->SetStandState(UNIT_STAND_STATE_STAND);
    }
    else if (bot->m_movementInfo.HasMovementFlag(MOVEMENTFLAG_WALKING))
        bot->m_movementInfo.RemoveMovementFlag(MOVEMENTFLAG_WALKING);
    else if ((nextAICheckDelay < 1000) && bot->IsSitState())
        bot->SetStandState(UNIT_STAND_STATE_STAND);

    bool hasMountAura = bot->HasAuraType(SPELL_AURA_MOD_INCREASE_MOUNTED_SPEED) ||
                        bot->HasAuraType(SPELL_AURA_MOD_INCREASE_MOUNTED_FLIGHT_SPEED);
    if (hasMountAura && !bot->IsMounted())
    {
        bot->RemoveAurasByType(SPELL_AURA_MOD_INCREASE_MOUNTED_SPEED);
        bot->RemoveAurasByType(SPELL_AURA_MOD_INCREASE_MOUNTED_FLIGHT_SPEED);
    }
}

void PlayerbotAI::ReInitCurrentEngine()
{
    currentEngine->Init();
}

void PlayerbotAI::ChangeStrategy(std::string const names, BotState type)
{
    Engine* e = engines[type];
    if (!e)
        return;

    e->ChangeStrategy(names);
}

void PlayerbotAI::ClearStrategies(BotState type)
{
    Engine* e = engines[type];
    if (!e)
        return;

    e->removeAllStrategies();
}

// Resets only the combat or non-combat engine: wipe strategies, repopulate with class/spec defaults,
// re-apply current map's instance strategy (if any), and call Init() to rebuild trigger/action lists.
void PlayerbotAI::SelectiveResetStrategies(BotState type)
{
    Engine* e = engines[type];
    if (!e)
        return;

    e->removeAllStrategies();

    if (type == BOT_STATE_COMBAT)
        AiFactory::AddDefaultCombatStrategies(bot, this, e);
    else if (type == BOT_STATE_NON_COMBAT)
        AiFactory::AddDefaultNonCombatStrategies(bot, this, e);

    if (sPlayerbotAIConfig.applyInstanceStrategies)
        ApplyInstanceStrategies(bot->GetMapId());

    e->Init();
}

std::vector<std::string> PlayerbotAI::GetStrategies(BotState type)
{
    Engine* e = engines[type];
    if (!e)
        return std::vector<std::string>();

    return e->GetStrategies();
}

void PlayerbotAI::ApplyInstanceStrategies(uint32 mapId, bool tellMaster)
{
    static const std::vector<std::string> allInstanceStrategies =
    {
        "aq20", "blacktemple", "bwl", "gruulslair", "hyjal", "icc", "karazhan",
        //By leewheel 2026-07-26: 补回参考项目遗漏的"rs"。否则离开红玉圣殿(map 724)后，
        //移除循环扫不到"rs"，RS团本策略会残留并在其它地图继续触发，导致错误行为。
        "magtheridon", "moltencore", "naxx", "onyxia", "rs", "ssc", "tbc-ac", "tempestkeep",
        //End By leewheel
        //By leewheel 2026-07-26: 新增brighton-chi 4个TBC五人本策略，禁止离开后残留。
        "tbc-hfr", "tbc-mgt", "tbc-ub", "tbc-mech", "tbc-seth",
        //End By leewheel
        "ulduar", "voa", "wotlk-an", "wotlk-cos", "wotlk-dtk", "wotlk-eoe", "wotlk-fos",
        "wotlk-gd", "wotlk-hol", "wotlk-hor", "wotlk-hos", "wotlk-nex", "wotlk-occ",
        "wotlk-ok", "wotlk-os", "wotlk-pos", "wotlk-toc", "wotlk-uk", "wotlk-up",
        //By leewheel 2026-07-26: 新增太阳井(SWP)团本策略，禁止离开后残留。
        "wotlk-vh", "zulaman", "sunwell"
        //End By leewheel
    };

    for (const std::string& strat : allInstanceStrategies)
    {
        engines[BOT_STATE_COMBAT]->removeStrategy(strat);
        engines[BOT_STATE_NON_COMBAT]->removeStrategy(strat);
    }

    std::string strategyName;
    switch (mapId)
    {
        case 249:
            strategyName = "onyxia";  // Onyxia's Lair
            break;
        case 409:
            strategyName = "moltencore";  // Molten Core
            break;
        case 469:
            strategyName = "bwl";  // Blackwing Lair
            break;
        case 509:
            strategyName = "aq20";  // Ruins of Ahn'Qiraj
            break;
        case 532:
            strategyName = "karazhan";  // Karazhan
            break;
        case 533:
            strategyName = "naxx";  // Naxxramas
            break;
        case 534:
            strategyName = "hyjal";  // The Battle for Mount Hyjal (Hyjal Summit)
            break;
        case 544:
            strategyName = "magtheridon";  // Magtheridon's Lair
            break;
        case 548:
            strategyName = "ssc";  // Serpentshrine Cavern
            break;
        //By leewheel 2026-07-26: 新增brighton-chi 4个TBC五人本mapId映射。
        case 543:
            strategyName = "tbc-hfr";  // Hellfire Citadel: Hellfire Ramparts
            break;
        case 546:
            strategyName = "tbc-ub";   // Coilfang Reservoir: The Underbog
            break;
        case 554:
            strategyName = "tbc-mech"; // Tempest Keep: The Mechanar
            break;
        case 556:
            strategyName = "tbc-seth"; // Auchindoun: Sethekk Halls
            break;
        //End By leewheel
        case 550:
            strategyName = "tempestkeep";  // Tempest Keep: The Eye
            break;
        case 558:
            strategyName = "tbc-ac"; // Auchindoun: Auchenai Crypts
            break;
        case 564:
            strategyName = "blacktemple";  // Black Temple
            break;
        case 565:
            strategyName = "gruulslair";  // Gruul's Lair
            break;
        case 568:
            strategyName = "zulaman";  // Zul'Aman
            break;
        //By leewheel 2026-07-26: 太阳井高地(Sunwell Plateau) map 580。
        case 580:
            strategyName = "sunwell";  // Sunwell Plateau
            break;
        //End By leewheel
        //By leewheel 2026-09-05: 上游8c000dfc——魔导师平台(Magisters' Terrace) map 585
        case 585:
            strategyName = "tbc-mgt";  // Magisters' Terrace
            break;
        //End By leewheel
        case 574:
            strategyName = "wotlk-uk";  // Utgarde Keep
            break;
        case 575:
            strategyName = "wotlk-up";  // Utgarde Pinnacle
            break;
        case 576:
            strategyName = "wotlk-nex";  // The Nexus
            break;
        case 578:
            strategyName = "wotlk-occ";  // The Oculus
            break;
        case 595:
            strategyName = "wotlk-cos";  // The Culling of Stratholme
            break;
        case 599:
            strategyName = "wotlk-hos";  // Halls of Stone
            break;
        case 600:
            strategyName = "wotlk-dtk";  // Drak'Tharon Keep
            break;
        case 601:
            strategyName = "wotlk-an";  // Azjol-Nerub
            break;
        case 602:
            strategyName = "wotlk-hol";  // Halls of Lightning
            break;
        case 603:
            strategyName = "ulduar";  // Ulduar
            break;
        case 604:
            strategyName = "wotlk-gd";  // Gundrak
            break;
        case 608:
            strategyName = "wotlk-vh";  // Violet Hold
            break;
        case 615:
            strategyName = "wotlk-os";  // Obsidian Sanctum
            break;
        case 616:
            strategyName = "wotlk-eoe";  // Eye Of Eternity
            break;
        case 619:
            strategyName = "wotlk-ok";  // Ahn'kahet: The Old Kingdom
            break;
        case 624:
            strategyName = "voa";  // Vault of Archavon
            break;
        case 631:
            strategyName = "icc";  // Icecrown Citadel
            break;
        case 632:
            strategyName = "wotlk-fos";  // The Forge of Souls
            break;
        case 650:
            strategyName = "wotlk-toc";  // Trial of the Champion
            break;
        case 658:
            strategyName = "wotlk-pos";  // Pit of Saron
            break;
        case 668:
            strategyName = "wotlk-hor";  // Halls of Reflection
            break;
        case 724:
            strategyName = "rs";  // Ruby Sanctum
            break;
        default:
            break;
    }

    if (strategyName.empty())
        return;

    engines[BOT_STATE_COMBAT]->addStrategy(strategyName);
    engines[BOT_STATE_NON_COMBAT]->addStrategy(strategyName);

    if (tellMaster && !strategyName.empty())
    {
        std::ostringstream out;
        //By leewheel 2026-08-01: 玩家可见文本中文化
        out << "已添加实例策略: " << strategyName;
        //End By leewheel
        TellMasterNoFacing(out.str());
    }
}

//By leewheel 2026-08-23: 合并 the-lab(#2571) —— 非团本地下城判断(ForceRebuff 只在非团本/野外使用)
bool PlayerbotAI::IsInNonRaidDungeon() const
{
    MapEntry const* mapEntry = sMapStore.LookupEntry(bot->GetMapId());
    return mapEntry && mapEntry->IsNonRaidDungeon();
}
//End By leewheel

bool PlayerbotAI::DoSpecificAction(std::string const name, Event event, bool silent, std::string const qualifier)
{
    std::ostringstream out;

    for (uint8 i = 0; i < BOT_STATE_MAX; i++)
    {
        ActionResult res = engines[i]->ExecuteAction(name, event, qualifier);
        switch (res)
        {
            case ACTION_RESULT_UNKNOWN:
                continue;
            case ACTION_RESULT_OK:
                if (!silent)
                {
                    PlaySound(TEXT_EMOTE_NOD);
                }
                return true;
            case ACTION_RESULT_IMPOSSIBLE:
                //By leewheel 2026-08-01: 玩家可见文本中文化
                out << name << ": 不可能";
                //End By leewheel
                if (!silent)
                {
                    TellError(out.str());
                    PlaySound(TEXT_EMOTE_NO);
                }
                return false;
            case ACTION_RESULT_USELESS:
                //By leewheel 2026-08-01: 玩家可见文本中文化
                out << name << ": 无用";
                //End By leewheel
                if (!silent)
                {
                    TellError(out.str());
                    PlaySound(TEXT_EMOTE_NO);
                }
                return false;
            case ACTION_RESULT_FAILED:
                if (!silent)
                {
                    //By leewheel 2026-08-01: 玩家可见文本中文化
                    out << name << ": 失败";
                    //End By leewheel
                    TellError(out.str());
                }
                return false;
        }
    }

    if (!silent)
    {
        //By leewheel 2026-08-01: 玩家可见文本中文化
        out << name << ": 未知动作";
        //End By leewheel
        TellError(out.str());
    }

    return false;
}

bool PlayerbotAI::PlaySound(uint32 emote)
{
    //By leewheel 2026-07-11: sDB2Manager是DB2Manager::Instance()宏，返回引用不是指针，用.替代->
    if (EmotesTextSoundEntry const* soundEntry = sDB2Manager.GetTextSoundEmoteFor(emote, bot->getRace(), bot->getGender(), bot->getClass()))
    {
        bot->PlayDistanceSound(soundEntry->SoundID);
        //End By leewheel
        return true;
    }

    return false;
}

bool PlayerbotAI::PlayEmote(uint32 emote)
{
//By leewheel 2026-07-11: TC的CTextEmote没有默认构造函数，需用WorldPacket构�?
    WorldPacket data(CMSG_SEND_TEXT_EMOTE);
    WorldPackets::Chat::CTextEmote packet(std::move(data));
    packet.EmoteID = emote;
    packet.SoundIndex = EmoteAction::GetNumberOfEmoteVariants((TextEmotes)emote, bot->getRace(), bot->getGender());
    packet.Target = (master && (ServerFacade::instance().GetDistance2d(bot, master) < 30.0f) && urand(0, 1)) ? master->GetGUID()
                 : (bot->GetTarget() && urand(0, 1))                                            ? bot->GetTarget()
                                                                                                : ObjectGuid::Empty;
    bot->GetSession()->HandleTextEmoteOpcode(packet);
    //End By leewheel

    return false;
}

bool PlayerbotAI::ContainsStrategy(StrategyType type)
{
    for (uint8 i = 0; i < BOT_STATE_MAX; i++)
    {
        if (engines[i]->HasStrategyType(type))
            return true;
    }

    return false;
}

bool PlayerbotAI::HasStrategy(std::string const name, BotState type) { return engines[type]->HasStrategy(name); }

Strategy* PlayerbotAI::GetStrategy(std::string const name, BotState type)
{
    return engines[type] ? engines[type]->GetStrategy(name) : nullptr;
}

//By leewheel 2026-07-26: 移植目标排除查询，转发至战斗引擎(与参考项目一致)。
bool PlayerbotAI::HasTargetExclusions() const
{
    return engines[BOT_STATE_COMBAT] && engines[BOT_STATE_COMBAT]->HasTargetExclusions();
}
//End By leewheel

void PlayerbotAI::ResetStrategies(bool /*load*/)
{
    for (uint8 i = 0; i < BOT_STATE_MAX; i++)
        engines[i]->removeAllStrategies();

    AiFactory::AddDefaultCombatStrategies(bot, this, engines[BOT_STATE_COMBAT]);
    AiFactory::AddDefaultNonCombatStrategies(bot, this, engines[BOT_STATE_NON_COMBAT]);
    AiFactory::AddDefaultDeadStrategies(bot, this, engines[BOT_STATE_DEAD]);
    if (sPlayerbotAIConfig.applyInstanceStrategies)
        ApplyInstanceStrategies(bot->GetMapId());

    for (uint8 i = 0; i < BOT_STATE_MAX; i++)
        engines[i]->Init();

    // if (load)
    //     PlayerbotRepository::instance().Load(this);
}

bool PlayerbotAI::IsRanged(Player* player, bool bySpec)
{
    PlayerbotAI* botAi = GET_PLAYERBOT_AI(player);
    if (!bySpec && botAi)
        return botAi->ContainsStrategy(STRATEGY_TYPE_RANGED);

    int tab = AiFactory::GetPlayerSpecTab(player);
    switch (player->getClass())
    {
        case CLASS_DEATH_KNIGHT:
        case CLASS_WARRIOR:
        case CLASS_ROGUE:
            return false;
            break;
        case CLASS_DRUID:
            if (tab == 1)
            {
                return false;
            }
            break;
        case CLASS_PALADIN:
            if (tab != 0)
            {
                return false;
            }
            break;
        case CLASS_SHAMAN:
            if (tab == 1)
            {
                return false;
            }
            break;
    }

    return true;
}

bool PlayerbotAI::IsMelee(Player* player, bool bySpec) { return !IsRanged(player, bySpec); }

bool PlayerbotAI::IsCaster(Player* player, bool bySpec)
{
    return IsRanged(player, bySpec) && player->getClass() != CLASS_HUNTER;
}

bool PlayerbotAI::IsCombo(Player* player)
{
    // int tab = AiFactory::GetPlayerSpecTab(player);
    return player->getClass() == CLASS_ROGUE ||
           (player->getClass() == CLASS_DRUID && player->HasAura(768));  // cat druid
}

bool PlayerbotAI::IsRangedDps(Player* player, bool bySpec)
{
    return IsRanged(player, bySpec) && IsDps(player, bySpec);
}

// If true, indexLivingOnly excludes dead group members from the index, meaning living members shift
// up to fill the gap. Example: if assist heal 0 dies, then assist heal 1 becomes assist heal 0.
// By default, it is false, meaning roles remain stable across deaths.
bool PlayerbotAI::IsAssistHealOfIndex(Player* player, uint8 index, bool indexLivingOnly)
{
    if (!IsHeal(player))
        return false;

    if (indexLivingOnly && !player->IsAlive())
        return false;

    Group* group = player->GetGroup();
    if (!group)
        return false;

    uint8 totalAssistants = 0;
    uint8 assistantsBeforePlayer = 0;
    uint8 nonAssistantsBeforePlayer = 0;
    bool playerFound = false;

    for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
    {
        Player* member = ref->GetSource();
        if (!member || (indexLivingOnly && !member->IsAlive()) || !IsHeal(member))
            continue;

        bool isAssistant = group->IsAssistant(member->GetGUID());

        if (isAssistant)
            totalAssistants++;

        if (member == player)
            playerFound = true;
        else if (!playerFound)
        {
            if (isAssistant)
                assistantsBeforePlayer++;
            else
                nonAssistantsBeforePlayer++;
        }
    }

    if (!playerFound)
        return false;

    // If the player is an assistant, their index is just the number of assistants before them.
    // If they are a non-assistant, their index is shifted by the total number of assistants.
    uint8 playerIndex = group->IsAssistant(player->GetGUID())
        ? assistantsBeforePlayer : (totalAssistants + nonAssistantsBeforePlayer);

    return playerIndex == index;
}

// If true, indexLivingOnly excludes dead group members from the index, meaning living members shift
// up to fill the gap. Example: if assist ranged dps 0 dies, then assist ranged dps 1 becomes assist
// ranged dps 0. By default, it is false, meaning roles remain stable across deaths.
bool PlayerbotAI::IsAssistRangedDpsOfIndex(Player* player, uint8 index, bool indexLivingOnly)
{
    if (!IsRangedDps(player))
        return false;

    if (indexLivingOnly && !player->IsAlive())
        return false;

    Group* group = player->GetGroup();
    if (!group)
        return false;

    uint8 totalAssistants = 0;
    uint8 assistantsBeforePlayer = 0;
    uint8 nonAssistantsBeforePlayer = 0;
    bool playerFound = false;

    for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
    {
        Player* member = ref->GetSource();
        if (!member || (indexLivingOnly && !member->IsAlive()) || !IsRangedDps(member))
            continue;

        bool isAssistant = group->IsAssistant(member->GetGUID());

        if (isAssistant)
            totalAssistants++;

        if (member == player)
            playerFound = true;
        else if (!playerFound)
        {
            if (isAssistant)
                assistantsBeforePlayer++;
            else
                nonAssistantsBeforePlayer++;
        }
    }

    if (!playerFound)
        return false;

    // If the player is an assistant, their index is just the number of assistants before them.
    // If they are a non-assistant, their index is shifted by the total number of assistants.
    uint8 playerIndex = group->IsAssistant(player->GetGUID())
        ? assistantsBeforePlayer : (totalAssistants + nonAssistantsBeforePlayer);

    return playerIndex == index;
}

bool PlayerbotAI::HasAggro(Unit* unit)
{
    if (!IsValidUnit(unit))
        return false;

    bool isMT = IsExplicitMainTank(bot);
    Unit* victim = unit->GetVictim();
    if (victim && (victim->GetGUID() == bot->GetGUID() || (!isMT && victim->ToPlayer() && IsTank(victim->ToPlayer()))))
    {
        return true;
    }
    return false;
}

bool PlayerbotAI::IsMovementImpaired(Unit* unit)
{
    return unit && (unit->HasAuraType(SPELL_AURA_MOD_ROOT) || unit->IsRooted() || unit->GetSpeedRate(MOVE_RUN) < 1.0f);
}

int32 PlayerbotAI::GetAssistTankIndex(Player* player)
{
    Group* group = player->GetGroup();
    if (!group)
    {
        return -1;
    }

    int counter = 0;
    for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
    {
        Player* member = ref->GetSource();
        if (!member)
        {
            continue;
        }

        if (player == member)
        {
            return counter;
        }

        if (IsTank(member, true) && group->IsAssistant(member->GetGUID()))
        {
            counter++;
        }
    }

    return 0;
}

int32 PlayerbotAI::GetGroupSlotIndex(Player* player)
{
    Group* group = bot->GetGroup();
    if (!group)
    {
        return -1;
    }
    int counter = 0;
    for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
    {
        Player* member = ref->GetSource();

        if (!member)
        {
            continue;
        }

        if (player == member)
        {
            return counter;
        }
        counter++;
    }
    return 0;
}

int32 PlayerbotAI::GetRangedIndex(Player* player)
{
    if (!IsRanged(player))
    {
        return -1;
    }
    Group* group = bot->GetGroup();
    if (!group)
    {
        return -1;
    }
    int counter = 0;
    for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
    {
        Player* member = ref->GetSource();

        if (!member)
        {
            continue;
        }

        if (player == member)
        {
            return counter;
        }
        if (IsRanged(member))
        {
            counter++;
        }
    }
    return 0;
}

int32 PlayerbotAI::GetClassIndex(Player* player, uint8 cls)
{
    if (player->getClass() != cls)
    {
        return -1;
    }
    Group* group = bot->GetGroup();
    if (!group)
    {
        return -1;
    }
    int counter = 0;
    for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
    {
        Player* member = ref->GetSource();

        if (!member)
        {
            continue;
        }

        if (player == member)
        {
            return counter;
        }
        if (member->getClass() == cls)
        {
            counter++;
        }
    }
    return 0;
}
int32 PlayerbotAI::GetRangedDpsIndex(Player* player)
{
    if (!IsRangedDps(player))
    {
        return -1;
    }
    Group* group = bot->GetGroup();
    if (!group)
    {
        return -1;
    }
    int counter = 0;
    for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
    {
        Player* member = ref->GetSource();

        if (!member)
        {
            continue;
        }

        if (player == member)
        {
            return counter;
        }
        if (IsRangedDps(member))
        {
            counter++;
        }
    }
    return 0;
}

int32 PlayerbotAI::GetMeleeIndex(Player* player)
{
    if (IsRanged(player))
    {
        return -1;
    }
    Group* group = bot->GetGroup();
    if (!group)
    {
        return -1;
    }
    int counter = 0;
    for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
    {
        Player* member = ref->GetSource();

        if (!member)
        {
            continue;
        }

        if (player == member)
        {
            return counter;
        }
        if (!IsRanged(member))
        {
            counter++;
        }
    }
    return 0;
}

bool PlayerbotAI::IsTank(Player* player, bool bySpec)
{
    PlayerbotAI* botAi = GET_PLAYERBOT_AI(player);
    if (!bySpec && botAi)
        return botAi->ContainsStrategy(STRATEGY_TYPE_TANK);

    int tab = AiFactory::GetPlayerSpecTab(player);
    switch (player->getClass())
    {
        case CLASS_DEATH_KNIGHT:
            // bySpec=true: 鲜血天赋页即为坦克，不需要冰霜灵气
            // bySpec=false(无botAI回退): 鲜血页或冰霜灵气均视为坦克
            if (tab == DEATH_KNIGHT_TAB_BLOOD || (!bySpec && player->HasAura(SPELL_DK_FROST_PRESENCE)))
            {
                return true;
            }
            break;
        case CLASS_PALADIN:
            if (tab == PALADIN_TAB_PROTECTION)
            {
                return true;
            }
            break;
        case CLASS_WARRIOR:
            if (tab == WARRIOR_TAB_PROTECTION)
            {
                return true;
            }
            break;
        case CLASS_DRUID:
            // bySpec=true: 野性天赋页即为坦克，不需要熊形态/厚皮光环
            // bySpec=false(无botAI回退): 需要熊形态或厚皮光环确认
            if (tab == DRUID_TAB_FERAL &&
                (bySpec || player->GetShapeshiftForm() == FORM_BEAR ||
                 player->GetShapeshiftForm() == FORM_DIREBEAR || player->HasAura(16931)))
            {
                return true;
            }
            break;
    }
    return false;
}

bool PlayerbotAI::IsHeal(Player* player, bool bySpec)
{
    PlayerbotAI* botAi = GET_PLAYERBOT_AI(player);
    if (!bySpec && botAi)
        return botAi->ContainsStrategy(STRATEGY_TYPE_HEAL);

    int tab = AiFactory::GetPlayerSpecTab(player);
    switch (player->getClass())
    {
        case CLASS_PRIEST:
            if (tab == PRIEST_TAB_DISCIPLINE || tab == PRIEST_TAB_HOLY)
            {
                return true;
            }
            break;
        case CLASS_DRUID:
            if (tab == DRUID_TAB_RESTORATION)
            {
                return true;
            }
            break;
        case CLASS_SHAMAN:
            if (tab == SHAMAN_TAB_RESTORATION)
            {
                return true;
            }
            break;
        case CLASS_PALADIN:
            if (tab == PALADIN_TAB_HOLY)
            {
                return true;
            }
            break;
    }
    return false;
}

bool PlayerbotAI::IsDps(Player* player, bool bySpec)
{
    PlayerbotAI* botAi = GET_PLAYERBOT_AI(player);
    if (!bySpec && botAi)
        return botAi->ContainsStrategy(STRATEGY_TYPE_DPS);

    int tab = AiFactory::GetPlayerSpecTab(player);
    switch (player->getClass())
    {
        case CLASS_MAGE:
        case CLASS_WARLOCK:
        case CLASS_HUNTER:
        case CLASS_ROGUE:
            return true;
        case CLASS_PRIEST:
            if (tab == PRIEST_TAB_SHADOW)
            {
                return true;
            }
            break;
        case CLASS_DRUID:
            if (tab == DRUID_TAB_BALANCE)
            {
                return true;
            }
            if (tab == DRUID_TAB_FERAL && !IsTank(player, bySpec))
            {
                return true;
            }
            break;
        case CLASS_SHAMAN:
            if (tab != SHAMAN_TAB_RESTORATION)
            {
                return true;
            }
            break;
        case CLASS_PALADIN:
            if (tab == PALADIN_TAB_RETRIBUTION)
            {
                return true;
            }
            break;
        case CLASS_DEATH_KNIGHT:
            if (tab != DEATH_KNIGHT_TAB_BLOOD)
            {
                return true;
            }
            break;
        case CLASS_WARRIOR:
            if (tab != WARRIOR_TAB_PROTECTION)
            {
                return true;
            }
            break;
    }
    return false;
}

//By leewheel 2026-08-01: 按上游(5e4d617a)重构——引入GetMainTankGuid消除嵌套循环。
//优先返回带主坦标记(MEMBER_FLAG_MAINTANK)的成员，否则返回第一个存活坦克。
ObjectGuid PlayerbotAI::GetMainTankGuid(Group* group)
{
    if (!group)
        return ObjectGuid::Empty;

    Group::MemberSlotList const& slots = group->GetMemberSlots();
    for (Group::member_citerator itr = slots.begin(); itr != slots.end(); ++itr)
    {
        if (itr->flags & MEMBER_FLAG_MAINTANK)
            return itr->guid;
    }

    for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
    {
        Player* member = ref->GetSource();
        if (member && IsTank(member) && member->IsAlive())
            return member->GetGUID();
    }

    return ObjectGuid::Empty;
}

bool PlayerbotAI::IsMainTank(Player* player)
{
    Group* group = player->GetGroup();
    if (!group)
        return IsTank(player);

    ObjectGuid const mainTankGuid = GetMainTankGuid(group);
    return !mainTankGuid.IsEmpty() && player->GetGUID() == mainTankGuid;
}
//End By leewheel

bool PlayerbotAI::IsExplicitMainTank(Player* player)
{
    Group* group = player->GetGroup();
    if (!group)
        return false;

    for (Group::member_citerator itr = group->GetMemberSlots().begin(); itr != group->GetMemberSlots().end(); ++itr)
    {
        if (itr->flags & MEMBER_FLAG_MAINTANK)
            return player->GetGUID() == itr->guid;
    }
    return false;
}

bool PlayerbotAI::IsBotMainTank(Player* player)
{
    if (!player || !player->IsInWorld() || player->IsDuringRemoveFromWorld())
        return false;

    WorldSession* session = player->GetSession();
    if (!session || !session->IsBot())
        return false;

    if (!IsTank(player))
        return false;

    if (IsMainTank(player))
        return true;

    Group* group = player->GetGroup();
    if (!group)
        return true;

    int32 botAssistTankIndex = GetAssistTankIndex(player);
    if (botAssistTankIndex == -1)
        return false;

    for (GroupReference* gref = group->GetFirstMember(); gref; gref = gref->next())
    {
        Player* member = gref->GetSource();
        if (!member)
            continue;

        int32 memberAssistTankIndex = GetAssistTankIndex(member);
        if (memberAssistTankIndex == -1)
            continue;

        if (memberAssistTankIndex == botAssistTankIndex && player == member)
            return true;

        if (memberAssistTankIndex < botAssistTankIndex && member->GetSession()->IsBot())
            return false;
    }

    return false;
}

uint32 PlayerbotAI::GetGroupTankNum(Player* player)
{
    Group* group = player->GetGroup();
    if (!group)
        return 0;

    uint32 result = 0;
    for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
    {
        Player* member = ref->GetSource();

        if (!member)
            continue;

        if (IsTank(member) && member->IsAlive())
            result++;
    }

    return result;
}

//By leewheel 2026-08-01: 按上游(5e4d617a)重构——基于GetMainTankGuid消除嵌套循环，行为等价
bool PlayerbotAI::IsAssistTank(Player* player)
{
    if (!IsTank(player))
        return false;

    Group* group = player->GetGroup();
    if (!group)
        return false;

    return player->GetGUID() != GetMainTankGuid(group);
}

// If true, indexLivingOnly excludes dead group members from the index, meaning living members shift
// up to fill the gap. Example: if assist tank 0 dies, then assist tank 1 becomes assist tank 0.
// By default, it is false, meaning roles remain stable across deaths.
bool PlayerbotAI::IsAssistTankOfIndex(Player* player, uint8 index, bool indexLivingOnly)
{
    if (!IsTank(player))
        return false;

    Group* group = player->GetGroup();
    if (!group)
        return false;

    ObjectGuid const mainTankGuid = GetMainTankGuid(group);

    if (player->GetGUID() == mainTankGuid)
        return false;

    if (indexLivingOnly && !player->IsAlive())
        return false;

    uint8 totalAssistants = 0;
    uint8 assistantsBeforePlayer = 0;
    uint8 nonAssistantsBeforePlayer = 0;
    bool playerFound = false;

    for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
    {
        Player* member = ref->GetSource();
        if (!member || (indexLivingOnly && !member->IsAlive()) || !IsTank(member) ||
            member->GetGUID() == mainTankGuid)
        {
            continue;
        }

        bool isAssistant = group->IsAssistant(member->GetGUID());

        if (isAssistant)
            totalAssistants++;

        if (member == player)
            playerFound = true;
        else if (!playerFound)
        {
            if (isAssistant)
                assistantsBeforePlayer++;
            else
                nonAssistantsBeforePlayer++;
        }
    }

    if (!playerFound)
        return false;

    // If the player is an assistant, their index is just the number of assistants before them.
    // If they are a non-assistant, their index is shifted by the total number of assistants.
    uint8 playerIndex = group->IsAssistant(player->GetGUID())
        ? assistantsBeforePlayer : (totalAssistants + nonAssistantsBeforePlayer);

    return playerIndex == index;
}
//End By leewheel

namespace acore
{
class UnitByGuidInRangeCheck
{
public:
    UnitByGuidInRangeCheck(WorldObject const* obj, ObjectGuid guid, float range)
        : i_obj(obj), i_range(range), i_guid(guid)
    {
    }
    WorldObject const& GetFocusObject() const { return *i_obj; }
    bool operator()(Unit* u) { return u->GetGUID() == i_guid && i_obj->IsWithinDistInMap(u, i_range); }

private:
    WorldObject const* i_obj;
    float i_range;
    ObjectGuid i_guid;
};

class GameObjectByGuidInRangeCheck
{
public:
    GameObjectByGuidInRangeCheck(WorldObject const* obj, ObjectGuid guid, float range)
        : i_obj(obj), i_range(range), i_guid(guid)
    {
    }
    WorldObject const& GetFocusObject() const { return *i_obj; }
    bool operator()(GameObject* u)
    {
        if (u && i_obj->IsWithinDistInMap(u, i_range) && u->isSpawned() && u->GetGOInfo() && u->GetGUID() == i_guid)
            return true;

        return false;
    }

private:
    WorldObject const* i_obj;
    float i_range;
    ObjectGuid i_guid;
};
};  // namespace acore

Unit* PlayerbotAI::GetUnit(ObjectGuid guid)
{
    if (!guid)
        return nullptr;

    return ObjectAccessor::GetUnit(*bot, guid);
}

Player* PlayerbotAI::GetPlayer(ObjectGuid guid)
{
    Unit* unit = GetUnit(guid);
    return unit ? unit->ToPlayer() : nullptr;
}

uint32 GetCreatureIdForCreatureTemplateId(uint32 creatureTemplateId)
{
    //By leewheel 2026-07-11: TC的Query不接受fmt格式字符串，使用PQuery
    QueryResult results =
        WorldDatabase.PQuery("SELECT guid FROM `creature` WHERE id = {} LIMIT 1;", creatureTemplateId);
    //End By leewheel
    if (results)
    {
        Field* fields = results->Fetch();
        return fields[0].Get<uint32>();
    }
    return 0;
}

Unit* PlayerbotAI::GetUnit(CreatureData const* creatureData)
{
    if (!creatureData)
        return nullptr;

    //By leewheel 2026-07-11: TC的mapid是方法而非成员，需�?)调用
    Map* map = sMapMgr->FindMap(creatureData->mapId, 0);
    //End By leewheel
    if (!map)
        return nullptr;

    uint32 spawnId = creatureData->spawnId;
    if (!spawnId)  // workaround for CreatureData with missing spawnId (this just uses first matching creatureId in DB,
                   // but thats ok this method is only used for battlemasters and theres only 1 of each type)
        spawnId = GetCreatureIdForCreatureTemplateId(creatureData->id);
    auto creatureBounds = map->GetCreatureBySpawnIdStore().equal_range(spawnId);
    if (creatureBounds.first == creatureBounds.second)
        return nullptr;

    return creatureBounds.first->second;
}

Creature* PlayerbotAI::GetCreature(ObjectGuid guid)
{
    if (!guid)
        return nullptr;

    return ObjectAccessor::GetCreature(*bot, guid);
}

GameObject* PlayerbotAI::GetGameObject(ObjectGuid guid)
{
    if (!guid)
        return nullptr;

    return ObjectAccessor::GetGameObject(*bot, guid);
}

// GameObject* PlayerbotAI::GetGameObject(GameObjectData const* gameObjectData)
// {
//     if (!gameObjectData)
//         return nullptr;

//     Map* map = sMapMgr->FindMap(gameObjectData->mapid, 0);
//     if (!map)
//         return nullptr;

//     auto gameobjectBounds = map->GetGameObjectBySpawnIdStore().equal_range(gameObjectData->spawnId);
//     if (gameobjectBounds.first == gameobjectBounds.second)
//         return nullptr;

//     return gameobjectBounds.first->second;
// }

WorldObject* PlayerbotAI::GetWorldObject(ObjectGuid guid)
{
    if (!guid)
        return nullptr;

    return ObjectAccessor::GetWorldObject(*bot, guid);
}

const AreaTableEntry* PlayerbotAI::GetCurrentArea()
{
    return sAreaTableStore.LookupEntry(
        bot->GetMap()->GetAreaId(bot->GetPhaseMask(), bot->GetPositionX(), bot->GetPositionY(), bot->GetPositionZ()));
}

const AreaTableEntry* PlayerbotAI::GetCurrentZone()
{
    return sAreaTableStore.LookupEntry(
        bot->GetMap()->GetZoneId(bot->GetPhaseMask(), bot->GetPositionX(), bot->GetPositionY(), bot->GetPositionZ()));
}

std::string PlayerbotAI::GetLocalizedAreaName(const AreaTableEntry* entry)
{
    std::string name;
    if (entry)
    {
    //By leewheel 2026-09-09: TC-Cata的AreaTableEntry用AreaName字段(LocalizedString)而非area_name()方法
        name = entry->AreaName[sWorld->GetDefaultDbcLocale()];
        if (name.empty())
            name = entry->AreaName[LOCALE_enUS];
    //End By leewheel
    }

    return name;
}

std::string PlayerbotAI::GetLocalizedCreatureName(uint32 entry)
{
    std::string name;
    const CreatureLocale* cl = sObjectMgr->GetCreatureLocale(entry);
    if (cl)
        ObjectMgr::GetLocaleString(cl->Name, sWorld->GetDefaultDbcLocale(), name);
    if (name.empty())
    {
        CreatureTemplate const* ct = sObjectMgr->GetCreatureTemplate(entry);
        if (ct)
            name = ct->Name;
    }
    return name;
}

std::string PlayerbotAI::GetLocalizedGameObjectName(uint32 entry)
{
    std::string name;
    const GameObjectLocale* gl = sObjectMgr->GetGameObjectLocale(entry);
    if (gl)
        ObjectMgr::GetLocaleString(gl->Name, sWorld->GetDefaultDbcLocale(), name);
    if (name.empty())
    {
        GameObjectTemplate const* gt = sObjectMgr->GetGameObjectTemplate(entry);
        if (gt)
            name = gt->name;
    }
    return name;
}

std::vector<Player*> PlayerbotAI::GetRealPlayersInGroup()
{
    std::vector<Player*> members;

    Group* group = bot->GetGroup();

    if (!group)
        return members;

    for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
    {
        Player* member = ref->GetSource();

        if (!member)
        {
            continue;
        }

        if (GET_PLAYERBOT_AI(member) && !GET_PLAYERBOT_AI(member)->IsRealPlayer())
            continue;

        members.push_back(ref->GetSource());
    }

    return members;
}

std::vector<Player*> PlayerbotAI::GetAllPlayersInGroup()
{
    std::vector<Player*> members;

    Group* group = bot->GetGroup();

    if (!group)
        return members;

    for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
    {
        Player* member = ref->GetSource();

        if (!member)
        {
            continue;
        }

        members.push_back(ref->GetSource());
    }

    return members;
}

bool PlayerbotAI::SayToGuild(const std::string& msg)
{
    if (msg.empty())
    {
        return false;
    }

    if (bot->GetGuildId())
    {
        if (Guild* guild = sGuildMgr->GetGuildById(bot->GetGuildId()))
        {
            if (!Guild_HasRankRight(guild, bot->GetGUID(), GR_RIGHT_GCHATSPEAK))
            {
                return false;
            }
            guild->BroadcastToGuild(bot->GetSession(), false, msg.c_str(), LANG_UNIVERSAL);
            return true;
        }
    }

    return false;
}

bool PlayerbotAI::SayToWorld(const std::string& msg)
{
    if (msg.empty())
    {
        return false;
    }

    //By leewheel 2026-07-14: TC使用ForTeam传入Team枚举(ALLIANCE/HORDE)，而非TeamId(0/1)
    ChannelMgr* cMgr = ChannelMgr::ForTeam(bot->GetTeam());
    //End By leewheel 2026-07-14
    if (!cMgr)
        return false;

    // no zone
    if (Channel* worldChannel = cMgr->GetCustomChannel("World"))
    //End By leewheel
    {
        worldChannel->Say(bot->GetGUID(), msg.c_str(), LANG_UNIVERSAL);
        return true;
    }

    return false;
}

bool PlayerbotAI::SayToChannel(const std::string& msg, const ChatChannelId& chanId)
{
    // Checks whether the message or ChannelMgr is valid
    if (msg.empty())
        return false;

    //By leewheel 2026-07-14: TC使用ForTeam传入Team枚举(ALLIANCE/HORDE)，而非TeamId(0/1)
    ChannelMgr* cMgr = ChannelMgr::ForTeam(bot->GetTeam());
    //End By leewheel 2026-07-14
    if (!cMgr)
        return false;

    AreaTableEntry const* current_zone = GetCurrentZone();
    if (!current_zone)
        return false;

    //By leewheel 2026-07-11: TC没有GetChannels()，使用GetSystemChannel直接获取
    const auto current_str_zone = GetLocalizedAreaName(current_zone);

    std::mutex socialMutex;
    std::lock_guard<std::mutex> lock(socialMutex);  // Blocking for thread safety when accessing SocialMgr

    Channel* channel = cMgr->GetSystemChannel(static_cast<uint32>(chanId), current_zone);
    if (channel)
    {
        // Checks if the channel name is empty
        if (!channel->GetName().empty())
        {
            channel->Say(bot->GetGUID(), msg.c_str(), LANG_UNIVERSAL);
            return true;
        }
    }
    //End By leewheel
    return false;
}

bool PlayerbotAI::SayToParty(const std::string& msg)
{
    if (!bot->GetGroup())
        return false;

    WorldPacket data;
    BuildChatPacket(data, CHAT_MSG_PARTY, msg.c_str(), Language(LANG_UNIVERSAL), uint16(CHAT_TAG_NONE), bot->GetGUID(),
                                 bot->GetName().c_str());

    for (auto receiver : GetRealPlayersInGroup())
    {
        ServerFacade::instance().SendPacket(receiver, &data);
    }

    return true;
}

bool PlayerbotAI::SayToRaid(const std::string& msg)
{
    if (!bot->GetGroup() || !bot->GetGroup()->isRaidGroup())
        return false;

    WorldPacket data;
    BuildChatPacket(data, CHAT_MSG_RAID, msg.c_str(), Language(LANG_UNIVERSAL), uint16(CHAT_TAG_NONE), bot->GetGUID(),
                                 bot->GetName().c_str());

    for (auto receiver : GetRealPlayersInGroup())
    {
        ServerFacade::instance().SendPacket(receiver, &data);
    }

    return true;
}

bool PlayerbotAI::Yell(const std::string& msg)
{
    if (bot->GetTeamId() == TeamId::TEAM_ALLIANCE)
    {
        bot->Yell(msg, LANG_COMMON);
    }
    else
    {
        bot->Yell(msg, LANG_ORCISH);
    }

    return true;
}

bool PlayerbotAI::Say(const std::string& msg)
{
    if (bot->GetTeamId() == TeamId::TEAM_ALLIANCE)
    {
        bot->Say(msg, LANG_COMMON);
    }
    else
    {
        bot->Say(msg, LANG_ORCISH);
    }

    return true;
}

bool PlayerbotAI::Whisper(const std::string& msg, const std::string& receiverName)
{
    const auto receiver = ObjectAccessor::FindPlayerByName(receiverName);
    if (!receiver)
    {
        return false;
    }

    if (bot->GetTeamId() == TeamId::TEAM_ALLIANCE)
    {
        bot->Whisper(msg, LANG_COMMON, receiver);
    }
    else
    {
        bot->Whisper(msg, LANG_ORCISH, receiver);
    }

    return true;
}

bool PlayerbotAI::TellMasterNoFacing(std::ostringstream& stream, PlayerbotSecurityLevel securityLevel)
{
    return TellMasterNoFacing(stream.str(), securityLevel);
}

bool PlayerbotAI::TellMasterNoFacing(std::string const text, PlayerbotSecurityLevel securityLevel)
{
    Player* master = GetMaster();
    PlayerbotAI* masterBotAI = nullptr;
    if (master)
        masterBotAI = GET_PLAYERBOT_AI(master);

    if ((!master || (masterBotAI && !masterBotAI->IsRealPlayer())) &&
        (sPlayerbotAIConfig.randomBotSayWithoutMaster || HasStrategy("debug", BOT_STATE_NON_COMBAT)))
    {
        bot->Say(text, (bot->GetTeamId() == TEAM_ALLIANCE ? LANG_COMMON : LANG_ORCISH));
        return true;
    }

    if (!IsTellAllowed(securityLevel))
        return false;

    time_t lastSaid = whispers[text];

    if (!lastSaid || (time(nullptr) - lastSaid) >= sPlayerbotAIConfig.repeatDelay / 1000)
    {
        whispers[text] = time(nullptr);

        ChatMsg type = CHAT_MSG_WHISPER;
        if (currentChat.second - time(nullptr) >= 1)
            type = currentChat.first;

        //By leewheel 2026-07-18: 诊断组队打招呼空消息问题 - 记录实际发送的 type 和 text
        // TC_LOG_INFO("playerbots", "[BotGreet] TellMasterNoFacing bot=\"{}\" master=\"{}\" type={} currentChat.first={} currentChat.second={} now={} text=\"{}\" textLen={}",
        //     bot->GetName(), master ? master->GetName() : "null",
        //     static_cast<uint32>(type),
        //     static_cast<uint32>(currentChat.first),
        //     static_cast<int64>(currentChat.second),
        //     static_cast<int64>(time(nullptr)),
        //     text, text.size());
        //End By leewheel

        WorldPacket data;
        //By leewheel 2026-07-18: 修复打招呼空消息BUG
        //原 ChatHandler::BuildChatPacket 写的是 AC 格式（uint32长度+字符串+null），但 TC WOTLK 8.x 客户端
        //期望的 SMSG_CHAT 包结构是 WorldPackets::Chat::Chat（位字段 WriteBits/WriteString 格式）。
        //两种格式完全不兼容，导致客户端解析错位，msg 显示为空。
        //改用 TC 原生的 WorldPackets::Chat::Chat::Initialize + Write 来正确构造包。
        {
            WorldPackets::Chat::Chat packet;
            ChatMsg realType = (type == CHAT_MSG_ADDON) ? CHAT_MSG_PARTY : type;
            uint32 realLang = (type == CHAT_MSG_ADDON) ? LANG_ADDON : LANG_UNIVERSAL;
            packet.Initialize(realType, Language(realLang), bot, master, text);
            data = *packet.Write();
        }
        //End By leewheel
        master->SendDirectMessage(&data);
    }

    return true;
}

bool PlayerbotAI::TellError(std::string const text, PlayerbotSecurityLevel securityLevel)
{
    Player* master = GetMaster();
    if (!IsTellAllowed(securityLevel) || !master || GET_PLAYERBOT_AI(master))
        return false;

    if (PlayerbotMgr* mgr = GET_PLAYERBOT_MGR(master))
        mgr->TellError(bot->GetName(), text);

    return false;
}

bool PlayerbotAI::IsTellAllowed(PlayerbotSecurityLevel securityLevel)
{
    Player* master = GetMaster();
    if (!master || master->IsBeingTeleported())
        return false;

    if (!GetSecurity()->CheckLevelFor(securityLevel, true, master))
        return false;

    if (sPlayerbotAIConfig.whisperDistance && !bot->GetGroup() && sRandomPlayerbotMgr.IsRandomBot(bot) &&
        !master->CanBeGameMaster() &&
        (bot->GetMapId() != master->GetMapId() ||
         ServerFacade::instance().GetDistance2d(bot, master) > sPlayerbotAIConfig.whisperDistance))
        return false;

    return true;
}

bool PlayerbotAI::TellMaster(std::ostringstream& stream, PlayerbotSecurityLevel securityLevel)
{
    return TellMaster(stream.str(), securityLevel);
}

bool PlayerbotAI::TellMaster(std::string const text, PlayerbotSecurityLevel securityLevel)
{
    if (!master)
    {
        if (sPlayerbotAIConfig.randomBotSayWithoutMaster)
            return TellMasterNoFacing(text, securityLevel);

        return false;
    }
    if (!TellMasterNoFacing(text, securityLevel))
        return false;

    if (!bot->isMoving() && !bot->IsInCombat() && bot->GetMapId() == master->GetMapId() &&
        !bot->HasUnitState(UNIT_STATE_IN_FLIGHT) && !bot->IsFlying())
    {
        if (!bot->HasInArc(EMOTE_ANGLE_IN_FRONT, master, sPlayerbotAIConfig.sightDistance))
            bot->SetFacingToObject(master);

        bot->HandleEmoteCommand(EMOTE_ONESHOT_TALK);
    }

    return true;
}

bool IsRealAura(Player* bot, AuraEffect const* aurEff, Unit const* unit)
{
    if (!unit || !unit->IsInWorld() || unit->IsDuringRemoveFromWorld())
        return false;

    if (!aurEff)
        return false;

    if (!unit->IsHostileTo(bot))
        return true;

    SpellInfo const* spellInfo = aurEff->GetSpellInfo();
    if (!spellInfo)
        return false;

    uint32 stacks = aurEff->GetBase()->GetStackAmount();
    if (stacks >= spellInfo->StackAmount)
        return true;

    if (aurEff->GetCaster() == bot || spellInfo->IsPositive() ||
        spellInfo->GetEffects()[aurEff->GetEffIndex()].IsAreaAuraEffect())
        return true;

    return false;
}

bool PlayerbotAI::HasAura(std::string const name, Unit* unit, bool maxStack, bool checkIsOwner, int maxAuraAmount,
                          bool checkDuration)
{
    if (!IsValidUnit(unit))
        return false;

    std::wstring wnamepart;
    if (!Utf8toWStr(name, wnamepart))
        return false;

    wstrToLower(wnamepart);

    int auraAmount = 0;

    // Iterate through all aura types
    for (uint32 auraType = SPELL_AURA_BIND_SIGHT; auraType < TOTAL_AURAS; auraType++)
    {
        Unit::AuraEffectList const& auras = unit->GetAuraEffectsByType((AuraType)auraType);
        if (auras.empty())
            continue;

        // Iterate through each aura effect
        for (AuraEffect const* aurEff : auras)
        {
            if (!aurEff)
                continue;

            SpellInfo const* spellInfo = aurEff->GetSpellInfo();
            if (!spellInfo)
                continue;

// Check if the aura name matches
//By leewheel 2026-07-13: 使用多locale辅助函数
std::string const auraNameStr = GetSpellNameBestLocaleWithCache(spellInfo->Id, spellInfo->SpellName);
std::string_view const auraName(auraNameStr);
//End By leewheel
if (auraName.empty() || auraName.length() != wnamepart.length() || !Utf8FitTo(auraName, wnamepart))
continue;

            // Check if this is a valid aura for the bot
            if (IsRealAura(bot, aurEff, unit))
            {
                // Check caster if necessary
                if (checkIsOwner && aurEff->GetCasterGUID() != bot->GetGUID())
                    continue;

                // Check aura duration if necessary
                if (checkDuration && aurEff->GetBase()->GetDuration() == -1)
                    continue;

                // Count stacks and charges
                uint32 maxStackAmount = spellInfo->StackAmount;
                uint32 maxProcCharges = spellInfo->ProcCharges;

                // Count the aura based on max stack and proc charges
                if (maxStack)
                {
                    if (maxStackAmount && aurEff->GetBase()->GetStackAmount() >= maxStackAmount)
                        auraAmount++;

                    if (maxProcCharges && aurEff->GetBase()->GetCharges() >= maxProcCharges)
                        auraAmount++;
                }
                else
                {
                    auraAmount++;
                }

                // Early exit if maxAuraAmount is reached
                if (maxAuraAmount < 0 && auraAmount > 0)
                    return true;
            }
        }
    }

    // Return based on the maximum aura amount conditions
    if (maxAuraAmount >= 0)
    {
        return auraAmount == maxAuraAmount || (auraAmount > 0 && auraAmount <= maxAuraAmount);
    }

    return false;
}

bool PlayerbotAI::HasSpell(std::string const spellName) const
{
    uint32 const spellId = aiObjectContext->GetValue<uint32>("spell id", spellName)->Get();
    return spellId && bot->HasSpell(spellId);
}

Aura* PlayerbotAI::GetAura(std::string const name, Unit* unit, bool checkIsOwner, bool checkDuration, int checkStack)
{
    if (!IsValidUnit(unit))
        return nullptr;

    std::wstring wnamepart;
    if (!Utf8toWStr(name, wnamepart))
        return nullptr;

    wstrToLower(wnamepart);

    for (uint32 auraType = SPELL_AURA_BIND_SIGHT; auraType < TOTAL_AURAS; ++auraType)
    {
        Unit::AuraEffectList const& auras = unit->GetAuraEffectsByType((AuraType)auraType);
        if (auras.empty())
            continue;

        for (AuraEffect const* aurEff : auras)
        {
            SpellInfo const* spellInfo = aurEff->GetSpellInfo();
            if (!spellInfo)
                continue;

//By leewheel 2026-07-13: 使用多locale辅助函数
std::string const auraName = GetSpellNameBestLocaleWithCache(spellInfo->Id, spellInfo->SpellName);
//End By leewheel

            // Directly skip if name mismatch (both length and content)
            if (auraName.empty() || auraName.length() != wnamepart.length() || !Utf8FitTo(auraName, wnamepart))
                continue;

            if (!IsRealAura(bot, aurEff, unit))
                continue;

            // Check owner if necessary
            if (checkIsOwner && aurEff->GetCasterGUID() != bot->GetGUID())
                continue;

            // Check duration if necessary
            if (checkDuration && aurEff->GetBase()->GetDuration() == -1)
                continue;

            // Check stack if necessary
            if (checkStack != -1 && aurEff->GetBase()->GetStackAmount() < checkStack)
                continue;

            return aurEff->GetBase();
        }
    }

    return nullptr;
}

bool PlayerbotAI::HasAnyAuraOf(Unit* player, ...)
{
    if (!player)
        return false;

    va_list vl;
    va_start(vl, player);

    const char* cur;
    while ((cur = va_arg(vl, const char*)) != nullptr)
    {
        if (HasAura(cur, player))
        {
            va_end(vl);
            return true;
        }
    }

    va_end(vl);
    return false;
}

bool PlayerbotAI::CanCastSpell(std::string const name, Unit* target, Item* itemTarget)
{
    return CanCastSpell(aiObjectContext->GetValue<uint32>("spell id", name)->Get(), target, true, itemTarget);
}

bool PlayerbotAI::CanCastSpell(uint32 spellid, Unit* target, bool checkHasSpell, Item* itemTarget, Item* castItem)
{
    if (!spellid)
    {
        if (!sPlayerbotAIConfig.logInGroupOnly || (bot->GetGroup() && HasRealPlayerMaster()))
        {
            // LOG_DEBUG("playerbots", "Can cast spell failed. No spellid. - spellid: {}, bot name: {}", spellid,
            //          bot->GetName());
        }
        return false;
    }

    if (bot->HasUnitState(UNIT_STATE_LOST_CONTROL))
    {
        if (!sPlayerbotAIConfig.logInGroupOnly || (bot->GetGroup() && HasRealPlayerMaster()))
        {
            // LOG_DEBUG("playerbots", "Can cast spell failed. Unit state lost control. - spellid: {}, bot name: {}",
            //By leewheel 2026-08-01: 修复历史日志清理残留——第二行参数未被注释导致C2059
            //           spellid, bot->GetName());
            //End By leewheel
        }
        return false;
    }

    if (!target)
        target = bot;

     if (!IsValidUnit(target))
        return false;

    if (Pet* pet = bot->GetPet())
        if (pet->HasSpell(spellid))
            return true;

    if (checkHasSpell && !bot->HasSpell(spellid))
    {
        if (!sPlayerbotAIConfig.logInGroupOnly || (bot->GetGroup() && HasRealPlayerMaster()))
        {
            // LOG_DEBUG("playerbots",
            //          "Can cast spell failed. Bot not has spell. - target name: {}, spellid: {}, bot name: {}",
            //          target->GetName(), spellid, bot->GetName());
        }
        return false;
    }

    //By leewheel 2026-08-04: 修复残留channel导致所有施法永久失败的死循环
    //原逻辑: 有CURRENT_CHANNELED_SPELL就return false → CastSpellAction::Execute永不执行
    //→ CastStop/InterruptSpell永不被调用 → channel永不清除 → bot完全瘫痪
    //修复: 若当前引导的法术与要施放的法术不同,非战斗中先中断残留channel允许施法新法术
    if (bot->GetCurrentSpell(CURRENT_CHANNELED_SPELL) != nullptr)
    {
        Spell const* currentChannel = bot->GetCurrentSpell(CURRENT_CHANNELED_SPELL);
        if (currentChannel && currentChannel->GetSpellInfo()->Id != spellid && !bot->IsInCombat())
        {
            bot->CastStop();
        }
        else
        {
            return false;
        }
    }
    //End By leewheel

    //By leewheel 2026-08-03: 修复崩溃——原代码先调HasSpellCooldown再查spellInfo，
    //spellid非0但DB2中不存在时 HasSpellCooldown→SpellHistory::HasCooldown→AssertSpellInfo 触发ASSERT崩溃
    //(玩家崩溃日志16-44-15)，将spellInfo有效性检查提前到HasSpellCooldown之前
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellid);
    if (!spellInfo)
    {
        if (!sPlayerbotAIConfig.logInGroupOnly || (bot->GetGroup() && HasRealPlayerMaster()))
        {
            // LOG_DEBUG("playerbots", "Can cast spell failed. No spellInfo. - target name: {}, spellid: {}, bot name: {}",
            //By leewheel 2026-08-01: 修复历史日志清理残留
            //           target->GetName(), spellid, bot->GetName());
            //End By leewheel
        }
        return false;
    }

    if (bot->HasSpellCooldown(spellid))
    {
        if (!sPlayerbotAIConfig.logInGroupOnly || (bot->GetGroup() && HasRealPlayerMaster()))
        {
            // LOG_DEBUG("playerbots",
            //          "Can cast spell failed. Spell not has cooldown. - target name: {}, spellid: {}, bot name: {}",
            //          target->GetName(), spellid, bot->GetName());
        }
        return false;
    }

    if ((bot->GetShapeshiftForm() == FORM_FLIGHT || bot->GetShapeshiftForm() == FORM_FLIGHT_EPIC) && !bot->IsInCombat())
    {
        if (!sPlayerbotAIConfig.logInGroupOnly || (bot->GetGroup() && HasRealPlayerMaster()))
        {
            // LOG_DEBUG(
            //     "playerbots",
            //     "Can cast spell failed. In flight form (not in combat). - target name: {}, spellid: {}, bot name: {}",
            //     target->GetName(), spellid, bot->GetName());
        }
        return false;
    }

    //By leewheel 2026-07-11: TC的CalcCastTime接受Spell*而非Unit*，传nullptr
    uint32 CastingTime = !spellInfo->IsChanneled() ? spellInfo->CalcCastTime(nullptr) : spellInfo->GetDuration();
    //End By leewheel
    // bool interruptOnMove = spellInfo->InterruptFlags & SPELL_INTERRUPT_FLAG_MOVEMENT;
    if ((CastingTime || spellInfo->IsAutoRepeatRangedSpell()) && bot->isMoving())
    {
        if (!sPlayerbotAIConfig.logInGroupOnly || (bot->GetGroup() && HasRealPlayerMaster()))
        {
            // LOG_DEBUG("playerbots", "Casting time and bot is moving - target name: {}, spellid: {}, bot name: {}",
            //By leewheel 2026-08-01: 修复历史日志清理残留
            //           target->GetName(), spellid, bot->GetName());
            //End By leewheel
        }
        return false;
    }

    if (!itemTarget)
    {
        // Exception for Deep Freeze (44572) - allow cast for damage on immune targets (e.g., bosses)
        //By leewheel 2026-09-09: TC-Cata的IsImmunedToSpell需要(spellInfo, effectMask, caster)三个参数
        if (Unit_IsImmunedToSpell(target, spellInfo, bot))
        //End By leewheel
        {
            if (spellid != 44572)  // Deep Freeze
            {
                if (!sPlayerbotAIConfig.logInGroupOnly || (bot->GetGroup() && HasRealPlayerMaster()))
                {
                    // LOG_DEBUG("playerbots", "target is immuned to spell - target name: {}, spellid: {}, bot name: {}",
                    //By leewheel 2026-08-01: 修复历史日志清理残留
                    //           target->GetName(), spellid, bot->GetName());
                    //End By leewheel
                }
                return false;
            }
            // Otherwise, allow Deep Freeze even if immune
        }

        if (bot != target && ServerFacade::instance().GetDistance2d(bot, target) > sPlayerbotAIConfig.sightDistance)
        {
            if (!sPlayerbotAIConfig.logInGroupOnly || (bot->GetGroup() && HasRealPlayerMaster()))
            {
                // LOG_DEBUG("playerbots", "target is out of sight distance - target name: {}, spellid: {}, bot name: {}",
                //By leewheel 2026-08-01: 修复历史日志清理残留
                //           target->GetName(), spellid, bot->GetName());
                //End By leewheel
            }
            return false;
        }
    }

    Unit* oldSel = bot->GetSelectedUnit();
    // TRIGGERED_IGNORE_POWER_AND_REAGENT_COST flag for not calling CheckPower in check
    // which avoids buff charge to be ineffectively reduced (e.g. dk freezing fog for howling blast)
    /// @TODO: Fix all calls to ApplySpellMod
    Spell* spell = new Spell(bot, spellInfo, TRIGGERED_IGNORE_POWER_AND_REAGENT_COST);

    spell->m_targets.SetUnitTarget(target);
    spell->m_CastItem = castItem;
    if (itemTarget == nullptr)
    {
        itemTarget = aiObjectContext->GetValue<Item*>("item for spell", spellid)->Get();
        ;
    }
    spell->m_targets.SetItemTarget(itemTarget);
    SpellCastResult result = spell->CheckCast(true);
    delete spell;

    if (oldSel)
        bot->SetSelection(oldSel->GetGUID());

    switch (result)
    {
        case SPELL_FAILED_NOT_INFRONT:
        case SPELL_FAILED_NOT_STANDING:
        case SPELL_FAILED_UNIT_NOT_INFRONT:
        case SPELL_FAILED_MOVING:
        case SPELL_FAILED_TRY_AGAIN:
        case SPELL_CAST_OK:
        case SPELL_FAILED_NOT_SHAPESHIFT:
        case SPELL_FAILED_OUT_OF_RANGE:
        //By leewheel 2026-07-20: TC的CheckCast内部调用IsValidAttackTarget→CanSeeOrDetect
        //playerbot无真实客户端导致CanSeeOrDetect永远false，返回BAD_TARGETS
        //playerbot代码已在调用CanCastSpell前做了自己的目标验证，此处放行安全
        case SPELL_FAILED_BAD_TARGETS:
        case SPELL_FAILED_BAD_IMPLICIT_TARGETS:
        //End By leewheel
            return true;
        default:
            if (!sPlayerbotAIConfig.logInGroupOnly || (bot->GetGroup() && HasRealPlayerMaster()))
            {
            // LOG_DEBUG("playerbots",
            //          "CanCastSpell Check Failed. - target name: {}, spellid: {}, bot name: {}, result: {}",
            //          target->GetName(), spellid, bot->GetName(), result);
            }
            return false;
    }
}

bool PlayerbotAI::CanCastSpell(uint32 spellid, GameObject* goTarget, bool checkHasSpell)
{
    if (!spellid)
        return false;

    if (bot->HasUnitState(UNIT_STATE_LOST_CONTROL))
        return false;

    Pet* pet = bot->GetPet();
    if (pet && pet->HasSpell(spellid))
        return true;

    if (checkHasSpell && !bot->HasSpell(spellid))
        return false;

    if (bot->HasSpellCooldown(spellid))
        return false;

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellid);
    if (!spellInfo)
        return false;

    //By leewheel 2026-07-11: TC的CalcCastTime接受Spell*而非Unit*，传nullptr
    int32 CastingTime = !spellInfo->IsChanneled() ? spellInfo->CalcCastTime(nullptr) : spellInfo->GetDuration();
    //End By leewheel
    if (CastingTime > 0 && bot->isMoving())
        return false;

    if (ServerFacade::instance().GetDistance2d(bot, goTarget) > sPlayerbotAIConfig.sightDistance)
        return false;

    // ObjectGuid oldSel = bot->GetTarget();
    // bot->SetTarget(goTarget->GetGUID());
    Spell* spell = new Spell(bot, spellInfo, TRIGGERED_NONE);

    spell->m_targets.SetGOTarget(goTarget);
    Item* item = aiObjectContext->GetValue<Item*>("item for spell", spellid)->Get();
    spell->m_targets.SetItemTarget(item);

    SpellCastResult result = spell->CheckCast(true);
    delete spell;
    // if (oldSel)
    //     bot->SetTarget(oldSel);

    switch (result)
    {
        case SPELL_FAILED_NOT_INFRONT:
        case SPELL_FAILED_NOT_STANDING:
        case SPELL_FAILED_UNIT_NOT_INFRONT:
        case SPELL_FAILED_MOVING:
        case SPELL_FAILED_TRY_AGAIN:
        case SPELL_CAST_OK:
        //By leewheel 2026-07-20: CanSeeOrDetect对playerbot不可靠
        case SPELL_FAILED_BAD_TARGETS:
        case SPELL_FAILED_BAD_IMPLICIT_TARGETS:
        //End By leewheel
            return true;
        default:
            break;
    }

    return false;
}

bool PlayerbotAI::CanCastSpell(uint32 spellid, float x, float y, float z, bool checkHasSpell, Item* itemTarget)
{
    if (!spellid)
        return false;

    Pet* pet = bot->GetPet();
    if (pet && pet->HasSpell(spellid))
        return true;

    if (checkHasSpell && !bot->HasSpell(spellid))
        return false;

    if (bot->HasSpellCooldown(spellid))
        return false;

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellid);
    if (!spellInfo)
        return false;

    if (!itemTarget)
    {
        if (bot->GetDistance(x, y, z) > sPlayerbotAIConfig.sightDistance)
            return false;
    }

    Spell* spell = new Spell(bot, spellInfo, TRIGGERED_NONE);

    spell->m_targets.SetDst(x, y, z, 0.f);

    Item* item = itemTarget ? itemTarget : aiObjectContext->GetValue<Item*>("item for spell", spellid)->Get();
    spell->m_targets.SetItemTarget(item);

    SpellCastResult result = spell->CheckCast(true);
    delete spell;

    switch (result)
    {
        case SPELL_FAILED_NOT_INFRONT:
        case SPELL_FAILED_NOT_STANDING:
        case SPELL_FAILED_UNIT_NOT_INFRONT:
        case SPELL_FAILED_MOVING:
        case SPELL_FAILED_TRY_AGAIN:
        case SPELL_CAST_OK:
        //By leewheel 2026-07-20: CanSeeOrDetect对playerbot不可靠
        case SPELL_FAILED_BAD_TARGETS:
        case SPELL_FAILED_BAD_IMPLICIT_TARGETS:
        //End By leewheel
            return true;
        default:
            return false;
    }
}

bool PlayerbotAI::CastSpell(std::string const name, Unit* target, Item* itemTarget)
{
    if (!IsValidUnit(target))
        return false;

    bool result = CastSpell(aiObjectContext->GetValue<uint32>("spell id", name)->Get(), target, itemTarget);
    if (result)
    {
        aiObjectContext->GetValue<time_t>("last spell cast time", name)->Set(time(nullptr));
    }

    return result;
}

bool PlayerbotAI::CastSpell(uint32 spellId, Unit* target, Item* itemTarget)
{
    if (!spellId)
        return false;

    if (!target)
        target = bot;

    if (!IsValidUnit(target))
        return false;

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);
    if (!spellInfo)
        return false;

    //By leewheel 2026-07-23: 防止bot对自己施放控制类法术（精神控制等）
    //当target为null时默认设为bot，但charm/possess类法术不能以自己为目标
    //By leewheel 2026-07-24: TC343中CHARM/POSSESS改为通过APPLY_AURA+AuraType实现
    //By leewheel 2026-08-01: 补充SPELL_AURA_AOE_CHARM(177)走HandleCharmConvert，同样不能对自身施放
    //By leewheel 2026-08-02: 扩展覆盖所有控制类Aura(MOD_CONFUSE/MOD_FEAR/MOD_TAUNT/MOD_STUN)——
    //  排查发现6358魅魔诱惑在TC343数据中实际是MOD_STUN(12)效果，原防护只覆盖2/6/177导致漏网
    if (target == bot)
    {
        for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
        {
            if (spellInfo->GetEffects()[i].Effect == SPELL_EFFECT_APPLY_AURA &&
                (spellInfo->GetEffects()[i].ApplyAuraName == SPELL_AURA_MOD_CHARM ||
                 spellInfo->GetEffects()[i].ApplyAuraName == SPELL_AURA_MOD_POSSESS ||
                 spellInfo->GetEffects()[i].ApplyAuraName == SPELL_AURA_AOE_CHARM ||
                 spellInfo->GetEffects()[i].ApplyAuraName == SPELL_AURA_MOD_CONFUSE ||
                 spellInfo->GetEffects()[i].ApplyAuraName == SPELL_AURA_MOD_FEAR ||
                 spellInfo->GetEffects()[i].ApplyAuraName == SPELL_AURA_MOD_TAUNT ||
                 spellInfo->GetEffects()[i].ApplyAuraName == SPELL_AURA_MOD_STUN))
                return false;
        }
    }
    //End By leewheel

    //By leewheel 2026-07-18: 精简施法日志 - 仅组队/团队时输出，含法术名/能量/血量
    //By leewheel 2026-09-03 修复C4189警告：按原设计恢复BotCast施法日志(log中文)，pt恢复实际引用
    if (bot->GetGroup())
    {
        Powers pt = bot->GetPowerType();
        TC_LOG_DEBUG("playerbots", "[BotCast] bot=\"{}\" 准备施法 spell=\"{}\" id={} target=\"{}\" power={}/{} hp={}/{}",
            bot->GetName(), chatHelper->FormatSpell(spellInfo), spellId,
            target->GetName(),
            bot->GetPower(pt), bot->GetMaxPower(pt),
            bot->GetHealth(), bot->GetMaxHealth());
    }
    //End By leewheel

    Pet* pet = bot->GetPet();
    if (pet && pet->HasSpell(spellId))
    {
        // List of spell IDs for which we do NOT want to toggle auto-cast or send message
        // We are excluding Spell Lock and Devour Magic because they are casted in the GenericWarlockStrategy
        // Without this exclusion, the skill would be togged for auto-cast and the player would
        // be spammed with messages about enabling/disabling auto-cast
        switch (spellId)
        {
            case 19244:  // Spell Lock rank 1
            case 19647:  // Spell Lock rank 2
            case 19505:  // Devour Magic rank 1
            case 19731:  // Devour Magic rank 2
            case 19734:  // Devour Magic rank 3
            case 19736:  // Devour Magic rank 4
            case 27276:  // Devour Magic rank 5
            case 27277:  // Devour Magic rank 6
            case 48011:  // Devour Magic rank 7
                // No message - just break out of the switch and let normal cast logic continue
                break;
            default:
                bool autocast = false;
                for (unsigned int& m_autospell : pet->m_autospells)
                {
                    if (m_autospell == spellId)
                    {
                        autocast = true;
                        break;
                    }
                }

                pet->ToggleAutocast(spellInfo, !autocast);
                std::ostringstream out;
                //By leewheel 2026-08-01: 玩家可见文本中文化
                out << (autocast ? "|cffff0000|禁用" : "|cFF00ff00|启用") << " 宠物自动施法: ";
                //End By leewheel
                out << chatHelper->FormatSpell(spellInfo);
                TellMaster(out);
                return true;
        }
    }

    // aiObjectContext->GetValue<LastMovement&>("last movement")->Get().Set(nullptr);
    // aiObjectContext->GetValue<time_t>("stay time")->Set(0);

    //By leewheel 2026-08-07: 择优移植 brighton-chi/the-lab——重力吸引(Gravity Lapse)期间 bot 悬空但仍可施法。
    //39432 = 风暴要塞凯尔萨斯 重力吸引, 44226 = 魔导师平台 重力吸引。
    //By leewheel 2026-08-16: 改用命名常量(SPELL_GRAVITY_LAPSE_TK/MGT)，MgT ID 44224→44226(对齐the-lab 8e3c12b)
    if ((bot->IsFlying() && !bot->HasAura(SPELL_GRAVITY_LAPSE_TK) && !bot->HasAura(SPELL_GRAVITY_LAPSE_MGT)) ||
        bot->HasUnitState(UNIT_STATE_IN_FLIGHT))
    {
        return false;
    }
    //End By leewheel

    // bot->ClearUnitState(UNIT_STATE_CHASE);
    // bot->ClearUnitState(UNIT_STATE_FOLLOW);

    bool failWithDelay = false;
    if (!bot->IsStandState())
    {
        bot->SetStandState(UNIT_STAND_STATE_STAND);
        failWithDelay = true;
    }

    ObjectGuid oldSel = bot->GetSelectedUnit() ? bot->GetSelectedUnit()->GetGUID() : ObjectGuid();
    bot->SetSelection(target->GetGUID());
    WorldObject* faceTo = target;
    //By leewheel 2026-07-20: 施法前始终面向目标
    //1. SetOrientation立即更新服务端数值，确保CheckCast的HasInArc通过
    //2. SetFacingToObject通过移动样条广播给客户端
    if (!bot->HasInArc(CAST_ANGLE_IN_FRONT, faceTo))
    {
        bot->SetOrientation(bot->GetAbsoluteAngle(faceTo));
        ServerFacade::instance().SetFacingTo(bot, faceTo);
    }
    //End By leewheel

    if (failWithDelay)
    {
        SetNextCheckDelay(sPlayerbotAIConfig.reactDelay);
        return false;
    }

    //By leewheel 2026-07-20: playerbot施法统一使用TRIGGERED_IGNORE_TARGET_CHECK
    //TC的CheckExplicitTarget→IsValidAttackTarget→CanSeeOrDetect对playerbot永远false
    //playerbot代码已在AttackAction/CanCastSpell中自行验证目标，跳过CheckExplicitTarget安全
    TriggerCastFlags triggerFlags = TRIGGERED_IGNORE_TARGET_CHECK;
    if (spellInfo->GetEffects()[0].Effect == SPELL_EFFECT_OPEN_LOCK || spellInfo->GetEffects()[0].Effect == SPELL_EFFECT_SKINNING)
        triggerFlags = TriggerCastFlags(TRIGGERED_IGNORE_TARGET_CHECK | TRIGGERED_DONT_REPORT_CAST_ERROR);
    Spell* spell = new Spell(bot, spellInfo, triggerFlags);
    //End By leewheel

    SpellCastTargets targets;
    if (spellInfo->GetEffects()[0].Effect != SPELL_EFFECT_OPEN_LOCK &&
        (spellInfo->Targets & TARGET_FLAG_ITEM || spellInfo->Targets & TARGET_FLAG_GAMEOBJECT_ITEM))
    {
        Item* item = itemTarget ? itemTarget : aiObjectContext->GetValue<Item*>("item for spell", spellId)->Get();
        targets.SetItemTarget(item);

        if (bot->GetTradeData())
        {
            bot->GetTradeData()->SetSpell(spellId);
            delete spell;
            // if (!sPlayerbotAIConfig.logInGroupOnly || (bot->GetGroup() && HasRealPlayerMaster()))
            // {
            //     LOG_DEBUG("playerbots", "Spell cast no item - target name: {}, spellid: {}, bot name: {}",
            //         target->GetName(), spellId, bot->GetName());
            // }
            return true;
        }
    }
    else if (spellInfo->Targets & TARGET_FLAG_DEST_LOCATION)
    {
        // WorldLocation aoe = aiObjectContext->GetValue<WorldLocation>("aoe position")->Get();
        // targets.SetDst(aoe);
        targets.SetDst(*target);
    }
    else if (spellInfo->Targets & TARGET_FLAG_SOURCE_LOCATION)
    {
        targets.SetDst(*bot);
    }
    else
    {
        targets.SetUnitTarget(target);
    }

    if (spellInfo->GetEffects()[0].Effect == SPELL_EFFECT_OPEN_LOCK || spellInfo->GetEffects()[0].Effect == SPELL_EFFECT_SKINNING)
    {
        LootObject loot = *aiObjectContext->GetValue<LootObject>("loot target");
        GameObject* go = GetGameObject(loot.guid);
        //By leewheel 2026-07-18: 诊断 SKINNING/OPEN_LOCK 目标设置问题（result=13 BAD_TARGETS 根因排查）
        //By leewheel 2026-09-03 修复C4189警告：恢复诊断日志(log中文)，diagUnit恢复实际引用
        if (bot->GetGroup())
        {
            Unit* diagUnit = GetUnit(loot.guid);
            TC_LOG_DEBUG("playerbots", "[BotCast] bot=\"{}\" SKINNING/OPEN_LOCK 诊断 spellId={} loot_guid={} loot_empty={} go={} getUnit_typeId={} target_typeId={}",
                bot->GetName(), spellId,
                loot.guid.GetCounter(),
                loot.IsEmpty() ? 1 : 0,
                go ? (go->isSpawned() ? "spawned" : "notspawned") : "null",
                diagUnit ? static_cast<uint32>(diagUnit->GetTypeId()) : 999u,
                static_cast<uint32>(target->GetTypeId()));
        }
        //End By leewheel
        if (go && go->isSpawned())
        {
            //By leewheel 2026-07-11: TC的HandleGameObjectUseOpcode接受GameObjUse&而非WorldPacket
            WorldPacket packetgouse(CMSG_GAME_OBJ_USE, 8);
            packetgouse << loot.guid;
            WorldPackets::GameObject::GameObjUse usePacket(std::move(packetgouse));
            usePacket.Guid = loot.guid;
            bot->GetSession()->HandleGameObjectUseOpcode(usePacket);
            //End By leewheel
            targets.SetGOTarget(go);
            faceTo = go;
        }
        else if (itemTarget)
        {
            Player* trader = bot->GetTrader();
            if (trader)
            {
                targets.SetTradeItemTarget(bot);
                targets.SetUnitTarget(bot);
                faceTo = trader;
            }
            else
            {
                targets.SetItemTarget(itemTarget);
            }
        }
        else
        {
            if (Unit* creature = GetUnit(loot.guid))
            {
                targets.SetUnitTarget(creature);
                faceTo = creature;
            }
        }
        //By leewheel 2026-07-18: 记录最终 targets 状态
        //By leewheel 2026-09-03 修复C4189警告：恢复诊断日志(log中文)，objTarget/unitTarget恢复实际引用
        if (bot->GetGroup())
        {
            WorldObject* objTarget = targets.GetObjectTarget();
            Unit* unitTarget = targets.GetUnitTarget();
            TC_LOG_DEBUG("playerbots", "[BotCast] bot=\"{}\" SKINNING/OPEN_LOCK 目标最终状态 objTarget={} unitTarget={}",
                bot->GetName(),
                objTarget ? static_cast<uint32>(objTarget->GetTypeId()) : 999u,
                unitTarget ? static_cast<uint32>(unitTarget->GetTypeId()) : 999u);
        }
        //End By leewheel
    }

    if (bot->isMoving() && spell->GetCastTime())
    {
        //By leewheel 2026-07-20: 先停下来再施法，而不是取消法术后继续移动
        //原逻辑：cancel+return false→下tick还在移动→又cancel→死循环永远读不出条
        //现逻辑：StopMoving让bot停下→下tick bot静止→读条成功→WaitForSpellCast保护读条
        bot->StopMoving();
        //End By leewheel
        SetNextCheckDelay(sPlayerbotAIConfig.reactDelay);
        spell->cancel();
        delete spell;
        return false;
    }

    // spell->m_targets.SetUnitTarget(target);
    // SpellCastResult spellSuccess = spell->CheckCast(true);
    // if (!sPlayerbotAIConfig.logInGroupOnly || (bot->GetGroup() && HasRealPlayerMaster()))
    // {
    //     LOG_DEBUG("playerbots", "Spell cast result - target name: {}, spellid: {}, bot name: {}, result: {}",
    //         target->GetName(), spellId, bot->GetName(), spellSuccess);
    // }
    // if (spellSuccess != SPELL_CAST_OK)
    //     return false;

    //By leewheel 2026-07-11: TC的prepare接受SpellCastTargets const&而非SpellCastTargets*
    //By leewheel 2026-07-18: SKINNING 法术的 Spell::prepare 虽然返回 OK（因 TRIGGERED_IGNORE_TARGET_CHECK），
    //但 Spell::AddUnitTarget 中的 SpellInfo::CheckTarget 会因 creature 已死亡返回 SPELL_FAILED_TARGETS_DEAD，
    //导致 m_UniqueTargetInfo 为空，EffectSkinning 永远不被调用，creature 状态不变化，bot 陷入死循环。
    //By leewheel 2026-09-09: TC-Cata的Creature无loot成员，SendLoot API变化大
    //暂时禁用剥皮快速路径，TODO: 适配TC-Cata的Loot系统后恢复
    //修复：对 SKINNING 法术绕过 Spell 系统，直接执行 EffectSkinning 的核心逻辑（设置 flag + SendLoot + UpdateGatherSkill）
    if (false && spellInfo->GetEffects()[0].Effect == SPELL_EFFECT_SKINNING)
    // if (spellInfo->GetEffects()[0].Effect == SPELL_EFFECT_SKINNING)
    {
        Creature* skinnedCreature = target->ToCreature();
        if (skinnedCreature && skinnedCreature->HasUnitFlag(UNIT_FLAG_SKINNABLE) &&
            (skinnedCreature->IsCritter() || Creature_IsLooted(skinnedCreature)) &&
            !skinnedCreature->HasUnitFlag3(UNIT_FLAG3_ALREADY_SKINNED))
        {
            uint32 skill = skinnedCreature->GetCreatureDifficulty()->GetRequiredLootSkill();
            int32 targetLevel = skinnedCreature->GetLevelForTarget(bot);
            int32 const reqValue = targetLevel < 10 ? 0 : (targetLevel < 20 ? (targetLevel - 10) * 10 : targetLevel * 5);
            int32 const skillValue = bot->GetPureSkillValue(skill);

            skinnedCreature->SetUnitFlag3(UNIT_FLAG3_ALREADY_SKINNED);
            skinnedCreature->SetDynamicFlag(UNIT_DYNFLAG_LOOTABLE);
            // TODO: TC-Cata的SendLoot需要Loot&参数，需先生成剥皮loot
            // bot->SendLoot(*skinnedCreature->GetLoot());
            bot->UpdateGatherSkill(skill, skillValue, reqValue, skinnedCreature->IsElite() ? 2 : 1);

            //By leewheel 2026-07-18: 施法成功日志
            // if (bot->GetGroup())
            // {
            //     TC_LOG_INFO("playerbots", "[BotCast] bot=\"{}\" 施法成功 spell=\"{}\" id={} target=\"{}\" (直接执行EffectSkinning)",
            //         bot->GetName(), chatHelper->FormatSpell(spellInfo), spellId, target->GetName());
            // }
            //End By leewheel

            aiObjectContext->GetValue<LastSpellCast&>("last spell cast")->Get().Set(spellId, target->GetGUID(), time(nullptr));
            aiObjectContext->GetValue<PositionMap&>("position")->Get()["random"].Reset();
            if (oldSel)
                bot->SetSelection(oldSel);
            delete spell;
            return true;
        }
        //条件不满足时走正常失败路径
        // if (bot->GetGroup())
        // {
        //     TC_LOG_INFO("playerbots", "[BotCast] bot=\"{}\" 施法失败 spell=\"{}\" id={} result=BAD_TARGETS target=\"{}\" (Skinning条件不满足)",
        //         bot->GetName(), chatHelper->FormatSpell(spellInfo), spellId, target->GetName());
        // }
        delete spell;
        return false;
    }
    //End By leewheel

    SpellCastResult result = spell->prepare(targets);

    if (result != SPELL_CAST_OK)
    {
        //By leewheel 2026-07-18: 施法失败日志 - 仅组队/团队时输出，含 result code
        // if (bot->GetGroup())
        // {
        //     TC_LOG_INFO("playerbots", "[BotCast] bot=\"{}\" 施法失败 spell=\"{}\" id={} result={} target=\"{}\"",
        //         bot->GetName(), chatHelper->FormatSpell(spellInfo), spellId,
        //         static_cast<uint32>(result), target->GetName());
        // }
        //End By leewheel
        if (HasStrategy("debug spell", BOT_STATE_NON_COMBAT))
        {
            std::ostringstream out;
            //By leewheel 2026-08-01: 玩家可见文本中文化
            out << "施法失败 - ";
            out << "法术ID: " << spellId << " (" << ChatHelper::FormatSpell(spellInfo) << "), ";
            out << "错误代码: " << static_cast<int>(result) << " (0x" << std::hex << static_cast<int>(result)
                << std::dec << "), ";
            out << "机器人: " << bot->GetName() << ", ";

            // Check spell target type
            if (targets.GetUnitTarget())
            {
                out << "目标: 单位 (" << targets.GetUnitTarget()->GetName()
                    << ", 低GUID: " << targets.GetUnitTarget()->GetGUID().GetCounter()
                    << ", 高GUID: " << static_cast<uint32>(targets.GetUnitTarget()->GetGUID().GetHigh()) << "), ";
            }

            if (targets.GetGOTarget())
            {
                out << "目标: 游戏对象 (低GUID: " << targets.GetGOTarget()->GetGUID().GetCounter()
                    << ", 高GUID: " << static_cast<uint32>(targets.GetGOTarget()->GetGUID().GetHigh()) << "), ";
            }

            if (targets.GetItemTarget())
            {
                out << "目标: 物品 (低GUID: " << targets.GetItemTarget()->GetGUID().GetCounter()
                    << ", 高GUID: " << static_cast<uint32>(targets.GetItemTarget()->GetGUID().GetHigh()) << "), ";
            }

            // Check if bot is in trade mode
            if (bot->GetTradeData())
            {
                out << "交易模式: 进行中, ";
                Item* tradeItem = bot->GetTradeData()->GetTraderData()->GetItem(TRADE_SLOT_NONTRADED);
                if (tradeItem)
                {
                    out << "交易物品: " << tradeItem->GetEntry()
                        << " (低GUID: " << tradeItem->GetGUID().GetCounter()
                        << ", 高GUID: " << static_cast<uint32>(tradeItem->GetGUID().GetHigh()) << "), ";
                }
                else
                {
                    out << "交易物品: 无, ";
                }
            }
            else
            {
                out << "交易模式: 未进行, ";
            }
            //End By leewheel

            TellMasterNoFacing(out);
        }

        return false;
    }
    // if (spellInfo->GetEffects()[0].Effect == SPELL_EFFECT_OPEN_LOCK || spellInfo->GetEffects()[0].Effect ==
    // SPELL_EFFECT_SKINNING)
    // {
    //     LootObject loot = *aiObjectContext->GetValue<LootObject>("loot target");
    //     if (!loot.IsLootPossible(bot))
    //     {
    //         spell->cancel();
    //         delete spell;
    //         if (!sPlayerbotAIConfig.logInGroupOnly || (bot->GetGroup() && HasRealPlayerMaster()))
    //         {
    //             LOG_DEBUG("playerbots", "Spell cast loot - target name: {}, spellid: {}, bot name: {}",
    //                 target->GetName(), spellId, bot->GetName());
    //         }
    //         return false;
    //     }
    // }

    //By leewheel 2026-07-20: 恢复WaitForSpellCast，读条期间冻结AI tick防止移动打断
    WaitForSpellCast(spell);
    //End By leewheel

    aiObjectContext->GetValue<LastSpellCast&>("last spell cast")->Get().Set(spellId, target->GetGUID(), time(nullptr));

    //By leewheel 2026-07-18: 施法成功日志 - 仅组队/团队时输出
    // if (bot->GetGroup())
    // {
    //     TC_LOG_INFO("playerbots", "[BotCast] bot=\"{}\" 施法成功 spell=\"{}\" id={} target=\"{}\"",
    //         bot->GetName(), chatHelper->FormatSpell(spellInfo), spellId, target->GetName());
    //     if (spellInfo->GetEffects()[0].Effect == SPELL_EFFECT_SKINNING)
    //     {
    //         if (Creature* c = target->ToCreature())
    //         {
    //             TC_LOG_INFO("playerbots", "[BotCast] bot=\"{}\" SKINNING prepare后检查 creature guid={} already_skinned={} lootable={}",
    //                 bot->GetName(),
    //                 c->GetGUID().GetCounter(),
    //                 c->HasUnitFlag3(UNIT_FLAG3_ALREADY_SKINNED) ? 1 : 0,
    //                 c->HasDynamicFlag(UNIT_DYNFLAG_LOOTABLE) ? 1 : 0);
    //         }
    //     }
    // }

    aiObjectContext->GetValue<PositionMap&>("position")->Get()["random"].Reset();

    if (oldSel)
        bot->SetSelection(oldSel);

    if (HasStrategy("debug spell", BOT_STATE_NON_COMBAT))
    {
        std::ostringstream out;
        //By leewheel 2026-08-01: 玩家可见文本中文化
        out << "正在施放 " << ChatHelper::FormatSpell(spellInfo);
        //End By leewheel
        TellMasterNoFacing(out);
    }

    forceRebuff.NoteCast(spellInfo);

    return true;
}

bool PlayerbotAI::CastSpell(uint32 spellId, float x, float y, float z, Item* itemTarget)
{
    if (!spellId)
        return false;

    Pet* pet = bot->GetPet();
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);
    if (pet && pet->HasSpell(spellId))
    {
        bool autocast = false;
        for (unsigned int& m_autospell : pet->m_autospells)
        {
            if (m_autospell == spellId)
            {
                autocast = true;
                break;
            }
        }

        pet->ToggleAutocast(spellInfo, !autocast);
        std::ostringstream out;
        //By leewheel 2026-08-01: 玩家可见文本中文化
        out << (autocast ? "|cffff0000|禁用" : "|cFF00ff00|启用") << " 宠物自动施法: ";
        //End By leewheel
        out << chatHelper->FormatSpell(spellInfo);
        TellMaster(out);
        return true;
    }

    // aiObjectContext->GetValue<LastMovement&>("last movement")->Get().Set(nullptr);
    // aiObjectContext->GetValue<time_t>("stay time")->Set(0);

    // MotionMaster& mm = *bot->GetMotionMaster();

    //By leewheel 2026-08-16: 对齐the-lab 8e3c12b(#2639)——地面目标施法同样豁免重力吸引期间(见上方unit-target版注释)
    if ((bot->IsFlying() && !bot->HasAura(SPELL_GRAVITY_LAPSE_TK) && !bot->HasAura(SPELL_GRAVITY_LAPSE_MGT)) ||
        bot->HasUnitState(UNIT_STATE_IN_FLIGHT))
    {
        return false;
    }
    //End By leewheel

    // bot->ClearUnitState(UNIT_STATE_CHASE);
    // bot->ClearUnitState(UNIT_STATE_FOLLOW);

    bool failWithDelay = false;
    if (!bot->IsStandState())
    {
        bot->SetStandState(UNIT_STAND_STATE_STAND);
        failWithDelay = true;
    }

    ObjectGuid oldSel = bot->GetSelectedUnit() ? bot->GetSelectedUnit()->GetGUID() : ObjectGuid();

    if (!bot->isMoving())
        bot->SetFacingTo(bot->GetAngle(x, y));

    if (failWithDelay)
    {
        SetNextCheckDelay(sPlayerbotAIConfig.globalCoolDown);
        return false;
    }

    //By leewheel 2026-07-20: playerbot施法跳过CheckExplicitTarget（CanSeeOrDetect不可靠）
    Spell* spell = new Spell(bot, spellInfo, TRIGGERED_IGNORE_TARGET_CHECK);
    //End By leewheel

    SpellCastTargets targets;
    if (spellInfo->Targets & TARGET_FLAG_ITEM || spellInfo->Targets & TARGET_FLAG_GAMEOBJECT_ITEM)
    {
        Item* item = itemTarget ? itemTarget : aiObjectContext->GetValue<Item*>("item for spell", spellId)->Get();
        targets.SetItemTarget(item);

        if (bot->GetTradeData())
        {
            bot->GetTradeData()->SetSpell(spellId);
            delete spell;
            return true;
        }
    }
    else if (spellInfo->Targets & TARGET_FLAG_DEST_LOCATION)
    {
        // WorldLocation aoe = aiObjectContext->GetValue<WorldLocation>("aoe position")->Get();
        targets.SetDst(x, y, z, 0.f);
    }
    else if (spellInfo->Targets & TARGET_FLAG_SOURCE_LOCATION)
    {
        targets.SetDst(bot->GetPositionX(), bot->GetPositionY(), bot->GetPositionZ(), 0.f);
    }
    else
    {
        return false;
    }

    if (spellInfo->GetEffects()[0].Effect == SPELL_EFFECT_OPEN_LOCK || spellInfo->GetEffects()[0].Effect == SPELL_EFFECT_SKINNING)
    {
        return false;
    }

    //By leewheel 2026-07-11: TC的prepare接受SpellCastTargets const&而非指针
    spell->prepare(targets);
    //End By leewheel

    if (bot->isMoving() && spell->GetCastTime())
    {
        //By leewheel 2026-07-20: 先停下来再施法，而不是取消法术后继续移动
        //原逻辑：cancel+return false→下tick还在移动→又cancel→死循环永远读不出条
        //现逻辑：StopMoving让bot停下→下tick bot静止→读条成功→WaitForSpellCast保护读条
        bot->StopMoving();
        //End By leewheel
        SetNextCheckDelay(sPlayerbotAIConfig.reactDelay);
        spell->cancel();
        delete spell;
        return false;
    }

    if (spellInfo->GetEffects()[0].Effect == SPELL_EFFECT_OPEN_LOCK || spellInfo->GetEffects()[0].Effect == SPELL_EFFECT_SKINNING)
    {
        LootObject loot = *aiObjectContext->GetValue<LootObject>("loot target");
        if (!loot.IsLootPossible(bot))
        {
            spell->cancel();
            delete spell;
            return false;
        }
    }

    //By leewheel 2026-07-20: 恢复WaitForSpellCast，读条期间冻结AI tick防止移动打断
    WaitForSpellCast(spell);
    //End By leewheel
    aiObjectContext->GetValue<LastSpellCast&>("last spell cast")->Get().Set(spellId, bot->GetGUID(), time(nullptr));
    aiObjectContext->GetValue<PositionMap&>("position")->Get()["random"].Reset();

    if (oldSel)
        bot->SetSelection(oldSel);

    if (HasStrategy("debug spell", BOT_STATE_NON_COMBAT))
    {
        std::ostringstream out;
        //By leewheel 2026-08-01: 玩家可见文本中文化
        out << "正在施放 " << ChatHelper::FormatSpell(spellInfo);
        //End By leewheel
        TellMasterNoFacing(out);
    }

    return true;
}

bool PlayerbotAI::CanCastVehicleSpell(uint32 spellId, Unit* target)
{
    if (!spellId)
        return false;

    if (!IsValidUnit(target))
        return false;

    Vehicle* vehicle = bot->GetVehicle();
    if (!vehicle)
        return false;

    // do not allow if no spells
    VehicleSeatEntry const* seat = vehicle->GetSeatForPassenger(bot);
    if (!seat || !(seat->Flags & VEHICLE_SEAT_FLAG_CAN_CAST))
        return false;

    Unit* vehicleBase = vehicle->GetBase();
    Unit* spellTarget = target;

    if (!spellTarget)
        spellTarget = vehicleBase;

    if (!IsValidUnit(spellTarget))
        return false;

    if (vehicleBase->HasSpellCooldown(spellId))
        return false;

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);
    if (!spellInfo)
        return false;

    // check BG siege position set in BG Tactics
    PositionInfo siegePos = GetAiObjectContext()->GetValue<PositionMap&>("position")->Get()["bg siege"];

    // do not cast spell on self if spell is location based
    if (!(siegePos.isSet() || spellTarget != vehicleBase) && spellInfo->Targets & TARGET_FLAG_DEST_LOCATION)
        return false;

    //By leewheel 2026-07-11: TC的CalcCastTime接受Spell*而非Unit*，传nullptr
    uint32 CastingTime = !spellInfo->IsChanneled() ? spellInfo->CalcCastTime(nullptr) : spellInfo->GetDuration();
    //End By leewheel
    if (CastingTime && vehicleBase->isMoving())
        return false;

    if (vehicleBase != spellTarget && ServerFacade::instance().GetDistance2d(vehicleBase, spellTarget) > 120.0f)
        return false;

    if (!target && siegePos.isSet())
    {
        if (ServerFacade::instance().GetDistance2d(vehicleBase, siegePos.x, siegePos.y) > 120.0f)
            return false;
    }

    Spell* spell = new Spell(vehicleBase, spellInfo, TRIGGERED_NONE);

    WorldLocation dest;
    if (siegePos.isSet())
        dest = WorldLocation(bot->GetMapId(), siegePos.x, siegePos.y, siegePos.z, 0);
    else if (spellTarget != vehicleBase)
        dest = WorldLocation(spellTarget->GetMapId(), spellTarget->GetPosition());

    if (spellInfo->Targets & TARGET_FLAG_DEST_LOCATION)
        spell->m_targets.SetDst(dest);
    else if (spellTarget != vehicleBase)
        spell->m_targets.SetUnitTarget(spellTarget);

    SpellCastResult result = spell->CheckCast(true);
    delete spell;

    switch (result)
    {
        case SPELL_FAILED_NOT_INFRONT:
        case SPELL_FAILED_NOT_STANDING:
        case SPELL_FAILED_UNIT_NOT_INFRONT:
        case SPELL_FAILED_MOVING:
        case SPELL_FAILED_TRY_AGAIN:
        case SPELL_CAST_OK:
        //By leewheel 2026-07-20: CanSeeOrDetect对playerbot不可靠
        case SPELL_FAILED_BAD_TARGETS:
        case SPELL_FAILED_BAD_IMPLICIT_TARGETS:
        //End By leewheel
            return true;
        default:
            return false;
    }

    return false;
}

bool PlayerbotAI::CastVehicleSpell(uint32 spellId, Unit* target)
{
    if (!spellId)
        return false;

    if (!IsValidUnit(target))
        return false;

    Vehicle* vehicle = bot->GetVehicle();
    if (!vehicle)
        return false;

    // do not allow if no spells
    VehicleSeatEntry const* seat = vehicle->GetSeatForPassenger(bot);
    if (!seat || !(seat->Flags & VEHICLE_SEAT_FLAG_CAN_CAST))
        return false;

    Unit* vehicleBase = vehicle->GetBase();
    Unit* spellTarget = target;

    if (!spellTarget)
        spellTarget = vehicleBase;

    if (!IsValidUnit(spellTarget))
        return false;

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);
    if (!spellInfo)
        return false;

    // check BG siege position set in BG Tactics
    PositionInfo siegePos = GetAiObjectContext()->GetValue<PositionMap&>("position")->Get()["bg siege"];
    if (!target && siegePos.isSet())
    {
        if (ServerFacade::instance().GetDistance2d(vehicleBase, siegePos.x, siegePos.y) > 120.0f)
            return false;
    }

    // do not cast spell on self if spell is location based
    if (!(siegePos.isSet() || spellTarget != vehicleBase) && (spellInfo->Targets & TARGET_FLAG_DEST_LOCATION) != 0)
        return false;

    if (seat->CanControl())
    {
        // aiObjectContext->GetValue<LastMovement&>("last movement")->Get().Set(nullptr);
        // aiObjectContext->GetValue<time_t>("stay time")->Set(0);
    }

    // bot->clearUnitState(UNIT_STAT_CHASE);
    // bot->clearUnitState(UNIT_STAT_FOLLOW);

    // ObjectGuid oldSel = bot->GetSelectionGuid();
    // bot->SetSelectionGuid(target->GetGUID());

    // turn vehicle if target is not in front
    bool failWithDelay = false;
    if (spellTarget != vehicleBase && (seat->CanControl() || (seat->Flags & VEHICLE_SEAT_FLAG_ALLOW_TURNING)))
    {
        if (!vehicleBase->HasInArc(CAST_ANGLE_IN_FRONT, spellTarget, 100.0f))
        {
            vehicleBase->SetFacingToObject(spellTarget);
            failWithDelay = true;
        }
    }

    if (siegePos.isSet() && (seat->CanControl() || (seat->Flags & VEHICLE_SEAT_FLAG_ALLOW_TURNING)))
    {
        vehicleBase->SetFacingTo(vehicleBase->GetAngle(siegePos.x, siegePos.y));
    }

    if (failWithDelay)
    {
        SetNextCheckDelay(sPlayerbotAIConfig.reactDelay);
        return false;
    }

    //By leewheel 2026-07-20: playerbot施法跳过CheckExplicitTarget（CanSeeOrDetect不可靠）
    Spell* spell = new Spell(vehicleBase, spellInfo, TRIGGERED_IGNORE_TARGET_CHECK);
    //End By leewheel

    SpellCastTargets targets;
    if ((spellTarget != vehicleBase || siegePos.isSet()) && (spellInfo->Targets & TARGET_FLAG_DEST_LOCATION))
    {
        WorldLocation dest;
        if (spellTarget != vehicleBase)
            dest = WorldLocation(spellTarget->GetMapId(), spellTarget->GetPosition());
        else if (siegePos.isSet())
            dest = WorldLocation(bot->GetMapId(), siegePos.x + frand(-5.0f, 5.0f), siegePos.y + frand(-5.0f, 5.0f),
                                 siegePos.z, 0.0f);
        else
            return false;

        targets.SetDst(dest);
        targets.SetSpeed(30.0f);
        float dist = vehicleBase->GetPosition().GetExactDist(dest);
        // very much an approximation of the real projectile arc
        float elev = dist >= 110.0f ? 1.0f : pow(((dist + 10.0f) / 120.0f), 2.0f);
        //By leewheel 2026-07-11: TC的SetElevation改为SetPitch
        targets.SetPitch(elev);
        //End By leewheel
    }

    if (spellInfo->Targets & TARGET_FLAG_SOURCE_LOCATION)
    {
        targets.SetSrc(vehicleBase->GetPositionX(), vehicleBase->GetPositionY(), vehicleBase->GetPositionZ());
    }

    if (target && !(spellInfo->Targets & TARGET_FLAG_DEST_LOCATION))
    {
        targets.SetUnitTarget(spellTarget);
    }

    //By leewheel 2026-07-11: TC的prepare接受SpellCastTargets const&而非指针
    spell->prepare(targets);
    //End By leewheel

    if (seat->CanControl() && vehicleBase->isMoving() && spell->GetCastTime())
    {
        vehicleBase->StopMoving();
        SetNextCheckDelay(sPlayerbotAIConfig.globalCoolDown);
        spell->cancel();
        // delete spell;
        return false;
    }

    //By leewheel 2026-07-20: 恢复WaitForSpellCast，读条期间冻结AI tick防止移动打断
    WaitForSpellCast(spell);
    //End By leewheel

    // aiObjectContext->GetValue<LastSpellCast&>("last spell cast")->Get().Set(spellId, target->GetGUID(), time(0));
    // aiObjectContext->GetValue<botAI::PositionMap&>("position")->Get()["random"].Reset();

    if (HasStrategy("debug spell", BOT_STATE_NON_COMBAT))
    {
        std::ostringstream out;
        //By leewheel 2026-08-01: 玩家可见文本中文化
        out << "正在施放载具技能" << ChatHelper::FormatSpell(spellInfo);
        //End By leewheel
        TellMasterNoFacing(out);
    }

    return true;
}

bool PlayerbotAI::IsInVehicle(bool canControl, bool canCast, bool canAttack, bool canTurn, bool fixed)
{
    Vehicle* vehicle = bot->GetVehicle();
    if (!vehicle)
        return false;

    // get vehicle
    Unit* vehicleBase = vehicle->GetBase();
    if (!vehicleBase || !vehicleBase->IsAlive())
        return false;

    if (!vehicle->GetVehicleInfo())
        return false;

    // get seat
    VehicleSeatEntry const* seat = vehicle->GetSeatForPassenger(bot);
    if (!seat)
        return false;

    if (!(canControl || canCast || canAttack || canTurn || fixed))
        return true;

    if (canControl)
        return seat->CanControl() && !(vehicle->GetVehicleInfo()->Flags & VEHICLE_FLAG_FIXED_POSITION);

    if (canCast)
        return (seat->Flags & VEHICLE_SEAT_FLAG_CAN_CAST) != 0;

    if (canAttack)
        return (seat->Flags & VEHICLE_SEAT_FLAG_CAN_ATTACK) != 0;

    if (canTurn)
        return (seat->Flags & VEHICLE_SEAT_FLAG_ALLOW_TURNING) != 0;

    if (fixed)
        return (vehicle->GetVehicleInfo()->Flags & VEHICLE_FLAG_FIXED_POSITION) != 0;

    return false;
}

void PlayerbotAI::WaitForSpellCast(Spell* spell)
{
    if (!spell)
        return;

    SpellInfo const* spellInfo = spell->GetSpellInfo();
    uint32 castTime = spell->GetCastTime();

    if (spellInfo && spellInfo->IsChanneled())
    {
        int32 duration = spellInfo->GetDuration();
        //By leewheel 2026-07-11: TC ApplySpellMod接受SpellInfo*而非spellId
        bot->ApplySpellMod(spellInfo, SpellModOp::Duration, duration);
        //End By leewheel
        if (duration > 0)
            castTime += duration;
    }

    SetNextCheckDelay(castTime + sPlayerbotAIConfig.reactDelay);
}

void PlayerbotAI::RemoveAura(std::string const name)
{
    uint32 spellid = aiObjectContext->GetValue<uint32>("spell id", name)->Get();
    if (spellid && bot->HasAura(spellid))
        bot->RemoveAurasDueToSpell(spellid);
}

void PlayerbotAI::RequestSpellInterrupt()
{
    Spell* currentSpell = bot->GetCurrentSpell(CURRENT_GENERIC_SPELL);
    if (currentSpell && currentSpell->getState() == SPELL_STATE_PREPARING)
    {
        spellInterruptRequested = true;
        return;
    }

    if (bot->GetCurrentSpell(CURRENT_CHANNELED_SPELL))
        spellInterruptRequested = true;
}

bool PlayerbotAI::IsInterruptableSpellCasting(Unit* target, std::string const spell)
{
    if (!IsValidUnit(target))
        return false;

    uint32 spellid = aiObjectContext->GetValue<uint32>("spell id", spell)->Get();
    if (!spellid || !target->IsNonMeleeSpellCast(true))
        return false;

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellid);
    if (!spellInfo)
        return false;

    for (uint8 i = EFFECT_0; i <= EFFECT_2; i++)
    {
        //By leewheel 2026-07-11: TC使用SpellInterruptFlags枚举
        if (spellInfo->InterruptFlags != SpellInterruptFlags::None &&
        //End By leewheel
            spellInfo->PreventionType == SPELL_PREVENTION_TYPE_SILENCE)
            return true;

        if (spellInfo->GetEffects()[i].Effect == SPELL_EFFECT_INTERRUPT_CAST &&
        //By leewheel 2026-07-11: TC的IsImmunedToSpellEffect第二个参数是SpellEffectInfo const&而非uint8
            !target->IsImmunedToSpellEffect(spellInfo, spellInfo->GetEffects()[i], bot))
        //End By leewheel
            return true;

        if ((spellInfo->GetEffects()[i].Effect == SPELL_EFFECT_APPLY_AURA) &&
            spellInfo->GetEffects()[i].ApplyAuraName == SPELL_AURA_MOD_SILENCE)
            return true;
    }

    return false;
}

bool PlayerbotAI::HasAuraToDispel(Unit* target, uint32 dispelType)
{
    if (!IsValidUnit(target) || !target->IsAlive())
        return false;

    if (!IsValidPlayer(bot))
        return false;

    bool isFriend = bot->IsFriendlyTo(target);

    //By leewheel 2026-07-11: TC使用VisibleAuraContainer(是set不是map)
    Unit::VisibleAuraContainer const& visibleAuras = target->GetVisibleAuras();
    //End By leewheel

    for (Unit::VisibleAuraContainer::const_iterator itr = visibleAuras.begin(); itr != visibleAuras.end(); ++itr)
    {
        //By leewheel 2026-07-11: TC set迭代器直接解引用为AuraApplication*
        AuraApplication* aurApp = *itr;
        if (!aurApp)
        //End By leewheel
            continue;

        Aura* aura = aurApp->GetBase();
        if (!aura || aura->IsPassive() || aura->IsRemoved())
            continue;

        if (sPlayerbotAIConfig.dispelAuraDuration && aura->GetDuration() &&
            aura->GetDuration() < (int32)sPlayerbotAIConfig.dispelAuraDuration)
            continue;

        SpellInfo const* spellInfo = aura->GetSpellInfo();
        if (!spellInfo)
            continue;

        bool isPositiveSpell = spellInfo->IsPositive();
        if (isPositiveSpell && isFriend)
            continue;

        if (!isPositiveSpell && !isFriend)
            continue;

        if (canDispel(spellInfo, dispelType))
            return true;
    }

    return false;
}

#ifndef WIN32
inline int strcmpi(char const* s1, char const* s2)
{
    for (; *s1 && *s2 && (toupper(*s1) == toupper(*s2)); ++s1, ++s2)
    {
    }
    return *s1 - *s2;
}
#endif

bool PlayerbotAI::canDispel(SpellInfo const* spellInfo, uint32 dispelType)
{
    if (spellInfo->Dispel != dispelType)
        return false;

//By leewheel 2026-07-13: 使用多locale辅助函数
if (!spellInfo->SpellName)
//End By leewheel
{
return true;
}

//By leewheel 2026-07-13: 获取最佳locale名称用于比较
std::string const spellNameStr = GetSpellNameBestLocaleWithCache(spellInfo->Id, spellInfo->SpellName);
//End By leewheel

for (std::string& wl : dispel_whitelist)
{
//By leewheel 2026-07-13: 使用最佳locale名称比较
if (strcmpi(spellNameStr.c_str(), wl.c_str()) == 0)
//End By leewheel
{
return false;
}
}

//By leewheel 2026-07-13: 使用最佳locale名称比较
//By leewheel 2026-09-04: 对齐上游2a417e14——删除"mana tap"引用(Mana Tap动作链已整体移除,名单残留清理)
return spellNameStr.empty() || (strcmpi(spellNameStr.c_str(), "demon skin") &&
strcmpi(spellNameStr.c_str(), "mage armor") &&
strcmpi(spellNameStr.c_str(), "frost armor") &&
strcmpi(spellNameStr.c_str(), "wavering will") &&
strcmpi(spellNameStr.c_str(), "chilled") &&
strcmpi(spellNameStr.c_str(), "ice armor"));
//End By leewheel
}

//By leewheel 2026-08-14: 移植brighton-chi the-lab——IsSelfBot判定(自身操控的bot)
bool IsSelfBot(Player* player)
{
    // Selfbot: "player"有PlayerbotAI且其master就是自己
    PlayerbotAI* botAI = GET_PLAYERBOT_AI(player);
    return botAI && botAI->GetMaster() == player;
}
//End By leewheel
//By leewheel 2026-08-24: 对齐master(17214b325)——IsRealPlayer(Player*)全局判定(无PlayerbotAI=真人手动操控)
bool IsRealPlayer(Player* player)
{
    // "player"空指针检查必须有, 否则 GET_PLAYERBOT_AI(nullptr) 会被误判为真人
    return player && !GET_PLAYERBOT_AI(player);
}
//End By leewheel

bool IsAlliance(uint8 race)
{
    return race == RACE_HUMAN || race == RACE_DWARF || race == RACE_NIGHTELF || race == RACE_GNOME ||
           race == RACE_DRAENEI;
}

Player* PlayerbotAI::FindNewMaster()
{
    // Ideally we want to have the leader as master.
    Group* group = bot->GetGroup();
    // Only allow real players as masters unless in battleground.
    if (!group)
        return nullptr;

    Player* groupLeader = GetGroupLeader();
    PlayerbotAI* leaderBotAI = GET_PLAYERBOT_AI(groupLeader);
    if (!leaderBotAI || leaderBotAI->IsRealPlayer())
        return groupLeader;

    // Find the real player in group
    for (GroupReference* gref = group->GetFirstMember(); gref; gref = gref->next())
    {
        Player* member = gref->GetSource();
        if (!member || member == bot || !member->IsInWorld() || !member->IsInSameRaidWith(bot))
            continue;

        PlayerbotAI* memberBotAI = GET_PLAYERBOT_AI(member);
        if ((!memberBotAI || memberBotAI->IsRealPlayer()) && !bot->InBattleground())
            return member;

        if (bot->InBattleground() && bot->GetBattleground() &&
            bot->GetBattleground()->GetBgTypeID() == BATTLEGROUND_AV && !GET_PLAYERBOT_AI(member) &&
            member->InBattleground() && bot->GetMapId() == member->GetMapId())
        {
            // Skip if same BG but same subgroup or lower level
            if (!group->SameSubGroup(bot, member) || member->GetLevel() < bot->GetLevel())
                continue;

            // Follow real player only if higher honor points
            uint32 honorpts = member->GetHonorPoints();
            if (bot->GetHonorPoints() && honorpts < bot->GetHonorPoints())
                continue;

            return member;
        }
    }
    return nullptr;
}

bool PlayerbotAI::HasRealPlayerMaster()
{
    if (master)
    {
        PlayerbotAI* masterBotAI = GET_PLAYERBOT_AI(master);
        return !masterBotAI || masterBotAI->IsRealPlayer();
    }

    return false;
}

bool PlayerbotAI::HasActivePlayerMaster() { return master && !GET_PLAYERBOT_AI(master); }

//By leewheel 2026-08-01: 按上游(4fb82ed0)统一命名约定，IsAlt改为IsAltBot
//By leewheel 2026-08-15: 按 the-lab 补 !IsSelfBot 排除——玩家用bot AI操控自己的角色(selfbot,
//master==bot)时不应被误判为alt bot，否则下游(AcceptQuest/DropQuest/GuildTriggers/LeaveGroup/
//PassLeadership/Trainer等)会错误套用alt-bot专属逻辑
bool PlayerbotAI::IsAltBot() { return HasRealPlayerMaster() && !sRandomPlayerbotMgr.IsRandomBot(bot) && !IsSelfBot(bot); }
//End By leewheel
//End By leewheel

Player* PlayerbotAI::GetGroupLeader()
{
    if (!bot->InBattleground())
        if (Group* group = bot->GetGroup())
            if (Player* player = ObjectAccessor::FindPlayer(group->GetLeaderGUID()))
                return player;

    return master;
}

uint32 PlayerbotAI::GetFixedBotNumber(uint32 maxNum)
{
    if (maxNum == 0)
        return 0;

    // Deterministic pseudo-random hash based on the bot GUID evenly distributed across active slots
    uint32 id = bot->GetGUID().GetCounter();
    uint32 h = id;
    h ^= h >> 16;
    h *= 0x7feb352d;
    h ^= h >> 15;
    h *= 0x846ca68b;
    h ^= h >> 16;

    // Current time slot
    uint32 timeSlot = (getMSTime() / 1000) / sPlayerbotAIConfig.BotActiveAloneDurationSeconds;

    // Mix timeSlot into the hash to reshuffle every rotation window
    uint32 mixed = h ^ (timeSlot * 0x9e3779b9);  // with multiplicative constant

    return mixed % maxNum;
}

/*
enum GrouperType
{
    SOLO = 0,
    MEMBER = 1,
    LEADER_2 = 2,
    LEADER_3 = 3,
    LEADER_4 = 4,
    LEADER_5 = 5
};
*/

GrouperType PlayerbotAI::GetGrouperType()
{
    uint32 grouperNumber = GetFixedBotNumber(100);

    if (grouperNumber < 20 && !HasRealPlayerMaster())
        return GrouperType::SOLO;

    if (grouperNumber < 80)
        return GrouperType::MEMBER;

    if (grouperNumber < 85)
        return GrouperType::LEADER_2;

    if (grouperNumber < 90)
        return GrouperType::LEADER_3;

    if (grouperNumber < 95)
        return GrouperType::LEADER_4;

    return GrouperType::LEADER_5;
}

GuilderType PlayerbotAI::GetGuilderType()
{
    uint32 grouperNumber = GetFixedBotNumber(100);

    if (grouperNumber < 20 && !HasRealPlayerMaster())
        return GuilderType::SOLO;

    if (grouperNumber < 30)
        return GuilderType::TINY;

    if (grouperNumber < 40)
        return GuilderType::SMALL;

    if (grouperNumber < 60)
        return GuilderType::MEDIUM;

    if (grouperNumber < 80)
        return GuilderType::LARGE;

    return GuilderType::VERY_LARGE;
}

bool PlayerbotAI::HasPlayerNearby(WorldPosition* pos, float range)
{
    float sqRange = range * range;
    for (auto& player : sRandomPlayerbotMgr.GetPlayers())
    {
        if (!player->IsGameMaster() || player->isGMVisible())
        {
            if (player->GetMapId() != bot->GetMapId())
                continue;

            if (pos->sqDistance(WorldPosition(player)) < sqRange)
                return true;

            WorldObject* viewObj = player->GetViewpoint();
            if (viewObj && viewObj != player)
            {
                if (pos->sqDistance(WorldPosition(viewObj)) < sqRange)
                    return true;
            }
        }
    }

    return false;
}

bool PlayerbotAI::HasPlayerNearby(float range)
{
    WorldPosition botPos(bot);
    return HasPlayerNearby(&botPos, range);
};

bool PlayerbotAI::AllowActive(ActivityType activityType)
{
    // bot is in an invalid state, not safe to process
    if (!bot || !bot->GetSession() || !bot->IsInWorld() || bot->IsBeingTeleported() ||
        bot->GetSession()->isLogingOut() || bot->IsDuringRemoveFromWorld())
        return false;

    // always allow packet handling (e.g. group invites, trade, loot, friend requests etc)
    if (activityType == PACKET_ACTIVITY)
        return true;

    // all bots forced active, no rotation or scaling needed
    if (sPlayerbotAIConfig.botActiveAlone >= 100 && !sPlayerbotAIConfig.botActiveAloneSmartScale)
        return true;

    // bot is in combat, always defend yourself
    if (activityType != OUT_OF_PARTY_ACTIVITY && activityType != PACKET_ACTIVITY)
    {
        if (bot->IsInCombat())
            return true;
    }

    // bot is inside a BG, dungeon, or raid �?always active
    if (!WorldPosition(bot).isOverworld())
        return true;

    // bot is waiting in a BG queue �?stay active to speed up join
    if (bot->InBattlegroundQueue())
        return true;

    // bot is in a guild that contains a real player
    if (sPlayerbotAIConfig.BotActiveAloneForceWhenInGuild)
    {
        if (IsInRealGuild())  // checks cache list
            return true;
    }

    // a real player is in the same zone (e.g. Elwynn Forest), same continent or within configured yard radius
    // combined into a single loop to multiple iterations since this function is called so often
    bool checkMap = sPlayerbotAIConfig.BotActiveAloneForceWhenInMap;
    bool checkZone = sPlayerbotAIConfig.BotActiveAloneForceWhenInZone;
    bool checkRadius = sPlayerbotAIConfig.BotActiveAloneForceWhenInRadius > 0;
    if (checkMap || checkZone || checkRadius)
    {
        uint32 botMapId = bot->GetMapId();
        uint32 botZoneId = checkZone ? bot->GetZoneId() : 0;
        float sqRange = 0.0f;
        WorldPosition botPos(bot);
        if (checkRadius)
        {
            float range = static_cast<float>(sPlayerbotAIConfig.BotActiveAloneForceWhenInRadius);
            sqRange = range * range;
        }

        for (auto& player : sRandomPlayerbotMgr.GetPlayers())
        {
            if (!player || player->GetMapId() != botMapId)
                continue;

            bool isGM = player->IsGameMaster();

            // map check
            if (checkMap && !(isGM && !player->IsVisible()))
                return true;

            // zone check
            if (checkZone && !(isGM && !player->IsVisible()) && player->GetZoneId() == botZoneId)
                return true;

            // radius check
            if (checkRadius && (!isGM || player->isGMVisible()))
            {
                if (botPos.sqDistance(WorldPosition(player)) < sqRange)
                    return true;

                WorldObject* viewObj = player->GetViewpoint();
                if (viewObj && viewObj != player && botPos.sqDistance(WorldPosition(viewObj)) < sqRange)
                    return true;
            }
        }
    }

    // bot has a real player master (not another bot)
    if (GetMaster())
    {
        PlayerbotAI* masterBotAI = GET_PLAYERBOT_AI(GetMaster());
        if (!masterBotAI || masterBotAI->IsRealPlayer())
            return true;
    }

    // bot is grouped with a real player (or a bot owned by one)
    Group* group = bot->GetGroup();
    if (group)
    {
        for (GroupReference* gref = group->GetFirstMember(); gref; gref = gref->next())
        {
            Player* member = gref->GetSource();
            if (!member || !member->IsInWorld() || member->GetMapId() != bot->GetMapId())
                continue;

            if (member == bot)
                continue;

            PlayerbotAI* memberBotAI = GET_PLAYERBOT_AI(member);

            // group member is a real player or owned by one �?stay active
            if (!memberBotAI || memberBotAI->HasRealPlayerMaster())
                return true;

            // if group leader (bot) is inactive, follow suit
            if (group->IsLeader(member->GetGUID()))
            {
                if (!memberBotAI->AllowActivity(PARTY_ACTIVITY))
                    return false;
            }
        }
    }

    // bot is in LFG queue �?stay active
    bool isLFG = false;
    if (group)
    {
        if (sLFGMgr->GetState(group->GetGUID()) != lfg::LFG_STATE_NONE)
            isLFG = true;
    }
    if (sLFGMgr->GetState(bot->GetGUID()) != lfg::LFG_STATE_NONE)
        isLFG = true;

    if (isLFG)
        return true;

    // a real player has this bot on their friends list
    if (sPlayerbotAIConfig.BotActiveAloneForceWhenIsFriend)
    {
        // shouldnt be needed analyse in future
        if (!bot->GetGUID())
            return false;

        for (auto& player : sRandomPlayerbotMgr.GetPlayers())
        {
            if (!player || !player->GetSession() || !player->IsInWorld() || player->IsDuringRemoveFromWorld() ||
                player->GetSession()->isLogingOut())
                continue;

            PlayerbotAI* playerAI = GET_PLAYERBOT_AI(player);
            if (!playerAI || !playerAI->IsRealPlayer())
                continue;

            PlayerSocial* social = player->GetSocial();
            if (social && social->HasFriend(bot->GetGUID()))
                return true;
        }
    }

    // pathfinding only runs for bots forced active by the rules above �?
    // skip it for bots that would only be active via random rotation
    if (activityType == DETAILED_MOVE_ACTIVITY)
        return false;

    // #######################################################################################
    // Acitivity throttling logic
    // #######################################################################################
    if (sPlayerbotAIConfig.botActiveAlone <= 0)
        return false;

    // base threshold capped at 100
    uint32 mod = sPlayerbotAIConfig.botActiveAlone > 100 ? 100 : sPlayerbotAIConfig.botActiveAlone;

    // reduce threshold based on server tick time when SmartScale is enabled
    if (sPlayerbotAIConfig.botActiveAloneSmartScale &&
        bot->GetLevel() >= sPlayerbotAIConfig.botActiveAloneSmartScaleWhenMinLevel &&
        bot->GetLevel() <= sPlayerbotAIConfig.botActiveAloneSmartScaleWhenMaxLevel)
    {
        mod = AutoScaleActivity(mod);
    }

    // deterministic rotation �?bot is active if its hash falls below the threshold
    uint32 ActivityNumber = GetFixedBotNumber(100);
    return ActivityNumber < mod;
}

bool PlayerbotAI::AllowActivity(ActivityType activityType, bool checkNow)
{
    const int activityIndex = static_cast<int>(activityType);

    if (!allowActiveCheckTimer[activityIndex])
        allowActiveCheckTimer[activityIndex] = getMSTime();

    // 4500ms base + 0�?99ms per-bot offset = 4500�?999ms, capping at just under 5 seconds
    uint32 offset = bot->GetGUID().GetCounter() % 500;

    if (!checkNow && getMSTime() < (allowActiveCheckTimer[activityIndex] + 4500 + offset))
        return allowActive[activityIndex];

    const bool allowed = AllowActive(activityType);
    allowActive[activityIndex] = allowed;
    allowActiveCheckTimer[activityIndex] = getMSTime();

    return allowed;
}

uint32 PlayerbotAI::AutoScaleActivity(uint32 mod)
{
    // Current max server update time (ms), and the configured floor/ceiling values for bot scaling
    //By leewheel 2026-07-11: TC的WorldUpdateTime没有GetMaxUpdateTimeOfCurrentTable，使用GetLastUpdateTime替代
    uint32 maxDiff = sWorldUpdateTime.GetLastUpdateTime();
    //End By leewheel
    uint32 diffLimitFloor = sPlayerbotAIConfig.botActiveAloneSmartScaleDiffLimitfloor;
    uint32 diffLimitCeiling = sPlayerbotAIConfig.botActiveAloneSmartScaleDiffLimitCeiling;

    if (diffLimitCeiling <= diffLimitFloor)
    {
        // Perfrom binary decision if ceiling <= floor: Either all bots are active or none are
        return (maxDiff > diffLimitCeiling) ? 0 : mod;
    }

    if (maxDiff > diffLimitCeiling)
        return 0;

    if (maxDiff <= diffLimitFloor)
        return mod;

    // Calculate lag progress from floor to ceiling (0 to 1)
    double lagProgress = (maxDiff - diffLimitFloor) / (double)(diffLimitCeiling - diffLimitFloor);

    // Apply the percentage of active bots (the complement of lag progress) to the mod value
    return static_cast<uint32>(mod * (1 - lagProgress));
}

bool PlayerbotAI::IsOpposing(Player* player) { return IsOpposing(player->getRace(), bot->getRace()); }

bool PlayerbotAI::IsOpposing(uint8 race1, uint8 race2)
{
    return (IsAlliance(race1) && !IsAlliance(race2)) || (!IsAlliance(race1) && IsAlliance(race2));
}

void PlayerbotAI::RemoveShapeshift()
{
    RemoveAura("bear form");
    RemoveAura("dire bear form");
    RemoveAura("moonkin form");
    RemoveAura("travel form");
    RemoveAura("cat form");
    RemoveAura("flight form");
    bot->RemoveAura(33943);  // The latter added for now as RemoveAura("flight form") currently does not work.
    RemoveAura("swift flight form");
    RemoveAura("aquatic form");
    RemoveAura("ghost wolf");
    // RemoveAura("tree of life");
}

// Mirrors Blizzard’s GetAverageItemLevel rules :
// https://wowpedia.fandom.com/wiki/API_GetAverageItemLevel
uint32 PlayerbotAI::GetEquipGearScore(Player* player)
{
    constexpr uint8 TOTAL_SLOTS = 17;  // every slot except Body & Tabard
    uint32 sumLevel = 0;

    /* ---------- 0.  Detect “ignore off-hand�?situations --------- */
    Item* main = player->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND);
    Item* off = player->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_OFFHAND);

    bool ignoreOffhand = false;  // true �?divisor = 16
    if (main)
    {
        //By leewheel 2026-07-11: TC使用GetInventoryType()而非直接访问InventoryType
        bool twoHand = (main->GetTemplate()->GetInventoryType() == INVTYPE_2HWEAPON);
        //End By leewheel
        if (twoHand && !player->HasAura(SPELL_TITAN_GRIP))
            ignoreOffhand = true;  // classic 2-hander
    }
    else if (!off)  // both hands empty
        ignoreOffhand = true;

    /* ---------- 1.  Sum up item-levels -------------------------- */
    for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
    {
        if (slot == EQUIPMENT_SLOT_BODY || slot == EQUIPMENT_SLOT_TABARD)
            continue;  // Blizzard never counts these

        if (ignoreOffhand && slot == EQUIPMENT_SLOT_OFFHAND)
            continue;  // skip off-hand in 2-H case

        if (Item* it = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
            //By leewheel 2026-07-11: TC使用GetItemLevel()而非直接访问ItemLevel
            sumLevel += it->GetTemplate()->GetItemLevel();  // missing items add 0
            //End By leewheel
    }

    /* ---------- 2.  Divide by 17 or 16 -------------------------- */
    const uint8 divisor = ignoreOffhand ? TOTAL_SLOTS - 1 : TOTAL_SLOTS;  // 16 or 17
    return sumLevel / divisor;
}

// NOTE : function rewritten as flags "withBags" and "withBank" not used, and _fillGearScoreData sometimes attribute
// one-hand/2H Weapon in wrong slots
/*uint32 PlayerbotAI::GetEquipGearScore(Player* player)
{
    // This function aims to calculate the equipped gear score

    uint32 sum = 0;
    uint8 count = EQUIPMENT_SLOT_END - 2;  // ignore body and tabard slots
    uint8 mh_type = 0;

    for (uint8 i = EQUIPMENT_SLOT_START; i < EQUIPMENT_SLOT_END; ++i)
    {
        Item* item =player->GetItemByPos(INVENTORY_SLOT_BAG_0, i);
        if (item && i != EQUIPMENT_SLOT_BODY && i != EQUIPMENT_SLOT_TABARD)
        {
            ItemTemplate const* proto = item->GetTemplate();
            //By leewheel 2026-07-11: TC使用GetItemLevel()和GetInventoryType()
            sum += proto->GetItemLevel();

            // If character is not warfury and have 2 hand weapon equipped, main hand will be counted twice
            if (i == SLOT_MAIN_HAND)
                mh_type = item->GetTemplate()->GetInventoryType();
            if (!player->HasAura(SPELL_TITAN_GRIP) && mh_type == INVTYPE_2HWEAPON && i == SLOT_MAIN_HAND)
                sum += item->GetTemplate()->GetItemLevel();
            //End By leewheel
        }
    }

    uint32 gs = uint32(sum / count);
    return gs;
}*/

/*uint32 PlayerbotAI::GetEquipGearScore(Player* player, bool withBags, bool withBank)
{
    std::vector<uint32> gearScore(EQUIPMENT_SLOT_END);
    uint32 twoHandScore = 0;

    for (uint8 i = EQUIPMENT_SLOT_START; i < EQUIPMENT_SLOT_END; ++i)
    {
        if (Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
            _fillGearScoreData(player, item, &gearScore, twoHandScore);
    }

    if (withBags)
    {
        // check inventory
        for (uint8 i = INVENTORY_SLOT_ITEM_START; i < INVENTORY_SLOT_ITEM_END; ++i)
        {
            if (Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
                _fillGearScoreData(player, item, &gearScore, twoHandScore);
        }

        // check bags
        for (uint8 i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; ++i)
        {
            if (Bag* pBag = (Bag*)player->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
            {
                for (uint32 j = 0; j < pBag->GetBagSize(); ++j)
                {
                    if (Item* item2 = pBag->GetItemByPos(j))
                        _fillGearScoreData(player, item2, &gearScore, twoHandScore);
                }
            }
        }
    }

    if (withBank)
    {
        for (uint8 i = BANK_SLOT_ITEM_START; i < BANK_SLOT_ITEM_END; ++i)
        {
            if (Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
                _fillGearScoreData(player, item, &gearScore, twoHandScore);
        }

        for (uint8 i = BANK_SLOT_BAG_START; i < BANK_SLOT_BAG_END; ++i)
        {
            if (Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
            {
                if (item->IsBag())
                {
                    Bag* bag = (Bag*)item;
                    for (uint8 j = 0; j < bag->GetBagSize(); ++j)
                    {
                        if (Item* item2 = bag->GetItemByPos(j))
                            _fillGearScoreData(player, item2, &gearScore, twoHandScore);
                    }
                }
            }
        }
    }

    uint8 count = EQUIPMENT_SLOT_END - 2;  // ignore body and tabard slots
    uint32 sum = 0;

    // check if 2h hand is higher level than main hand + off hand
    if (gearScore[EQUIPMENT_SLOT_MAINHAND] + gearScore[EQUIPMENT_SLOT_OFFHAND] < twoHandScore * 2)
    {
        gearScore[EQUIPMENT_SLOT_OFFHAND] = 0;  // off hand is ignored in calculations if 2h weapon has higher score
        --count;
        gearScore[EQUIPMENT_SLOT_MAINHAND] = twoHandScore;
    }

    for (uint8 i = EQUIPMENT_SLOT_START; i < EQUIPMENT_SLOT_END; ++i)
    {
        sum += gearScore[i];
    }

    if (count)
    {
        uint32 res = uint32(sum / count);
        return res;
    }

    return 0;
}*/
uint32 PlayerbotAI::GetMixedGearScore(Player* player, bool withBags, bool withBank, uint32 topN)
{
    std::vector<uint32> gearScore(EQUIPMENT_SLOT_END);
    uint32 twoHandScore = 0;

    for (uint8 i = EQUIPMENT_SLOT_START; i < EQUIPMENT_SLOT_END; ++i)
    {
        if (Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
            _fillGearScoreData(player, item, &gearScore, twoHandScore, true);
    }

    if (withBags)
    {
        // check inventory
        for (uint8 i = INVENTORY_SLOT_ITEM_START; i < INVENTORY_SLOT_ITEM_END; ++i)
        {
            if (Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
                _fillGearScoreData(player, item, &gearScore, twoHandScore, true);
        }

        // check bags
        for (uint8 i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; ++i)
        {
            if (Bag* pBag = (Bag*)player->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
            {
                for (uint32 j = 0; j < pBag->GetBagSize(); ++j)
                {
                    if (Item* item2 = pBag->GetItemByPos(j))
                        _fillGearScoreData(player, item2, &gearScore, twoHandScore, true);
                }
            }
        }
    }

    if (withBank)
    {
        for (uint8 i = BANK_SLOT_ITEM_START; i < BANK_SLOT_ITEM_END; ++i)
        {
            if (Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
                _fillGearScoreData(player, item, &gearScore, twoHandScore, true);
        }

        for (uint8 i = BANK_SLOT_BAG_START; i < BANK_SLOT_BAG_END; ++i)
        {
            if (Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
            {
                if (item->IsBag())
                {
                    Bag* bag = (Bag*)item;
                    for (uint8 j = 0; j < bag->GetBagSize(); ++j)
                    {
                        if (Item* item2 = bag->GetItemByPos(j))
                            _fillGearScoreData(player, item2, &gearScore, twoHandScore, true);
                    }
                }
            }
        }
    }
    if (!topN)
    {
        uint8 count = EQUIPMENT_SLOT_END - 2;  // ignore body and tabard slots
        uint32 sum = 0;

        // check if 2h hand is higher level than main hand + off hand
        if (gearScore[EQUIPMENT_SLOT_MAINHAND] + gearScore[EQUIPMENT_SLOT_OFFHAND] < twoHandScore * 2)
        {
            gearScore[EQUIPMENT_SLOT_OFFHAND] = 0;  // off hand is ignored in calculations if 2h weapon has higher score
            --count;
            gearScore[EQUIPMENT_SLOT_MAINHAND] = twoHandScore;
        }

        for (uint8 i = EQUIPMENT_SLOT_START; i < EQUIPMENT_SLOT_END; ++i)
        {
            sum += gearScore[i];
        }

        if (count)
        {
            uint32 res = uint32(sum / count);
            return res;
        }

        return 0;
    }
    // topN != 0
    if (gearScore[EQUIPMENT_SLOT_MAINHAND] + gearScore[EQUIPMENT_SLOT_OFFHAND] < twoHandScore * 2)
    {
        gearScore[EQUIPMENT_SLOT_OFFHAND] = twoHandScore;
        gearScore[EQUIPMENT_SLOT_MAINHAND] = twoHandScore;
    }
    std::vector<uint32> topGearScore;
    for (uint8 i = EQUIPMENT_SLOT_START; i < EQUIPMENT_SLOT_END; ++i)
    {
        topGearScore.push_back(gearScore[i]);
    }
    std::sort(topGearScore.begin(), topGearScore.end(), [&](const uint32 lhs, const uint32 rhs) { return lhs > rhs; });
    uint32 sum = 0;
    for (uint32 i = 0; i < std::min((uint32)topGearScore.size(), topN); i++)
    {
        sum += topGearScore[i];
    }
    return sum / topN;
}

void PlayerbotAI::_fillGearScoreData(Player* player, Item* item, std::vector<uint32>* gearScore, uint32& twoHandScore,
                                     bool mixed)
{
    if (!item)
        return;

    ItemTemplate const* proto = item->GetTemplate();
    if (player->CanUseItem(proto) != EQUIP_ERR_OK)
        return;

    //By leewheel 2026-07-11: TC使用GetInventoryType()/GetItemLevel()/GetQuality()
    uint8 type = proto->GetInventoryType();
    uint32 level = mixed ? proto->GetItemLevel() * PlayerbotAI::GetItemScoreMultiplier(ItemQualities(proto->GetQuality()))
                         : proto->GetItemLevel();
    //End By leewheel

    switch (type)
    {
        case INVTYPE_2HWEAPON:
            twoHandScore = std::max(twoHandScore, level);
            break;
        case INVTYPE_WEAPON:
        case INVTYPE_WEAPONMAINHAND:
            (*gearScore)[SLOT_MAIN_HAND] = std::max((*gearScore)[SLOT_MAIN_HAND], level);
            break;
        case INVTYPE_SHIELD:
        case INVTYPE_WEAPONOFFHAND:
            (*gearScore)[EQUIPMENT_SLOT_OFFHAND] = std::max((*gearScore)[EQUIPMENT_SLOT_OFFHAND], level);
            break;
        case INVTYPE_THROWN:
        case INVTYPE_RANGEDRIGHT:
        case INVTYPE_RANGED:
        case INVTYPE_QUIVER:
        case INVTYPE_RELIC:
            (*gearScore)[EQUIPMENT_SLOT_RANGED] = std::max((*gearScore)[EQUIPMENT_SLOT_RANGED], level);
            break;
        case INVTYPE_HEAD:
            (*gearScore)[EQUIPMENT_SLOT_HEAD] = std::max((*gearScore)[EQUIPMENT_SLOT_HEAD], level);
            break;
        case INVTYPE_NECK:
            (*gearScore)[EQUIPMENT_SLOT_NECK] = std::max((*gearScore)[EQUIPMENT_SLOT_NECK], level);
            break;
        case INVTYPE_SHOULDERS:
            (*gearScore)[EQUIPMENT_SLOT_SHOULDERS] = std::max((*gearScore)[EQUIPMENT_SLOT_SHOULDERS], level);
            break;
        case INVTYPE_BODY:  // Shouldn't be considered when calculating average ilevel
            (*gearScore)[EQUIPMENT_SLOT_BODY] = std::max((*gearScore)[EQUIPMENT_SLOT_BODY], level);
            break;
        case INVTYPE_CHEST:
            (*gearScore)[EQUIPMENT_SLOT_CHEST] = std::max((*gearScore)[EQUIPMENT_SLOT_CHEST], level);
            break;
        case INVTYPE_WAIST:
            (*gearScore)[EQUIPMENT_SLOT_WAIST] = std::max((*gearScore)[EQUIPMENT_SLOT_WAIST], level);
            break;
        case INVTYPE_LEGS:
            (*gearScore)[EQUIPMENT_SLOT_LEGS] = std::max((*gearScore)[EQUIPMENT_SLOT_LEGS], level);
            break;
        case INVTYPE_FEET:
            (*gearScore)[EQUIPMENT_SLOT_FEET] = std::max((*gearScore)[EQUIPMENT_SLOT_FEET], level);
            break;
        case INVTYPE_WRISTS:
            (*gearScore)[EQUIPMENT_SLOT_WRISTS] = std::max((*gearScore)[EQUIPMENT_SLOT_WRISTS], level);
            break;
        case INVTYPE_HANDS:
            (*gearScore)[EQUIPMENT_SLOT_HANDS] = std::max((*gearScore)[EQUIPMENT_SLOT_HANDS], level);
            break;
        // equipped gear score check uses both rings and trinkets for calculation, assume that for bags/banks it is the
        // same with keeping second highest score at second slot
        case INVTYPE_FINGER:
        {
            if ((*gearScore)[EQUIPMENT_SLOT_FINGER1] < level)
            {
                (*gearScore)[EQUIPMENT_SLOT_FINGER2] = (*gearScore)[EQUIPMENT_SLOT_FINGER1];
                (*gearScore)[EQUIPMENT_SLOT_FINGER1] = level;
            }
            else if ((*gearScore)[EQUIPMENT_SLOT_FINGER2] < level)
                (*gearScore)[EQUIPMENT_SLOT_FINGER2] = level;
            break;
        }
        case INVTYPE_TRINKET:
        {
            if ((*gearScore)[EQUIPMENT_SLOT_TRINKET1] < level)
            {
                (*gearScore)[EQUIPMENT_SLOT_TRINKET2] = (*gearScore)[EQUIPMENT_SLOT_TRINKET1];
                (*gearScore)[EQUIPMENT_SLOT_TRINKET1] = level;
            }
            else if ((*gearScore)[EQUIPMENT_SLOT_TRINKET2] < level)
                (*gearScore)[EQUIPMENT_SLOT_TRINKET2] = level;
            break;
        }
        case INVTYPE_CLOAK:
            (*gearScore)[EQUIPMENT_SLOT_BACK] = std::max((*gearScore)[EQUIPMENT_SLOT_BACK], level);
            break;
        default:
            break;
    }
}

std::string const PlayerbotAI::HandleRemoteCommand(std::string const command)
{
    if (command == "state")
    {
        switch (currentState)
        {
            case BOT_STATE_COMBAT:
                return "combat";
            case BOT_STATE_DEAD:
                return "dead";
            case BOT_STATE_NON_COMBAT:
                return "non-combat";
            default:
                return "unknown";
        }
    }
    else if (command == "position")
    {
        std::ostringstream out;
        out << bot->GetPositionX() << " " << bot->GetPositionY() << " " << bot->GetPositionZ() << " " << bot->GetMapId()
            << " " << bot->GetOrientation();

        if (AreaTableEntry const* zoneEntry = sAreaTableStore.LookupEntry(bot->GetZoneId()))
            //By leewheel 2026-09-09: TC-Cata的AreaTableEntry用AreaName字段而非area_name()方法
            out << " |" << zoneEntry->AreaName[DEFAULT_LOCALE] << "|";
            //End By leewheel

        return out.str();
    }
    else if (command == "tpos")
    {
        Unit* target = *GetAiObjectContext()->GetValue<Unit*>("current target");
        if (!target)
        {
            return "";
        }

        std::ostringstream out;
        out << target->GetPositionX() << " " << target->GetPositionY() << " " << target->GetPositionZ() << " "
            << target->GetMapId() << " " << target->GetOrientation();
        return out.str();
    }
    else if (command == "movement")
    {
        LastMovement& data = *GetAiObjectContext()->GetValue<LastMovement&>("last movement");
        std::ostringstream out;
        out << data.lastMoveShort.GetPositionX() << " " << data.lastMoveShort.GetPositionY() << " "
            << data.lastMoveShort.GetPositionZ() << " " << data.lastMoveShort.GetMapId() << " "
            << data.lastMoveShort.GetOrientation();
        return out.str();
    }
    else if (command == "target")
    {
        Unit* target = *GetAiObjectContext()->GetValue<Unit*>("current target");
        if (!target)
        {
            return "";
        }

        return target->GetName();
    }
    else if (command == "hp")
    {
        uint32 pct = static_cast<uint32>(bot->GetHealthPct());
        std::ostringstream out;
        out << pct << "%";

        Unit* target = *GetAiObjectContext()->GetValue<Unit*>("current target");
        if (!target)
        {
            return out.str();
        }

        pct = static_cast<uint32>(target->GetHealthPct());
        out << " / " << pct << "%";
        return out.str();
    }
    else if (command == "strategy")
    {
        return currentEngine->ListStrategies();
    }
    else if (command == "action")
    {
        return currentEngine->GetLastAction();
    }
    else if (command == "values")
    {
        return GetAiObjectContext()->FormatValues();
    }
    else if (command == "travel")
    {
        std::ostringstream out;

        TravelTarget* target = GetAiObjectContext()->GetValue<TravelTarget*>("travel target")->Get();
        if (target->getDestination())
        {
            //By leewheel 2026-08-01: 玩家可见文本中文化
            out << "目的地 = " << target->getDestination()->getName();

            out << ": " << target->getDestination()->getTitle();

            out << " 访客: " << target->getDestination()->getVisitors();

            if (!(*target->getPosition() == WorldPosition()))
            {
                out << "(" << target->getPosition()->getAreaName() << ")";
                out << " 距离: " << target->getPosition()->distance(bot) << "码";
                out << " 访客: " << target->getPosition()->getVisitors();
            }
        }

        out << " 状态 = ";

        if (target->getStatus() == TRAVEL_STATUS_NONE)
            out << " 无";
        else if (target->getStatus() == TRAVEL_STATUS_PREPARE)
            out << " 准备";
        else if (target->getStatus() == TRAVEL_STATUS_TRAVEL)
            out << " 旅行";
        else if (target->getStatus() == TRAVEL_STATUS_WORK)
            out << " 工作";
        else if (target->getStatus() == TRAVEL_STATUS_COOLDOWN)
            out << " 冷却";
        else if (target->getStatus() == TRAVEL_STATUS_EXPIRED)
            out << " 已过期";

        if (target->getStatus() != TRAVEL_STATUS_EXPIRED)
            out << " 剩余 " << (target->getTimeLeft() / 1000) << " 秒";

        out << " 重试 " << target->getRetryCount(true) << "/" << target->getRetryCount(false);
        //End By leewheel

        return out.str();
    }
    else if (command == "budget")
    {
        std::ostringstream out;

        AiObjectContext* context = GetAiObjectContext();

        //By leewheel 2026-08-01: 玩家可见文本中文化
        out << "当前金钱: " << ChatHelper::formatMoney(bot->GetMoney()) << " 可自由使用:"
            << ChatHelper::formatMoney(AI_VALUE2(uint32, "free money for", (uint32)NeedMoneyFor::anything)) << "\n";
        out << "用途 | 可用 / 需要 \n";

        for (uint32 i = 1; i < (uint32)NeedMoneyFor::anything; i++)
        {
            NeedMoneyFor needMoneyFor = NeedMoneyFor(i);

            switch (needMoneyFor)
            {
                case NeedMoneyFor::none:
                    out << "无";
                    break;
                case NeedMoneyFor::repair:
                    out << "修理";
                    break;
                case NeedMoneyFor::ammo:
                    out << "弹药";
                    break;
                case NeedMoneyFor::spells:
                    out << "法术";
                    break;
                case NeedMoneyFor::travel:
                    out << "旅行";
                    break;
                case NeedMoneyFor::consumables:
                    out << "消耗品";
                    break;
                case NeedMoneyFor::gear:
                    out << "装备";
                    break;
                case NeedMoneyFor::guild:
                    out << "公会";
                    break;
                default:
                    break;
            }
            //End By leewheel

            out << " | " << ChatHelper::formatMoney(AI_VALUE2(uint32, "free money for", i)) << " / "
                << ChatHelper::formatMoney(AI_VALUE2(uint32, "money needed for", i)) << "\n";
        }

        return out.str();
    }

    std::ostringstream out;
    //By leewheel 2026-08-01: 玩家可见文本中文化
    out << "无效命令: " << command;
    //End By leewheel
    return out.str();
}

bool PlayerbotAI::HasSkill(SkillType skill) { return bot->HasSkill(skill) && bot->GetSkillValue(skill) > 0; }

float PlayerbotAI::GetRange(std::string const type)
{
    float val = 0;
    if (aiObjectContext)
        val = aiObjectContext->GetValue<float>("range", type)->Get();

    if (abs(val) >= 0.1f)
        return val;

    if (type == "spell")
        return sPlayerbotAIConfig.spellDistance;

    if (type == "shoot")
        return sPlayerbotAIConfig.shootDistance;

    if (type == "flee")
        return sPlayerbotAIConfig.fleeDistance;

    if (type == "heal")
        return sPlayerbotAIConfig.healDistance;

    if (type == "melee")
        return sPlayerbotAIConfig.meleeDistance;

    return 0;
}

void PlayerbotAI::Ping(float x, float y)
{
    WorldPacket data(MSG_MINIMAP_PING, (8 + 4 + 4));
    data << bot->GetGUID();
    data << x;
    data << y;

    if (bot->GetGroup())
    {
        bot->GetGroup()->BroadcastPacket(&data, true, -1, bot->GetGUID());
    }
    else
    {
        bot->GetSession()->SendPacket(&data);
    }
}

// Helper function to iterate through items in the inventory and bags
Item* PlayerbotAI::FindItemInInventory(std::function<bool(ItemTemplate const*)> checkItem) const
{
    // List out items in the main backpack
    for (uint8 slot = INVENTORY_SLOT_ITEM_START; slot < INVENTORY_SLOT_ITEM_END; ++slot)
    {
        if (Item* const pItem = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
        {
            ItemTemplate const* pItemProto = pItem->GetTemplate();
            if (pItemProto && bot->CanUseItem(pItemProto) == EQUIP_ERR_OK && checkItem(pItemProto))
                return pItem;
        }
    }

    // List out items in other removable backpacks
    for (uint8 bag = INVENTORY_SLOT_BAG_START; bag < INVENTORY_SLOT_BAG_END; ++bag)
    {
        if (Bag const* const pBag = (Bag*)bot->GetItemByPos(INVENTORY_SLOT_BAG_0, bag))
        {
            for (uint8 slot = 0; slot < pBag->GetBagSize(); ++slot)
            {
                if (Item* const pItem = bot->GetItemByPos(bag, slot))
                {
                    ItemTemplate const* pItemProto = pItem->GetTemplate();
                    if (pItemProto && bot->CanUseItem(pItemProto) == EQUIP_ERR_OK && checkItem(pItemProto))
                        return pItem;
                }
            }
        }
    }

    return nullptr;
}

// Find Poison
Item* PlayerbotAI::FindPoison() const
{
    return FindItemInInventory([](ItemTemplate const* pItemProto) -> bool
                               { return pItemProto->GetClass() == ITEM_CLASS_CONSUMABLE && pItemProto->SubClass == 6; });
}

// Find Ammo
Item* PlayerbotAI::FindAmmo() const
{
    // Get equipped ranged weapon
    if (Item* rangedWeapon = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_RANGED))
    {
        uint32 weaponSubClass = rangedWeapon->GetTemplate()->SubClass;
        uint32 requiredAmmoType = 0;

        // Determine the correct ammo type based on the weapon
        switch (weaponSubClass)
        {
            case ITEM_SUBCLASS_WEAPON_GUN:
                requiredAmmoType = ITEM_SUBCLASS_BULLET;
                break;
            case ITEM_SUBCLASS_WEAPON_BOW:
            case ITEM_SUBCLASS_WEAPON_CROSSBOW:
                requiredAmmoType = ITEM_SUBCLASS_ARROW;
                break;
            default:
                return nullptr;  // Not a ranged weapon that requires ammo
        }

        // Search inventory for the correct ammo type
        return FindItemInInventory(
            [requiredAmmoType](ItemTemplate const* pItemProto) -> bool
            { return pItemProto->GetClass() == ITEM_CLASS_PROJECTILE && pItemProto->SubClass == requiredAmmoType; });
    }

    return nullptr;  // No ranged weapon equipped
}

// Find Consumable
Item* PlayerbotAI::FindConsumable(uint32 itemId) const
{
    return FindItemInInventory(
        [itemId](ItemTemplate const* pItemProto) -> bool
        {
            return (pItemProto->GetClass() == ITEM_CLASS_CONSUMABLE || pItemProto->GetClass() == ITEM_CLASS_TRADE_GOODS) &&
                   pItemProto->GetId() == itemId;
        });
}

// Find Bandage
Item* PlayerbotAI::FindBandage() const
{
    return FindItemInInventory(
        [](ItemTemplate const* pItemProto) -> bool
        { return pItemProto->GetClass() == ITEM_CLASS_CONSUMABLE && pItemProto->SubClass == ITEM_SUBCLASS_BANDAGE; });
}

Item* PlayerbotAI::FindOpenableItem() const
{
    return FindItemInInventory(
        [this](ItemTemplate const* itemTemplate) -> bool
        {
            //By leewheel 2026-07-11: 修复缺少分号和lambda返回语句
            return itemTemplate->HasFlag(ITEM_FLAG_HAS_LOOT) &&
                   (itemTemplate->GetLockID() == 0 || !this->bot->GetItemByEntry(itemTemplate->GetId())->IsLocked());
            //End By leewheel
        });
}

Item* PlayerbotAI::FindLockedItem() const
{
    return FindItemInInventory(
        [this](ItemTemplate const* itemTemplate) -> bool
        {
            if (!this->bot->HasSkill(SKILL_LOCKPICKING))  // Ensure bot has Lockpicking skill
                return false;

            //By leewheel 2026-07-11: TC使用GetLockID()
    if (itemTemplate->GetLockID() == 0)  // Ensure the item is actually locked
//End By leewheel
                return false;

            Item* item = this->bot->GetItemByEntry(itemTemplate->GetId());
            if (!item || !item->IsLocked())  // Ensure item instance is locked
                return false;

            // Check if bot has enough Lockpicking skill
            //By leewheel 2026-07-11: TC使用GetLockID()
    LockEntry const* lockInfo = sLockStore.LookupEntry(itemTemplate->GetLockID());
//End By leewheel
            if (!lockInfo)
                return false;

            for (uint8 j = 0; j < 8; ++j)
            {
                if (lockInfo->Type[j] == LOCK_KEY_SKILL)
                {
                    uint32 skillId = SkillByLockType(LockType(lockInfo->Index[j]));
                    if (skillId == SKILL_LOCKPICKING)
                    {
                        uint32 requiredSkill = lockInfo->Skill[j];
                        uint32 botSkill = this->bot->GetSkillValue(SKILL_LOCKPICKING);
                        return botSkill >= requiredSkill;
                    }
                }
            }

            return false;
        });
}

Item* PlayerbotAI::FindStoneFor(Item* weapon) const
{
    if (!weapon)
        return nullptr;

    const ItemTemplate* item_template = weapon->GetTemplate();
    if (!item_template)
        return nullptr;

    static const std::vector<uint32_t> uPrioritizedSharpStoneIds = {
        ADAMANTITE_SHARPENING_STONE, FEL_SHARPENING_STONE,   ELEMENTAL_SHARPENING_STONE, DENSE_SHARPENING_STONE,
        SOLID_SHARPENING_STONE,      HEAVY_SHARPENING_STONE, COARSE_SHARPENING_STONE,    ROUGH_SHARPENING_STONE};

    static const std::vector<uint32_t> uPrioritizedWeightStoneIds = {
        ADAMANTITE_WEIGHTSTONE, FEL_WEIGHTSTONE,    ELEMENTAL_SHARPENING_STONE, DENSE_WEIGHTSTONE, SOLID_WEIGHTSTONE,
        HEAVY_WEIGHTSTONE,      COARSE_WEIGHTSTONE, ROUGH_WEIGHTSTONE};

    Item* stone = nullptr;
    ItemTemplate const* pProto = weapon->GetTemplate();
    if (pProto && (pProto->SubClass == ITEM_SUBCLASS_WEAPON_SWORD || pProto->SubClass == ITEM_SUBCLASS_WEAPON_SWORD2 ||
                   pProto->SubClass == ITEM_SUBCLASS_WEAPON_AXE || pProto->SubClass == ITEM_SUBCLASS_WEAPON_AXE2 ||
                   pProto->SubClass == ITEM_SUBCLASS_WEAPON_DAGGER || pProto->SubClass == ITEM_SUBCLASS_WEAPON_POLEARM))
    {
        for (uint8 i = 0; i < std::size(uPrioritizedSharpStoneIds); ++i)
        {
            stone = FindConsumable(uPrioritizedSharpStoneIds[i]);
            if (stone)
            {
                return stone;
            }
        }
    }
    else if (pProto &&
             (pProto->SubClass == ITEM_SUBCLASS_WEAPON_MACE || pProto->SubClass == ITEM_SUBCLASS_WEAPON_MACE2 ||
              pProto->SubClass == ITEM_SUBCLASS_WEAPON_STAFF || pProto->SubClass == ITEM_SUBCLASS_WEAPON_FIST))
    {
        for (uint8 i = 0; i < std::size(uPrioritizedWeightStoneIds); ++i)
        {
            stone = FindConsumable(uPrioritizedWeightStoneIds[i]);
            if (stone)
            {
                return stone;
            }
        }
    }

    return stone;
}

Item* PlayerbotAI::FindOilFor(Item* weapon) const
{
    if (!weapon)
        return nullptr;

    const ItemTemplate* item_template = weapon->GetTemplate();
    if (!item_template)
        return nullptr;

    static const std::vector<uint32_t> uPrioritizedWizardOilIds = {
        BRILLIANT_WIZARD_OIL, SUPERIOR_WIZARD_OIL, WIZARD_OIL,      LESSER_WIZARD_OIL, MINOR_WIZARD_OIL,
        BRILLIANT_MANA_OIL,   SUPERIOR_MANA_OIL,   LESSER_MANA_OIL, MINOR_MANA_OIL};

    static const std::vector<uint32_t> uPrioritizedManaOilIds = {
        BRILLIANT_MANA_OIL,  SUPERIOR_MANA_OIL, LESSER_MANA_OIL,   MINOR_MANA_OIL,  BRILLIANT_WIZARD_OIL,
        SUPERIOR_WIZARD_OIL, WIZARD_OIL,        LESSER_WIZARD_OIL, MINOR_WIZARD_OIL};

    Item* oil = nullptr;
    int botClass = bot->getClass();
    int specTab = AiFactory::GetPlayerSpecTab(bot);

    const std::vector<uint32_t>* prioritizedOils = nullptr;
    switch (botClass)
    {
        case CLASS_PRIEST:
            prioritizedOils = (specTab == 2) ? &uPrioritizedWizardOilIds : &uPrioritizedManaOilIds;
            break;
        case CLASS_MAGE:
            prioritizedOils = &uPrioritizedWizardOilIds;
            break;
        case CLASS_DRUID:
            if (specTab == 0)  // Balance
                prioritizedOils = &uPrioritizedWizardOilIds;
            else if (specTab == 1)  // Feral
                prioritizedOils = nullptr;
            else  // Restoration (specTab == 2) or any other/unspecified spec
                prioritizedOils = &uPrioritizedManaOilIds;
            break;
        case CLASS_HUNTER:
            prioritizedOils = &uPrioritizedManaOilIds;
            break;
        case CLASS_PALADIN:
            if (specTab == 1)  // Protection
                prioritizedOils = &uPrioritizedWizardOilIds;
            else if (specTab == 2)  // Retribution
                prioritizedOils = nullptr;
            else  // Holy (specTab == 0) or any other/unspecified spec
                prioritizedOils = &uPrioritizedManaOilIds;
            break;
        default:
            prioritizedOils = &uPrioritizedManaOilIds;
            break;
    }

    if (prioritizedOils)
    {
        for (auto const& id : *prioritizedOils)
        {
            oil = FindConsumable(id);
            if (oil)
            {
                return oil;
            }
        }
    }

    return oil;
}

std::vector<Item*> PlayerbotAI::GetInventoryAndEquippedItems()
{
    std::vector<Item*> items;

    for (int i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; ++i)
    {
        if (Bag* pBag = (Bag*)bot->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
        {
            for (uint32 j = 0; j < pBag->GetBagSize(); ++j)
            {
                if (Item* pItem = pBag->GetItemByPos(j))
                {
                    items.push_back(pItem);
                }
            }
        }
    }

    for (int i = INVENTORY_SLOT_ITEM_START; i < INVENTORY_SLOT_ITEM_END; ++i)
    {
        if (Item* pItem = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
        {
            items.push_back(pItem);
        }
    }

    for (int i = KEYRING_SLOT_START; i < KEYRING_SLOT_END; ++i)
    {
        if (Item* pItem = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
        {
            items.push_back(pItem);
        }
    }

    for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; slot++)
    {
        if (Item* pItem = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
        {
            items.push_back(pItem);
        }
    }

    return items;
}

std::vector<Item*> PlayerbotAI::GetInventoryItems()
{
    std::vector<Item*> items;

    for (int i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; ++i)
    {
        if (Bag* pBag = (Bag*)bot->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
        {
            for (uint32 j = 0; j < pBag->GetBagSize(); ++j)
            {
                if (Item* pItem = pBag->GetItemByPos(j))
                {
                    items.push_back(pItem);
                }
            }
        }
    }

    for (int i = INVENTORY_SLOT_ITEM_START; i < INVENTORY_SLOT_ITEM_END; ++i)
    {
        if (Item* pItem = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
        {
            items.push_back(pItem);
        }
    }

    for (int i = KEYRING_SLOT_START; i < KEYRING_SLOT_END; ++i)
    {
        if (Item* pItem = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
        {
            items.push_back(pItem);
        }
    }

    return items;
}

uint32 PlayerbotAI::GetInventoryItemsCountWithId(uint32 itemId)
{
    uint32 count = 0;

    for (int i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; ++i)
    {
        if (Bag* pBag = (Bag*)bot->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
        {
            for (uint32 j = 0; j < pBag->GetBagSize(); ++j)
            {
                if (Item* pItem = pBag->GetItemByPos(j))
                {
                    if (pItem->GetTemplate()->GetId() == itemId)
                    {
                        count += pItem->GetCount();
                    }
                }
            }
        }
    }

    for (int i = INVENTORY_SLOT_ITEM_START; i < INVENTORY_SLOT_ITEM_END; ++i)
    {
        if (Item* pItem = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
        {
            if (pItem->GetTemplate()->GetId() == itemId)
            {
                count += pItem->GetCount();
            }
        }
    }

    for (int i = KEYRING_SLOT_START; i < KEYRING_SLOT_END; ++i)
    {
        if (Item* pItem = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
        {
            if (pItem->GetTemplate()->GetId() == itemId)
            {
                count += pItem->GetCount();
            }
        }
    }

    return count;
}

bool PlayerbotAI::HasItemInInventory(uint32 itemId)
{
    for (int i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; ++i)
    {
        if (Bag* pBag = (Bag*)bot->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
        {
            for (uint32 j = 0; j < pBag->GetBagSize(); ++j)
            {
                if (Item* pItem = pBag->GetItemByPos(j))
                {
                    if (pItem->GetTemplate()->GetId() == itemId)
                    {
                        return true;
                    }
                }
            }
        }
    }

    for (int i = INVENTORY_SLOT_ITEM_START; i < INVENTORY_SLOT_ITEM_END; ++i)
    {
        if (Item* pItem = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
        {
            if (pItem->GetTemplate()->GetId() == itemId)
            {
                return true;
            }
        }
    }

    for (int i = KEYRING_SLOT_START; i < KEYRING_SLOT_END; ++i)
    {
        if (Item* pItem = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
        {
            if (pItem->GetTemplate()->GetId() == itemId)
            {
                return true;
            }
        }
    }

    return false;
}

std::vector<std::pair<const Quest*, uint32>> PlayerbotAI::GetCurrentQuestsRequiringItemId(uint32 itemId)
{
    std::vector<std::pair<const Quest*, uint32>> result;

    if (!itemId)
    {
        return result;
    }

    for (uint16 slot = 0; slot < MAX_QUEST_LOG_SIZE; ++slot)
    {
        uint32 questId = bot->GetQuestSlotQuestId(slot);
        if (!questId)
            continue;

        // QuestStatus status = bot->GetQuestStatus(questId);
        const Quest* quest = sObjectMgr->GetQuestTemplate(questId);
        for (uint8 i = 0; i < std::size(quest->RequiredItemId); ++i)
        {
            if (quest->RequiredItemId[i] == itemId)
            {
                result.push_back(std::pair(quest, quest->RequiredItemId[i]));
                break;
            }
        }
    }

    return result;
}

//  on self
void PlayerbotAI::ImbueItem(Item* item) { ImbueItem(item, TARGET_FLAG_NONE, ObjectGuid::Empty); }

//  item on unit
void PlayerbotAI::ImbueItem(Item* item, Unit* target)
{
    if (!IsValidUnit(target))
        return;

    ImbueItem(item, TARGET_FLAG_UNIT, target->GetGUID());
}

//  item on equipped item
void PlayerbotAI::ImbueItem(Item* item, uint8 targetInventorySlot)
{
    if (targetInventorySlot >= EQUIPMENT_SLOT_END)
        return;

    Item* const targetItem = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, targetInventorySlot);
    if (!targetItem)
        return;

    ImbueItem(item, TARGET_FLAG_ITEM, targetItem->GetGUID());
}

// generic item use method
//By leewheel 2026-07-14: 重写为使用WorldPackets::Spells::UseItem直接调用HandleUseItemOpcode
// 旧代码用AC格式构造WorldPacket并QueuePacket，TC的UseItem::Read()格式完全不同会导致ByteBufferException崩溃
void PlayerbotAI::ImbueItem(Item* item, uint32 targetFlag, ObjectGuid targetGUID)
{
    if (!item)
        return;

    uint8 bagIndex = item->GetBagSlot();
    uint8 slot = item->GetSlot();
    ObjectGuid item_guid = item->GetGUID();

    uint32 spellId = 0;
    //By leewheel 2026-08-30: 修复——TC 343 物品法术在 Effects 中, Spells[] 恒为空, 旧代码读不到物品使用技能
    for (ItemEffectEntry const* effect : item->GetTemplate()->Effects)
    {
        if (effect && effect->SpellID > 0)
        {
            spellId = effect->SpellID;
            break;
        }
    }
    //End By leewheel

    if (!spellId)
        return;

    WorldPacket useItemPacket(CMSG_USE_ITEM);
    WorldPackets::Spells::UseItem useItem(std::move(useItemPacket));
    useItem.PackSlot = bagIndex;
    useItem.Slot = slot;
    useItem.CastItem = item_guid;
    useItem.Cast.SpellID = spellId;
    useItem.Cast.Target.Flags = targetFlag;
    if (targetFlag & TARGET_FLAG_ITEM)
        useItem.Cast.Target.Item = targetGUID;
    else
        useItem.Cast.Target.Unit = targetGUID;
    bot->GetSession()->HandleUseItemOpcode(useItem);
}
//End By leewheel 2026-07-14

void PlayerbotAI::EnchantItemT(uint32 spellid, uint8 slot)
{
    Item* pItem = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
    if (!pItem || !pItem->IsInWorld() || !pItem->GetOwner() || !pItem->GetOwner()->IsInWorld() ||
        !pItem->GetOwner()->GetSession())
        return;

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellid);
    if (!spellInfo)
        return;

    uint32 enchantid = spellInfo->GetEffects()[0].MiscValue;
    if (!enchantid)
    {
        // LOG_ERROR("playerbots", "{}: Invalid enchantid ", enchantid, " report to devs", bot->GetName().c_str());
        return;
    }

    //By leewheel 2026-07-11: TC使用GetSubClass()/GetInventoryType()
    if (!((1 << pItem->GetTemplate()->GetSubClass()) & spellInfo->EquippedItemSubClassMask) &&
        !((1 << pItem->GetTemplate()->GetInventoryType()) & spellInfo->EquippedItemInventoryTypeMask))
    {
        // LOG_ERROR("playerbots", "{}: items could not be enchanted, wrong item type equipped",
        // bot->GetName().c_str());
        return;
    }
    //End By leewheel

    bot->ApplyEnchantment(pItem, PERM_ENCHANTMENT_SLOT, false);
    pItem->SetEnchantment(PERM_ENCHANTMENT_SLOT, enchantid, 0, 0);
    bot->ApplyEnchantment(pItem, PERM_ENCHANTMENT_SLOT, true);

    LOG_INFO("playerbots", "{}: items was enchanted successfully!", bot->GetName().c_str());
}

int32 PlayerbotAI::GetNearGroupMemberCount(float dis)
{
    int count = 1;  // yourself
    if (Group* group = bot->GetGroup())
    {
        for (GroupReference* gref = group->GetFirstMember(); gref; gref = gref->next())
        {
            Player* member = gref->GetSource();
            if (member == bot)  // calculated
                continue;

            if (!member || !member->IsInWorld())
                continue;

            if (member->GetMapId() != bot->GetMapId())
                continue;

            if (member->GetExactDist(bot) > dis)
                continue;

            count++;
        }
    }
    return count;
}

bool PlayerbotAI::CanMove()
{
    // Most common checks: confused, stunned, fleeing, jumping, charging. All these
    // states are set when handling certain aura effects. We don't check against
    // UNIT_STATE_ROOT here, because this state is used by vehicles.
    if (bot->HasUnitState(UNIT_STATE_LOST_CONTROL))
        return false;

    // Death state (w/o spirit release) and Spirit of Redemption aura (priest)
    if ((bot->isDead() && !bot->HasPlayerFlag(PLAYER_FLAGS_GHOST)) || bot->HasSpiritOfRedemptionAura())
        return false;

    // Common CC effects, ordered by frequency: rooted > charmed > frozen > polymorphed.
    // NOTE: Can't find proper way to check if bot is rooted or charmed w/o additional
    // vehicle check -- when a passenger is added, they become rooted and charmed.
    if (!bot->GetVehicle() && (bot->IsRooted() || bot->IsCharmed()))
        return false;
    if (bot->isFrozen() || bot->IsPolymorphed())
        return false;

    // Check for the MM controlled slot types: feared, confused, fleeing, etc.
    //By leewheel 2026-07-11: TC没有MOTION_SLOT_CONTROLLED/GetMotionSlotType/NULL_MOTION_TYPE
    //使用GetCurrentMovementGeneratorType检查CC运动类型
    MovementGeneratorType curMotionType = bot->GetMotionMaster()->GetCurrentMovementGeneratorType(MOTION_SLOT_ACTIVE);
    if (curMotionType == CONFUSED_MOTION_TYPE || curMotionType == FLEEING_MOTION_TYPE || curMotionType == TIMED_FLEEING_MOTION_TYPE)
        return false;
    //End By leewheel

    // Traveling state: taxi flight and being teleported (relatively rare)
    if (bot->IsInFlight() || bot->GetMotionMaster()->GetCurrentMovementGeneratorType() == FLIGHT_MOTION_TYPE ||
        bot->IsBeingTeleported())
        return false;

    // Vehicle state: is in the vehicle and can control it (rare, content-specific)
    if ((bot->GetVehicle() && !IsInVehicle(true)))
        return false;

    return true;
}

bool PlayerbotAI::IsInRealGuild()
{
    if (!bot->GetGuildId())
        return false;

    return PlayerbotGuildMgr::instance().IsRealGuild(bot->GetGuildId());
}

void PlayerbotAI::QueueChatResponse(const ChatQueuedReply chatReply) { chatReplies.push_back(std::move(chatReply)); }

bool PlayerbotAI::EqualLowercaseName(std::string s1, std::string s2)
{
    if (s1.length() != s2.length())
    {
        return false;
    }
    for (std::string::size_type i = 0; i < s1.length(); i++)
    {
        if (tolower(s1[i]) != tolower(s2[i]))
        {
            return false;
        }
    }
    return true;
}

// A custom CanEquipItem (remove AutoUnequipOffhand in FindEquipSlot to prevent unequip on `item usage` calculation)
InventoryResult PlayerbotAI::CanEquipItem(uint8 slot, uint16& dest, Item* pItem, bool swap, bool not_loading) const
{
    dest = 0;
    if (pItem)
    {
        // LOG_DEBUG("entities.player.items", "STORAGE: CanEquipItem slot = {}, item = {}, count = {}", slot,
        //          pItem->GetEntry(), pItem->GetCount());
        ItemTemplate const* pProto = pItem->GetTemplate();
        if (pProto)
        {
            //By leewheel 2026-07-11: TC没有ScriptMgr::OnPlayerCanEquipItem钩子，AC中默认返回true(允许装备)，此处无需替代
            //End By leewheel

            // item used
            if (pItem->m_lootGenerated)
                return EQUIP_ERR_ALREADY_LOOTED;

            if (pItem->IsBindedNotWith(bot))
                return EQUIP_ERR_DONT_OWN_THAT_ITEM;

            InventoryResult res = bot->CanTakeMoreSimilarItems(pItem);
            if (res != EQUIP_ERR_OK)
                return res;

            //By leewheel 2026-07-11: TC使用GetScalingStatDistributionID()替代直接成员访问
            ScalingStatDistributionEntry const* ssd =
                pProto->GetScalingStatDistributionID()
                    ? sScalingStatDistributionStore.LookupEntry(pProto->GetScalingStatDistributionID())
                    : 0;
            //End By leewheel
            // check allowed level (extend range to upper values if MaxLevel more or equal max player level, this let GM
            // set high level with 1...max range items)
            //By leewheel 2026-09-09: TC-Cata的ScalingStatDistributionEntry字段名是Maxlevel(小写l)
            if (ssd && ssd->Maxlevel < DEFAULT_MAX_LEVEL && ssd->Maxlevel < bot->GetLevel())
                return EQUIP_ERR_ITEM_CANT_BE_EQUIPPED;

            uint8 eslot = FindEquipSlot(pProto, slot, swap);
            if (eslot == NULL_SLOT)
                return EQUIP_ERR_ITEM_CANT_BE_EQUIPPED;

            // Xinef: dont allow to equip items on disarmed slot
            //By leewheel 2026-07-11: TC的GetAttackBySlot需要两个参�?slot, inventoryType)
            WeaponAttackType attackType = bot->GetAttackBySlot(eslot, pProto->GetInventoryType());
            if (attackType != MAX_ATTACK && !bot->CanUseAttackType(attackType))
                return EQUIP_ERR_NOT_WHILE_DISARMED;
            //End By leewheel

            res = bot->CanUseItem(pItem, not_loading);
            if (res != EQUIP_ERR_OK)
                return res;

            if (!swap && bot->GetItemByPos(INVENTORY_SLOT_BAG_0, eslot))
                return EQUIP_ERR_NO_EQUIPMENT_SLOT_AVAILABLE;

            // if we are swapping 2 equiped items, CanEquipUniqueItem check
            // should ignore the item we are trying to swap, and not the
            // destination item. CanEquipUniqueItem should ignore destination
            // item only when we are swapping weapon from bag
            uint8 ignore = uint8(NULL_SLOT);
            switch (eslot)
            {
                case EQUIPMENT_SLOT_MAINHAND:
                    ignore = EQUIPMENT_SLOT_OFFHAND;
                    break;
                case EQUIPMENT_SLOT_OFFHAND:
                    ignore = EQUIPMENT_SLOT_MAINHAND;
                    break;
                case EQUIPMENT_SLOT_FINGER1:
                    ignore = EQUIPMENT_SLOT_FINGER2;
                    break;
                case EQUIPMENT_SLOT_FINGER2:
                    ignore = EQUIPMENT_SLOT_FINGER1;
                    break;
                case EQUIPMENT_SLOT_TRINKET1:
                    ignore = EQUIPMENT_SLOT_TRINKET2;
                    break;
                case EQUIPMENT_SLOT_TRINKET2:
                    ignore = EQUIPMENT_SLOT_TRINKET1;
                    break;
            }

            if (ignore == uint8(NULL_SLOT) || pItem != bot->GetItemByPos(INVENTORY_SLOT_BAG_0, ignore))
                ignore = eslot;

            InventoryResult res2 = bot->CanEquipUniqueItem(pItem, swap ? ignore : uint8(NULL_SLOT));
            if (res2 != EQUIP_ERR_OK)
                return res2;

            // check unique-equipped special item classes
            if (pProto->GetClass() == ITEM_CLASS_QUIVER)
                for (uint8 i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; ++i)
                    if (Item* pBag = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
                        if (pBag != pItem)
                            if (ItemTemplate const* pBagProto = pBag->GetTemplate())
                                if (pBagProto->GetClass() == pProto->GetClass() && (!swap || pBag->GetSlot() != eslot))
                                    return (pBagProto->GetSubClass() == ITEM_SUBCLASS_AMMO_POUCH)
                                               ? EQUIP_ERR_CAN_EQUIP_ONLY1_AMMOPOUCH
                                               : EQUIP_ERR_CAN_EQUIP_ONLY1_QUIVER;

            //By leewheel 2026-07-11: TC使用GetInventoryType()/GetSubClass()
            uint32 type = pProto->GetInventoryType();

            if (eslot == EQUIPMENT_SLOT_OFFHAND)
            {
                // Do not allow polearm to be equipped in the offhand (rare case for the only 1h polearm 41750)
                // xinef: same for fishing poles
                if (type == INVTYPE_WEAPON && (pProto->GetSubClass() == ITEM_SUBCLASS_WEAPON_POLEARM ||
                                               pProto->GetSubClass() == ITEM_SUBCLASS_WEAPON_FISHING_POLE))
                    return EQUIP_ERR_ITEM_DOESNT_GO_TO_SLOT;
            //End By leewheel

                else if (type == INVTYPE_WEAPON || type == INVTYPE_WEAPONOFFHAND)
                {
                    if (!bot->CanDualWield())
                        return EQUIP_ERR_CANT_DUAL_WIELD;
                }
                else if (type == INVTYPE_2HWEAPON)
                {
                    if (!bot->CanDualWield() || !bot->CanTitanGrip())
                        return EQUIP_ERR_CANT_DUAL_WIELD;
                }

                if (bot->IsTwoHandUsed())
                    return EQUIP_ERR_CANT_EQUIP_WITH_TWOHANDED;
            }

            // equip two-hand weapon case (with possible unequip 2 items)
            if (type == INVTYPE_2HWEAPON)
            {
                if (eslot == EQUIPMENT_SLOT_OFFHAND)
                {
                    if (!bot->CanTitanGrip())
                        return EQUIP_ERR_ITEM_CANT_BE_EQUIPPED;
                }
                else if (eslot != EQUIPMENT_SLOT_MAINHAND)
                    return EQUIP_ERR_ITEM_CANT_BE_EQUIPPED;

                if (!bot->CanTitanGrip())
                {
                    // offhand item must can be stored in inventory for offhand item and it also must be unequipped
                    Item* offItem = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_OFFHAND);
                    ItemPosCountVec off_dest;
                    if (offItem && (!not_loading ||
                                    bot->CanUnequipItem(uint16(INVENTORY_SLOT_BAG_0) << 8 | EQUIPMENT_SLOT_OFFHAND,
                                                        false) != EQUIP_ERR_OK ||
                                    bot->CanStoreItem(NULL_BAG, NULL_SLOT, off_dest, offItem, false) != EQUIP_ERR_OK))
                        return swap ? EQUIP_ERR_ITEMS_CANT_BE_SWAPPED : EQUIP_ERR_INVENTORY_FULL;
                }
            }
            dest = ((INVENTORY_SLOT_BAG_0 << 8) | eslot);
            return EQUIP_ERR_OK;
        }
    }

    return !swap ? EQUIP_ERR_ITEM_NOT_FOUND : EQUIP_ERR_ITEMS_CANT_BE_SWAPPED;
}

uint8 PlayerbotAI::FindEquipSlot(ItemTemplate const* proto, uint32 slot, bool swap) const
{
    uint8 slots[4];
    slots[0] = NULL_SLOT;
    slots[1] = NULL_SLOT;
    slots[2] = NULL_SLOT;
    slots[3] = NULL_SLOT;
    //By leewheel 2026-07-11: TC使用GetInventoryType()
    switch (proto->GetInventoryType())
    {
        case INVTYPE_HEAD:
            slots[0] = EQUIPMENT_SLOT_HEAD;
            break;
        case INVTYPE_NECK:
            slots[0] = EQUIPMENT_SLOT_NECK;
            break;
        case INVTYPE_SHOULDERS:
            slots[0] = EQUIPMENT_SLOT_SHOULDERS;
            break;
        case INVTYPE_BODY:
            slots[0] = EQUIPMENT_SLOT_BODY;
            break;
        case INVTYPE_CHEST:
        case INVTYPE_ROBE:
            slots[0] = EQUIPMENT_SLOT_CHEST;
            break;
        case INVTYPE_WAIST:
            slots[0] = EQUIPMENT_SLOT_WAIST;
            break;
        case INVTYPE_LEGS:
            slots[0] = EQUIPMENT_SLOT_LEGS;
            break;
        case INVTYPE_FEET:
            slots[0] = EQUIPMENT_SLOT_FEET;
            break;
        case INVTYPE_WRISTS:
            slots[0] = EQUIPMENT_SLOT_WRISTS;
            break;
        case INVTYPE_HANDS:
            slots[0] = EQUIPMENT_SLOT_HANDS;
            break;
        case INVTYPE_FINGER:
            slots[0] = EQUIPMENT_SLOT_FINGER1;
            slots[1] = EQUIPMENT_SLOT_FINGER2;
            break;
        case INVTYPE_TRINKET:
            slots[0] = EQUIPMENT_SLOT_TRINKET1;
            slots[1] = EQUIPMENT_SLOT_TRINKET2;
            break;
        case INVTYPE_CLOAK:
            slots[0] = EQUIPMENT_SLOT_BACK;
            break;
        case INVTYPE_WEAPON:
        {
            slots[0] = EQUIPMENT_SLOT_MAINHAND;

            // suggest offhand slot only if know dual wielding
            // (this will be replace mainhand weapon at auto equip instead unwonted "you don't known dual wielding" ...
            if (bot->CanDualWield())
                slots[1] = EQUIPMENT_SLOT_OFFHAND;
            break;
        }
        case INVTYPE_SHIELD:
        case INVTYPE_WEAPONOFFHAND:
        case INVTYPE_HOLDABLE:
            slots[0] = EQUIPMENT_SLOT_OFFHAND;
            break;
        case INVTYPE_RANGED:
        case INVTYPE_RANGEDRIGHT:
        case INVTYPE_THROWN:
            slots[0] = EQUIPMENT_SLOT_RANGED;
            break;
        case INVTYPE_2HWEAPON:
            slots[0] = EQUIPMENT_SLOT_MAINHAND;
            if (Item* mhWeapon = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND))
            {
                if (ItemTemplate const* mhWeaponProto = mhWeapon->GetTemplate())
                {
                    //By leewheel 2026-07-11: TC使用GetSubClass()
                    if (mhWeaponProto->GetSubClass() == ITEM_SUBCLASS_WEAPON_POLEARM ||
                        mhWeaponProto->GetSubClass() == ITEM_SUBCLASS_WEAPON_STAFF)
                    {
                        bot->AutoUnequipOffhandIfNeed(true);
                        break;
                    }
                }
            }

            if (bot->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_OFFHAND))
            {
                if (proto->GetSubClass() == ITEM_SUBCLASS_WEAPON_POLEARM || proto->GetSubClass() == ITEM_SUBCLASS_WEAPON_STAFF)
                {
                    break;
                }
            }
            if (bot->CanDualWield() && bot->CanTitanGrip() && proto->GetSubClass() != ITEM_SUBCLASS_WEAPON_POLEARM &&
                proto->GetSubClass() != ITEM_SUBCLASS_WEAPON_STAFF && proto->GetSubClass() != ITEM_SUBCLASS_WEAPON_FISHING_POLE)
                slots[1] = EQUIPMENT_SLOT_OFFHAND;
            //End By leewheel
            break;
        case INVTYPE_TABARD:
            slots[0] = EQUIPMENT_SLOT_TABARD;
            break;
        case INVTYPE_WEAPONMAINHAND:
            slots[0] = EQUIPMENT_SLOT_MAINHAND;
            break;
        case INVTYPE_BAG:
            slots[0] = INVENTORY_SLOT_BAG_START + 0;
            slots[1] = INVENTORY_SLOT_BAG_START + 1;
            slots[2] = INVENTORY_SLOT_BAG_START + 2;
            slots[3] = INVENTORY_SLOT_BAG_START + 3;
            break;
        case INVTYPE_RELIC:
        {
            switch (proto->SubClass)
            {
                //By leewheel 2026-07-11: TC没有IsClass/CLASS_CONTEXT_EQUIP_RELIC，使用GetClass()比较
                case ITEM_SUBCLASS_ARMOR_LIBRAM:
                    if (bot->GetClass() == CLASS_PALADIN)
                        slots[0] = EQUIPMENT_SLOT_RANGED;
                    break;
                case ITEM_SUBCLASS_ARMOR_IDOL:
                    if (bot->GetClass() == CLASS_DRUID)
                        slots[0] = EQUIPMENT_SLOT_RANGED;
                    break;
                case ITEM_SUBCLASS_ARMOR_TOTEM:
                    if (bot->GetClass() == CLASS_SHAMAN)
                        slots[0] = EQUIPMENT_SLOT_RANGED;
                    break;
                case ITEM_SUBCLASS_ARMOR_MISC:
                    if (bot->GetClass() == CLASS_WARLOCK)
                        slots[0] = EQUIPMENT_SLOT_RANGED;
                    break;
                case ITEM_SUBCLASS_ARMOR_SIGIL:
                    if (bot->GetClass() == CLASS_DEATH_KNIGHT)
                        slots[0] = EQUIPMENT_SLOT_RANGED;
                    break;
                //End By leewheel
            }
            break;
        }
        default:
            return NULL_SLOT;
    }

    if (slot != NULL_SLOT)
    {
        if (swap || !bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
            for (uint8 i = 0; i < 4; ++i)
                if (slots[i] == slot)
                    return slot;
    }
    else
    {
        // search free slot at first
        for (uint8 i = 0; i < 4; ++i)
            if (slots[i] != NULL_SLOT && !bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slots[i]))
                // in case 2hand equipped weapon (without titan grip) offhand slot empty but not free
                if (slots[i] != EQUIPMENT_SLOT_OFFHAND || !bot->IsTwoHandUsed())
                    return slots[i];

        // if not found free and can swap return first appropriate from used
        for (uint8 i = 0; i < 4; ++i)
            if (slots[i] != NULL_SLOT && swap)
                return slots[i];
    }

    // no free position
    return NULL_SLOT;
}

bool PlayerbotAI::IsSafe(Player* player)
{
    return player && player->GetMapId() == bot->GetMapId() && player->GetInstanceId() == bot->GetInstanceId() &&
           !player->IsBeingTeleported();
}
bool PlayerbotAI::IsSafe(WorldObject* obj)
{
    return obj && obj->GetMapId() == bot->GetMapId() && obj->GetInstanceId() == bot->GetInstanceId() &&
           (!obj->IsPlayer() || !((Player*)obj)->IsBeingTeleported());
}
ChatChannelSource PlayerbotAI::GetChatChannelSource(Player* bot, uint32 type, std::string channelName)
{
    if (type == CHAT_MSG_CHANNEL)
    {
        if (channelName == "World")
            return ChatChannelSource::SRC_WORLD;
        else
        {
            //By leewheel 2026-09-09: TC-Cata的ChannelMgr::ForTeam需要Team类型，用GetTeam()而非GetTeamId()
            ChannelMgr* cMgr = ChannelMgr::ForTeam(bot->GetTeam());
            //End By leewheel
            if (!cMgr)
            {
                return ChatChannelSource::SRC_UNDEFINED;
            }

            const Channel* channel = cMgr->GetChannel(0, channelName, bot);
            if (channel)
            {
                switch (channel->GetChannelId())
                {
                    case ChatChannelId::GENERAL:
                    {
                        return ChatChannelSource::SRC_GENERAL;
                    }
                    case ChatChannelId::TRADE:
                    {
                        return ChatChannelSource::SRC_TRADE;
                    }
                    case ChatChannelId::LOCAL_DEFENSE:
                    {
                        return ChatChannelSource::SRC_LOCAL_DEFENSE;
                    }
                    case ChatChannelId::WORLD_DEFENSE:
                    {
                        return ChatChannelSource::SRC_WORLD_DEFENSE;
                    }
                    case ChatChannelId::LOOKING_FOR_GROUP:
                    {
                        return ChatChannelSource::SRC_LOOKING_FOR_GROUP;
                    }
                    case ChatChannelId::GUILD_RECRUITMENT:
                    {
                        return ChatChannelSource::SRC_GUILD_RECRUITMENT;
                    }
                    default:
                    {
                        return ChatChannelSource::SRC_UNDEFINED;
                    }
                }
            }
        }
    }
    else
    {
        switch (type)
        {
            case CHAT_MSG_WHISPER:
            {
                return ChatChannelSource::SRC_WHISPER;
            }
            case CHAT_MSG_SAY:
            {
                return ChatChannelSource::SRC_SAY;
            }
            case CHAT_MSG_YELL:
            {
                return ChatChannelSource::SRC_YELL;
            }
            case CHAT_MSG_GUILD:
            {
                return ChatChannelSource::SRC_GUILD;
            }
            case CHAT_MSG_PARTY:
            {
                return ChatChannelSource::SRC_PARTY;
            }
            case CHAT_MSG_RAID:
            {
                return ChatChannelSource::SRC_RAID;
            }
            case CHAT_MSG_EMOTE:
            {
                return ChatChannelSource::SRC_EMOTE;
            }
            case CHAT_MSG_TEXT_EMOTE:
            {
                return ChatChannelSource::SRC_TEXT_EMOTE;
            }
            default:
            {
                return ChatChannelSource::SRC_UNDEFINED;
            }
        }
    }
    return ChatChannelSource::SRC_UNDEFINED;
}

bool PlayerbotAI::StarterLevelDistanceCheck(Player* player, const WorldLocation& loc, bool fromStartUp)
{
    if (player->GetLevel() > 16)
        return true;

    float dis = 0.0f;
    if (fromStartUp)
    {
        //By leewheel 2026-07-11: TC的GetRace()不接受bool参数
        PlayerInfo const* pInfo = sObjectMgr->GetPlayerInfo(player->GetRace(), player->GetClass());
        //End By leewheel
        //By leewheel 2026-07-11: TC的PlayerInfo使用createPosition.Loc替代直接成员mapId/positionX/Y/Z
        if (loc.GetMapId() != pInfo->createPosition.Loc.GetMapId())
            return false;
        dis = loc.GetExactDist(pInfo->createPosition.Loc.GetPositionX(), pInfo->createPosition.Loc.GetPositionY(), pInfo->createPosition.Loc.GetPositionZ());
        //End By leewheel
    }
    else
    {
        if (loc.GetMapId() != player->GetMapId())
            return false;
        dis = loc.GetExactDist(player);
    }

    float bound = 10000.0f;
    if (player->GetLevel() <= 4)
        bound = 500.0f;
    else if (player->GetLevel() <= 10)
        bound = 2500.0f;

    return dis <= bound;
}

std::vector<const Quest*> PlayerbotAI::GetAllCurrentQuests()
{
    std::vector<const Quest*> result;

    for (uint16 slot = 0; slot < MAX_QUEST_LOG_SIZE; ++slot)
    {
        uint32 questId = bot->GetQuestSlotQuestId(slot);
        if (!questId)
        {
            continue;
        }

        result.push_back(sObjectMgr->GetQuestTemplate(questId));
    }

    return result;
}

std::vector<const Quest*> PlayerbotAI::GetCurrentIncompleteQuests()
{
    std::vector<const Quest*> result;

    for (uint16 slot = 0; slot < MAX_QUEST_LOG_SIZE; ++slot)
    {
        uint32 questId = bot->GetQuestSlotQuestId(slot);
        if (!questId)
        {
            continue;
        }

        QuestStatus status = bot->GetQuestStatus(questId);
        if (status == QUEST_STATUS_INCOMPLETE || status == QUEST_STATUS_NONE)
        {
            result.push_back(sObjectMgr->GetQuestTemplate(questId));
        }
    }

    return result;
}

std::set<uint32> PlayerbotAI::GetAllCurrentQuestIds()
{
    std::set<uint32> result;

    for (uint16 slot = 0; slot < MAX_QUEST_LOG_SIZE; ++slot)
    {
        uint32 questId = bot->GetQuestSlotQuestId(slot);
        if (!questId)
        {
            continue;
        }

        result.insert(questId);
    }

    return result;
}

std::set<uint32> PlayerbotAI::GetCurrentIncompleteQuestIds()
{
    std::set<uint32> result;

    for (uint16 slot = 0; slot < MAX_QUEST_LOG_SIZE; ++slot)
    {
        uint32 questId = bot->GetQuestSlotQuestId(slot);
        if (!questId)
        {
            continue;
        }

        QuestStatus status = bot->GetQuestStatus(questId);
        if (status == QUEST_STATUS_INCOMPLETE || status == QUEST_STATUS_NONE)
        {
            result.insert(questId);
        }
    }

    return result;
}

uint32 PlayerbotAI::GetReactDelay()
{
    uint32 base = sPlayerbotAIConfig.reactDelay;  // Default 100(ms)

    // If dynamic react delay is disabled, use a static calculation
    if (!sPlayerbotAIConfig.dynamicReactDelay)
    {
        if (HasRealPlayerMaster())
            return base;

        bool inBG = bot->InBattleground() || bot->InArena();

        if (sPlayerbotAIConfig.fastReactInBG && inBG)
            return base;

        bool inCombat = bot->IsInCombat();

        if (!inCombat)
            return base * 10;

        else if (inCombat)
            return static_cast<uint32>(base * 2.5f);

        return base;
    }

    // Dynamic react delay calculation:

    if (HasRealPlayerMaster())
        return base;

    bool inBG = bot->InBattleground() || bot->InArena();

    if (inBG)
    {
        if (bot->IsInCombat() || currentState == BOT_STATE_COMBAT)
        {
            return static_cast<uint32>(base * (sPlayerbotAIConfig.fastReactInBG ? 2.5f : 5.0f));
        }
        else
        {
            return static_cast<uint32>(base * (sPlayerbotAIConfig.fastReactInBG ? 1.0f : 10.0f));
        }
    }

    // When in combat, return 5 times the base
    if (bot->IsInCombat() || currentState == BOT_STATE_COMBAT)
        return base * 5;

    // When not resting, return 10-30 times the base
    //By leewheel 2026-07-11: TC使用HasPlayerFlag替代HasFlag(PLAYER_FLAGS, ...)
    if (!bot->HasPlayerFlag(PLAYER_FLAGS_RESTING))
    //End By leewheel
        return base * urand(10, 30);

    // In other cases, return 20-200 times the base
    return base * urand(20, 200);
}

void PlayerbotAI::PetFollow()
{
    Pet* pet = bot->GetPet();
    if (!pet)
        return;
    //By leewheel 2026-07-11: TC中Pet没有ClearInPetCombat/GetFollowAngle/ClearCastWhenWillAvailable
    pet->AttackStop();
    pet->CastStop();
    pet->GetMotionMaster()->MoveFollow(bot, PET_FOLLOW_DIST, DEFAULT_FOLLOW_ANGLE);
    //End By leewheel
    CharmInfo* charmInfo = pet->GetCharmInfo();
    if (!charmInfo)
        return;
    charmInfo->SetCommandState(COMMAND_FOLLOW);

    charmInfo->SetIsCommandAttack(false);
    charmInfo->SetIsAtStay(false);
    charmInfo->SetIsReturning(true);
    charmInfo->SetIsCommandFollow(true);
    charmInfo->SetIsFollowing(false);
    charmInfo->RemoveStayPosition();
    //By leewheel 2026-09-09: TC-Cata的CharmInfo无SetForcedSpell/SetForcedTargetGUID方法
    // charmInfo->SetForcedSpell(0);
    // charmInfo->SetForcedTargetGUID(ObjectGuid::Empty);
    //End By leewheel
}

float PlayerbotAI::GetItemScoreMultiplier(ItemQualities quality)
{
    switch (quality)
    {
        // each quality increase 1.1x
        case ITEM_QUALITY_POOR:
            return 1.0f;
            break;
        case ITEM_QUALITY_NORMAL:
            return 1.1f;
            break;
        case ITEM_QUALITY_UNCOMMON:
            return 1.21f;
            break;
        case ITEM_QUALITY_RARE:
            return 1.331f;
            break;
        case ITEM_QUALITY_EPIC:
            return 1.4641f;
            break;
        case ITEM_QUALITY_LEGENDARY:
            return 1.61051f;
            break;
        default:
            break;
    }
    return 1.0f;
}

//By leewheel 2026-07-11: TC使用flag128代替AC的flag96
bool PlayerbotAI::IsHealingSpell(uint32 spellFamilyName, flag128 spellFalimyFlags)
{
    if (!spellFamilyName)
        return false;
    flag128 healingFlags;
    switch (spellFamilyName)
    {
        case SPELLFAMILY_DRUID:
        {
            uint32 healingFlagsA = 0x10 | 0x40 | 0x20 | 0x80;  // rejuvenation | regrowth | healing touch | tranquility
            uint32 healingFlagsB = 0x4000000 | 0x2000000 | 0x2 | 0x10;  // wild growth | nourish | swiftmend | lifebloom
            uint32 healingFlagsC = 0x0;
            healingFlags = flag128(healingFlagsA, healingFlagsB, healingFlagsC, 0);
            break;
        }
        case SPELLFAMILY_PALADIN:
        {
            uint32 healingFlagsA = 0x80000000 | 0x40000000 | 0x8000 |
                                   0x80000;  // holy light | flash of light | lay on hands | judgement of light
            uint32 healingFlagsB = 0x10000;  // holy shock
            uint32 healingFlagsC = 0x0;
            healingFlags = flag128(healingFlagsA, healingFlagsB, healingFlagsC, 0);
            break;
        }
        case SPELLFAMILY_SHAMAN:
        {
            uint32 healingFlagsA = 0x80 | 0x40 | 0x100;  // lesser healing wave | healing wave | chain heal
            uint32 healingFlagsB = 0x400;                // earth shield
            uint32 healingFlagsC = 0x10;                 // riptide
            healingFlags = flag128(healingFlagsA, healingFlagsB, healingFlagsC, 0);
            break;
        }
        case SPELLFAMILY_PRIEST:
        {
            uint32 healingFlagsA = 0x40 | 0x200 | 0x40000 | 0x1000 | 0x800 | 0x400 |
                                   0x10000000;  // renew | prayer of healing | lesser heal | greater heal | flash heal |
                                                // heal | circle of healing
            uint32 healingFlagsB = 0x800000 | 0x20 | 0x4;  // penance | prayer of mending | binding heal
            uint32 healingFlagsC = 0x0;
            healingFlags = flag128(healingFlagsA, healingFlagsB, healingFlagsC, 0);
            break;
        }
        default:
            break;
    }
    //By leewheel 2026-07-11: FlagsArray的operator bool是explicit，需显式转换
    return (bool)(spellFalimyFlags & healingFlags);
    //End By leewheel
}

SpellFamilyNames PlayerbotAI::Class2SpellFamilyName(uint8 cls)
{
    switch (cls)
    {
        case CLASS_WARRIOR:
            return SPELLFAMILY_WARRIOR;
        case CLASS_PALADIN:
            return SPELLFAMILY_PALADIN;
        case CLASS_HUNTER:
            return SPELLFAMILY_HUNTER;
        case CLASS_ROGUE:
            return SPELLFAMILY_ROGUE;
        case CLASS_PRIEST:
            return SPELLFAMILY_PRIEST;
        case CLASS_DEATH_KNIGHT:
            return SPELLFAMILY_DEATHKNIGHT;
        case CLASS_SHAMAN:
            return SPELLFAMILY_SHAMAN;
        case CLASS_MAGE:
            return SPELLFAMILY_MAGE;
        case CLASS_WARLOCK:
            return SPELLFAMILY_WARLOCK;
        case CLASS_DRUID:
            return SPELLFAMILY_DRUID;
        default:
            break;
    }
    return SPELLFAMILY_GENERIC;
}

void PlayerbotAI::AddTimedEvent(std::function<void()> callback, uint32 delayMs)
{
    class LambdaEvent final : public BasicEvent
    {
        std::function<void()> _cb;

    public:
        explicit LambdaEvent(std::function<void()> cb) : _cb(std::move(cb)) {}
        bool Execute(uint64 /*execTime*/, uint32 /*diff*/) override
        {
            _cb();
            return true;  // remove after execution
        }
    };

    // Every Player already owns an EventMap called m_Events
    //By leewheel 2026-07-11: TC的CalculateTime接受Milliseconds而非uint32
    bot->m_Events.AddEvent(new LambdaEvent(std::move(callback)), bot->m_Events.CalculateTime(Milliseconds(delayMs)));
    //End By leewheel
}

void PlayerbotAI::EvaluateHealerDpsStrategy()
{
    if (!IsHeal(bot, true))
        return;

    if (sPlayerbotAIConfig.IsRestrictedHealerDPSMap(bot->GetMapId()))
        ChangeStrategy("-healer dps", BOT_STATE_COMBAT);
    else
        ChangeStrategy("+healer dps", BOT_STATE_COMBAT);
}
