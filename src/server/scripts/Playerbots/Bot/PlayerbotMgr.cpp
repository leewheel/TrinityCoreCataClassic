//By leewheel 2026-07-12
// 机器人管理器完整实现
// 从 AzerothCore mod-playerbots 移植到 TrinityCore
// TC适配: WorldSession构造函数、HandlePlayerLogin、SaveToDB、ObjectGuid等

//By leewheel 2026-08-01: 日志清理——注释掉机器人会话/登录过程DEBUG与INFO，仅保留错误与警告
//End By leewheel

#include "PlayerbotMgr.h"

#include <cstdio>
#include <cstring>
#include <sstream> //By leewheel 2026-08-15: 账号关联命令的ostringstream
#include <string>
#include <unordered_set>
#include <openssl/sha.h>
#include <iomanip>
#include <algorithm>

#include "ChannelMgr.h"
#include "Channel.h" //By leewheel 2026-07-12: TC的ChannelMgr.h只前向声明Channel，需要完整定义
#include "CharacterCache.h"
#include "CharacterPackets.h"
#include "Common.h"
#include "Config.h" //By leewheel 2026-07-12: 提供sConfigMgr定义
#include "Define.h"
#include "Group.h"
#include "GuildMgr.h"
#include "ObjectAccessor.h"
#include "ObjectGuid.h"
#include "ObjectMgr.h"
#include "PlayerbotAIConfig.h"
#include "PlayerbotRepository.h"
#include "PlayerbotFactory.h"
#include "PlayerbotOperations.h"
#include "PlayerbotSecurity.h"
#include "PlayerbotTextMgr.h"
#include "PlayerbotWorldThreadProcessor.h"
#include "Playerbots.h"
#include "PlayerbotGuildMgr.h"
#include "RandomPlayerbotMgr.h"
#include "SharedDefines.h"
#include "WorldSession.h"
#include "BroadcastHelper.h"
#include "WorldSessionMgr.h"
#include "DatabaseEnv.h"
#include "Opcodes.h"
#include "AccountMgr.h"
#include "Helpers.h"

// BotInitGuard - 防止重复初始化同一个机器人
class BotInitGuard
{
public:
    BotInitGuard(ObjectGuid guid) : guid(guid), active(false)
    {
        if (botsBeingInitialized.find(guid) == botsBeingInitialized.end())
        {
            botsBeingInitialized.insert(guid);
            active = true;
        }
    }

    ~BotInitGuard()
    {
        if (active)
            botsBeingInitialized.erase(guid);
    }

    bool IsLocked() const { return !active; }

private:
    ObjectGuid guid;
    bool active;
    static std::unordered_set<ObjectGuid> botsBeingInitialized;
};

std::unordered_set<ObjectGuid> BotInitGuard::botsBeingInitialized;
std::unordered_map<ObjectGuid, uint32> PlayerbotHolder::botLoading;

PlayerbotHolder::PlayerbotHolder() : PlayerbotAIBase(false) {}

//By leewheel 2026-07-12: PlayerbotHolder析构函数定义（头文件中声明但未定义，导致LNK2001）
PlayerbotHolder::~PlayerbotHolder() {}
//End By leewheel

// === AddPlayerBot - 机器人登录核心函数 ===
void PlayerbotHolder::AddPlayerBot(ObjectGuid playerGuid, uint32 masterAccountId)
{
    if (botLoading.find(playerGuid) != botLoading.end())
        return;

    // 已经在线的机器人不需要重复添加
    Player* bot = ObjectAccessor::FindConnectedPlayer(playerGuid);
    if (bot && bot->IsInWorld())
        return;

    uint32 accountId = sCharacterCache->GetCharacterAccountIdByGuid(playerGuid);
    if (!accountId)
        return;

    WorldSession* masterSession = masterAccountId ? sWorldSessionMgr->FindSession(masterAccountId) : nullptr;
    Player* masterPlayer = masterSession ? masterSession->GetPlayer() : nullptr;

    bool isRndbot = !masterAccountId;
    bool sameAccount = sPlayerbotAIConfig.allowAccountBots && accountId == masterAccountId;
    Guild* guild = masterPlayer ? sGuildMgr->GetGuildById(masterPlayer->GetGuildId()) : nullptr;
    //By leewheel 2026-09-09: TC的Guild::GetMember是私有方法，用IsMember判断公会成员即可
    bool sameGuild = sPlayerbotAIConfig.allowGuildBots && guild && guild->IsMember(playerGuid);
    bool addClassBot = sRandomPlayerbotMgr.IsAddclassBot(playerGuid.GetCounter());
    bool linkedAccount = sPlayerbotAIConfig.allowTrustedAccountBots && IsAccountLinked(accountId, masterAccountId);
    //By leewheel 2026-07-20: 快速组队系统需要能添加随机账号中的离线机器人
    //IsRandomBot要求角色已在currentBots中（已上线），但FastGroup选的是离线角色
    //改用IsInRandomAccountList只检查账号级别，允许FastGroup添加随机账号的离线角色
    bool isRandomAccountBot = sPlayerbotAIConfig.IsInRandomAccountList(accountId);
    //End By leewheel

    bool allowed = true;
    std::ostringstream out;
    std::string botName;
    sCharacterCache->GetCharacterNameByGuid(playerGuid, botName);
    if (!isRndbot && !sameAccount && !sameGuild && !addClassBot && !linkedAccount && !isRandomAccountBot)
    {
        allowed = false;
        //By leewheel 2026-08-01: 玩家可见文本中文化
        out << "失败：你无权控制机器人 " << botName.c_str();
        //End By leewheel
    }
    if (masterAccountId && masterPlayer)
    {
        PlayerbotMgr* mgr = GET_PLAYERBOT_MGR(masterPlayer);
        if (!mgr)
        {
            // TC_LOG_DEBUG("playerbots", "PlayerbotMgr not found for master player with GUID: {}", masterPlayer->GetGUID().GetCounter());
            return;
        }
        uint32 loadingForMaster = 0;
        for (auto const& [guid, acctId] : botLoading)
        {
            if (acctId == masterAccountId)
                ++loadingForMaster;
        }
        uint32 count = mgr->GetPlayerbotsCount() + loadingForMaster;
        if (count >= uint32(PlayerbotAIConfig::instance().maxAddedBots))
        {
            allowed = false;
            //By leewheel 2026-08-01: 玩家可见文本中文化
            out << "失败：你添加的机器人太多了（超过 " << sPlayerbotAIConfig.maxAddedBots << " 个）";
            //End By leewheel
        }
    }
    if (!allowed)
    {
        if (masterSession)
        {
            ChatHandler ch(masterSession);
            ch.SendSysMessage(out.str());
        }
        return;
    }

    std::shared_ptr<PlayerbotLoginQueryHolder> holder =
        std::make_shared<PlayerbotLoginQueryHolder>(masterAccountId, accountId, playerGuid);
    if (!holder->Initialize())
    {
        return;
    }

    botLoading.emplace(playerGuid, masterAccountId);

    //By leewheel 2026-09-09: TC-Cata的World无AddQueryHolderCallback方法
    //使用同步方式执行查询并调用回调（机器人登录不频繁，同步可接受）
    SQLQueryHolderCallback queryCallback = CharacterDatabase.DelayQueryHolder(holder);
    queryCallback.AfterComplete(
        [](SQLQueryHolderBase const& queryHolder)
        {
            PlayerbotLoginQueryHolder const& holder = static_cast<PlayerbotLoginQueryHolder const&>(queryHolder);
            uint32 masterAccountId = holder.GetMasterAccountId();

            if (masterAccountId)
            {
                // 验证并找到主人的当前WorldSession
                WorldSession* masterSession = sWorldSessionMgr->FindSession(masterAccountId);
                Player* masterPlayer = masterSession ? masterSession->GetPlayer() : nullptr;

                if (masterPlayer)
                {
                    PlayerbotHolder* mgr = PlayerbotsMgr::instance().GetPlayerbotMgr(masterPlayer);

                    if (mgr != nullptr)
                    {
                        mgr->HandlePlayerBotLoginCallback(holder);
                        return;
                    }

                    PlayerbotHolder::botLoading.erase(holder.GetGuid());
                    return;
                }
            }

            sRandomPlayerbotMgr.HandlePlayerBotLoginCallback(holder);
        });
    // 等待查询完成并同步执行回调
    queryCallback.m_future.wait();
    queryCallback.InvokeIfReady();
}

// === IsAccountLinked - 检查账号是否关联 ===
bool PlayerbotHolder::IsAccountLinked(uint32 accountId, uint32 linkedAccountId)
{
    //By leewheel 2026-07-12: TC的Query只接受const char*，格式化查询需用PQuery
    QueryResult result = PlayerbotsDatabase.PQuery(
        "SELECT 1 FROM playerbots_account_links WHERE account_id = {} AND linked_account_id = {}", accountId, linkedAccountId);
    //End By leewheel
    return result != nullptr;
}

