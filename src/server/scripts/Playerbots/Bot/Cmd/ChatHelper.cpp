/*
 * 聊天辅助工具实现
 * 从 AzerothCore mod-playerbots 移植到 TrinityCore
 */

#include "ChatHelper.h"

#include "Common.h"
#include "DB2Stores.h"
#include "ItemTemplate.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Playerbots.h"
#include "PlayerbotAI.h"
#include "SpellInfo.h"
#include "World.h"

#include <regex>
#include <charconv>

std::map<std::string, uint32> ChatHelper::consumableSubClasses;
std::map<std::string, uint32> ChatHelper::tradeSubClasses;
std::map<std::string, uint32> ChatHelper::itemQualities;
std::map<std::string, uint32> ChatHelper::projectileSubClasses;
std::map<std::string, uint32> ChatHelper::slots;
std::map<std::string, uint32> ChatHelper::skills;
std::map<std::string, ChatMsg> ChatHelper::chats;
std::map<uint8, std::string> ChatHelper::classes;
std::map<uint8, std::string> ChatHelper::races;
std::map<uint8, std::map<uint8, std::string>> ChatHelper::specs;

template <class T>
static bool substrContainsInMap(std::string const searchTerm, std::map<std::string, T> searchIn)
{
    for (typename std::map<std::string, T>::iterator i = searchIn.begin(); i != searchIn.end(); ++i)
    {
        std::string const term = i->first;
        if (term.size() > 1 && searchTerm.find(term) != std::string::npos)
            return true;
    }

    return false;
}

