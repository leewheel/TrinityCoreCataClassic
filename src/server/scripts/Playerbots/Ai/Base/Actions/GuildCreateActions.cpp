/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "GuildCreateActions.h"

#include "ArenaTeam.h"
#include "BudgetValues.h"
#include "Event.h"
#include "GuildMgr.h"
#include "Playerbots.h"
#include "RandomPlayerbotFactory.h"
#include "ServerFacade.h"
#include "SharedDefines.h"

bool BuyPetitionAction::Execute(Event /*event*/)
{
    GuidVector vendors = botAI->GetAiObjectContext()->GetValue<GuidVector>("nearest npcs")->Get();
    for (GuidVector::iterator i = vendors.begin(); i != vendors.end(); ++i)
    {
        ObjectGuid vendorguid = *i;
        Creature* pCreature = bot->GetNPCIfCanInteractWith(vendorguid, UNIT_NPC_FLAG_PETITIONER, UNIT_NPC_FLAG_2_NONE); // By leewheel 2026-07-08
        if (!pCreature)
            continue;

        std::string const guildName = RandomPlayerbotFactory::CreateRandomGuildName();
        if (guildName.empty())
            continue;

        //By leewheel 2026-08-15: 修复——原AC扁平包经PetitionBuy::Read()按bit-packed解析(先ReadBits(7)读标题长度)位错位，
        //读入乱码标题或抛ByteBufferException，机器人永远买不到公会章程。改用typed构造直接填Unit/Title/Unused910
        //注：PetitionBuy(WorldPacket&&)的形参是右值引用，直接WorldPacket(CMSG_PETITION_BUY)会被MSVC解析为函数声明(Most Vexing Parse)，
        //必须两步构造：先建WorldPacket再move进PetitionBuy
        WorldPacket data(CMSG_PETITION_BUY);
        WorldPackets::Petition::PetitionBuy packet(std::move(data));
        packet.Unit = pCreature->GetGUID();
        packet.Title = guildName;
        packet.Unused910 = 0;
        bot->GetSession()->HandlePetitionBuy(packet);
        //End By leewheel

        return true;
    }

    return false;
}

bool BuyPetitionAction::isUseful() { return canBuyPetition(bot); };

bool BuyPetitionAction::canBuyPetition(Player* bot)
{
    if (bot->GetGuildId())
        return false;

    if (bot->GetGuildIdInvited())
        return false;

    PlayerbotAI* botAI = GET_PLAYERBOT_AI(bot);
    AiObjectContext* context = botAI->GetAiObjectContext();

    if (AI_VALUE2(uint32, "item count", "Hitem:5863:"))
        return false;

    if (botAI->GetGuilderType() == GuilderType::SOLO)
        return false;

    if (botAI->GetGrouperType() == GrouperType::SOLO)
        return false;

    if (!botAI->HasStrategy("guild", BOT_STATE_NON_COMBAT))
        return false;

    uint32 cost = 1000;  // GUILD_CHARTER_COST;

    if (AI_VALUE2(uint32, "free money for", uint32(NeedMoneyFor::guild)) < cost)
        return false;

    return true;
}

bool PetitionOfferAction::Execute(Event event)
{
    std::vector<Item*> petitions = AI_VALUE2(std::vector<Item*>, "inventory items", chat->FormatQItem(5863));

    if (petitions.empty())
        return false;

    ObjectGuid guid = event.getObject();

    Player* master = GetMaster();
    if (!master)
    {
        if (!guid)
            guid = bot->GetTarget();
    }
    else
    {
        if (!guid)
            guid = master->GetTarget();
    }

    if (!guid)
        return false;

    Player* player = ObjectAccessor::FindPlayer(guid);

    if (!player)
        return false;

    WorldPacket data(CMSG_OFFER_PETITION);

    data << uint32(0);
    data << petitions.front()->GetGUID();
    data << guid;

    //By leewheel 2026-07-10: TC使用PQuery进行格式化查询
    QueryResult result =
        CharacterDatabase.PQuery("SELECT playerguid FROM petition_sign WHERE player_account = {} AND petitionguid = {}",
                                player->GetSession()->GetAccountId(), petitions.front()->GetGUID().GetCounter());
    //End By leewheel
    if (result)
    {
        return false;
    }

        // By leewheel 2026-07-09: 使用WPPCompat兼容层替代直接调用
        WPPCompat::OfferPetitionOpcode(bot->GetSession(), data);
        // End By leewheel

    //By leewheel 2026-07-10: TC使用PQuery进行格式化查询
    result = CharacterDatabase.PQuery("SELECT playerguid FROM petition_sign WHERE petitionguid = {}",
                                     petitions.front()->GetGUID().GetCounter());
    //End By leewheel
    uint8 signs = result ? (uint8)result->GetRowCount() : 0;

    context->GetValue<uint8>("petition signs")->Set(signs);

    return true;
}