// === HandlePlayerBotLoginCallback - 登录回调处理 ===
void PlayerbotHolder::HandlePlayerBotLoginCallback(PlayerbotLoginQueryHolder const& holder)
{
    uint32 botAccountId = holder.GetAccountId();

    // TC适配: WorldSession构造函数参数不同（14个参数）
    //By leewheel 2026-09-09: TC-Cata的WorldSession构造函数需14个参数，新增build和clientBuildVariant
    WorldSession* botSession =
        new WorldSession(botAccountId, "", 0x0, nullptr, SEC_PLAYER, EXPANSION_WRATH_OF_THE_LICH_KING, time_t(0),
                         "", Minutes(0), 0, ClientBuild::VariantId{0, 0, 0}, sWorld->GetDefaultDbcLocale(), 0, false);
    // 标记为机器人会话
    botSession->SetBot(true);

    // TC适配: HandlePlayerLoginFromDB → HandlePlayerLogin
    botSession->HandlePlayerLogin(holder);

    Player* bot = botSession->GetPlayer();
    if (!bot)
    {
        //By leewheel 2026-07-13: 修复bot加载失败后无限重试的问题
        //原代码只清理botLoading,不从currentBots和playerbots_random_bots中移除
        //导致ProcessBot反复尝试登录同一个失败的角色,形成无限循环
        //修复: 调用OnPlayerLoginError移除该bot的调度记录
        uint32 botGuid = holder.GetGuid().GetCounter();
        TC_LOG_ERROR("playerbots",
            "Bot加载失败! account={}, guid={}, master={}. "
            "可能原因: 1)角色不存在 2)账号不匹配 3)AT_LOGIN_RENAME标记 4)homebind缺失 5)map无效",
            botAccountId, botGuid, holder.GetMasterAccountId());

        botSession->LogoutPlayer(true);
        delete botSession;
        PlayerbotHolder::botLoading.erase(holder.GetGuid());

        // 对于随机bot(无master),调用OnPlayerLoginError清理调度记录
        if (!holder.GetMasterAccountId())
        {
            sRandomPlayerbotMgr.OnPlayerLoginError(botGuid);
        }
        //End By leewheel
        return;
    }

    uint32 masterAccountId = holder.GetMasterAccountId();
    WorldSession* masterSession = masterAccountId ? sWorldSessionMgr->FindSession(masterAccountId) : nullptr;

    Player* masterPlayer = masterSession ? masterSession->GetPlayer() : nullptr;
    if (masterSession && !masterPlayer)
    {
        // TC_LOG_DEBUG("playerbots", "Master session found but no player is associated for master account ID: {}",
        //           masterAccountId);
    }

    sRandomPlayerbotMgr.OnPlayerLogin(bot);
    auto op = std::make_unique<OnBotLoginOperation>(bot->GetGUID(), masterAccountId);
    PlayerbotWorldThreadProcessor::instance().QueueOperation(std::move(op));

    PlayerbotHolder::botLoading.erase(holder.GetGuid());
}

// === UpdateSessions - 更新所有机器人会话 ===
void PlayerbotHolder::UpdateSessions()
{
    for (PlayerBotMap::const_iterator itr = GetPlayerBotsBegin(); itr != GetPlayerBotsEnd(); ++itr)
    {
        Player* const bot = itr->second;
        if (!bot)
            continue;

        if (bot->IsBeingTeleported())
        {
            PlayerbotAI* botAI = GET_PLAYERBOT_AI(bot);
            if (botAI)
            {
                botAI->HandleTeleportAck();
            }
        }
        else if (bot->IsInWorld())
        {
            HandleBotPackets(bot->GetSession());
        }
    }
}

// === HandleBotPackets - 处理机器人数据包 ===
//By leewheel 2026-07-14: 添加try-catch防护，防止旧格式包导致ByteBufferException崩溃
void PlayerbotHolder::HandleBotPackets(WorldSession* session)
{
    WorldPacket* packet;
    while (session->GetPacketQueue().next(packet))
    {
        OpcodeClient opcode = static_cast<OpcodeClient>(packet->GetOpcode());
        ClientOpcodeHandler const* opHandle = opcodeTable[opcode];
        if (!opHandle)
        {
            //By leewheel 2026-07-20: bot session无真实客户端，无handler的opcode(如0)丢弃即可，降为DEBUG避免刷屏
            // TC_LOG_DEBUG("playerbots", "Unhandled opcode {} queued for bot session {}. Packet dropped.", static_cast<uint32>(opcode), session->GetAccountId());
            //End By leewheel
            delete packet;
            continue;
        }
        try
        {
            opHandle->Call(session, *packet);
        }
        catch (ByteBufferException const& e)
        {
            TC_LOG_ERROR("playerbots", "ByteBufferException in HandleBotPackets for opcode {} bot session {}: {}", static_cast<uint32>(opcode), session->GetAccountId(), e.what());
        }
        catch (std::exception const& e)
        {
            TC_LOG_ERROR("playerbots", "Exception in HandleBotPackets for opcode {} bot session {}: {}", static_cast<uint32>(opcode), session->GetAccountId(), e.what());
        }
        delete packet;
    }
}
//End By leewheel 2026-07-14

// === LogoutAllBots - 登出所有机器人 ===
void PlayerbotHolder::LogoutAllBots()
{
    PlayerBotMap bots = playerBots;
    for (auto& itr : bots)
    {
        Player* bot = itr.second;
        if (!bot)
            continue;

        PlayerbotAI* botAI = GET_PLAYERBOT_AI(bot);
        if (!botAI || botAI->IsRealPlayer())
            continue;

        LogoutPlayerBot(bot->GetGUID());
    }
}

// === CancelLogout - 取消所有机器人的登出 ===
void PlayerbotMgr::CancelLogout()
{
    Player* master = GetMaster();
    if (!master)
        return;

    for (PlayerBotMap::const_iterator it = GetPlayerBotsBegin(); it != GetPlayerBotsEnd(); ++it)
    {
        Player* const bot = it->second;
        PlayerbotAI* botAI = GET_PLAYERBOT_AI(bot);
        if (!botAI || botAI->IsRealPlayer())
            continue;

        if (bot->GetSession()->isLogingOut())
        {
            // TC适配: 使用两步构造WorldPackets
            WorldPacket data(CMSG_LOGOUT_CANCEL);
            WorldPackets::Character::LogoutCancel logoutCancel(std::move(data));
            bot->GetSession()->HandleLogoutCancelOpcode(logoutCancel);
            botAI->TellMaster(PlayerbotTextMgr::instance().GetBotTextOrDefault(
                "logout_cancel", "已取消注销！", {}));
        }
    }

    for (PlayerBotMap::const_iterator it = sRandomPlayerbotMgr.GetPlayerBotsBegin();
         it != sRandomPlayerbotMgr.GetPlayerBotsEnd(); ++it)
    {
        Player* const bot = it->second;
        PlayerbotAI* botAI = GET_PLAYERBOT_AI(bot);
        if (!botAI || botAI->IsRealPlayer())
            continue;

        if (botAI->GetMaster() != master)
            continue;

        if (bot->GetSession()->isLogingOut())
        {
            WorldPacket data(CMSG_LOGOUT_CANCEL);
            WorldPackets::Character::LogoutCancel logoutCancel(std::move(data));
            bot->GetSession()->HandleLogoutCancelOpcode(logoutCancel);
        }
    }
}

// === LogoutPlayerBot - 登出单个机器人 ===
void PlayerbotHolder::LogoutPlayerBot(ObjectGuid guid)
{
    if (Player* bot = GetPlayerBot(guid))
    {
        PlayerbotAI* botAI = GET_PLAYERBOT_AI(bot);
        if (!botAI)
            return;

        // 将组队清理操作排入世界线程
        auto cleanupOp = std::make_unique<BotLogoutGroupCleanupOperation>(guid);
        PlayerbotWorldThreadProcessor::instance().QueueOperation(std::move(cleanupOp));

        // TC_LOG_DEBUG("playerbots", "Bot {} logging out", bot->GetName().c_str());

        // 移除出租车作弊标记
        if (!sRandomPlayerbotMgr.IsRandomBot(bot) && bot->isTaxiCheater())
            bot->SetTaxiCheater(false);

        // TC适配: SaveToDB只接受1个bool参数
        bot->SaveToDB(false);

        WorldSession* botWorldSessionPtr = bot->GetSession();

        if (botWorldSessionPtr->isLogingOut())
            return;

        // 即时登出
        {
            std::string message = PlayerbotTextMgr::instance().GetBotTextOrDefault(
                "goodbye", "Goodbye!", {});
            botAI->TellMaster(message);
            RemoveFromPlayerbotsMap(guid);
            botWorldSessionPtr->LogoutPlayer(true);
            delete botWorldSessionPtr;
        }
    }
}

