//By leewheel 2026-07-12
// 机器人脚本注册 - 完整实现
// 从 AzerothCore mod-playerbots 移植到 TrinityCore
// 注册 WorldScript, PlayerScript, ServerScript, MiscScript, DatabaseScript, PlayerbotScript, BGScript 等脚本
// TC适配: 无hook列表参数、BattlefieldScript有纯虚函数需跳过、OnGiveXP签名不同、CHAR_UPD_CHAR_OFFLINE不存在
//End By leewheel

#include "ScriptMgr.h"
#include "PlayerbotAIConfig.h"
#include "PlayerbotMgr.h"
#include "PlayerbotAI.h"
#include "Player.h"
#include "WorldSession.h"
#include "Chat.h"
#include "Config.h"
#include "Log.h"
#include "ObjectAccessor.h"
#include "Timer.h"
#include "DatabaseEnv.h"
#include "DatabaseLoader.h"
#include "GuildTaskMgr.h"
#include "PlayerbotGuildMgr.h"
#include "PlayerbotSpellRepository.h"
#include "PlayerbotWorldThreadProcessor.h"
#include "RandomPlayerbotMgr.h"
#include "PlayerbotCommandScript.h"
#include "BattleGroundTactics.h"
#include "Channel.h"
#include "Group.h"
#include "Guild.h"
#include "AchievementMgr.h"
#include "SharedDefines.h"
#include "Opcodes.h"
#include "LfgPlayerbots.h"
#include "Playerbots.h"
//By leewheel 2026-07-14: 添加TravelMgr.h以使用sTravelMgr.Init()
#include "TravelMgr.h"
//End By leewheel
//By leewheel 2026-07-20: 添加SpellMgr.h以遍历SpellInfo获取enUS技能名
#include "SpellMgr.h"
//End By leewheel
#include <cmath>

// === DatabaseScript: Playerbots数据库加载和管理 ===
class PlayerbotsDatabaseScript : public DatabaseScript
{
public:
    PlayerbotsDatabaseScript() : DatabaseScript("PlayerbotsDatabaseScript") {}

    bool OnDatabasesLoading() override
    {
        //By leewheel 2026-07-12: TC的DatabaseLoader构造函数接受updateMask参数，而非AC的SetUpdateFlags方法
        //By leewheel 2026-07-12: TC的ConfigMgr使用GetBoolDefault而非AC的GetOption<bool>
        uint32 updateMask = sConfigMgr->GetBoolDefault("Playerbots.Updates.EnableDatabases", true)
                                ? DatabaseLoader::DATABASE_PLAYERBOTS
                                : 0;
        //End By leewheel
        DatabaseLoader playerbotLoader("server.playerbots", updateMask);
        //End By leewheel
        playerbotLoader.AddDatabase(PlayerbotsDatabase, "Playerbots");

        return playerbotLoader.Load();
    }

    void OnDatabasesKeepAlive() override { PlayerbotsDatabase.KeepAlive(); }

    void OnDatabasesClosing() override { PlayerbotsDatabase.Close(); }

    void OnDatabaseWarnAboutSyncQueries(bool apply) override { PlayerbotsDatabase.WarnAboutSyncQueries(apply); }

    void OnDatabaseSelectIndexLogout(Player* player, uint32& statementIndex, uint32& statementParam) override
    {
        //By leewheel 2026-07-12: TC没有CHAR_UPD_CHAR_OFFLINE，使用CHAR_UPD_CHAR_ONLINE
        //TC的WorldSession已在LogoutPlayer中处理online状态更新，此处仅设置参数
        statementIndex = CHAR_UPD_CHAR_ONLINE;
        statementParam = player->GetGUID().GetCounter();
        //End By leewheel
    }

    void OnDatabaseGetDBRevision(std::string& revision) override
    {
        if (QueryResult resultPlayerbot =
                PlayerbotsDatabase.Query("SELECT date FROM version_db_playerbots ORDER BY date DESC LIMIT 1"))
        {
            Field* fields = resultPlayerbot->Fetch();
            revision = fields[0].Get<std::string>();
        }

        if (revision.empty())
            revision = "Unknown Playerbots Database Revision";
    }
};