bool PetitionOfferAction::isUseful() { return !bot->GetGuildId(); }

bool PetitionOfferNearbyAction::Execute(Event /*event*/)
{
    uint32 found = 0;

    GuidVector nearGuids = botAI->GetAiObjectContext()->GetValue<GuidVector>("nearest friendly players")->Get();
    for (auto& i : nearGuids)
    {
        Player* player = ObjectAccessor::FindPlayer(i);

        if (!player)
            continue;

        if (player->GetGuildId())
            continue;

        if (player->GetGuildIdInvited())
            continue;

        PlayerbotAI* botAI = GET_PLAYERBOT_AI(player);

        if (botAI)
        {
            /*
            if (botAI->GetGrouperType() == SOLO && !botAI->HasRealPlayerMaster()) //Do not invite solo players.
                continue;

            */
            if (botAI->HasActivePlayerMaster())  // Do not invite alts of active players.
                continue;
        }
        else
        {
            if (!sPlayerbotAIConfig.randomBotGroupNearby)
                return false;
        }

        if (ServerFacade::instance().GetDistance2d(bot, player) > sPlayerbotAIConfig.sightDistance)
            continue;

        // Parse rpg target to quest action.
        WorldPacket p(CMSG_QUESTGIVER_ACCEPT_QUEST);
        p << i;
        p.rpos(0);

        if (PetitionOfferAction::Execute(Event("petition offer nearby", p)))
            found++;
    }

    return found > 0;
}

bool PetitionOfferNearbyAction::isUseful()
{
    return !bot->GetGuildId() && AI_VALUE2(uint32, "item count", chat->FormatQItem(5863)) &&
           AI_VALUE(uint8, "petition signs") < sWorld->getIntConfig(CONFIG_MIN_PETITION_SIGNS);
}