// === DisablePlayerBot - 禁用机器人（不删除会话） ===
void PlayerbotHolder::DisablePlayerBot(ObjectGuid guid)
{
    if (Player* bot = GetPlayerBot(guid))
    {
        PlayerbotAI* botAI = GET_PLAYERBOT_AI(bot);
        if (!botAI)
        {
            return;
        }
        botAI->TellMaster(PlayerbotTextMgr::instance().GetBotTextOrDefault(
            "goodbye", "Goodbye!", {}));
        bot->StopMoving();
        bot->GetMotionMaster()->Clear();

        Group* group = bot->GetGroup();
        if (group && !bot->InBattleground() && !bot->InBattlegroundQueue() && botAI->HasActivePlayerMaster())
        {
            PlayerbotRepository::instance().Save(botAI);
        }

        // TC_LOG_DEBUG("playerbots", "Bot {} logged out", bot->GetName().c_str());

        // TC适配: SaveToDB只接受1个bool参数
        bot->SaveToDB(false);

        //By leewheel 2026-08-09: 移植上游7c41f51d(#2598)——删除手动delete TravelTarget，
        //TravelTargetValue析构拥有该指针所有权(~TravelTargetValue{ delete value; })，botAI析构
        //(AiObjectContext→TravelTargetValue)时会释放它；此处手动delete导致同一块内存二次释放
        //(double free)，在bot在线时删除其账号(account delete)即可触发崩溃。
        //End By leewheel

        RemoveFromPlayerbotsMap(guid);

        delete botAI;
    }
}

// === RemoveFromPlayerbotsMap ===
void PlayerbotHolder::RemoveFromPlayerbotsMap(ObjectGuid guid)
{
    playerBots.erase(guid);
}

// === GetPlayerBot ===
Player* PlayerbotHolder::GetPlayerBot(ObjectGuid playerGuid) const
{
    PlayerBotMap::const_iterator it = playerBots.find(playerGuid);
    return (it == playerBots.end()) ? nullptr : it->second;
}

// === OnBotLogin - 机器人登录后初始化 ===
void PlayerbotHolder::OnBotLogin(Player* const bot)
{
    //By leewheel 2026-07-14: 添加诊断日志，追踪OnBotLogin调用
    //static uint32 callCount = 0;
    //callCount++;
    //TC_LOG_INFO("playerbots", "OnBotLogin被调用: bot={}, GUID={}, 总调用次数={}", 
    //    bot->GetName(), bot->GetGUID().GetCounter(), callCount);
    //End By leewheel

    // 防止重复登录
    if (playerBots.find(bot->GetGUID()) != playerBots.end())
    {
        TC_LOG_WARN("playerbots", "OnBotLogin: bot={}已经在playerBots中，跳过", bot->GetName());
        return;
    }

    PlayerbotsMgr::instance().AddPlayerbotData(bot, true);
    playerBots[bot->GetGUID()] = bot;

    OnBotLoginInternal(bot);

    PlayerbotAI* botAI = GET_PLAYERBOT_AI(bot);
    if (!botAI)
    {
        TC_LOG_ERROR("playerbots", "OnBotLogin: PlayerbotAI is null for bot with GUID: {}", bot->GetGUID().GetCounter());
        return;
    }

    Player* master = botAI->GetMaster();

    // 检查队伍有效性
    Group* group = bot->GetGroup();
    if (group)
    {
        bool groupValid = false;
        Group::MemberSlotList const& slots = group->GetMemberSlots();
        for (Group::MemberSlotList::const_iterator i = slots.begin(); i != slots.end(); ++i)
        {
            ObjectGuid member = i->guid;
            if (master)
            {
                if (master->GetGUID() == member)
                {
                    groupValid = true;
                    break;
                }
            }

            if (sPlayerbotAIConfig.KeepAltsInGroup())
            {
                uint32 account = sCharacterCache->GetCharacterAccountIdByGuid(member);
                if (!sPlayerbotAIConfig.IsInRandomAccountList(account))
                {
                    groupValid = true;
                    break;
                }
            }
        }

        if (!groupValid)
        {
            botAI->LeaveOrDisbandGroup();
        }
    }

    group = bot->GetGroup();
    if (group)
    {
        botAI->ResetStrategies();
    }
    else
    {
        botAI->ResetStrategies(!sRandomPlayerbotMgr.IsRandomBot(bot));
    }
    PlayerbotRepository::instance().Load(botAI);

    if (master && !master->HasUnitState(UNIT_STATE_IN_FLIGHT))
    {
        //By leewheel 2026-07-12: TC的MotionMaster没有MovementExpired，用Clear()替代
        bot->GetMotionMaster()->Clear();
        //End By leewheel
        bot->CleanupAfterTaxiFlight();
    }

    // 检查活动状态
    botAI->AllowActivity(ALL_ACTIVITY, true);

    // 设置登录延迟
    botAI->SetNextCheckDelay(urand(2000, 4000));

    botAI->TellMaster(PlayerbotTextMgr::instance().GetBotTextOrDefault(
        "hello", "Hello!", {}), PLAYERBOT_SECURITY_TALK);

    // 排入组队操作到世界线程
    if (master && master->GetGroup() && !group)
    {
        Group* mgroup = master->GetGroup();
        if (mgroup->GetMembersCount() >= 5)
        {
            if (mgroup->isRaidGroup() || (!mgroup->isLFGGroup() && !mgroup->isBGGroup() && !mgroup->isBFGroup()))
            {
                auto addOp = std::make_unique<GroupInviteOperation>(master->GetGUID(), bot->GetGUID());
                PlayerbotWorldThreadProcessor::instance().QueueOperation(std::move(addOp));
            }
        }
        else
        {
            auto addOp = std::make_unique<GroupInviteOperation>(master->GetGUID(), bot->GetGUID());
            PlayerbotWorldThreadProcessor::instance().QueueOperation(std::move(addOp));
        }
    }
    else if (master && !group)
    {
        auto inviteOp = std::make_unique<GroupInviteOperation>(master->GetGUID(), bot->GetGUID());
        PlayerbotWorldThreadProcessor::instance().QueueOperation(std::move(inviteOp));
    }

    uint32 accountId = bot->GetSession()->GetAccountId();
    bool isRandomAccount = sPlayerbotAIConfig.IsInRandomAccountList(accountId);

    if (isRandomAccount && sPlayerbotAIConfig.randomBotFixedLevel)
    {
        bot->SetPlayerFlag(PLAYER_FLAGS_NO_XP_GAIN);
    }
    else if (isRandomAccount && !sPlayerbotAIConfig.randomBotFixedLevel)
    {
        bot->RemovePlayerFlag(PLAYER_FLAGS_NO_XP_GAIN);
    }

    // TC适配: SaveToDB只接受1个bool参数
    bot->SaveToDB(false);
    bool addClassBot = sRandomPlayerbotMgr.IsAccountType(accountId, 2);
    if (addClassBot && master && abs((int)master->GetLevel() - (int)bot->GetLevel()) > 3)
    {
        uint32 mixedGearScore =
            PlayerbotAI::GetMixedGearScore(master, true, false, 12) * sPlayerbotAIConfig.autoInitEquipLevelLimitRatio;
        if (mixedGearScore == 0)
            mixedGearScore = 1;
        PlayerbotFactory factory(bot, master->GetLevel(), ITEM_QUALITY_LEGENDARY, mixedGearScore);
        factory.Randomize(false);
    }

    // 机器人加入World频道（如果不是solo类型）
    if (bot->GetLevel() >= 10 && sRandomPlayerbotMgr.IsRandomBot(bot) && GET_PLAYERBOT_AI(bot) &&
        GET_PLAYERBOT_AI(bot)->GetGrouperType() != GrouperType::SOLO)
    {
        // TC适配: 使用直接Channel API而非数据包处理器
        ChannelMgr* cMgr = ChannelMgr::ForTeam(bot->GetTeam());
        if (cMgr)
        {
            if (Channel* worldChannel = cMgr->GetCustomChannel("World"))
                worldChannel->JoinChannel(bot);
            else if (Channel* worldChannel = cMgr->CreateCustomChannel("World"))
                worldChannel->JoinChannel(bot);
        }
    }

    // 加入标准频道 - TC会自动处理，此处跳过手动加入
    // 原AC代码通过sChatChannelsStore遍历并手动加入各频道
    // TC的Player::UpdateLocalChannels会自动处理标准频道加入
}

