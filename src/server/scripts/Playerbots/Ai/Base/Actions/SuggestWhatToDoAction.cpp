/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include <functional>

#include "SuggestWhatToDoAction.h"
#include "ServerFacade.h"
#include "Event.h"
#include "ItemVisitors.h"
#include "AiFactory.h"
#include "ChatHelper.h"
#include "Playerbots.h"
#include "BroadcastHelper.h"
#include "AiFactory.h"
#include "ChatHelper.h"
#include "Event.h"
#include "ItemVisitors.h"
#include "Playerbots.h"
#include "ServerFacade.h"
#include "Channel.h"
#include "LFGMgr.h"

using namespace lfg;

// ========== DB2中文名称读取 ==========
// 按阵营ID从sFactionStore直接读取服务端默认locale（zhCN）的名称
// 副本名直接从sLFGDungeonsStore遍历读取，不再硬编码任何英文名
// 注意：不影响技能名称读取（GetSpellNameBestLocale保持不变）
static std::string GetFactionNameById(uint32 factionId)
{
    LocaleConstant locale = static_cast<LocaleConstant>(sWorld->GetDefaultDbcLocale());
    FactionEntry const* faction = sFactionStore.LookupEntry(factionId);
    if (faction)
    {
        char const* name = faction->Name[locale];
        if (name && *name)
            return name;
    }
    return "";
}

enum eTalkType
{
    /*0x18*/ General = ChannelFlags::CHANNEL_FLAG_GENERAL | ChannelFlags::CHANNEL_FLAG_NOT_LFG,
    /*0x3C*/ Trade = ChannelFlags::CHANNEL_FLAG_CITY | ChannelFlags::CHANNEL_FLAG_GENERAL |
                     ChannelFlags::CHANNEL_FLAG_NOT_LFG | ChannelFlags::CHANNEL_FLAG_TRADE,
    /*0x18*/ LocalDefence = ChannelFlags::CHANNEL_FLAG_GENERAL | ChannelFlags::CHANNEL_FLAG_NOT_LFG,
    /*x038*/ GuildRecruitment =
        ChannelFlags::CHANNEL_FLAG_CITY | ChannelFlags::CHANNEL_FLAG_GENERAL | ChannelFlags::CHANNEL_FLAG_NOT_LFG,
    /*0x50*/ LookingForGroup = ChannelFlags::CHANNEL_FLAG_LFG | ChannelFlags::CHANNEL_FLAG_GENERAL
};

std::map<uint32, uint8> SuggestWhatToDoAction::factions;  // factionID → 最低等级

SuggestWhatToDoAction::SuggestWhatToDoAction(PlayerbotAI* botAI, std::string const name)
    : InventoryAction{botAI, name}, _dbc_locale{sWorld->GetDefaultDbcLocale()}
{
    suggestions.push_back(std::bind(&SuggestWhatToDoAction::specificQuest, this));
    suggestions.push_back(std::bind(&SuggestWhatToDoAction::grindReputation, this));
    suggestions.push_back(std::bind(&SuggestWhatToDoAction::something, this));
    suggestions.push_back(std::bind(&SuggestWhatToDoAction::grindMaterials, this));
    suggestions.push_back(std::bind(&SuggestWhatToDoAction::somethingToxic, this));
    suggestions.push_back(std::bind(&SuggestWhatToDoAction::toxicLinks, this));
}

bool SuggestWhatToDoAction::isUseful()
{
    if (!sRandomPlayerbotMgr.IsRandomBot(bot) || bot->GetGroup() || bot->GetInstanceId() || bot->GetBattleground())
        return false;

    std::string qualifier = "suggest what to do";
    time_t lastSaid = AI_VALUE2(time_t, "last said", qualifier);
    return (time(0) - lastSaid) > 30;
}

bool SuggestWhatToDoAction::Execute(Event /*event*/)
{
    uint32 index = rand() % suggestions.size();
    auto fnct_ptr = suggestions[index];
    fnct_ptr();

    std::string const qualifier = "suggest what to do";
    botAI->GetAiObjectContext()->GetValue<time_t>("last said", qualifier)->Set(time(nullptr) + urand(1, 60));

    return true;
}

std::vector<uint32> SuggestWhatToDoAction::GetIncompletedQuests()
{
    std::vector<uint32> result;

    for (uint16 slot = 0; slot < MAX_QUEST_LOG_SIZE; ++slot)
    {
        uint32 questId = bot->GetQuestSlotQuestId(slot);
        if (!questId)
            continue;

        QuestStatus status = bot->GetQuestStatus(questId);
        if (status == QUEST_STATUS_INCOMPLETE || status == QUEST_STATUS_NONE)
            result.push_back(questId);
    }

    return result;
}

