#ifndef PLAYERBOTS_PLAYERBOTGUILDMGR_H
#define PLAYERBOTS_PLAYERBOTGUILDMGR_H

#include "Guild.h"
#include "Player.h"
#include "PlayerbotAI.h"

class PlayerbotGuildMgr
{
public:
    static PlayerbotGuildMgr& instance()
    {
        static PlayerbotGuildMgr instance;

        return instance;
    }

    void Init();
    std::string AssignToGuild(Player* player);
    void LoadGuildNames();
    void ValidateGuildCache();
    void ResetGuildCache();
    bool CreateGuild(Player* player, std::string guildName);
    void OnGuildUpdate  (Guild* guild);
    bool SetGuildEmblem(uint32 guildId);
    //By leewheel 2026-09-05: 上游c1fed461——更名为DeleteRandomBotGuilds(按会长账号判定随机机器人公会)
    void DeleteRandomBotGuilds();
    //End By leewheel
    bool IsRealGuild(uint32 guildId);
    bool IsRealGuild(Player* bot);

private:
    PlayerbotGuildMgr() = default;
    ~PlayerbotGuildMgr() = default;

    PlayerbotGuildMgr(const PlayerbotGuildMgr&) = delete;
    PlayerbotGuildMgr& operator=(const PlayerbotGuildMgr&) = delete;

    PlayerbotGuildMgr(PlayerbotGuildMgr&&) = delete;
    PlayerbotGuildMgr& operator=(PlayerbotGuildMgr&&) = delete;

    std::unordered_map<std::string, bool> _guildNames;

    struct GuildCache
    {
        std::string name;
        uint8 status;
        uint32 maxMembers = 0;
        uint32 memberCount = 0;
        uint8 faction = 0;
        bool hasRealPlayer = false;
    };
    std::unordered_map<uint32 , GuildCache> _guildCache;
    std::vector<std::string> _shuffled_guild_keys;
};

void PlayerBotsGuildValidationScript();

#endif