ChatHelper::ChatHelper(PlayerbotAI* botAI) : PlayerbotAIAware(botAI)
{
    itemQualities["poor"] = ITEM_QUALITY_POOR;
    itemQualities["gray"] = ITEM_QUALITY_POOR;
    itemQualities["normal"] = ITEM_QUALITY_NORMAL;
    itemQualities["white"] = ITEM_QUALITY_NORMAL;
    itemQualities["uncommon"] = ITEM_QUALITY_UNCOMMON;
    itemQualities["green"] = ITEM_QUALITY_UNCOMMON;
    itemQualities["rare"] = ITEM_QUALITY_RARE;
    itemQualities["blue"] = ITEM_QUALITY_RARE;
    itemQualities["epic"] = ITEM_QUALITY_EPIC;
    itemQualities["violet"] = ITEM_QUALITY_EPIC;
    itemQualities["legendary"] = ITEM_QUALITY_LEGENDARY;
    itemQualities["yellow"] = ITEM_QUALITY_LEGENDARY;
    //By leewheel 2026-07-22: 中文品质别名
    itemQualities["劣质"] = ITEM_QUALITY_POOR;
    itemQualities["灰色"] = ITEM_QUALITY_POOR;
    itemQualities["普通"] = ITEM_QUALITY_NORMAL;
    itemQualities["白色"] = ITEM_QUALITY_NORMAL;
    itemQualities["优秀"] = ITEM_QUALITY_UNCOMMON;
    itemQualities["绿色"] = ITEM_QUALITY_UNCOMMON;
    itemQualities["精良"] = ITEM_QUALITY_RARE;
    itemQualities["蓝色"] = ITEM_QUALITY_RARE;
    itemQualities["史诗"] = ITEM_QUALITY_EPIC;
    itemQualities["紫色"] = ITEM_QUALITY_EPIC;
    itemQualities["传说"] = ITEM_QUALITY_LEGENDARY;
    itemQualities["橙色"] = ITEM_QUALITY_LEGENDARY;
    //End By leewheel

    consumableSubClasses["potion"] = ITEM_SUBCLASS_POTION;
    consumableSubClasses["elixir"] = ITEM_SUBCLASS_ELIXIR;
    consumableSubClasses["flask"] = ITEM_SUBCLASS_FLASK;
    consumableSubClasses["scroll"] = ITEM_SUBCLASS_SCROLL;
    consumableSubClasses["food"] = ITEM_SUBCLASS_FOOD_DRINK;
    consumableSubClasses["bandage"] = ITEM_SUBCLASS_BANDAGE;
    consumableSubClasses["enchant"] = ITEM_SUBCLASS_CONSUMABLE_OTHER;
    //By leewheel 2026-07-22: 中文消耗品别名
    consumableSubClasses["药水"] = ITEM_SUBCLASS_POTION;
    consumableSubClasses["药剂"] = ITEM_SUBCLASS_ELIXIR;
    consumableSubClasses["合剂"] = ITEM_SUBCLASS_FLASK;
    consumableSubClasses["卷轴"] = ITEM_SUBCLASS_SCROLL;
    consumableSubClasses["食物"] = ITEM_SUBCLASS_FOOD_DRINK;
    consumableSubClasses["绷带"] = ITEM_SUBCLASS_BANDAGE;
    //End By leewheel

    projectileSubClasses["arrows"] = ITEM_SUBCLASS_ARROW;
    projectileSubClasses["bullets"] = ITEM_SUBCLASS_BULLET;

    slots["head"] = EQUIPMENT_SLOT_HEAD;
    slots["neck"] = EQUIPMENT_SLOT_NECK;
    slots["shoulder"] = EQUIPMENT_SLOT_SHOULDERS;
    slots["shirt"] = EQUIPMENT_SLOT_BODY;
    slots["chest"] = EQUIPMENT_SLOT_CHEST;
    slots["waist"] = EQUIPMENT_SLOT_WAIST;
    slots["legs"] = EQUIPMENT_SLOT_LEGS;
    slots["feet"] = EQUIPMENT_SLOT_FEET;
    slots["wrist"] = EQUIPMENT_SLOT_WRISTS;
    slots["hands"] = EQUIPMENT_SLOT_HANDS;
    slots["finger 1"] = EQUIPMENT_SLOT_FINGER1;
    slots["finger 2"] = EQUIPMENT_SLOT_FINGER2;
    slots["trinket 1"] = EQUIPMENT_SLOT_TRINKET1;
    slots["trinket 2"] = EQUIPMENT_SLOT_TRINKET2;
    slots["back"] = EQUIPMENT_SLOT_BACK;
    slots["main hand"] = EQUIPMENT_SLOT_MAINHAND;
    slots["off hand"] = EQUIPMENT_SLOT_OFFHAND;
    slots["ranged"] = EQUIPMENT_SLOT_RANGED;
    slots["tabard"] = EQUIPMENT_SLOT_TABARD;
    //By leewheel 2026-07-22: 中文装备槽别名
    slots["头部"] = EQUIPMENT_SLOT_HEAD;
    slots["项链"] = EQUIPMENT_SLOT_NECK;
    slots["肩膀"] = EQUIPMENT_SLOT_SHOULDERS;
    slots["衬衣"] = EQUIPMENT_SLOT_BODY;
    slots["胸部"] = EQUIPMENT_SLOT_CHEST;
    slots["腰带"] = EQUIPMENT_SLOT_WAIST;
    slots["腿部"] = EQUIPMENT_SLOT_LEGS;
    slots["脚部"] = EQUIPMENT_SLOT_FEET;
    slots["手腕"] = EQUIPMENT_SLOT_WRISTS;
    slots["手套"] = EQUIPMENT_SLOT_HANDS;
    slots["手指1"] = EQUIPMENT_SLOT_FINGER1;
    slots["手指2"] = EQUIPMENT_SLOT_FINGER2;
    slots["饰品1"] = EQUIPMENT_SLOT_TRINKET1;
    slots["饰品2"] = EQUIPMENT_SLOT_TRINKET2;
    slots["背部"] = EQUIPMENT_SLOT_BACK;
    slots["主手"] = EQUIPMENT_SLOT_MAINHAND;
    slots["副手"] = EQUIPMENT_SLOT_OFFHAND;
    slots["远程"] = EQUIPMENT_SLOT_RANGED;
    slots["战袍"] = EQUIPMENT_SLOT_TABARD;
    //End By leewheel

    skills["first aid"] = SKILL_FIRST_AID;
    skills["fishing"] = SKILL_FISHING;
    skills["cooking"] = SKILL_COOKING;
    skills["alchemy"] = SKILL_ALCHEMY;
    skills["enchanting"] = SKILL_ENCHANTING;
    skills["engineering"] = SKILL_ENGINEERING;
    skills["leatherworking"] = SKILL_LEATHERWORKING;
    skills["blacksmithing"] = SKILL_BLACKSMITHING;
    skills["tailoring"] = SKILL_TAILORING;
    skills["herbalism"] = SKILL_HERBALISM;
    skills["mining"] = SKILL_MINING;
    skills["skinning"] = SKILL_SKINNING;
    skills["jewelcrafting"] = SKILL_JEWELCRAFTING;
    //By leewheel 2026-07-22: 中文技能别名
    skills["急救"] = SKILL_FIRST_AID;
    skills["钓鱼"] = SKILL_FISHING;
    skills["烹饪"] = SKILL_COOKING;
    skills["炼金"] = SKILL_ALCHEMY;
    skills["附魔"] = SKILL_ENCHANTING;
    skills["工程"] = SKILL_ENGINEERING;
    skills["制皮"] = SKILL_LEATHERWORKING;
    skills["锻造"] = SKILL_BLACKSMITHING;
    skills["裁缝"] = SKILL_TAILORING;
    skills["草药"] = SKILL_HERBALISM;
    skills["采矿"] = SKILL_MINING;
    skills["剥皮"] = SKILL_SKINNING;
    skills["珠宝"] = SKILL_JEWELCRAFTING;
    //End By leewheel

    chats["party"] = CHAT_MSG_PARTY;
    chats["p"] = CHAT_MSG_PARTY;
    chats["guild"] = CHAT_MSG_GUILD;
    chats["g"] = CHAT_MSG_GUILD;
    chats["raid"] = CHAT_MSG_RAID;
    chats["r"] = CHAT_MSG_RAID;
    chats["whisper"] = CHAT_MSG_WHISPER;
    chats["w"] = CHAT_MSG_WHISPER;
    //By leewheel 2026-07-22: 中文聊天频道别名
    chats["队伍"] = CHAT_MSG_PARTY;
    chats["公会"] = CHAT_MSG_GUILD;
    chats["团队"] = CHAT_MSG_RAID;
    chats["密语"] = CHAT_MSG_WHISPER;
    //End By leewheel

    //By leewheel 2026-08-01: 玩家可见文本中文化
    classes[CLASS_DRUID] = "德鲁伊";
    specs[CLASS_DRUID][0] = "平衡";
    specs[CLASS_DRUID][1] = "野性战斗";
    specs[CLASS_DRUID][2] = "恢复";

    classes[CLASS_HUNTER] = "猎人";
    specs[CLASS_HUNTER][0] = "野兽控制";
    specs[CLASS_HUNTER][1] = "射击";
    specs[CLASS_HUNTER][2] = "生存";

    classes[CLASS_MAGE] = "法师";
    specs[CLASS_MAGE][0] = "奥术";
    specs[CLASS_MAGE][1] = "火焰";
    specs[CLASS_MAGE][2] = "冰霜";

    classes[CLASS_PALADIN] = "圣骑士";
    specs[CLASS_PALADIN][0] = "神圣";
    specs[CLASS_PALADIN][1] = "防护";
    specs[CLASS_PALADIN][2] = "惩戒";

    classes[CLASS_PRIEST] = "牧师";
    specs[CLASS_PRIEST][0] = "戒律";
    specs[CLASS_PRIEST][1] = "神圣";
    specs[CLASS_PRIEST][2] = "暗影";

    classes[CLASS_ROGUE] = "潜行者";
    specs[CLASS_ROGUE][0] = "刺杀";
    specs[CLASS_ROGUE][1] = "战斗";
    specs[CLASS_ROGUE][2] = "敏锐";

    classes[CLASS_SHAMAN] = "萨满祭司";
    specs[CLASS_SHAMAN][0] = "元素";
    specs[CLASS_SHAMAN][1] = "增强";
    specs[CLASS_SHAMAN][2] = "恢复";

    classes[CLASS_WARLOCK] = "术士";
    specs[CLASS_WARLOCK][0] = "痛苦";
    specs[CLASS_WARLOCK][1] = "恶魔学识";
    specs[CLASS_WARLOCK][2] = "毁灭";

    classes[CLASS_WARRIOR] = "战士";
    specs[CLASS_WARRIOR][0] = "武器";
    specs[CLASS_WARRIOR][1] = "狂怒";
    specs[CLASS_WARRIOR][2] = "防护";

    classes[CLASS_DEATH_KNIGHT] = "死亡骑士";
    specs[CLASS_DEATH_KNIGHT][0] = "鲜血";
    specs[CLASS_DEATH_KNIGHT][1] = "冰霜";
    specs[CLASS_DEATH_KNIGHT][2] = "邪恶";
    //End By leewheel

    //By leewheel 2026-08-01: 玩家可见文本中文化
    races[RACE_DWARF] = "矮人";
    races[RACE_GNOME] = "侏儒";
    races[RACE_HUMAN] = "人类";
    races[RACE_NIGHTELF] = "暗夜精灵";
    races[RACE_ORC] = "兽人";
    races[RACE_TAUREN] = "牛头人";
    races[RACE_TROLL] = "巨魔";
    races[RACE_UNDEAD_PLAYER] = "亡灵";
    races[RACE_BLOODELF] = "血精灵";
    races[RACE_DRAENEI] = "德莱尼";
    //End By leewheel
}