//By leewheel 2026-07-14: spellnameeng表缓存定义和加载
//By leewheel 2026-07-22: 改从classic_db2.spellname表加载完整英文名(49358条)
// 原因: TC的DB2 loader以zhCN为主locale，enUS SpellName只加载部分(31803/49358)
// 导致大量技能(如DK/盗贼核心技能)无法通过英文名查找，bot只近战不放技能
// classic_db2.spellname由用户从enUS/SpellName.db2完整导入，ID即SpellID
std::unordered_map<uint32, std::string> sSpellNameEngCache;

void LoadSpellNameEngCache()
{
    uint32 oldMSTime = getMSTime();
    sSpellNameEngCache.clear();

    // ========== 主数据源: classic_db2.spellname表(完整enUS SpellName.db2) ==========
    uint32 dbCount = 0;
    if (QueryResult result = HotfixDatabase.Query("SELECT ID, Name_lang FROM classic_db2.spellname"))
    {
        do
        {
            Field* fields = result->Fetch();
            uint32 spellId = fields[0].Get<uint32>();
            std::string name = fields[1].Get<std::string>();
            if (!name.empty())
            {
                sSpellNameEngCache[spellId] = std::move(name);
                dbCount++;
            }
        } while (result->NextRow());
    }

    TC_LOG_INFO("server.loading", "spellnameeng: [主源-classic_db2.spellname] 加载了 {} 条英文技能名", dbCount);

    // ========== 补充源: SpellInfo DB2 enUS locale(覆盖表中没有的条目) ==========
    uint32 db2Supplement = 0;
    uint32 storeSize = sSpellMgr->GetSpellInfoStoreSize();
    for (uint32 spellId = 1; spellId < storeSize; ++spellId)
    {
        if (sSpellNameEngCache.find(spellId) != sSpellNameEngCache.end())
            continue;

        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);
        if (!spellInfo || !spellInfo->SpellName)
            continue;

        const char* enName = (*spellInfo->SpellName)[LOCALE_enUS];
        if (enName && *enName)
        {
            sSpellNameEngCache[spellId] = enName;
            db2Supplement++;
        }
    }

    if (db2Supplement > 0)
        TC_LOG_INFO("server.loading", "spellnameeng: [补充-DB2 enUS] 额外补充 {} 条", db2Supplement);

    if (sSpellNameEngCache.empty())
    {
        TC_LOG_ERROR("server.loading", "spellnameeng: 未加载到任何英文技能名！bot施法系统将失效！");
        TC_LOG_ERROR("server.loading", "spellnameeng: 请确认classic_db2.spellname表存在且有数据");
    }

    TC_LOG_INFO("server.loading", "spellnameeng: 缓存总计 {} 条英文技能名称，耗时 {} 毫秒",
        sSpellNameEngCache.size(), GetMSTimeDiffToNow(oldMSTime));

    // 输出几个常见技能的诊断信息，确认名称正确
    if (!sSpellNameEngCache.empty())
    {
        uint32 testSpells[] = { 133, 2098, 53, 78, 49020, 49892, 11294, 11300, 48263 };
        for (uint32 testId : testSpells)
        {
            auto it = sSpellNameEngCache.find(testId);
            TC_LOG_INFO("server.loading", "spellnameeng诊断: spellId={} name='{}'",
                testId, (it != sSpellNameEngCache.end()) ? it->second.c_str() : "<无>");
        }
    }
}
//End By leewheel

// === WorldScript: 服务器初始化和更新 ===
//By leewheel 2026-07-12: 将重初始化从OnStartup移到OnConfigLoad(false)
// OnConfigLoad(false)在SetInitialWorldSettings()中调用，此时冻结检测器尚未启动
// OnStartup在冻结检测器之后调用，CreateRandomBots()耗时超60秒会触发崩溃
//By leewheel 2026-07-14: sTravelMgr.Init()移回OnStartup，因为需要PreloadContinents完成
//End By leewheel
class PlayerbotsWorldScript : public WorldScript
{
public:
    PlayerbotsWorldScript() : WorldScript("PlayerbotsWorldScript") {}

