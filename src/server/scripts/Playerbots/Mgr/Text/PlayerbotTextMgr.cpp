/*
 * 机器人文本管理器
 * 从 AzerothCore mod-playerbots 移植到 TrinityCore
 * 管理机器人聊天文本和回复
 */

#include "DatabaseEnv.h"
#include "World.h"
#include "Random.h"
#include "QueryResult.h"

#include "PlayerbotTextMgr.h"

void PlayerbotTextMgr::replaceAll(std::string& str, const std::string& from, const std::string& to)
{
    if (from.empty())
        return;
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos)
    {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();  // 防止 'to' 包含 'from' 时出现无限循环
    }
}

//By leewheel 2026-07-12: DB表列名为text_loc1..text_loc8, 非locale_1等;
//TC的TOTAL_LOCALES=12但DB只有8个语言列, 用OLD_TOTAL_LOCALES(9)限制循环
void PlayerbotTextMgr::LoadBotTexts()
{
    TC_LOG_INFO("playerbots", "正在加载机器人文本...");

    uint32 count = 0;
    if (QueryResult result = PlayerbotsDatabase.Query(
            "SELECT name, text, say_type, reply_type, text_loc1, text_loc2, text_loc3, text_loc4, text_loc5, text_loc6, text_loc7, text_loc8 FROM ai_playerbot_texts"))
    {
        do
        {
            std::map<uint32, std::string> text;
            Field* fields = result->Fetch();
            std::string name = fields[0].GetString();
            text[0] = fields[1].GetString();
            uint8 sayType = fields[2].GetUInt8();
            uint8 replyType = fields[3].GetUInt8();
            for (uint8 i = 1; i < OLD_TOTAL_LOCALES; ++i)
            {
                text[i] = fields[i + 3].GetString();
            }

            botTexts[name].push_back(BotTextEntry(name, text, sayType, replyType));
            ++count;
        } while (result->NextRow());
    }

    TC_LOG_INFO("playerbots", "已加载 {} 条机器人文本", count);
}
//End By leewheel 2026-07-12

void PlayerbotTextMgr::LoadBotTextChance()
{
    if (botTextChance.empty())
    {
        QueryResult results = PlayerbotsDatabase.Query("SELECT name, probability FROM ai_playerbot_texts_chance");
        if (results)
        {
            do
            {
                Field* fields = results->Fetch();
                std::string name = fields[0].GetString();
                uint32 probability = (uint32)fields[1].Get<uint64>(); //By leewheel 2026-07-12: TC的ai_playerbot_texts_chance.probability是bigint

                botTextChance[name] = probability;
            } while (results->NextRow());
        }
    }
}

// 通用文本

std::string PlayerbotTextMgr::GetBotText(std::string name)
{
    if (botTexts.empty())
    {
        TC_LOG_ERROR("playerbots", "无法获取机器人文本 {}！未加载任何机器人文本！", name);
        return "";
    }

    if (botTexts[name].empty())
    {
        TC_LOG_ERROR("playerbots", "无法获取机器人文本 {}！该名称下无文本！", name);
        return "";
    }

    std::vector<BotTextEntry>& list = botTexts[name];
    BotTextEntry textEntry = list[urand(0, list.size() - 1)];
    return !textEntry.m_text[GetLocalePriority()].empty() ? textEntry.m_text[GetLocalePriority()] : textEntry.m_text[0];
}

std::string PlayerbotTextMgr::GetBotText(std::string name, std::map<std::string, std::string> placeholders)
{
    std::string botText = GetBotText(name);
    if (botText.empty())
        return "";

    for (std::map<std::string, std::string>::iterator i = placeholders.begin(); i != placeholders.end(); ++i)
        replaceAll(botText, i->first, i->second);

    return botText;
}

