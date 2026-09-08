/*
 * 机器人文本管理器
 * 从 AzerothCore mod-playerbots 移植到 TrinityCore
 * 管理机器人聊天文本和回复
 */

#ifndef PLAYERBOTS_PLAYERBOTTEXTMGR_H
#define PLAYERBOTS_PLAYERBOTTEXTMGR_H

#include <map>
#include <vector>

#include "Common.h"

struct BotTextEntry
{
    BotTextEntry(std::string name, std::map<uint32, std::string> text, uint32 say_type, uint32 reply_type)
        : m_name(name), m_text(text), m_sayType(say_type), m_replyType(reply_type)
    {
    }
    std::string m_name;
    std::map<uint32, std::string> m_text;
    uint32 m_sayType;
    uint32 m_replyType;
};

struct ChatReplyData
{
    ChatReplyData(uint32 guid, uint32 type, std::string chat) : m_type(type), m_guid(guid), m_chat(chat) {}
    uint32 m_type, m_guid = 0;
    std::string m_chat = "";
};

// ChatQueuedReply 结构体定义在 PlayerbotAI.h 中，避免重定义

enum ChatReplyType
{
    REPLY_NOT_UNDERSTAND,
    REPLY_GRUDGE,
    REPLY_VICTIM,
    REPLY_ATTACKER,
    REPLY_HELLO,
    REPLY_NAME,
    REPLY_ADMIN_ABUSE
};

class PlayerbotTextMgr
{
public:
    static PlayerbotTextMgr& instance()
    {
        static PlayerbotTextMgr instance;

        return instance;
    }

    std::string GetBotText(std::string name, std::map<std::string, std::string> placeholders);
    std::string GetBotText(std::string name);
    std::string GetBotText(ChatReplyType replyType, std::map<std::string, std::string> placeholders);
    std::string GetBotText(ChatReplyType replyType, std::string name);
    bool GetBotText(std::string name, std::string& text);
    bool GetBotText(std::string name, std::string& text, std::map<std::string, std::string> placeholders);
    std::string GetBotTextOrDefault(std::string name, std::string defaultText,
                                    std::map<std::string, std::string> placeholders);
    void LoadBotTexts();
    void LoadBotTextChance();
    static void replaceAll(std::string& str, const std::string& from, const std::string& to);
    bool rollTextChance(std::string text);

    uint32 GetLocalePriority();
    void AddLocalePriority(uint32 locale);
    void ResetLocalePriority();

private:
    PlayerbotTextMgr()
    {
        for (uint8 i = 0; i < TOTAL_LOCALES; ++i)
        {
            botTextLocalePriority[i] = 0;
        }
    };
    ~PlayerbotTextMgr() = default;

    PlayerbotTextMgr(const PlayerbotTextMgr&) = delete;
    PlayerbotTextMgr& operator=(const PlayerbotTextMgr&) = delete;

    PlayerbotTextMgr(PlayerbotTextMgr&&) = delete;
    PlayerbotTextMgr& operator=(PlayerbotTextMgr&&) = delete;

    std::map<std::string, std::vector<BotTextEntry>> botTexts;
    std::map<std::string, uint32> botTextChance;
    uint32 botTextLocalePriority[TOTAL_LOCALES];
};

#define sPlayerbotTextMgr PlayerbotTextMgr::instance()

#endif