// === ProcessBotCommand - 处理机器人命令 ===
std::string const PlayerbotHolder::ProcessBotCommand(std::string const cmd, ObjectGuid guid, ObjectGuid masterguid,
                                                     bool admin, uint32 masterAccountId, uint32)
{
    if (!sPlayerbotAIConfig.enabled || guid.IsEmpty())
        //By leewheel 2026-08-01: 玩家可见文本中文化
        return "机器人系统已禁用";
        //End By leewheel

    if (cmd == "add" || cmd == "addaccount" || cmd == "login")
    {
        if (ObjectAccessor::FindPlayer(guid))
            //By leewheel 2026-08-01: 玩家可见文本中文化
            return "玩家已登录";
            //End By leewheel

        if (cmd == "addaccount")
        {
            uint32 accountId = sCharacterCache->GetCharacterAccountIdByGuid(guid);
            if (!accountId)
            {
                //By leewheel 2026-08-01: 玩家可见文本中文化
                return "角色未找到";
                //End By leewheel
            }

            if (!sPlayerbotAIConfig.allowAccountBots && accountId != masterAccountId &&
                !(sPlayerbotAIConfig.allowTrustedAccountBots && IsAccountLinked(accountId, masterAccountId)))
            {
                //By leewheel 2026-08-01: 玩家可见文本中文化
                return "你只能添加自己账号或关联账号的机器人";
                //End By leewheel
            }
        }

        AddPlayerBot(guid, masterAccountId);
        return "ok";
    }
    else if (cmd == "remove" || cmd == "logout" || cmd == "rm")
    {
        if (!ObjectAccessor::FindPlayer(guid))
            //By leewheel 2026-08-01: 玩家可见文本中文化
            return "玩家离线";
            //End By leewheel

        if (!GetPlayerBot(guid))
            //By leewheel 2026-08-01: 玩家可见文本中文化
            return "不是你的机器人";
            //End By leewheel

        LogoutPlayerBot(guid);
        return "ok";
    }

    Player* bot = GetPlayerBot(guid);
    if (!bot)
        bot = sRandomPlayerbotMgr.GetPlayerBot(guid);

    if (!bot)
        //By leewheel 2026-08-01: 玩家可见文本中文化
        return "未找到机器人";
        //End By leewheel

    bool addClassBot = sRandomPlayerbotMgr.IsAddclassBot(guid.GetCounter());

    if (!addClassBot)
    {
        if (!(cmd == "refresh=raid" && sPlayerbotAIConfig.resetInstanceIdForAltBots))
            //By leewheel 2026-08-01: 玩家可见文本中文化
            return "错误：此命令仅可用于 addclass 机器人。";
            //End By leewheel
    }

    if (!admin)
    {
        Player* master = ObjectAccessor::FindConnectedPlayer(masterguid);
        if (master && (master->IsInCombat() || bot->IsInCombat()))
        {
            //By leewheel 2026-08-01: 玩家可见文本中文化
            return "错误：战斗中无法使用此命令。";
            //End By leewheel
        }
    }

    if (GET_PLAYERBOT_AI(bot))
    {
        if (Player* master = GET_PLAYERBOT_AI(bot)->GetMaster())
        {
            if (master->GetSession()->GetSecurity() <= SEC_PLAYER && sPlayerbotAIConfig.autoInitOnly &&
                cmd != "init=auto")
            {
                //By leewheel 2026-08-01: 玩家可见文本中文化
                return "不允许使用该命令，请改用 init=auto。";
                //End By leewheel
            }

            BotInitGuard guard(bot->GetGUID());
            if (guard.IsLocked())
            {
                //By leewheel 2026-08-01: 玩家可见文本中文化
                return "初始化正在进行中，请稍候。";
                //End By leewheel
            }

            int gs;
            if (cmd == "init=white" || cmd == "init=common")
            {
                PlayerbotFactory factory(bot, master->GetLevel(), ITEM_QUALITY_NORMAL);
                factory.Randomize(false);
                return "ok";
            }
            else if (cmd == "init=green" || cmd == "init=uncommon")
            {
                PlayerbotFactory factory(bot, master->GetLevel(), ITEM_QUALITY_UNCOMMON);
                factory.Randomize(false);
                return "ok";
            }
            else if (cmd == "init=blue" || cmd == "init=rare")
            {
                PlayerbotFactory factory(bot, master->GetLevel(), ITEM_QUALITY_RARE);
                factory.Randomize(false);
                return "ok";
            }
            else if (cmd == "init=epic" || cmd == "init=purple")
            {
                PlayerbotFactory factory(bot, master->GetLevel(), ITEM_QUALITY_EPIC);
                factory.Randomize(false);
                return "ok";
            }
            else if (cmd == "init=legendary" || cmd == "init=yellow")
            {
                PlayerbotFactory factory(bot, master->GetLevel(), ITEM_QUALITY_LEGENDARY);
                factory.Randomize(false);
                return "ok";
            }
            else if (cmd == "init=auto")
            {
                uint32 mixedGearScore = PlayerbotAI::GetMixedGearScore(master, true, false, 12) *
                                        sPlayerbotAIConfig.autoInitEquipLevelLimitRatio;
                if (mixedGearScore == 0)
                    mixedGearScore = 1;
                PlayerbotFactory factory(bot, master->GetLevel(), ITEM_QUALITY_LEGENDARY, mixedGearScore);
                factory.Randomize(false);
                //By leewheel 2026-08-01: 玩家可见文本中文化
                return "成功，装备分数上限: " + std::to_string(mixedGearScore / PlayerbotAI::GetItemScoreMultiplier(ItemQualities(ITEM_QUALITY_EPIC))) +
                       "（史诗级）";
                //End By leewheel
            }
            else if (cmd.starts_with("init=") && sscanf(cmd.c_str(), "init=%d", &gs) != -1)
            {
                PlayerbotFactory factory(bot, master->GetLevel(), ITEM_QUALITY_LEGENDARY, gs);
                factory.Randomize(false);
                //By leewheel 2026-08-01: 玩家可见文本中文化
                return "成功，装备分数上限: " + std::to_string(gs / PlayerbotAI::GetItemScoreMultiplier(ItemQualities(ITEM_QUALITY_EPIC))) + "（史诗级）";
                //End By leewheel
            }
        }

        if (cmd == "refresh=raid")
        {
            PlayerbotFactory factory(bot, bot->GetLevel());
            factory.UnbindInstance();
            return "ok";
        }
    }

    if (cmd == "levelup" || cmd == "level")
    {
        PlayerbotFactory factory(bot, bot->GetLevel());
        factory.Randomize(true);
        return "ok";
    }
    else if (cmd == "refresh")
    {
        PlayerbotFactory factory(bot, bot->GetLevel());
        factory.Refresh();
        return "ok";
    }
    else if (cmd == "random")
    {
        sRandomPlayerbotMgr.Randomize(bot);
        return "ok";
    }
    else if (cmd == "quests")
    {
        PlayerbotFactory factory(bot, bot->GetLevel());
        factory.InitInstanceQuests();
        //By leewheel 2026-08-01: 玩家可见文本中文化
        return "初始化任务中";
        //End By leewheel
    }

    //By leewheel 2026-08-01: 玩家可见文本中文化
    return "未知命令";
    //End By leewheel
}

// 获取离线角色性别
static uint8 GetOfflinePlayerGender(ObjectGuid guid)
{
    QueryResult result = CharacterDatabase.PQuery(
        "SELECT gender FROM characters WHERE guid = {}", guid.GetCounter());

    if (result)
        return (*result)[0].Get<uint8>();

    return GENDER_MALE;
}

// === HandlePlayerbotMgrCommand - .bot命令入口 ===
bool PlayerbotMgr::HandlePlayerbotMgrCommand(ChatHandler* handler, char const* args)
{
    if (!sPlayerbotAIConfig.enabled)
    {
        //By leewheel 2026-08-01: 玩家可见文本中文化
        handler->PSendSysMessage("|cffff0000机器人系统当前已禁用！");
        //End By leewheel
        return false;
    }

    WorldSession* m_session = handler->GetSession();
    if (!m_session)
    {
        //By leewheel 2026-08-01: 玩家可见文本中文化
        handler->PSendSysMessage("你只能从活动会话中添加机器人");
        //End By leewheel
        return false;
    }

    Player* player = m_session->GetPlayer();
    PlayerbotMgr* mgr = GET_PLAYERBOT_MGR(player);
    if (!mgr)
    {
        //By leewheel 2026-08-01: 玩家可见文本中文化
        handler->PSendSysMessage("你还不能控制机器人");
        //End By leewheel
        return false;
    }

    std::vector<std::string> messages = mgr->HandlePlayerbotCommand(args, player);
    if (messages.empty())
        return true;

    for (std::vector<std::string>::iterator i = messages.begin(); i != messages.end(); ++i)
    {
        handler->PSendSysMessage("{}", *i);
    }

    return true;
}