void SuggestWhatToDoAction::specificQuest()
{
    std::vector<uint32> quests = GetIncompletedQuests();
    if (quests.empty())
        return;

    BroadcastHelper::BroadcastSuggestQuest(botAI, quests, bot);
}

void SuggestWhatToDoAction::grindMaterials()
{
    /*if (bot->GetLevel() <= 5)
        return;

    auto result = CharacterDatabase.Query("SELECT distinct category, multiplier FROM ahbot_category where category not
    in ('other', 'quest', 'trade', 'reagent') and multiplier > 3 order by multiplier desc limit 10"); if (!result)
        return;

    std::map<std::string, double> categories;
    do
    {
        Field* fields = result->Fetch();
        categories[fields[0].Get<std::string>()] = fields[1].Get<float>();
    } while (result->NextRow());

    for (std::map<std::string, double>::iterator i = categories.begin(); i != categories.end(); ++i)
    {
        if (urand(0, 10) < 3)
        {
            std::string name = i->first;
            double multiplier = i->second;

            for (int j = 0; j < ahbot::CategoryList::instance.size(); j++)
            {
                ahbot::Category* category = ahbot::CategoryList::instance[j];
                if (name == category->GetName())
                {
                    std::string item = category->GetLabel();
                    transform(item.begin(), item.end(), item.begin(), ::tolower);
                    std::ostringstream itemout;
                    itemout << "|c0000b000" << item << "|r";
                    item = itemout.str();

                    std::map<std::string, std::string> placeholders;
                    placeholders["%role"] = chat->formatClass(bot, AiFactory::GetPlayerSpecTab(bot));
                    placeholders["%category"] = item;

                    spam(PlayerbotTextMgr::instance().GetBotText("suggest_trade", placeholders), urand(0, 1) ? 0x3C : 0x18, !urand(0, 2), !urand(0,
    3)); return;
                }
            }
        }
    }*/
}

void SuggestWhatToDoAction::grindReputation()
{
    if (factions.empty())
    {
        // 经典旧世
        factions[529]  = 60;  // Argent Dawn
        factions[87]   = 40;  // Bloodsail Buccaneers
        factions[910]  = 60;  // Brood of Nozdormu
        factions[609]  = 55;  // Cenarion Circle
        factions[909]  = 20;  // Darkmoon Faire
        factions[749]  = 60;  // Hydraxian Waterlords
        factions[349]  = 20;  // Ravenholdt
        factions[59]   = 40;  // Thorium Brotherhood
        factions[576]  = 50;  // Timbermaw Hold
        factions[589]  = 50;  // Wintersaber Trainers
        factions[21]   = 30;  // Booty Bay
        factions[577]  = 40;  // Everlook
        factions[369]  = 50;  // Gadgetzan
        factions[470]  = 20;  // Ratchet

        // 燃烧的远征
        factions[1012] = 70;  // Ashtongue Deathsworn
        factions[942]  = 62;  // Cenarion Expedition
        factions[933]  = 65;  // The Consortium
        factions[946]  = 66;  // Honor Hold
        factions[989]  = 68;  // Keepers of Time
        factions[1015] = 65;  // Netherwing
        factions[1038] = 65;  // Ogri'la
        factions[990]  = 65;  // The Scale of the Sands
        factions[970]  = 65;  // Sporeggar
        factions[922]  = 10;  // Tranquillien
        factions[967]  = 70;  // The Violet Eye

        // 巫妖王之怒
        factions[1106] = 75;  // Argent Crusade
        factions[1156] = 75;  // Ashen Verdict
        factions[1073] = 72;  // The Kalu'ak
        factions[1090] = 75;  // Kirin Tor
        factions[1098] = 77;  // Knights of the Ebon Blade
        factions[1119] = 78;  // The Sons of Hodir
        factions[1091] = 77;  // The Wyrmrest Accord
    }

    std::vector<std::string> levels;
    levels.push_back("尊敬");
    levels.push_back("崇敬");
    levels.push_back("崇拜");

    std::vector<std::string> allowedFactions;
    for (auto const& [factionId, minLevel] : factions)
    {
        if (bot->GetLevel() >= minLevel)
        {
            std::string name = GetFactionNameById(factionId);
            if (!name.empty())
                allowedFactions.push_back(name);
        }
    }

    if (allowedFactions.empty()) return;

    BroadcastHelper::BroadcastSuggestGrindReputation(botAI, levels, allowedFactions, bot);
}

void SuggestWhatToDoAction::something()
{
    BroadcastHelper::BroadcastSuggestSomething(botAI, bot);
}