std::string PlayerbotTextMgr::GetBotTextOrDefault(std::string name, std::string defaultText,
    std::map<std::string, std::string> placeholders)
{
    //By leewheel 2026-08-02: 静默fallback——有默认值时不再调用GetBotText(内部会打ERROR日志)
    //原实现: 调用GetBotText(name)找不到文本时打"无法获取机器人文本"ERROR日志刷屏
    //(实例: meeting_stone_in_combat等文本未录入ai_playerbot_texts表时, 每次触发都刷屏)
    //修复: 直接查botTexts映射, 找不到时静默使用默认值(不产生错误日志)
    if (!botTexts.empty())
    {
        auto itr = botTexts.find(name);
        if (itr != botTexts.end() && !itr->second.empty())
        {
            std::vector<BotTextEntry>& list = itr->second;
            BotTextEntry textEntry = list[urand(0, list.size() - 1)];
            std::string botText = !textEntry.m_text[GetLocalePriority()].empty() ? textEntry.m_text[GetLocalePriority()] : textEntry.m_text[0];
            for (std::map<std::string, std::string>::iterator i = placeholders.begin(); i != placeholders.end(); ++i)
                replaceAll(botText, i->first, i->second);
            return botText;
        }
    }
    //End By leewheel

    for (std::map<std::string, std::string>::iterator i = placeholders.begin(); i != placeholders.end(); ++i)
    {
        replaceAll(defaultText, i->first, i->second);
    }
    return defaultText;
}

// 聊天回复

std::string PlayerbotTextMgr::GetBotText(ChatReplyType replyType, std::map<std::string, std::string> placeholders)
{
    if (botTexts.empty())
    {
        TC_LOG_ERROR("playerbots", "无法获取机器人回复文本 {}！未加载任何机器人文本！", replyType);
        return "";
    }
    if (botTexts["reply"].empty())
    {
        TC_LOG_ERROR("playerbots", "无法获取机器人回复文本 {}！无回复文本！", replyType);
        return "";
    }

    std::vector<BotTextEntry>& list = botTexts["reply"];
    std::vector<BotTextEntry> proper_list;
    for (auto text : list)
    {
        //By leewheel 2026-09-04: 修 C4389——m_replyType 为 uint32, 枚举参数显式转同类型比较
        if (text.m_replyType == static_cast<uint32>(replyType))
            proper_list.push_back(text);
    }

    if (proper_list.empty())
        return "";

    BotTextEntry textEntry = proper_list[urand(0, proper_list.size() - 1)];
    std::string botText =
        !textEntry.m_text[GetLocalePriority()].empty() ? textEntry.m_text[GetLocalePriority()] : textEntry.m_text[0];
    for (auto& placeholder : placeholders)
        replaceAll(botText, placeholder.first, placeholder.second);

    return botText;
}

std::string PlayerbotTextMgr::GetBotText(ChatReplyType replyType, std::string name)
{
    std::map<std::string, std::string> placeholders;
    placeholders["%s"] = name;

    return GetBotText(replyType, placeholders);
}

// 概率

bool PlayerbotTextMgr::rollTextChance(std::string name)
{
    if (!botTextChance[name])
        return true;

    return urand(0, 100) < botTextChance[name];
}

bool PlayerbotTextMgr::GetBotText(std::string name, std::string& text)
{
    if (!rollTextChance(name))
        return false;

    text = GetBotText(name);
    return !text.empty();
}

bool PlayerbotTextMgr::GetBotText(std::string name, std::string& text, std::map<std::string, std::string> placeholders)
{
    if (!rollTextChance(name))
        return false;

    text = GetBotText(name, placeholders);
    return !text.empty();
}

void PlayerbotTextMgr::AddLocalePriority(uint32 locale)
{
    if (locale >= TOTAL_LOCALES)
    {
        TC_LOG_WARN("playerbots", "忽略语言 {} 的机器人文本优先级，因为超出了 TOTAL_LOCALES ({})", locale, TOTAL_LOCALES - 1);
        return;
    }

    botTextLocalePriority[locale]++;
}

uint32 PlayerbotTextMgr::GetLocalePriority()
{
    //By leewheel 2026-07-21: 强制使用服务器默认语言(DBC.Locale=4即zhCN)。
    // 原逻辑按在线玩家语言投票选topLocale, 但机器人session固定enUS(0)且数量远超真实玩家,
    // 导致topLocale恒为0, 已填充的中文text_loc4永远用不上, 机器人只说英文。
    // 直接返回服务器默认语言, 确保机器人聊天使用中文。
    return sWorld->GetDefaultDbcLocale();
    //End By leewheel
}

void PlayerbotTextMgr::ResetLocalePriority()
{
    for (uint8 i = 0; i < TOTAL_LOCALES; ++i)
    {
        botTextLocalePriority[i] = 0;
    }
}