// === HandlePlayerbotCommand - 命令解析 ===
std::vector<std::string> PlayerbotHolder::HandlePlayerbotCommand(char const* args, Player* master)
{
    std::vector<std::string> messages;

    if (!*args)
    {
        //By leewheel 2026-08-01: 玩家可见文本中文化
        messages.push_back("用法: list/reload/tweak/self 或 add/addaccount/init/remove 玩家名\n");
        messages.push_back("用法: addclass 职业名 [male|female|0|1]");
        //End By leewheel
        return messages;
    }

    char* cmd = strtok((char*)args, " ");
    char* charname = strtok(nullptr, " ");
    char* genderArg = strtok(nullptr, " ");

    if (!cmd)
    {
        //By leewheel 2026-08-01: 玩家可见文本中文化
        messages.push_back("用法: list/reload/tweak/self 或 add/init/remove 玩家名 或 addclass 职业名 [male|female]");
        //End By leewheel
        return messages;
    }

    if (!strcmp(cmd, "initself"))
    {
        if (master->CanBeGameMaster())
        {
            PlayerbotFactory factory(master, master->GetLevel(), ITEM_QUALITY_EPIC);
            factory.Randomize(false);
            //By leewheel 2026-08-01: 玩家可见文本中文化
            messages.push_back("自我初始化成功");
            //End By leewheel
            return messages;
        }
        else
        {
            //By leewheel 2026-08-01: 玩家可见文本中文化
            messages.push_back("错误：只有GM可以使用此命令。");
            //End By leewheel
            return messages;
        }
    }

    if (!strncmp(cmd, "initself=", 9))
    {
        if (!strcmp(cmd, "initself=uncommon"))
        {
            if (master->CanBeGameMaster())
            {
                PlayerbotFactory factory(master, master->GetLevel(), ITEM_QUALITY_UNCOMMON);
                factory.Randomize(false);
                //By leewheel 2026-08-01: 玩家可见文本中文化
                messages.push_back("自我初始化成功");
                //End By leewheel
                return messages;
            }
            else
            {
                //By leewheel 2026-08-01: 玩家可见文本中文化
                messages.push_back("错误：只有GM可以使用此命令。");
                //End By leewheel
                return messages;
            }
        }
        if (!strcmp(cmd, "initself=rare"))
        {
            if (master->CanBeGameMaster())
            {
                PlayerbotFactory factory(master, master->GetLevel(), ITEM_QUALITY_RARE);
                factory.Randomize(false);
                //By leewheel 2026-08-01: 玩家可见文本中文化
                messages.push_back("自我初始化成功");
                //End By leewheel
                return messages;
            }
            else
            {
                //By leewheel 2026-08-01: 玩家可见文本中文化
                messages.push_back("错误：只有GM可以使用此命令。");
                //End By leewheel
                return messages;
            }
        }
        if (!strcmp(cmd, "initself=epic"))
        {
            if (master->CanBeGameMaster())
            {
                PlayerbotFactory factory(master, master->GetLevel(), ITEM_QUALITY_EPIC);
                factory.Randomize(false);
                //By leewheel 2026-08-01: 玩家可见文本中文化
                messages.push_back("自我初始化成功");
                //End By leewheel
                return messages;
            }
            else
            {
                //By leewheel 2026-08-01: 玩家可见文本中文化
                messages.push_back("错误：只有GM可以使用此命令。");
                //End By leewheel
                return messages;
            }
        }
        if (!strcmp(cmd, "initself=legendary"))
        {
            if (master->CanBeGameMaster())
            {
                PlayerbotFactory factory(master, master->GetLevel(), ITEM_QUALITY_LEGENDARY);
                factory.Randomize(false);
                //By leewheel 2026-08-01: 玩家可见文本中文化
                messages.push_back("自我初始化成功");
                //End By leewheel
                return messages;
            }
            else
            {
                //By leewheel 2026-08-01: 玩家可见文本中文化
                messages.push_back("错误：只有GM可以使用此命令。");
                //End By leewheel
                return messages;
            }
        }
        int32 gs;
        if (sscanf(cmd, "initself=%d", &gs) != -1)
        {
            if (master->CanBeGameMaster())
            {
                PlayerbotFactory factory(master, master->GetLevel(), ITEM_QUALITY_LEGENDARY, gs);
                factory.Randomize(false);
                //By leewheel 2026-08-01: 玩家可见文本中文化
                messages.push_back("自我初始化成功，装备分数 = " + std::to_string(gs));
                //End By leewheel
                return messages;
            }
            else
            {
                //By leewheel 2026-08-01: 玩家可见文本中文化
                messages.push_back("错误：只有GM可以使用此命令。");
                //End By leewheel
                return messages;
            }
        }
    }

    if (!strcmp(cmd, "list"))
    {
        messages.push_back(ListBots(master));
        return messages;
    }

    if (!strcmp(cmd, "reload"))
    {
        if (master->CanBeGameMaster())
        {
            sPlayerbotAIConfig.Initialize();
            //By leewheel 2026-08-01: 玩家可见文本中文化
            messages.push_back("配置已重新加载。");
            //End By leewheel
            return messages;
        }
        else
        {
            //By leewheel 2026-08-01: 玩家可见文本中文化
            messages.push_back("错误：只有GM可以使用此命令。");
            //End By leewheel
            return messages;
        }
    }

    if (!strcmp(cmd, "tweak"))
    {
        sPlayerbotAIConfig.tweakValue = sPlayerbotAIConfig.tweakValue++;
        if (sPlayerbotAIConfig.tweakValue > 2)
            sPlayerbotAIConfig.tweakValue = 0;

        //By leewheel 2026-08-01: 玩家可见文本中文化
        messages.push_back("已设置调整值为 " + std::to_string(sPlayerbotAIConfig.tweakValue));
        //End By leewheel
        return messages;
    }

    if (!strcmp(cmd, "self"))
    {
        if (GET_PLAYERBOT_AI(master))
        {
            //By leewheel 2026-08-01: 玩家可见文本中文化
            messages.push_back("禁用玩家机器人AI");
            //End By leewheel
            delete GET_PLAYERBOT_AI(master);
        }
        else if (sPlayerbotAIConfig.selfBotLevel == 0)
            //By leewheel 2026-08-01: 玩家可见文本中文化
            messages.push_back("自我机器人已禁用");
            //End By leewheel
        else if (sPlayerbotAIConfig.selfBotLevel == 1 && !master->CanBeGameMaster())
            //By leewheel 2026-08-01: 玩家可见文本中文化
            messages.push_back("你没有权限启用玩家机器人AI");
            //End By leewheel
        else
        {
            //By leewheel 2026-08-01: 玩家可见文本中文化
            messages.push_back("启用玩家机器人AI");
            //End By leewheel
            PlayerbotsMgr::instance().AddPlayerbotData(master, true);
            GET_PLAYERBOT_AI(master)->SetMaster(master);
            PlayerbotRepository::instance().Load(GET_PLAYERBOT_AI(master));
        }

        return messages;
    }

    if (!strcmp(cmd, "lookup"))
    {
        messages.push_back(LookupBots(master));
        return messages;
    }

    if (!strcmp(cmd, "addclass"))
    {
        if (sPlayerbotAIConfig.addClassCommand == 0 && !master->CanBeGameMaster())
        {
            //By leewheel 2026-08-01: 玩家可见文本中文化
        messages.push_back("你没有权限通过 addclass 命令创建机器人");
        //End By leewheel
            return messages;
        }
        if (!charname)
        {
            messages.push_back(
                //By leewheel 2026-08-01: 玩家可见文本中文化
                "addclass: 无效的职业名(战士/圣骑士/猎人/盗贼/牧师/萨满祭司/法师/术士/德鲁伊/死亡骑士)");
            //End By leewheel
            return messages;
        }
        uint8 claz;
        if (!strcmp(charname, "warrior"))
        {
            claz = 1;
        }
        else if (!strcmp(charname, "paladin"))
        {
            claz = 2;
        }
        else if (!strcmp(charname, "hunter"))
        {
            claz = 3;
        }
        else if (!strcmp(charname, "rogue"))
        {
            claz = 4;
        }
        else if (!strcmp(charname, "priest"))
        {
            claz = 5;
        }
        else if (!strcmp(charname, "shaman"))
        {
            claz = 7;
        }
        else if (!strcmp(charname, "mage"))
        {
            claz = 8;
        }
        else if (!strcmp(charname, "warlock"))
        {
            claz = 9;
        }
        else if (!strcmp(charname, "druid"))
        {
            claz = 11;
        }
        else if (!strcmp(charname, "dk"))
        {
            claz = 6;
        }
        else
        {
            //By leewheel 2026-08-01: 玩家可见文本中文化
            messages.push_back("错误：无效职业。请重试。");
            //End By leewheel
            return messages;
        }

        // 解析性别参数
        int8 gender = -1;
        if (genderArg)
        {
            std::string g = genderArg;
            std::transform(g.begin(), g.end(), g.begin(), ::tolower);

            if (g == "male" || g == "0")
                gender = GENDER_MALE;
            else if (g == "female" || g == "1")
                gender = GENDER_FEMALE;
            else
            {
                //By leewheel 2026-08-01: 玩家可见文本中文化
                messages.push_back("未知性别: " + g + " (male/female/0/1)");
                //End By leewheel
                return messages;
            }
        }

        if (claz == 6 && master->GetLevel() < sWorld->getIntConfig(CONFIG_START_HEROIC_PLAYER_LEVEL))
        {
            //By leewheel 2026-08-01: 玩家可见文本中文化
            messages.push_back("你的等级太低，无法召唤死亡骑士");
            //End By leewheel
            return messages;
        }
        //By leewheel 2026-07-12: TC的GetTeamId()不接受参数
        uint8 teamId = master->GetTeamId();
        //End By leewheel
        const std::unordered_set<ObjectGuid> &guidCache = sRandomPlayerbotMgr.addclassCache[RandomPlayerbotMgr::GetTeamClassIdx(teamId == TEAM_ALLIANCE, claz)];
        for (const ObjectGuid &guid: guidCache)
        {
            if (gender != -1 && GetOfflinePlayerGender(guid) != gender)
                continue;
            if (botLoading.find(guid) != botLoading.end())
                continue;
            if (ObjectAccessor::FindConnectedPlayer(guid))
                continue;
            uint32 guildId = sCharacterCache->GetCharacterGuildIdByGuid(guid);
            if (guildId && PlayerbotGuildMgr::instance().IsRealGuild(guildId))
                continue;
            AddPlayerBot(guid, master->GetSession()->GetAccountId());
            //By leewheel 2026-08-01: 玩家可见文本中文化
            messages.push_back("添加职业 " + std::string(charname));
            //End By leewheel
            return messages;
        }
        //By leewheel 2026-08-01: 玩家可见文本中文化
        messages.push_back("添加职业失败，没有可用的角色！");
        //End By leewheel
        return messages;
    }

    std::string charnameStr;

    if (!charname)
    {
        std::string name;
        bool isPlayer = sCharacterCache->GetCharacterNameByGuid(master->GetTarget(), name);
        if (isPlayer)
        {
            charnameStr = name;
        }
        else
        {
            //By leewheel 2026-08-01: 玩家可见文本中文化
            messages.push_back("用法: list/reload/tweak/self 或 add/init/remove 玩家名");
            //End By leewheel
            return messages;
        }
    }
    else
    {
        charnameStr = charname;
    }

    std::string const cmdStr = cmd;

    std::unordered_set<std::string> bots;
    if (charnameStr == "*" && master)
    {
        Group* group = master->GetGroup();
        if (!group)
        {
            //By leewheel 2026-08-01: 玩家可见文本中文化
            messages.push_back("你必须在队伍中");
            //End By leewheel
            return messages;
        }

        Group::MemberSlotList slots = group->GetMemberSlots();
        for (Group::member_citerator i = slots.begin(); i != slots.end(); i++)
        {
            ObjectGuid member = i->guid;

            if (member == master->GetGUID())
                continue;

            std::string bot;
            if (sCharacterCache->GetCharacterNameByGuid(member, bot))
                bots.insert(bot);
        }
    }

    if (charnameStr == "!" && master && master->GetSession()->GetSecurity() > SEC_GAMEMASTER)
    {
        for (PlayerBotMap::const_iterator i = GetPlayerBotsBegin(); i != GetPlayerBotsEnd(); ++i)
        {
            if (Player* bot = i->second)
                if (bot->IsInWorld())
                    bots.insert(bot->GetName());
        }
    }

    std::vector<std::string> chars = split(charnameStr, ',');
    for (std::vector<std::string>::iterator i = chars.begin(); i != chars.end(); i++)
    {
        std::string s = *i;

        if (!strcmp(cmd, "addaccount"))
        {
            uint32 accountId = GetAccountId(s);
            if (!accountId)
            {
                std::string charName = s;
                if (!normalizePlayerName(charName))
                {
                    //By leewheel 2026-08-01: 玩家可见文本中文化
                    messages.push_back("未找到账号或角色 '" + s + "'");
                    //End By leewheel
                    continue;
                }
                ObjectGuid charGuid = sCharacterCache->GetCharacterGuidByName(charName);
                if (!charGuid)
                {
                    //By leewheel 2026-08-01: 玩家可见文本中文化
                    messages.push_back("未找到账号或角色 '" + s + "'");
                    //End By leewheel
                    continue;
                }
                accountId = sCharacterCache->GetCharacterAccountIdByGuid(charGuid);
                if (!accountId)
                {
                    //By leewheel 2026-08-01: 玩家可见文本中文化
                    messages.push_back("无法找到角色 '" + s + "' 的账号");
                    //End By leewheel
                    continue;
                }
            }

            QueryResult results = CharacterDatabase.PQuery("SELECT name FROM characters WHERE account = {}", accountId);
            if (results)
            {
                do
                {
                    Field* fields = results->Fetch();
                    std::string const charName = fields[0].Get<std::string>();
                    bots.insert(charName);
                } while (results->NextRow());
            }
        }
        else
        {
            if (!normalizePlayerName(s))
            {
                //By leewheel 2026-08-01: 玩家可见文本中文化
                messages.push_back("角色 '" + *i + "' 未找到");
                //End By leewheel
                continue;
            }
            ObjectGuid charGuid = sCharacterCache->GetCharacterGuidByName(s);
            if (!charGuid)
            {
                //By leewheel 2026-08-01: 玩家可见文本中文化
                messages.push_back("角色 '" + s + "' 未找到");
                //End By leewheel
                continue;
            }
            bots.insert(s);
        }
    }

    for (auto i = bots.begin(); i != bots.end(); ++i)
    {
        std::string const bot = *i;

        std::ostringstream out;
        out << cmdStr << ": " << bot << " - ";

        ObjectGuid member = sCharacterCache->GetCharacterGuidByName(bot);
        if (!member)
        {
            //By leewheel 2026-08-01: 玩家可见文本中文化
            out << "角色未找到";
            //End By leewheel
        }
        else if (master && member != master->GetGUID())
        {
            out << ProcessBotCommand(cmdStr, member, master->GetGUID(),
                                     master->CanBeGameMaster(),
                                     master->GetSession()->GetAccountId(), master->GetGuildId());
        }
        else if (!master)
        {
            out << ProcessBotCommand(cmdStr, member, ObjectGuid::Empty, true, -1, -1);
        }

        messages.push_back(out.str());
    }

    return messages;
}