bool PetitionTurnInAction::Execute(Event /*event*/)
{
    GuidVector vendors = botAI->GetAiObjectContext()->GetValue<GuidVector>("nearest npcs")->Get();
    std::vector<Item*> petitions = AI_VALUE2(std::vector<Item*>, "inventory items", chat->FormatQItem(5863));

    if (petitions.empty())
        return false;

    for (GuidVector::iterator i = vendors.begin(); i != vendors.end(); ++i)
    {
        ObjectGuid vendorguid = *i;
        Creature* pCreature = bot->GetNPCIfCanInteractWith(vendorguid, UNIT_NPC_FLAG_PETITIONER, UNIT_NPC_FLAG_2_NONE); // By leewheel 2026-07-08
        if (!pCreature)
            continue;

        WorldPacket data(CMSG_TURN_IN_PETITION, 8);

        Item* petition = petitions.front();

        if (!petition)
            return false;

        data << petition->GetGUID();

        // By leewheel 2026-07-09: 使用WPPCompat兼容层替代直接调用
        WPPCompat::TurnInPetitionOpcode(bot->GetSession(), data);
        // End By leewheel

        if (bot->GetGuildId())
        {
            // Ensure that bot has at least 10g for HandleSetEmblem can be managed core side
            // (EMBLEM_PRICE = 10 * GOLD in core)
            static constexpr uint32 REQUIRED = 10 * GOLD;
            uint32 have = bot->GetMoney();               // actual money earned by bot in copper
            if (have < REQUIRED)
            {
                bot->ModifyMoney(int32(REQUIRED - have)); // add only the missing amount to bot to reach 10g
            }

            Guild* guild = sGuildMgr->GetGuildById(bot->GetGuildId());

            uint32 st, cl, br, bc, bg;
            bg = urand(0, 51);
            bc = urand(0, 17);
            cl = urand(0, 17);
            br = urand(0, 7);
            st = urand(0, 180);
            EmblemInfo emblemInfo(st, cl, br, bc, bg);

            //By leewheel 2026-07-10: TC使用HandleSetEmblem需要WorldSession参数
            guild->HandleSetEmblem(bot->GetSession(), emblemInfo);
            //End By leewheel 2026-07-10

            //By leewheel 2026-07-11: TC的HandleSetRankInfo需要WorldSession、GuildRankId、名字、权限、每日金币、银行标签权限数组
            // 设置老会员等级(rank 2)权限：可以听公会聊天、说公会聊天、邀请成员
            std::array<GuildBankRightsAndSlots, GUILD_BANK_MAX_TABS> rightsAndSlots = {};
            guild->HandleSetRankInfo(bot->GetSession(), static_cast<GuildRankId>(2), "老兵", GR_RIGHT_GCHATLISTEN | GR_RIGHT_GCHATSPEAK | GR_RIGHT_INVITE, 0, rightsAndSlots);
            //End By leewheel
        }

        return true;
    }

    TravelTarget* oldTarget = context->GetValue<TravelTarget*>("travel target")->Get();

    // Select a new target to travel to.
    TravelTarget newTarget = TravelTarget(botAI);

    bool foundTarget = SetNpcFlagTarget(&newTarget, {UNIT_NPC_FLAG_PETITIONER});

    if (!foundTarget || !newTarget.isActive())
        return false;

    newTarget.setRadius(INTERACTION_DISTANCE);

    setNewTarget(&newTarget, oldTarget);

    return true;
}

bool PetitionTurnInAction::isUseful()
{
    bool inCity = false;
    if (AreaTableEntry const* zone = sAreaTableStore.LookupEntry(bot->GetZoneId()))
    {
        if (zone->flags() & AREA_FLAG_CAPITAL)
            inCity = true;
    }

    return inCity && !bot->GetGuildId() && AI_VALUE2(uint32, "item count", chat->FormatQItem(5863)) &&
           AI_VALUE(uint8, "petition signs") >= sWorld->getIntConfig(CONFIG_MIN_PETITION_SIGNS) &&
           !context->GetValue<TravelTarget*>("travel target")->Get()->isTraveling();
}

bool BuyTabardAction::Execute(Event /*event*/)
{
    bool canBuy = botAI->DoSpecificAction("buy", Event("buy tabard", "Hitem:5976:"), true);
    if (canBuy && AI_VALUE2(uint32, "item count", chat->FormatQItem(5976)))
        return true;

    TravelTarget* oldTarget = context->GetValue<TravelTarget*>("travel target")->Get();

    // Select a new target to travel to.
    TravelTarget newTarget = TravelTarget(botAI);

    bool foundTarget = SetNpcFlagTarget(&newTarget, {UNIT_NPC_FLAG_TABARDDESIGNER}, "Tabard Vendor", {5976});

    if (!foundTarget || !newTarget.isActive())
        return false;

    newTarget.setRadius(INTERACTION_DISTANCE);

    setNewTarget(&newTarget, oldTarget);

    return true;
};

bool BuyTabardAction::isUseful()
{
    bool inCity = false;
    if (AreaTableEntry const* zone = sAreaTableStore.LookupEntry(bot->GetZoneId()))
    {
        if (zone->flags() & AREA_FLAG_CAPITAL)
            inCity = true;
    }

    return inCity && bot->GetGuildId() && !AI_VALUE2(uint32, "item count", chat->FormatQItem(5976)) &&
           AI_VALUE2(uint32, "free money for", uint32(NeedMoneyFor::guild)) >= 10000 &&
           !context->GetValue<TravelTarget*>("travel target")->Get()->isTraveling();
}