    void OnConfigLoad(bool reload) override
    {
        // 仅在首次加载时初始化（非reload）
        if (reload)
            return;

        TC_LOG_INFO("server.loading", "╔══════════════════════════════════════════════════════════╗");
        TC_LOG_INFO("server.loading", "║                                                          ║");
        TC_LOG_INFO("server.loading", "║              机器人模块 (Playerbots Module)              ║");
        TC_LOG_INFO("server.loading", "║                                                          ║");
        TC_LOG_INFO("server.loading", "╟──────────────────────────────────────────────────────────╢");
        TC_LOG_INFO("server.loading", "║           基于liyunfan 的 mod-playerbots 开发            ║");
        TC_LOG_INFO("server.loading", "╟──────────────────────────────────────────────────────────╢");
        TC_LOG_INFO("server.loading", "╚══════════════════════════════════════════════════════════╝");

        uint32 oldMSTime = getMSTime();

        TC_LOG_INFO("server.loading", " ");
        TC_LOG_INFO("server.loading", "正在加载机器人配置...");

        sPlayerbotAIConfig.Initialize();

        TC_LOG_INFO("server.loading", ">> 机器人配置加载完成，耗时 {} 毫秒", GetMSTimeDiffToNow(oldMSTime));
        TC_LOG_INFO("server.loading", " ");

        // 初始化法术仓库
        PlayerbotSpellRepository::Instance().Initialize();

        //By leewheel 2026-07-14: 加载spellnameeng表（英文技能名缓存）
        LoadSpellNameEngCache();
        //End By leewheel

        //By leewheel 2026-08-15: 加载CharStartOutfit初始装备缓存(低等级bot出生装备)
        LoadCharStartOutfitCache();
        //End By leewheel

        TC_LOG_INFO("server.loading", "Playerbots 世界线程处理器已初始化");
    }

    void OnStartup() override
    {
        //By leewheel 2026-07-14: sTravelMgr.Init()必须在PreloadContinents之后调用
        //OnStartup在SetInitialWorldSettings(包含PreloadContinents)之后执行
        //PrepareDestinationCache中sMapMgr->FindMap需要Map实例已创建
        //此处不会触发冻结检测器，因为PrepareDestinationCache远比CreateRandomBots快
        TC_LOG_INFO("server.loading", "正在初始化旅行管理器...");
        uint32 travelTimer = getMSTime();
        sTravelMgr.Init();
        TC_LOG_INFO("server.loading", "旅行管理器初始化完成，耗时 {} 毫秒", GetMSTimeDiffToNow(travelTimer));
        //End By leewheel
    }

    //By leewheel 2026-07-13: 添加OnShutdown，服务器关闭时登出所有机器人
    void OnShutdown() override
    {
        TC_LOG_INFO("playerbots", "服务器关闭，正在登出所有机器人...");
        sRandomPlayerbotMgr.LogoutAllBots();
    }
    //End By leewheel 2026-07-13

void OnUpdate(uint32 diff) override
{
    // 世界线程处理：处理线程不安全的操作（组队、LFG、公会等）
    PlayerbotWorldThreadProcessor::instance().Update(diff);

    // 随机机器人管理器AI更新（仅世界线程）
    // UpdateSessions已移至OnPlayerbotUpdate钩子，由World::Update调用
    sRandomPlayerbotMgr.UpdateAI(diff);
}
};

// === PlayerScript: 玩家登录、更新、聊天 ===
class PlayerbotsPlayerScript : public PlayerScript
{
public:
    PlayerbotsPlayerScript() : PlayerScript("PlayerbotsPlayerScript") {}