std::string const ChatHelper::formatMoney(uint32 copper)
{
    std::ostringstream out;
    if (!copper)
    {
        out << "0";
        return out.str();
    }

    uint32 gold = uint32(copper / 10000);
    copper -= (gold * 10000);
    uint32 silver = uint32(copper / 100);
    copper -= (silver * 100);

    //By leewheel 2026-08-01: 玩家可见文本中文化
    bool space = false;
    if (gold > 0)
    {
        out << gold << "金";
        space = true;
    }

    if (silver > 0 && gold < 50)
    {
        if (space)
            out << " ";

        out << silver << "银";
        space = true;
    }

    if (copper > 0 && gold < 10)
    {
        if (space)
            out << " ";

        out << copper << "铜";
    }
    //End By leewheel

    return out.str();
}

std::string ChatHelper::parseValue(std::string const& type, std::string const& text)
{
    std::string retString;

    std::string pattern = "Hvalue:" + type + ":";

    int pos = text.find(pattern, 0);
    if (pos == -1)
        return retString;

    pos += pattern.size();

    int endPos = text.find('|', pos);
    if (endPos == -1)
        return retString;

    retString = text.substr(pos, endPos - pos);
    return retString;
}

uint32 ChatHelper::parseMoney(std::string const text)
{
    std::string acum = "";
    uint32 copper = 0;
    for (uint8 i = 0; i < text.length(); i++)
    {
        //By leewheel 2026-08-01: 玩家可见文本中文化，解析中文货币单位(金/银/铜，UTF-8三字节)
        if (text.compare(i, 3, "金") == 0)
        {
            copper += (atol(acum.c_str()) * 100 * 100);
            acum = "";
            i += 2;
        }
        else if (text.compare(i, 3, "铜") == 0)
        {
            copper += atol(acum.c_str());
            acum = "";
            i += 2;
        }
        else if (text.compare(i, 3, "银") == 0)
        {
            copper += (atol(acum.c_str()) * 100);
            acum = "";
            i += 2;
        }
        //End By leewheel
        else if (text[i] == 'g')
        {
            copper += (atol(acum.c_str()) * 100 * 100);
            acum = "";
        }
        else if (text[i] == 'c')
        {
            copper += atol(acum.c_str());
            acum = "";
        }
        else if (text[i] == 's')
        {
            copper += (atol(acum.c_str()) * 100);
            acum = "";
        }
        else if (text[i] == ' ')
            break;
        else if (text[i] >= 48 && text[i] <= 57)
            acum += text[i];
        else
        {
            copper = 0;
            break;
        }
    }
    return copper;
}