// === GetAccountId ===
uint32 PlayerbotHolder::GetAccountId(std::string const name) { return AccountMgr::GetId(name); }

uint32 PlayerbotHolder::GetAccountId(ObjectGuid guid)
{
    if (!guid.IsPlayer())
        return 0;

    // 在线玩家直接获取
    if (Player* player = ObjectAccessor::FindConnectedPlayer(guid))
        return player->GetSession()->GetAccountId();

    ObjectGuid::LowType lowguid = guid.GetCounter();

    if (QueryResult result = CharacterDatabase.PQuery("SELECT account FROM characters WHERE guid = {}", lowguid))
    {
        uint32 acc = (*result)[0].Get<uint32>();
        return acc;
    }

    return 0;
}

// === ListBots ===
std::string const PlayerbotHolder::ListBots(Player* master)
{
    std::set<std::string> bots;
    std::map<uint8, std::string> classNames;

    //By leewheel 2026-08-01: 玩家可见文本中文化
    classNames[CLASS_DEATH_KNIGHT] = "死亡骑士";
    classNames[CLASS_DRUID] = "德鲁伊";
    classNames[CLASS_HUNTER] = "猎人";
    classNames[CLASS_MAGE] = "法师";
    classNames[CLASS_PALADIN] = "圣骑士";
    classNames[CLASS_PRIEST] = "牧师";
    classNames[CLASS_ROGUE] = "盗贼";
    classNames[CLASS_SHAMAN] = "萨满祭司";
    classNames[CLASS_WARLOCK] = "术士";
    classNames[CLASS_WARRIOR] = "战士";
    //End By leewheel

    std::map<std::string, std::string> online;
    std::vector<std::string> names;
    std::map<std::string, std::string> classes;

    for (PlayerBotMap::const_iterator it = GetPlayerBotsBegin(); it != GetPlayerBotsEnd(); ++it)
    {
        Player* const bot = it->second;
        std::string const name = bot->GetName();
        bots.insert(name);

        names.push_back(name);
        online[name] = "+";
        classes[name] = classNames[bot->GetClass()];
    }

    if (master)
    {
        QueryResult results = CharacterDatabase.PQuery("SELECT class, name FROM characters WHERE account = {}",
                                                      master->GetSession()->GetAccountId());
        if (results)
        {
            do
            {
                Field* fields = results->Fetch();
                uint8 cls = fields[0].Get<uint8>();
                std::string const name = fields[1].Get<std::string>();
                if (bots.find(name) == bots.end() && name != master->GetSession()->GetPlayerName())
                {
                    names.push_back(name);
                    online[name] = "-";
                    classes[name] = classNames[cls];
                }
            } while (results->NextRow());
        }
    }

    std::sort(names.begin(), names.end());

    if (Group* group = master->GetGroup())
    {
        Group::MemberSlotList const& groupSlot = group->GetMemberSlots();
        for (Group::member_citerator itr = groupSlot.begin(); itr != groupSlot.end(); itr++)
        {
            Player* member = ObjectAccessor::FindPlayer(itr->guid);
            if (member && sRandomPlayerbotMgr.IsRandomBot(member))
            {
                std::string const name = member->GetName();

                names.push_back(name);
                online[name] = "+";
                classes[name] = classNames[member->GetClass()];
            }
        }
    }

    std::ostringstream out;
    bool first = true;
    //By leewheel 2026-08-01: 玩家可见文本中文化
    out << "机器人名单: ";
    //End By leewheel
    for (std::vector<std::string>::iterator i = names.begin(); i != names.end(); ++i)
    {
        if (first)
            first = false;
        else
            out << ", ";

        std::string const name = *i;
        out << online[name] << name << " " << classes[name];
    }

    return out.str();
}

// === LookupBots ===
std::string const PlayerbotHolder::LookupBots(Player*)
{
    std::list<std::string> messages;
    //By leewheel 2026-08-01: 玩家可见文本中文化
    messages.push_back("可用职业:");
    messages.push_back("|TInterface\\icons\\INV_Sword_27.png:25:25:0:-1|t 战士");
    messages.push_back("|TInterface\\icons\\INV_Hammer_01.png:25:25:0:-1|t 圣骑士");
    messages.push_back("|TInterface\\icons\\INV_Weapon_Bow_07.png:25:25:0:-1|t 猎人");
    messages.push_back("|TInterface\\icons\\INV_ThrowingKnife_04.png:25:25:0:-1|t 盗贼");
    messages.push_back("|TInterface\\icons\\INV_Staff_30.png:25:25:0:-1|t 牧师");
    messages.push_back("|TInterface\\icons\\inv_jewelry_talisman_04.png:25:25:0:-1|t 萨满祭司");
    messages.push_back("|TInterface\\icons\\INV_staff_30.png:25:25:0:-1|t 法师");
    messages.push_back("|TInterface\\icons\\INV_staff_30.png:25:25:0:-1|t 术士");
    messages.push_back("|TInterface\\icons\\Ability_Druid_Maul.png:25:25:0:-1|t 德鲁伊");
    messages.push_back("死亡骑士");
    messages.push_back("(用法: .bot lookup 职业)");
    //End By leewheel
    std::string ret_msg;
    for (std::string msg : messages)
    {
        ret_msg += msg + "\n";
    }
    return ret_msg;
}