void SuggestWhatToDoAction::somethingToxic()
{
    BroadcastHelper::BroadcastSuggestSomethingToxic(botAI, bot);
}

void SuggestWhatToDoAction::toxicLinks()
{
    BroadcastHelper::BroadcastSuggestToxicLinks(botAI, bot);
}

void SuggestWhatToDoAction::thunderfury()
{
    BroadcastHelper::BroadcastSuggestThunderfury(botAI, bot);
}

class FindTradeItemsVisitor : public IterateItemsVisitor
{
public:
    FindTradeItemsVisitor(uint32 quality) : IterateItemsVisitor(), quality(quality) {}

    bool Visit(Item* item) override
    {
        ItemTemplate const* proto = item->GetTemplate();
        if (proto->GetQuality() != quality)
            return true;

        if (proto->GetClass() == ITEM_CLASS_TRADE_GOODS && proto->GetBonding() == NO_BIND)
        {
            if (proto->GetQuality() == ITEM_QUALITY_NORMAL && item->GetCount() > 1 &&
                item->GetCount() == item->GetMaxStackCount())
                stacks.push_back(proto->GetId());

            items.push_back(proto->GetId());
            count[proto->GetId()] += item->GetCount();
        }

        return true;
    }

    std::map<uint32, uint32> count;
    std::vector<uint32> stacks;
    std::vector<uint32> items;

private:
    uint32 quality;
};

SuggestDungeonAction::SuggestDungeonAction(PlayerbotAI* botAI) : SuggestWhatToDoAction(botAI, "suggest dungeon") {}

bool SuggestDungeonAction::Execute(Event /*event*/)
{
    if (!sPlayerbotAIConfig.randomBotSuggestDungeons || bot->GetGroup())
        return false;

    // 直接从sLFGDungeonsStore遍历读取，名称/等级范围全部来自DB2（zhCN locale）
    LocaleConstant locale = static_cast<LocaleConstant>(sWorld->GetDefaultDbcLocale());
    uint8 botLevel = bot->GetLevel();
    std::vector<std::string> allowedInstances;

    for (LFGDungeonsEntry const* dungeon : sLFGDungeonsStore)
    {
        // 只推荐普通副本和英雄副本（不含团队副本、随机、区域）
        if (dungeon->TypeID != LFG_TYPE_DUNGEON && dungeon->TypeID != LFG_TYPE_HEROIC)
            continue;

        // 等级过滤
        if (dungeon->MinLevel && botLevel < dungeon->MinLevel)
            continue;
        if (dungeon->MaxLevel && botLevel > dungeon->MaxLevel)
            continue;

        char const* name = dungeon->Name[locale];
        if (name && *name)
            allowedInstances.push_back(name);
    }

    if (allowedInstances.empty()) return false;

    BroadcastHelper::BroadcastSuggestInstance(botAI, allowedInstances, bot);
    return true;
}

SuggestTradeAction::SuggestTradeAction(PlayerbotAI* botAI) : SuggestWhatToDoAction(botAI, "suggest trade") {}

bool SuggestTradeAction::Execute(Event /*event*/)
{
    uint32 quality = urand(0, 100);
    if (quality > 95)
        quality = ITEM_QUALITY_LEGENDARY;
    else if (quality > 90)
        quality = ITEM_QUALITY_EPIC;
    else if (quality > 75)
        quality = ITEM_QUALITY_RARE;
    else if (quality > 50)
        quality = ITEM_QUALITY_UNCOMMON;
    else
        quality = ITEM_QUALITY_NORMAL;

    uint32 item = 0, count = 0;
    while (quality-- > ITEM_QUALITY_POOR)
    {
        FindTradeItemsVisitor visitor(quality);
        IterateItems(&visitor);
        if (!visitor.stacks.empty())
        {
            uint32 index = urand(0, visitor.stacks.size() - 1);
            item = visitor.stacks[index];
        }

        if (!item)
        {
            if (!visitor.items.empty())
            {
                uint32 index = urand(0, visitor.items.size() - 1);
                item = visitor.items[index];
            }
        }

        if (item)
        {
            count = visitor.count[item];
            break;
        }
    }

    if (!item || !count)
        return false;

    ItemTemplate const* proto = sObjectMgr->GetItemTemplate(item);
    if (!proto)
        return false;

    uint32 price = proto->GetSellPrice() * sRandomPlayerbotMgr.GetSellMultiplier(bot) * count;
    if (!price)
        return false;

    BroadcastHelper::BroadcastSuggestSell(botAI, proto, count, price, bot);
    return true;
}