ItemIds ChatHelper::parseItems(std::string const text)
{
    ItemIds itemIds;

    uint8 pos = 0;
    while (true)
    {
        auto i = text.find("Hitem:", pos);
        if (i == std::string::npos)
            break;

        pos = i + 6;
        auto endPos = text.find(':', pos);
        if (endPos == std::string::npos)
            break;

        std::string const idC = text.substr(pos, endPos - pos);
        auto id = atol(idC.c_str());
        pos = endPos;
        if (id)
            itemIds.insert(id);
    }

    return itemIds;
}

ItemWithRandomProperty ChatHelper::parseItemWithRandomProperty(std::string const text)
{
    ItemWithRandomProperty res;

    size_t itemStart = text.find("Hitem:");
    if (itemStart == std::string::npos)
        return res;

    itemStart += 6;
    if (itemStart >= text.length())
        return res;

    size_t colonPos = text.find(':', itemStart);
    if (colonPos == std::string::npos)
        return res;

    std::string itemIdStr = text.substr(itemStart, colonPos - itemStart);
    res.itemId = atoi(itemIdStr.c_str());

    std::vector<std::string> params;
    size_t currentPos = colonPos + 1;

    while (currentPos < text.length())
    {
        size_t nextColon = text.find(':', currentPos);
        if (nextColon == std::string::npos)
        {
            size_t hTag = text.find("|h", currentPos);
            if (hTag != std::string::npos)
            {
                params.push_back(text.substr(currentPos, hTag - currentPos));
            }
            break;
        }

        params.push_back(text.substr(currentPos, nextColon - currentPos));
        currentPos = nextColon + 1;
    }

    if (params.size() >= 6)
    {
        res.randomPropertyId = atoi(params[5].c_str());
    }

    return res;
}

std::string const ChatHelper::FormatQuest(Quest const* quest)
{
    if (!quest)
    {
        //By leewheel 2026-08-01: 玩家可见文本中文化
        return "无效任务";
        //End By leewheel
    }

    std::ostringstream out;
    std::string questTitle = quest->GetLogTitle();
    //By leewheel 2026-07-21: 使用服务器默认语言(zhCN)的任务名, 原GetLogTitle()恒为英文
    // 从quest_template_locale读取本地化LogTitle, 无本地化数据时保留英文回退
    if (QuestTemplateLocale const* questLocale = sObjectMgr->GetQuestLocale(quest->GetQuestId()))
        ObjectMgr::GetLocaleString(questLocale->LogTitle, sWorld->GetDefaultDbcLocale(), questTitle);
    //End By leewheel

    out << "|cFFFFFF00|Hquest:" << quest->GetQuestId() << ':' << quest->GetQuestLevel() << "|h[" << questTitle << "]|h|r";
    return out.str();
}

std::string const ChatHelper::FormatGameobject(GameObject* go)
{
    std::ostringstream out;
    out << "|cFFFFFF00|Hfound:" << go->GetGUID().ToString() << ":" << go->GetEntry() << ":"
        << "|h[" << go->GetNameForLocaleIdx(sWorld->GetDefaultDbcLocale()) << "]|h|r";
    return out.str();
}