// === GetPlayerbotsCountByClass ===
uint32 PlayerbotHolder::GetPlayerbotsCountByClass(uint32 cls)
{
    uint32 count = 0;
    for (PlayerBotMap::const_iterator it = GetPlayerBotsBegin(); it != GetPlayerBotsEnd(); ++it)
    {
        Player* const bot = it->second;
        if (bot && bot->IsInWorld() && bot->GetClass() == cls)
        {
            count++;
        }
    }
    return count;
}

// === PlayerbotMgr ===

PlayerbotMgr::PlayerbotMgr(Player* const master) : PlayerbotHolder(), master(master), lastErrorTell(0) {}

PlayerbotMgr::~PlayerbotMgr()
{
    if (master)
        PlayerbotsMgr::instance().RemovePlayerBotData(master->GetGUID(), false);
}

void PlayerbotMgr::UpdateAIInternal(uint32 elapsed, bool /*minimal*/)
{
    SetNextCheckDelay(sPlayerbotAIConfig.reactDelay);
    CheckTellErrors(elapsed);

    //By leewheel 2026-07-13: 处理玩家控制bot的会话包队列
    //OnPlayerAfterUpdate调用此函数，需要在这里处理bot的包队列
    UpdateSessions();
    //End By leewheel 2026-07-13

    //By leewheel 2026-07-20: 移除遍历bot调用UpdateAI的循环，OnPlayerAfterUpdate已为每个bot单独调用botAI->UpdateAI(diff)，
    //此循环导致双重更新(每bot每tick两次)，500bot时浪费大量CPU
    //End By leewheel
}

void PlayerbotMgr::HandleCommand(uint32 type, std::string const text)
{
    Player* master = GetMaster();
    if (!master)
        return;

    if (text.find(sPlayerbotAIConfig.commandSeparator) != std::string::npos)
    {
        std::vector<std::string> commands;
        split(commands, text, sPlayerbotAIConfig.commandSeparator.c_str());
        for (std::vector<std::string>::iterator i = commands.begin(); i != commands.end(); ++i)
        {
            HandleCommand(type, *i);
        }

        return;
    }

    for (PlayerBotMap::const_iterator it = GetPlayerBotsBegin(); it != GetPlayerBotsEnd(); ++it)
    {
        Player* const bot = it->second;
        PlayerbotAI* botAI = GET_PLAYERBOT_AI(bot);
        if (botAI)
            botAI->HandleCommand(type, text, master);
    }

    for (PlayerBotMap::const_iterator it = sRandomPlayerbotMgr.GetPlayerBotsBegin();
         it != sRandomPlayerbotMgr.GetPlayerBotsEnd(); ++it)
    {
        Player* const bot = it->second;
        PlayerbotAI* botAI = GET_PLAYERBOT_AI(bot);
        if (botAI && botAI->GetMaster() == master)
            botAI->HandleCommand(type, text, master);
    }
}

void PlayerbotMgr::HandleMasterIncomingPacket(WorldPacket const& packet)
{
    for (PlayerBotMap::const_iterator it = GetPlayerBotsBegin(); it != GetPlayerBotsEnd(); ++it)
    {
        Player* const bot = it->second;
        if (!bot)
            continue;
        PlayerbotAI* botAI = GET_PLAYERBOT_AI(bot);
        if (botAI)
            botAI->HandleMasterIncomingPacket(packet);
    }

    for (PlayerBotMap::const_iterator it = sRandomPlayerbotMgr.GetPlayerBotsBegin();
         it != sRandomPlayerbotMgr.GetPlayerBotsEnd(); ++it)
    {
        Player* const bot = it->second;
        PlayerbotAI* botAI = GET_PLAYERBOT_AI(bot);
        if (botAI && botAI->GetMaster() == GetMaster())
            botAI->HandleMasterIncomingPacket(packet);
    }

    switch (packet.GetOpcode())
    {
        // 主人登出时，登出所有机器人
        case CMSG_LOGOUT_REQUEST:
        {
            Player* master = GetMaster();
            if (master)
            {
                AreaTableEntry const* areaEntry = sAreaTableStore.LookupEntry(master->GetAreaId());
                //By leewheel 2026-07-12: TC没有CONFIG_AFK_PREVENT_LOGOUT枚举，直接用ConfigMgr读取
                uint32 afkPreventLogout = sConfigMgr->GetIntDefault("PreventAFKLogout", 0);
                bool preventAfkSanctuaryLogout = afkPreventLogout == 1
                                                 && master->isAFK() && areaEntry && areaEntry->IsSanctuary();

                bool preventAfkLogout = afkPreventLogout == 2
                                        && master->isAFK();
                //End By leewheel

                if (preventAfkSanctuaryLogout || preventAfkLogout)
                {
                    break;
                }
            }

            LogoutAllBots();
            break;
        }
        // 主人取消登出时，机器人也取消
        case CMSG_LOGOUT_CANCEL:
        {
            CancelLogout();
            break;
        }
    }
}

void PlayerbotMgr::HandleMasterOutgoingPacket(WorldPacket const& packet)
{
    for (PlayerBotMap::const_iterator it = GetPlayerBotsBegin(); it != GetPlayerBotsEnd(); ++it)
    {
        Player* const bot = it->second;
        PlayerbotAI* botAI = GET_PLAYERBOT_AI(bot);
        if (botAI)
            botAI->HandleMasterOutgoingPacket(packet);
    }

    for (PlayerBotMap::const_iterator it = sRandomPlayerbotMgr.GetPlayerBotsBegin();
         it != sRandomPlayerbotMgr.GetPlayerBotsEnd(); ++it)
    {
        Player* const bot = it->second;
        PlayerbotAI* botAI = GET_PLAYERBOT_AI(bot);
        if (botAI && botAI->GetMaster() == GetMaster())
            botAI->HandleMasterOutgoingPacket(packet);
    }
}

void PlayerbotMgr::SaveToDB()
{
    for (PlayerBotMap::const_iterator it = GetPlayerBotsBegin(); it != GetPlayerBotsEnd(); ++it)
    {
        Player* const bot = it->second;
        // TC适配: SaveToDB只接受1个bool参数
        bot->SaveToDB(false);
    }

    for (PlayerBotMap::const_iterator it = sRandomPlayerbotMgr.GetPlayerBotsBegin();
         it != sRandomPlayerbotMgr.GetPlayerBotsEnd(); ++it)
    {
        Player* const bot = it->second;
        if (GET_PLAYERBOT_AI(bot) && GET_PLAYERBOT_AI(bot)->GetMaster() == GetMaster())
            // TC适配: SaveToDB只接受1个bool参数
            bot->SaveToDB(false);
    }
}

void PlayerbotMgr::OnBotLoginInternal(Player* const bot)
{
    PlayerbotAI* botAI = GET_PLAYERBOT_AI(bot);
    if (!botAI)
    {
        return;
    }
    botAI->SetMaster(master);
    botAI->ResetStrategies();

    // TC_LOG_INFO("playerbots", "机器人 {} 上线了", bot->GetName().c_str());
}

void PlayerbotMgr::OnPlayerLogin(Player* player)
{
    if (!player)
        return;

    WorldSession* session = player->GetSession();
    if (!session)
    {
        TC_LOG_WARN("playerbots", "Unable to register locale priority for player {} because the session is missing", player->GetName());
        return;
    }

    // 设置机器人文本的本地化优先级
    LocaleConstant const databaseLocale = session->GetSessionDbLocaleIndex();

    LocaleConstant usedLocale = databaseLocale;
    if (usedLocale >= TOTAL_LOCALES)
        usedLocale = LOCALE_enUS;

    PlayerbotTextMgr::instance().AddLocalePriority(usedLocale);

    if (sPlayerbotAIConfig.selfBotLevel > 2)
        HandlePlayerbotCommand("self", player);

    if (!sPlayerbotAIConfig.botAutologin)
        return;

    uint32 accountId = session->GetAccountId();
    QueryResult results = CharacterDatabase.PQuery("SELECT name FROM characters WHERE account = {}", accountId);
    if (results)
    {
        std::ostringstream out;
        out << "add ";
        bool first = true;
        do
        {
            Field* fields = results->Fetch();

            if (first)
                first = false;
            else
                out << ",";

            out << fields[0].Get<std::string>();
        } while (results->NextRow());

        HandlePlayerbotCommand(out.str().c_str(), player);
    }
}