    //By leewheel 2026-09-03 修复C4100警告：firstLogin参数在本钩子中未使用，显式省略参数名
    void OnLogin(Player* player, bool /*firstLogin*/) override
    //End By leewheel
    {
        if (!player || !player->GetSession())
            return;

        //By leewheel 2026-07-14: bot登录后恢复满血满蓝
        //根因：LoadFromDB从数据库恢复保存的power值(可能为0)，而不是设为最大值
        //UpdateAllStats正确计算了maxPower，但current power仍为数据库中的0
        if (player->GetSession()->IsBot())
        {
            player->SetFullHealth();
            Powers powerType = player->GetPowerType();
            if (player->GetMaxPower(powerType) > 0)
                player->SetFullPower(powerType);
            // 同时恢复所有类型的power
            for (uint8 i = 0; i < MAX_POWERS; ++i)
            {
                if (i == powerType || i == POWER_HEALTH)
                    continue;
                if (player->GetMaxPower(Powers(i)) > 0)
                    player->SetFullPower(Powers(i));
            }

            //By leewheel 2026-07-17: 登录恢复power日志已禁用（刷屏严重）
            //End By leewheel
        }

        if (!player->GetSession()->IsBot())
        {
            PlayerbotsMgr::instance().AddPlayerbotData(player, false);
            sRandomPlayerbotMgr.OnPlayerLogin(player);

            if (sPlayerbotAIConfig.enabled)
            {
                ChatHandler(player->GetSession()).SendSysMessage(
                    "|cff00ff00本服务器运行 |cff00ccff机器人模块(Playerbots)|r");
            }

            if (sPlayerbotAIConfig.enabled || sPlayerbotAIConfig.randomBotAutologin)
            {
                std::string maxAllowedBotCount = std::to_string(sRandomPlayerbotMgr.GetMaxAllowedBotCount());

                ChatHandler(player->GetSession()).SendSysMessage(
                    "|cff00ff00机器人:|r 服务器已配置 " + maxAllowedBotCount + " 个机器人。");
            }
        }
    }

    //By leewheel 2026-07-13: 驱动机器人AI更新
    void OnPlayerAfterUpdate(Player* player, uint32 diff) override
    {
        if (!player)
            return;

        PlayerbotAI* const botAI = PlayerbotsMgr::instance().GetPlayerbotAI(player);

        if (botAI != nullptr)
        {
            botAI->UpdateAI(diff);
        }

        if (PlayerbotMgr* playerbotMgr = GET_PLAYERBOT_MGR(player))
        {
            playerbotMgr->UpdateAI(diff);
        }
    }
    //End By leewheel

    using PlayerScript::OnPlayerCanUseChat;  // keep the base overloads visible

    bool OnPlayerCanUseChat(Player* player, uint32 type, uint32 /*lang*/, std::string& msg, Player* receiver) override
    {
        if (type != CHAT_MSG_WHISPER)
            return true;

        PlayerbotAI* const botAI = PlayerbotsMgr::instance().GetPlayerbotAI(receiver);

        if (botAI == nullptr)
            return true;

        botAI->HandleCommand(type, msg, player);

        // 临时修复：whisper logout 时服务器崩溃问题
        if (msg == "logout")
            return false;

        return true;
    }

    bool OnPlayerCanUseChat(Player* player, uint32 type, uint32 /*lang*/, std::string& msg, Group* group) override
    {
        // 组聊天命令处理：遍历队伍成员，对每个机器人调用HandleCommand
        for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
        {
            Player* const member = itr->GetSource();

            if (member == nullptr)
                continue;

            PlayerbotAI* const botAI = PlayerbotsMgr::instance().GetPlayerbotAI(member);

            if (botAI == nullptr)
                continue;

            botAI->HandleCommand(type, msg, player);
        }

        return true;
    }

    bool OnPlayerCanUseChat(Player* player, uint32 type, uint32 /*lang*/, std::string& msg, Guild* /*guild*/) override
    {
        // 公会聊天命令处理：只处理公会频道消息
        if (type != CHAT_MSG_GUILD)
            return true;

        PlayerbotMgr* playerbotMgr = GET_PLAYERBOT_MGR(player);

        if (playerbotMgr == nullptr)
            return true;

        // 遍历玩家所有的机器人，对同公会的机器人转发消息
        for (PlayerBotMap::const_iterator it = playerbotMgr->GetPlayerBotsBegin(); it != playerbotMgr->GetPlayerBotsEnd(); ++it)
        {
            Player* const bot = it->second;

            if (bot == nullptr)
                continue;

            if (bot->GetGuildId() != player->GetGuildId())
                continue;

            PlayerbotsMgr::instance().GetPlayerbotAI(bot)->HandleCommand(type, msg, player);
        }

        return true;
    }