std::string const ChatHelper::FormatWorldobject(WorldObject* wo)
{
    std::ostringstream out;
    out << "|cFFFFFF00|Hfound:" << wo->GetGUID().ToString() << ":" << wo->GetEntry() << ":"
        << "|h[";
    out << (wo->ToGameObject() ? ((GameObject*)wo)->GetNameForLocaleIdx(sWorld->GetDefaultDbcLocale())
                               : wo->GetNameForLocaleIdx(sWorld->GetDefaultDbcLocale()))
        << "]|h|r";
    return out.str();
}

std::string const ChatHelper::FormatWorldEntry(int32 entry)
{
    CreatureTemplate const* cInfo = nullptr;
    GameObjectTemplate const* gInfo = nullptr;

    if (entry > 0)
        cInfo = sObjectMgr->GetCreatureTemplate(entry);
    else
        gInfo = sObjectMgr->GetGameObjectTemplate(entry * -1);

    std::ostringstream out;
    out << "|cFFFFFF00|Hentry:" << abs(entry) << ":"
        << "|h[";

    if (entry < 0 && gInfo)
        out << gInfo->name;
    else if (entry > 0 && cInfo)
        out << cInfo->Name;
    else
        //By leewheel 2026-08-01: 玩家可见文本中文化
        out << "未知";
    //End By leewheel

    out << "]|h|r";
    return out.str();
}

std::string const ChatHelper::FormatSpell(SpellInfo const* spellInfo)
{
    std::ostringstream out;
    //By leewheel 2026-07-13: 使用多locale辅助函数
    out << "|cffffffff|Hspell:" << spellInfo->Id << "|h[" << GetSpellNameBestLocaleWithCache(spellInfo->Id, spellInfo->SpellName) << "]|h|r";
    //End By leewheel
    return out.str();
}

std::string const ChatHelper::FormatItem(ItemTemplate const* proto, uint32 count, uint32 total)
{
    char color[32];
    snprintf(color, sizeof(color), "%x", ItemQualityColors[proto->GetQuality()]);

    //By leewheel 2026-07-21: 使用服务器默认语言(zhCN)的物品名, 原硬编码LOCALE_enUS导致聊天里装备名是英文
    std::string itemName = proto->ExtendedData->Display[sWorld->GetDefaultDbcLocale()];
    if (itemName.empty())
        itemName = proto->ExtendedData->Display[LOCALE_enUS];
    //End By leewheel

    std::ostringstream out;
    out << "|c" << color << "|Hitem:" << proto->GetId() << ":0:0:0:0:0:0:0"
        << "|h[" << itemName << "]|h|r";

    if (count > 1)
        out << "x" << count;

    if (total > 0)
        out << " (" << total << ")";

    return out.str();
}

std::string const ChatHelper::FormatQItem(uint32 itemId)
{
    char color[32];
    snprintf(color, sizeof(color), "%x", ItemQualityColors[0]);

    std::ostringstream out;
    out << "|c" << color << "|Hitem:" << itemId << ":0:0:0:0:0:0:0"
        //By leewheel 2026-08-01: 玩家可见文本中文化
        << "|h[物品"
        //End By leewheel
        << "]|h|r";

    return out.str();
}

ChatMsg ChatHelper::parseChat(std::string const text)
{
    if (chats.find(text) != chats.end())
        return chats[text];

    return CHAT_MSG_SYSTEM;
}

std::string const ChatHelper::FormatChat(ChatMsg chat)
{
    switch (chat)
    {
        //By leewheel 2026-08-01: 玩家可见文本中文化
        case CHAT_MSG_GUILD:
            return "公会";
        case CHAT_MSG_PARTY:
            return "队伍";
        case CHAT_MSG_WHISPER:
            return "密语";
        case CHAT_MSG_RAID:
            return "团队";
        default:
            break;
        //End By leewheel
    }

    //By leewheel 2026-08-01: 玩家可见文本中文化
    return "未知";
    //End By leewheel
}

uint32 ChatHelper::parseSpell(std::string const text)
{
    PlayerbotChatHandler handler(botAI->GetBot());
    return handler.extractSpellId(text);
}

GuidVector ChatHelper::parseGameobjects(std::string const text)
{
    GuidVector gos;

    uint8 pos = 0;
    while (true)
    {
        auto i = text.find("Hfound:", pos);
        if (i == std::string::npos)
            break;

        pos = i + 7;
        auto endPos = text.find(':', pos);
        if (endPos == std::string::npos)
            break;

        std::string guidStr = text.substr(pos, endPos - pos);
        ObjectGuid lootCurrent = ObjectGuid::FromString(guidStr);

        pos = endPos + 1;
        endPos = text.find(':', pos);
        if (endPos == std::string::npos)
            break;

        if (!lootCurrent.IsEmpty())
            gos.push_back(lootCurrent);
    }

    return gos;
}