void PlayerbotMgr::TellError(std::string const botName, std::string const text)
{
    std::set<std::string> names = errors[text];
    if (names.find(botName) == names.end())
    {
        names.insert(botName);
    }

    errors[text] = names;
}

void PlayerbotMgr::CheckTellErrors(uint32 /*elapsed*/)
{
    time_t now = time(nullptr);
    if ((now - lastErrorTell) < sPlayerbotAIConfig.errorDelay / 1000)
        return;

    lastErrorTell = now;

    for (PlayerBotErrorMap::iterator i = errors.begin(); i != errors.end(); ++i)
    {
        std::string const text = i->first;
        std::set<std::string> names = i->second;

        std::ostringstream out;
        bool first = true;
        for (std::set<std::string>::iterator j = names.begin(); j != names.end(); ++j)
        {
            if (!first)
                out << ", ";
            else
                first = false;

            out << *j;
        }

        out << "|cfff00000: " << text;

        ChatHandler(master->GetSession()).PSendSysMessage(out.str().c_str());
    }

    errors.clear();
}

// === PlayerbotsMgr 单例方法 ===

void PlayerbotsMgr::AddPlayerbotData(Player* player, bool isBotAI)
{
    if (!player)
    {
        return;
    }

    if (!isBotAI)
    {
        // 添加 PlayerbotMgr 数据
        //By leewheel 2026-07-12: 安全清理旧条目，先erase再delete防止双重delete
        auto itr = _playerbotsMgrMap.find(player->GetGUID());
        if (itr != _playerbotsMgrMap.end())
        {
            PlayerbotAIBase* old = itr->second;
            _playerbotsMgrMap.erase(itr);
            delete old;
        }
        //End By leewheel
        PlayerbotMgr* playerbotMgr = new PlayerbotMgr(player);
        _playerbotsMgrMap.emplace(player->GetGUID(), playerbotMgr);

        playerbotMgr->OnPlayerLogin(player);
    }
    else
    {
        // 添加 PlayerbotAI 数据
        //By leewheel 2026-07-12: 安全清理旧条目，先erase再delete防止双重delete
        auto itr = _playerbotsAIMap.find(player->GetGUID());
        if (itr != _playerbotsAIMap.end())
        {
            PlayerbotAIBase* old = itr->second;
            _playerbotsAIMap.erase(itr);
            delete old;
        }
        //End By leewheel
        PlayerbotAI* botAI = new PlayerbotAI(player);
        _playerbotsAIMap.emplace(player->GetGUID(), botAI);
    }
}

//By leewheel 2026-07-12: 修复双重delete崩溃！
// AC的RemovePlayerBotData只从map中erase，不调用delete。
// 实际的delete由OnDestructPlayer钩子负责。
// 旧代码中delete会导致PlayerbotAI析构函数再次调用RemovePlayerBotData，
// 造成双重delete和内存损坏，最终导致aiObjectContext空指针崩溃。
void PlayerbotsMgr::RemovePlayerBotData(ObjectGuid const& guid, bool is_AI)
{
    if (is_AI)
    {
        auto itr = _playerbotsAIMap.find(guid);
        if (itr != _playerbotsAIMap.end())
        {
            _playerbotsAIMap.erase(itr);
        }
    }
    else
    {
        auto itr = _playerbotsMgrMap.find(guid);
        if (itr != _playerbotsMgrMap.end())
        {
            _playerbotsMgrMap.erase(itr);
        }
    }
}
//End By leewheel

PlayerbotAI* PlayerbotsMgr::GetPlayerbotAI(Player* player)
{
    if (!(sPlayerbotAIConfig.enabled) || !player)
        return nullptr;

    auto itr = _playerbotsAIMap.find(player->GetGUID());
    if (itr != _playerbotsAIMap.end())
    {
        if (itr->second->IsBotAI())
            return dynamic_cast<PlayerbotAI*>(itr->second);
    }

    return nullptr;
}

PlayerbotMgr* PlayerbotsMgr::GetPlayerbotMgr(Player* player)
{
    if (!(sPlayerbotAIConfig.enabled) || !player)
    {
        return nullptr;
    }

    auto itr = _playerbotsMgrMap.find(player->GetGUID());
    if (itr != _playerbotsMgrMap.end())
    {
        if (!itr->second->IsBotAI())
            return dynamic_cast<PlayerbotMgr*>(itr->second);
    }

    return nullptr;
}
//End By leewheel

//By leewheel 2026-08-15: 账号信任链4命令(移植the-lab)——setKey/link/linkedAccounts/unlink。
//TC适配: 用OpenSSL SHA256(openssl/sha.h已在include)哈希; 十六进制转换同上游。
//依赖表: playerbots_account_keys(account_id, security_key)、playerbots_account_links
//(account_id, linked_account_id)——随Playerbots库DDL部署。

void PlayerbotMgr::HandleSetSecurityKeyCommand(Player* player, std::string const& key)
{
    uint32 accountId = player->GetSession()->GetAccountId();

    // 用SHA-256哈希安全密钥
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<unsigned char const*>(key.c_str()), key.size(), hash);

    // 转十六进制字符串
    std::ostringstream hashedKey;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i)
        hashedKey << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);

    // 存库
    PlayerbotsDatabase.PExecute(
        "REPLACE INTO playerbots_account_keys (account_id, security_key) VALUES ({}, '{}')",
        accountId, hashedKey.str());

    //By leewheel 2026-08-16: 玩家可见文本中文化
    ChatHandler(player->GetSession()).PSendSysMessage("安全密钥设置成功。");
    //End By leewheel
}

void PlayerbotMgr::HandleLinkAccountCommand(Player* player, std::string const& accountName, std::string const& key)
{
    QueryResult result = LoginDatabase.PQuery("SELECT id FROM account WHERE username = '{}'", accountName);
    if (!result)
    {
        ChatHandler(player->GetSession()).PSendSysMessage("账号不存在。");
        return;
    }

    Field* fields = result->Fetch();
    uint32 linkedAccountId = fields[0].Get<uint32>();

    result = PlayerbotsDatabase.PQuery("SELECT security_key FROM playerbots_account_keys WHERE account_id = {}", linkedAccountId);
    if (!result)
    {
        ChatHandler(player->GetSession()).PSendSysMessage("安全密钥错误。");
        return;
    }

    // 哈希提供的密钥
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<unsigned char const*>(key.c_str()), key.size(), hash);

    // 转十六进制字符串
    std::ostringstream hashedKey;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i)
        hashedKey << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);

    // 与库中哈希比较
    std::string storedKey = result->Fetch()->Get<std::string>();
    if (hashedKey.str() != storedKey)
    {
        ChatHandler(player->GetSession()).PSendSysMessage("安全密钥错误。");
        return;
    }

    uint32 accountId = player->GetSession()->GetAccountId();
    PlayerbotsDatabase.PExecute(
        "INSERT IGNORE INTO playerbots_account_links (account_id, linked_account_id) VALUES ({}, {})",
        accountId, linkedAccountId);
    PlayerbotsDatabase.PExecute(
        "INSERT IGNORE INTO playerbots_account_links (account_id, linked_account_id) VALUES ({}, {})",
        linkedAccountId, accountId);

    ChatHandler(player->GetSession()).PSendSysMessage("账号关联成功。");
}

void PlayerbotMgr::HandleViewLinkedAccountsCommand(Player* player)
{
    uint32 accountId = player->GetSession()->GetAccountId();
    QueryResult result = PlayerbotsDatabase.PQuery("SELECT linked_account_id FROM playerbots_account_links WHERE account_id = {}", accountId);

    if (!result)
    {
        ChatHandler(player->GetSession()).PSendSysMessage("没有关联账号。");
        return;
    }

    ChatHandler(player->GetSession()).PSendSysMessage("已关联账号：");
    do
    {
        Field* fields = result->Fetch();
        uint32 linkedAccountId = fields[0].Get<uint32>();

        QueryResult accountResult = LoginDatabase.PQuery("SELECT username FROM account WHERE id = {}", linkedAccountId);
        if (accountResult)
        {
            Field* accountFields = accountResult->Fetch();
            std::string username = accountFields[0].Get<std::string>();
            ChatHandler(player->GetSession()).PSendSysMessage("- {}", username.c_str());
        }
        else
        {
            ChatHandler(player->GetSession()).PSendSysMessage("- 未知账号");
        }
    } while (result->NextRow());
}

void PlayerbotMgr::HandleUnlinkAccountCommand(Player* player, std::string const& accountName)
{
    QueryResult result = LoginDatabase.PQuery("SELECT id FROM account WHERE username = '{}'", accountName);
    if (!result)
    {
        ChatHandler(player->GetSession()).PSendSysMessage("账号不存在。");
        return;
    }

    Field* fields = result->Fetch();
    uint32 linkedAccountId = fields[0].Get<uint32>();
    uint32 accountId = player->GetSession()->GetAccountId();

    PlayerbotsDatabase.PExecute("DELETE FROM playerbots_account_links WHERE (account_id = {} AND linked_account_id = {}) OR (account_id = {} AND linked_account_id = {})",
                                accountId, linkedAccountId, linkedAccountId, accountId);

    ChatHandler(player->GetSession()).PSendSysMessage("账号解除关联成功。");
}
//End By leewheel