    bool OnPlayerCanUseChat(Player* player, uint32 type, uint32 /*lang*/, std::string& msg, Channel* channel) override
    {
        // 频道聊天命令处理：转发给玩家的机器人管理器和随机机器人管理器
        PlayerbotMgr* const playerbotMgr = GET_PLAYERBOT_MGR(player);

        //By leewheel 2026-07-12: AC使用channel->GetFlags() & 0x18检查是否为自定义频道
        if (playerbotMgr != nullptr && channel->GetFlags() & 0x18)
            playerbotMgr->HandleCommand(type, msg);
        //End By leewheel

        sRandomPlayerbotMgr.HandleCommand(type, msg, player);

        return true;
    }

    bool OnPlayerBeforeTeleport(Player* /*player*/, uint32 /*mapid*/, float /*x*/, float /*y*/, float /*z*/,
                                float /*orientation*/, uint32 /*options*/, Unit* /*target*/) override
    {
        // 传送前检查 - 目前不需要阻止传送
        return true;
    }

    bool OnPlayerBeforeAchievementComplete(Player* player, AchievementEntry const* achievement) override
    {
        // 随机机器人不获得服务器首杀成就
        if ((sRandomPlayerbotMgr.IsRandomBot(player) || sRandomPlayerbotMgr.IsAddclassBot(player)) &&
            //By leewheel 2026-07-12: TC的AchievementEntry使用大写Flags而非小写flags
            (achievement->Flags & (ACHIEVEMENT_FLAG_REALM_FIRST_REACH | ACHIEVEMENT_FLAG_REALM_FIRST_KILL)))
            //End By leewheel
        {
            return false;
        }

        return true;
    }

    void OnGiveXP(Player* player, uint32& amount, Unit* /*victim*/) override
    {
        // 早期返回
        if (sPlayerbotAIConfig.randomBotXPRate == 1.0f || !player)
            return;

        // 非机器人不乘经验
        if (!player->GetSession()->IsBot() || !sRandomPlayerbotMgr.IsRandomBot(player))
            return;

        // 与真实玩家组队的机器人不乘经验
        if (Group* group = player->GetGroup())
        {
            for (GroupReference* gref = group->GetFirstMember(); gref; gref = gref->next())
            {
                Player* member = gref->GetSource();
                if (!member)
                    continue;

                if (!member->GetSession()->IsBot())
                    return;
            }
        }

        // 应用机器人经验倍率
        amount = static_cast<uint32>(std::round(static_cast<float>(amount) * sPlayerbotAIConfig.randomBotXPRate));
    }

//By leewheel 2026-07-13: 将PlayerbotsScript的OnPlayerbot*方法合并到PlayerbotsPlayerScript中
// 原因: ScriptMgr::OnPlayerbot*已改为FOREACH_SCRIPT(PlayerScript)，
// PlayerbotsScript继承PlayerbotScript不会被遍历到，必须放在PlayerScript子类中

    // LFG队列检查 - 返回true表示有非机器人玩家在队列中
    bool OnPlayerbotCheckLFGQueue(lfg::Lfg5Guids const& guidsList) override
    {
        bool nonBotFound = false;

        for (ObjectGuid const& guid : guidsList.guids)
        {
            Player* player = ObjectAccessor::FindPlayer(guid);

            //By leewheel 2026-09-05: 上游ce244c32——全机器人队列检查需把 selfbot 也计入玩家(不再判定为非bot)
            if (guid.IsParty() || IsRealPlayer(player) || IsSelfBot(player))
            {
                nonBotFound = true;
                break;
            }
        }

        return nonBotFound;
    }

    // 击杀任务检查
    void OnPlayerbotCheckKillTask(Player* player, Unit* victim) override
    {
        if (player)
            GuildTaskMgr::instance().CheckKillTask(player, victim);
    }

    // 公会请愿书检查 - 机器人不能签署
    void OnPlayerbotCheckPetitionAccount(Player* player, bool& found) override
    {
        if (!found)
            return;

        if (PlayerbotsMgr::instance().GetPlayerbotAI(player) != nullptr)
            found = false;
    }

    // 是否需要向该玩家发送更新 - 机器人不需要
    bool OnPlayerbotCheckUpdatesToSend(Player* player) override
    {
        PlayerbotAI* botAI = PlayerbotsMgr::instance().GetPlayerbotAI(player);

        if (botAI == nullptr)
            return true;

        return botAI->IsRealPlayer();
    }