std::string const ChatHelper::FormatQuestObjective(std::string const name, uint32 available, uint32 required)
{
    std::ostringstream out;
    out << "|cFFFFFFFF" << name << (available >= required ? "|c0000FF00: " : "|c00FF0000: ") << available << "/"
        << required << "|r";

    return out.str();
}

uint32 ChatHelper::parseItemQuality(std::string const text)
{
    if (itemQualities.find(text) == itemQualities.end())
        return MAX_ITEM_QUALITY;

    return itemQualities[text];
}

bool ChatHelper::parseItemClass(std::string const text, uint32* itemClass, uint32* itemSubClass)
{
    if (text == "questitem")
    {
        *itemClass = ITEM_CLASS_QUEST;
        *itemSubClass = ITEM_SUBCLASS_QUEST;
        return true;
    }

    if (consumableSubClasses.find(text) != consumableSubClasses.end())
    {
        *itemClass = ITEM_CLASS_CONSUMABLE;
        *itemSubClass = consumableSubClasses[text];
        return true;
    }

    if (tradeSubClasses.find(text) != tradeSubClasses.end())
    {
        *itemClass = ITEM_CLASS_TRADE_GOODS;
        *itemSubClass = tradeSubClasses[text];
        return true;
    }

    if (projectileSubClasses.find(text) != projectileSubClasses.end())
    {
        *itemClass = ITEM_CLASS_PROJECTILE;
        *itemSubClass = projectileSubClasses[text];
        return true;
    }

    return false;
}

uint32 ChatHelper::parseSlot(std::string const text)
{
    if (slots.find(text) != slots.end())
        return slots[text];

    return EQUIPMENT_SLOT_END;
}

bool ChatHelper::parseableItem(std::string const text)
{
    return text.find("|Hitem:") != std::string::npos || text == "questitem" || text == "ammo" ||
           substrContainsInMap<uint32>(text, consumableSubClasses) ||
           substrContainsInMap<uint32>(text, tradeSubClasses) || substrContainsInMap<uint32>(text, itemQualities) ||
           substrContainsInMap<uint32>(text, slots) || substrContainsInMap<ChatMsg>(text, chats) ||
           substrContainsInMap<uint32>(text, skills) || parseMoney(text) > 0;
}

std::string const ChatHelper::FormatClass(Player* player, int8 spec)
{
    uint8 cls = player->GetClass();

    std::ostringstream out;
    out << specs[cls][spec] << " (";

    // 内联获取玩家天赋标签页计数（原 AiFactory::GetPlayerSpecTabs）
    std::map<uint8, uint32> tabs;
    for (uint32 i = 0; i < MAX_TALENT_TABS; ++i)
    {
        tabs[i] = 0;
    }

    //By leewheel 2026-07-23: 修复天赋页计数，TabID是TalentTabEntry ID不能直接当索引，
    //需与职业天赋页ID比较确定索引；Rank是0-based需+1
    uint32 const* talentTabIds = sDB2Manager.GetTalentTabPages(cls);
    PlayerTalentMap const& talentMap = player->GetPlayerTalentMap(player->GetActiveTalentGroup());
    for (auto const& itr : talentMap)
    {
        if (itr.second.State == PLAYERSPELL_REMOVED)
            continue;

        TalentEntry const* talentInfo = sTalentStore.LookupEntry(itr.first);
        if (!talentInfo)
            continue;

        uint32 rank = itr.second.Rank + 1;
        if (talentInfo->TabID == talentTabIds[0])
            tabs[0] += rank;
        else if (talentInfo->TabID == talentTabIds[1])
            tabs[1] += rank;
        else if (talentInfo->TabID == talentTabIds[2])
            tabs[2] += rank;
    }
    //End By leewheel

    uint32 c0 = tabs[0];
    uint32 c1 = tabs[1];
    uint32 c2 = tabs[2];

    out << (c0 ? "|h|cff00ff00" : "") << c0 << "|h|cffffffff/";
    out << (c1 ? "|h|cff00ff00" : "") << c1 << "|h|cffffffff/";
    out << (c2 ? "|h|cff00ff00" : "") << c2 << "|h|cffffffff";

    out << ")|r " << classes[cls];
    return out.str();
}

std::string const ChatHelper::FormatClass(uint8 cls) { return classes[cls]; }

std::string const ChatHelper::FormatRace(uint8 race) { return races[race]; }

uint32 ChatHelper::parseSkill(std::string const text)
{
    if (skills.find(text) != skills.end())
        return skills[text];

    return SKILL_NONE;
}

std::string const ChatHelper::FormatSkill(uint32 skill)
{
    //By leewheel 2026-08-01: 还原为英文返回——FormatSkill返回值同时被WhoAction::QuerySkill
    //用作"spell id"查找键(按英文技能名匹配)，返回中文会破坏该查找逻辑。
    //技能名显示由客户端本地化处理，无需在此中文化。
    for (std::map<std::string, uint32>::iterator i = skills.begin(); i != skills.end(); ++i)
    {
        if (i->second == skill)
            return i->first;
    }
    //End By leewheel

    return "";
}

