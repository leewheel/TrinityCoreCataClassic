/*
 * 机器人管理器
 * 从 AzerothCore mod-playerbots 移植到 TrinityCore
 * 包含 PlayerbotHolder, PlayerbotMgr 和 PlayerbotsMgr 单例
 */

#ifndef PLAYERBOTS_PLAYERBOTMGR_H
#define PLAYERBOTS_PLAYERBOTMGR_H

//By leewheel 2026-07-12: 添加LoginQueryHolder和WorldSession头文件
#include "ObjectGuid.h"
#include "PlayerbotAIBase.h"
#include "WorldSession.h"
//End By leewheel

class Player;
class ChatHandler;
class PlayerbotAI;
class WorldPacket;
class WorldSession;

//By leewheel 2026-07-12: PlayerbotLoginQueryHolder继承LoginQueryHolder，添加masterAccountId
class PlayerbotLoginQueryHolder : public LoginQueryHolder
{
private:
    uint32 masterAccountId;
public:
    PlayerbotLoginQueryHolder(uint32 masterAccount, uint32 accountId, ObjectGuid guid)
        : LoginQueryHolder(accountId, guid), masterAccountId(masterAccount) {}
    uint32 GetMasterAccountId() const { return masterAccountId; }
};
//End By leewheel

typedef std::map<ObjectGuid, Player*> PlayerBotMap;
typedef std::map<std::string, std::set<std::string>> PlayerBotErrorMap;

class PlayerbotHolder : public PlayerbotAIBase
{
public:
    PlayerbotHolder();
    virtual ~PlayerbotHolder();

    void AddPlayerBot(ObjectGuid guid, uint32 masterAccountId);
    //By leewheel 2026-07-12: 添加登录回调函数声明
    void HandlePlayerBotLoginCallback(PlayerbotLoginQueryHolder const& holder);
    //End By leewheel
    bool IsAccountLinked(uint32 accountId, uint32 masterAccountId);
    void LogoutPlayerBot(ObjectGuid guid);
    void DisablePlayerBot(ObjectGuid guid);
    void RemoveFromPlayerbotsMap(ObjectGuid guid);
    Player* GetPlayerBot(ObjectGuid guid) const;
    PlayerBotMap::const_iterator GetPlayerBotsBegin() const { return playerBots.begin(); }
    PlayerBotMap::const_iterator GetPlayerBotsEnd() const { return playerBots.end(); }

    //By leewheel 2026-09-03 修复C4100警告：空实现的override参数未引用，显式省略参数名消除告警
    void UpdateAIInternal(uint32 /*elapsed*/, bool /*minimal*/ = false) override {}
    //End By leewheel
    void UpdateSessions();
    void HandleBotPackets(WorldSession* session);

    void LogoutAllBots();
    void OnBotLogin(Player* const bot);

    std::vector<std::string> HandlePlayerbotCommand(char const* args, Player* master = nullptr);
    std::string const ProcessBotCommand(std::string const cmd, ObjectGuid guid, ObjectGuid masterguid, bool admin,
                                        uint32 masterAccountId, uint32 masterGuildId);
    uint32 GetAccountId(std::string const name);
    uint32 GetAccountId(ObjectGuid guid);
    std::string const ListBots(Player* master);
    std::string const LookupBots(Player* master);
    uint32 GetPlayerbotsCount() { return (uint32)playerBots.size(); }
    uint32 GetPlayerbotsCountByClass(uint32 cls);

protected:
    virtual void OnBotLoginInternal(Player* const bot) = 0;

    PlayerBotMap playerBots;
    static std::unordered_map<ObjectGuid, uint32> botLoading;
};

class PlayerbotMgr : public PlayerbotHolder
{
public:
    PlayerbotMgr(Player* const master);
    virtual ~PlayerbotMgr();

    static bool HandlePlayerbotMgrCommand(ChatHandler* handler, char const* args);
    //By leewheel 2026-08-15: 账号信任链4命令(移植the-lab)——setKey存SHA-256密钥,
    //link校验密钥后双向写playerbots_account_links,linkedAccounts查看,unlink解除
    void HandleSetSecurityKeyCommand(Player* player, std::string const& key);
    void HandleLinkAccountCommand(Player* player, std::string const& accountName, std::string const& key);
    void HandleViewLinkedAccountsCommand(Player* player);
    void HandleUnlinkAccountCommand(Player* player, std::string const& accountName);
    //End By leewheel
    void HandleMasterIncomingPacket(WorldPacket const& packet);
    void HandleMasterOutgoingPacket(WorldPacket const& packet);
    void HandleCommand(uint32 type, std::string const text);
    void OnPlayerLogin(Player* player);
    void CancelLogout();

    void UpdateAIInternal(uint32 elapsed, bool minimal = false) override;
    void TellError(std::string const botName, std::string const text);

    Player* GetMaster() const { return master; };

    void SaveToDB();

protected:
    void OnBotLoginInternal(Player* const bot) override;
    void CheckTellErrors(uint32 elapsed);

private:
    Player* const master;
    PlayerBotErrorMap errors;
    time_t lastErrorTell = 0;
};

class PlayerbotsMgr
{
public:
    static PlayerbotsMgr& instance()
    {
        static PlayerbotsMgr instance;
        return instance;
    }

    void AddPlayerbotData(Player* player, bool isBotAI);
    void RemovePlayerBotData(ObjectGuid const& guid, bool is_AI);

    PlayerbotAI* GetPlayerbotAI(Player* player);
    PlayerbotMgr* GetPlayerbotMgr(Player* player);

private:
    PlayerbotsMgr() = default;
    ~PlayerbotsMgr() = default;

    PlayerbotsMgr(const PlayerbotsMgr&) = delete;
    PlayerbotsMgr& operator=(const PlayerbotsMgr&) = delete;

    PlayerbotsMgr(PlayerbotsMgr&&) = delete;
    PlayerbotsMgr& operator=(PlayerbotsMgr&&) = delete;

    std::unordered_map<ObjectGuid, PlayerbotAIBase*> _playerbotsAIMap;
    std::unordered_map<ObjectGuid, PlayerbotAIBase*> _playerbotsMgrMap;
};

#define sPlayerbotsMgr PlayerbotsMgr::instance()

#endif