    //By leewheel 2026-07-13: 处理bot发送的数据包
    void OnPlayerbotPacketSent(Player* player, WorldPacket const* packet) override
    {
        if (player == nullptr)
            return;

        PlayerbotAI* botAI = PlayerbotsMgr::instance().GetPlayerbotAI(player);

        if (botAI != nullptr)
            botAI->HandleBotOutgoingPacket(*packet);

        if (PlayerbotMgr* playerbotMgr = GET_PLAYERBOT_MGR(player))
            playerbotMgr->HandleMasterOutgoingPacket(*packet);
    }
    //End By leewheel

    // 世界更新时调用 - 处理机器人会话包队列
    void OnPlayerbotUpdate(uint32 /*diff*/) override
    {
        sRandomPlayerbotMgr.UpdateSessions();
    }

    // 玩家更新时调用 - 处理玩家机器人会话
    void OnPlayerbotUpdateSessions(Player* player) override
    {
        if (player)
            if (PlayerbotMgr* playerbotMgr = GET_PLAYERBOT_MGR(player))
                playerbotMgr->UpdateSessions();
    }

    // 玩家登出时调用
    void OnPlayerbotLogout(Player* player) override
    {
        if (PlayerbotMgr* playerbotMgr = GET_PLAYERBOT_MGR(player))
        {
            PlayerbotAI* botAI = PlayerbotsMgr::instance().GetPlayerbotAI(player);

            if (botAI == nullptr || botAI->IsRealPlayer())
            {
                playerbotMgr->LogoutAllBots();
            }
        }

        sRandomPlayerbotMgr.OnPlayerLogout(player);
    }

    // 服务器关闭时登出所有机器人
    void OnPlayerbotLogoutBots() override
    {
        TC_LOG_INFO("playerbots", "正在登出所有机器人...");
        sRandomPlayerbotMgr.LogoutAllBots();
    }
//End By leewheel 2026-07-13
};

// === MiscScript: 对象析构 ===
class PlayerbotsMiscScript : public MiscScript
{
public:
    PlayerbotsMiscScript() : MiscScript("PlayerbotsMiscScript") {}

    void OnDestructPlayer(Player* player) override
    {
        if (!player)
            return;

        PlayerbotAI* botAI = PlayerbotsMgr::instance().GetPlayerbotAI(player);

        if (botAI != nullptr)
            delete botAI;

        if (PlayerbotMgr* playerbotMgr = GET_PLAYERBOT_MGR(player))
            delete playerbotMgr;
    }
};

// === ServerScript: 数据包接收 ===
class PlayerbotsServerScript : public ServerScript
{
public:
    PlayerbotsServerScript() : ServerScript("PlayerbotsServerScript") {}

    //By leewheel 2026-07-12: TC核心已按AC语义扩展 CanPacketReceive(可拦截,const包),见 ScriptMgr.h
    bool CanPacketReceive(WorldSession* session, WorldPacket const& packet) override
    //End By leewheel
    {
        if (Player* player = session->GetPlayer())
            if (PlayerbotMgr* playerbotMgr = GET_PLAYERBOT_MGR(player))
                playerbotMgr->HandleMasterIncomingPacket(packet);

        return true;
    }
};

//By leewheel 2026-07-13: PlayerbotsScript类已删除
// 所有OnPlayerbot*方法已合并到PlayerbotsPlayerScript中
// 原因: ScriptMgr::OnPlayerbot*改为FOREACH_SCRIPT(PlayerScript)，
// PlayerbotScript子类不会被遍历到
//End By leewheel

// === BGScript: 战场策略 ===
class PlayerBotsBGScript : public AllBattlegroundScript
{
public:
    PlayerBotsBGScript() : AllBattlegroundScript("PlayerBotsBGScript") {}