//By leewheel 2026-08-01: 玩家可见文本中文化
std::string const ChatHelper::FormatBoolean(bool flag) { return flag ? "|cff00ff00开启|r" : "|cffffff00关闭|r"; }
//End By leewheel

void ChatHelper::eraseAllSubStr(std::string& mainStr, std::string const toErase)
{
    size_t pos = std::string::npos;

    while ((pos = mainStr.find(toErase)) != std::string::npos)
    {
        mainStr.erase(pos, toErase.length());
    }
}

std::set<uint32> extractGeneric(std::string_view text, std::string_view prefix)
{
    std::set<uint32_t> ids;
    std::string_view text_view = text;

    size_t pos = 0;
    while ((pos = text_view.find(prefix, pos)) != std::string::npos)
    {
        pos += prefix.size();

        size_t end_pos = text_view.find_first_not_of("0123456789", pos);
        std::string_view number_str = text_view.substr(pos, end_pos - pos);

        uint32 number = 0;
        auto [ptr, ec] = std::from_chars(number_str.data(), number_str.data() + number_str.size(), number);

        if (ec == std::errc())
        {
            ids.insert(number);
        }
        pos = end_pos;
    }

    return ids;
}

std::set<uint32> ChatHelper::ExtractAllQuestIds(std::string const& text)
{
    return extractGeneric(text, "Hquest:");
}

std::set<uint32> ChatHelper::ExtractAllItemIds(std::string const& text)
{
    return extractGeneric(text, "Hitem:");
}