    void OnBattlegroundStart(Battleground* bg) override
    {
        // 战场开始时随机选择阵营策略
        BGStrategyData data;

        //By leewheel 2026-07-12: TC的Battleground使用GetTypeID()而非AC的GetBgTypeID()
        switch (bg->GetTypeID())
        //End By leewheel
        {
            case BATTLEGROUND_WS:
                data.allianceStrategy = urand(0, WS_STRATEGY_MAX - 1);
                data.hordeStrategy = urand(0, WS_STRATEGY_MAX - 1);
                break;
            case BATTLEGROUND_AB:
                data.allianceStrategy = urand(0, AB_STRATEGY_MAX - 1);
                data.hordeStrategy = urand(0, AB_STRATEGY_MAX - 1);
                break;
            case BATTLEGROUND_AV:
                data.allianceStrategy = urand(0, AV_STRATEGY_MAX - 1);
                data.hordeStrategy = urand(0, AV_STRATEGY_MAX - 1);
                break;
            case BATTLEGROUND_EY:
                data.allianceStrategy = urand(0, EY_STRATEGY_MAX - 1);
                data.hordeStrategy = urand(0, EY_STRATEGY_MAX - 1);
                break;
            default:
                break;
        }

        bgStrategies[bg->GetInstanceID()] = data;
    }

    void OnBattlegroundEnd(Battleground* bg, TeamId /*winnerTeam*/) override
    {
        // 战场结束时清理策略数据
        bgStrategies.erase(bg->GetInstanceID());
    }
};

// === 脚本注册函数声明 ===
void AddPlayerbotsSecureLoginScripts();
//By leewheel 2026-09-05: 上游6704d553——selfbot AFK防登出脚本
void AddPlayerbotsSelfBotAfkScripts();
//End By leewheel
//By leewheel 2026-07-27: 移植brighton-chi玛瑟里顿团本服务端脚本钩子。
void AddSC_MagtheridonBotScripts();
//End By leewheel
void AddSC_TempestKeepBotScripts();
void AddSC_IcecrownBotScripts();
void AddSC_RubySanctumBotScripts();
void AddSC_HyjalSummitBotScripts();
//By leewheel 2026-07-26: 移植brighton-chi太阳井(SWP)团本服务端脚本钩子。
void AddSC_SunwellPlateauBotScripts();
//End By leewheel
void PlayerBotsGuildValidationScript();
//By leewheel 2026-07-20: 快速组队系统
void AddSC_FastGroup();
//End By leewheel
//By leewheel 2026-07-22: 玩家自用机器人辅助命令（机器人宠物嘲讽等）
void AddSC_ForPlayerCommand();
//End By leewheel
//By leewheel 2026-08-15: 随机机器人等级分档+满级重置
void AddSC_randombot_level_mgr();
//End By leewheel

// === 脚本注册入口 ===
void AddPlayerbotsScripts()
{
    new PlayerbotsDatabaseScript();
    new PlayerbotsPlayerScript();
    new PlayerbotsMiscScript();
    new PlayerbotsServerScript();
    new PlayerbotsWorldScript();
    //By leewheel 2026-07-13: PlayerbotsScript已删除，方法合并到PlayerbotsPlayerScript
    //End By leewheel
    new PlayerBotsBGScript();

    // 注册子脚本
    AddPlayerbotsSecureLoginScripts();
    AddPlayerbotsSelfBotAfkScripts();
    AddPlayerbotsCommandscripts();
    PlayerBotsGuildValidationScript();

    // 注册副本脚本
    //By leewheel 2026-07-27: 注册brighton-chi玛瑟里顿团本服务端脚本。
    AddSC_MagtheridonBotScripts();
    //End By leewheel
    AddSC_TempestKeepBotScripts();
    AddSC_IcecrownBotScripts();
    AddSC_RubySanctumBotScripts();
    AddSC_HyjalSummitBotScripts();
    //By leewheel 2026-07-26: 注册太阳井团本服务端脚本。
    AddSC_SunwellPlateauBotScripts();
    //End By leewheel

    //By leewheel 2026-07-20: 注册快速组队系统
    AddSC_FastGroup();
    //End By leewheel
    //By leewheel 2026-07-22: 注册玩家自用机器人辅助命令
    AddSC_ForPlayerCommand();
    //End By leewheel
    //By leewheel 2026-08-15: 注册随机机器人等级分档+满级重置
    AddSC_randombot_level_mgr();
    //End By leewheel
}