//By leewheel 2026-07-22
// 中文命令别名完整对照表，涵盖Playerbots所有英文密语/队伍/团队/公会命令
// 中英文命令均有效，中文别名会被解析为对应英文触发器名称
std::string ChatHelper::ResolveChatCommandAlias(std::string const& command)
{
    static const std::pair<const char*, const char*> aliases[] = {
        // ==================== 用户特别要求的别名 ====================
        {"集合", "summon"},         // 召唤机器人集合
        {"过来", "summon"},         // 召唤机器人过来
        {"来", "summon"},           // 召唤机器人来
        {"回来", "summon"},         // 召唤机器人回来

        // ==================== 基本命令 ====================
        {"帮助", "help"},
        {"属性", "stats"},
        {"位置", "position"},
        {"目标", "target"},
        {"攻击者", "attackers"},
        {"谁", "who"},
        {"日志", "log"},
        {"视线", "los"},
        {"光环", "aura"},

        // ==================== 移动/跟随 ====================
        {"跟随", "follow"},
        {"停留", "stay"},
        {"别动", "stay"},
        {"召唤", "summon"},
        {"前往", "go"},
        {"练级", "grind"},
        {"逃跑", "flee"},
        {"远离", "runaway"},
        {"脱离队伍", "move from group"},

        // ==================== 队伍/组队 ====================
        {"邀请", "invite"},
        {"离开", "leave"},
        {"随机副本", "lfg"},
        {"给予队长", "give leader"},
        {"就绪", "ready"},
        {"就绪检查", "ready check"},

        // ==================== 战斗 ====================
        {"攻击", "attack"},
        {"攻击目标", "attack"},
        {"打", "attack"},
        {"拉怪", "pull"},
        {"拉目标", "pull"},
        {"拉回", "pull back"},
        {"拉怪标记", "pull rti"},
        {"坦克攻击", "tank attack"},
        {"最大输出", "max dps"},
        {"输出", "dps"},
        {"驱散", "disperse"},
        {"擦除", "wipe"},
        {"警告", "warning"},
        {"等待攻击", "wait for attack time"},
        {"随机攻击", "ra"},
        {"焦点治疗", "focus heal"},
        {"保存法力", "save mana"},

        // ==================== 物品/装备 ====================
        {"使用", "u"},
        {"使用物品", "use"},
        {"数量", "c"},
        {"物品", "items"},
        {"物品栏", "inv"},
        {"装备", "e"},
        {"穿戴", "equip"},
        {"卸下", "ue"},
        {"出售", "s"},
        {"卖", "s"},
        {"购买", "b"},
        {"买", "b"},
        {"奖励", "r"},
        {"交易", "t"},
        {"不交易", "nt"},
        {"销毁", "destroy"},
        {"修理", "repair"},
        {"升级装备", "equip upgrade"},
        {"装备升级", "equip upgrade"},
        {"自动装备", "autogear"},
        {"自动装备极品", "autogear bis"},
        {"套装", "outfit"},
        {"徽记", "emblems"},
        {"打开物品", "open items"},
        {"解锁物品", "unlock items"},
        {"解锁交易物品", "unlock traded item"},

        // ==================== 拾取 ====================
        {"低威胁拾取", "ll"},
        {"灵魂拾取", "ss"},
        {"全部拾取", "add all loot"},
        {"拾取全部", "loot all"},
        {"全部捡取", "add all loot"},

        // ==================== 法术/技能 ====================
        {"施法", "cast"},
        {"非战斗施法", "castnc"},
        {"法术", "spells"},
        {"法术书", "spells"},
        {"法术信息", "spell"},
        {"天赋", "talents"},
        {"增益", "buff"},
        {"雕文", "glyphs"},
        {"雕文装备", "glyph equip"},
        {"移除雕文", "remove glyph"},
        {"喝水", "drink"},
        {"驯服", "tame"},

        // ==================== 任务 ====================
        {"任务", "q"},
        {"查询任务", "q"},
        {"任务列表", "quests"},
        {"接受", "accept"},
        {"接受任务", "accept"},
        {"放弃", "drop"},
        {"分享", "share"},
        {"查询物品", "qi"},

        // ==================== 金币/银行/邮件 ====================
        {"银行", "bank"},
        {"大祝福", "gb"},
        {"公会银行", "gbank"},
        {"邮件", "mail"},
        {"发邮件", "sendmail"},
        {"出售物品", "wts"},
        {"制作", "craft"},
        {"旗帜", "flag"},
        {"计算", "calc"},

        // ==================== 声望/PVP ====================
        {"声望", "rep"},
        {"声望值", "reputation"},
        {"PVP统计", "pvp stats"},

        // ==================== 死亡/复活 ====================
        {"死亡", "de"},
        {"释放", "release"},
        {"复活", "revive"},
        {"灵魂医者", "revive"},

        // ==================== 传送/旅行 ====================
        {"传送", "teleport"},
        {"飞行", "taxi"},
        {"炉石", "home"},
        {"进入载具", "enter vehicle"},
        {"离开载具", "leave vehicle"},

        // ==================== 策略/AI ====================
        {"战斗", "co"},
        {"非战斗", "nc"},
        {"重置AI", "reset botAI"},
        {"重置机器人AI", "reset botAI"},
        {"策略", "cs"},
        {"记录点", "rtsc"},
        {"调试", "debug"},
        {"副本调试", "cdebug"},
        {"作弊", "cheat"},
        {"阵型", "formation"},
        {"姿态", "stance"},

        // ==================== 训练 ====================
        {"训练师", "trainer"},
        {"维护", "maintenance"},

        // ==================== 社交 ====================
        {"聊天", "chat"},
        {"说话", "talk"},
        {"对话", "talk"},
        {"表情", "emote"},
        {"团队标记", "rti"},
        // {"雇佣", "hire"},  // hire 触发器在 ChatTriggerContext.h 被注释（会崩溃），中文别名一并禁用
        {"范围", "range"},
        {"掷点", "roll"},

        // ==================== 公会 ====================
        {"公会邀请", "ginvite"},
        {"公会晋升", "guild promote"},
        {"公会降级", "guild demote"},
        {"公会移除", "guild remove"},
        {"离开公会", "guild leave"},

        // ==================== 宠物 ====================
        {"宠物", "pet"},
        {"宠物攻击", "pet attack"},

        // ==================== RPG ====================
        {"RPG状态", "rpg status"},
        {"RPG任务", "rpg do quest"},

        // ==================== 德鲁伊形态取消 ====================
        {"取消树形态", "cancel tree form"},
        {"取消旅行形态", "cancel travel form"},
        {"取消熊形态", "cancel bear form"},
        {"取消巨熊形态", "cancel dire bear form"},
        {"取消猫形态", "cancel cat form"},
        {"取消枭兽形态", "cancel moonkin form"},
        {"取消水栖形态", "cancel aquatic form"},

        // ==================== 副本快捷命令 ====================
        {"纳克萨玛斯", "naxx"},
        {"NAXX", "naxx"},
        {"黑翼之巢", "bwl"},
        {"BWL", "bwl"},

        // ==================== 背包别名 ====================
        {"背包", "inventory"},
    };

    // 1. 精确匹配（无参数命令，如 "集合" -> "summon"）
    for (auto const& [zh, en] : aliases)
    {
        if (command == zh)
            return en;
    }

    // 2. 前缀匹配（带参数命令，如 "施法 火球术" -> "cast 火球术"）
    //    取第一个空格前的部分作为命令前缀，在别名表里查找
    //    匹配成功则替换前缀为英文，保留参数部分
    size_t spacePos = command.find(' ');
    if (spacePos != std::string::npos)
    {
        std::string prefix = command.substr(0, spacePos);
        std::string param = command.substr(spacePos + 1);
        for (auto const& [zh, en] : aliases)
        {
            if (prefix == zh)
                return std::string(en) + " " + param;
        }
    }

    return command;
}
//End By leewheel
