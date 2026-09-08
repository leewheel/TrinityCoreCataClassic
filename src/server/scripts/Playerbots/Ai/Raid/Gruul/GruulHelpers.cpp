/* 副本机器人策略 */
//移植来源: AC mod-playerbots GruulHelpers.cpp 移植适配 TC 框架
//业务对标: AC azerothcore-wotlk mod-playerbots src/Ai/Raid/Gruul/GruulHelpers.cpp
//By leewheel 2026-08-29 引入 mod-playerbots 新提交(19196110..0633d66f): Gruul 策略更新——
//  主坦判定需为真实坦克、盲眼坦克索引修正、月火熊坦按成员专精判定、新增巨岩践踏检测等
//End By leewheel
#include "GruulHelpers.h"
#include "AiFactory.h"
//By leewheel 2026-09-04: 上游3b0ee7f7——值缓存包装需要ObjectAccessor
#include "ObjectAccessor.h"
//End By leewheel
#include "Playerbots.h"
//By leewheel 2026-09-04: 上游——标准库头
#include <algorithm>
#include <list>
//End By leewheel

namespace GruulHelpers
{

bool IsMaulgarTank(Player* bot)
{
    // 注意: IsMainTank() 标记的不一定是坦克(可能是任意带主坦标记的人)。
    // 团本策略对非坦克主坦会有问题, 因此这里假定主坦必须是真正的坦克。
    return PlayerbotAI::IsTank(bot) && PlayerbotAI::IsMainTank(bot);
}

bool IsOlmTank(Player* bot)
{
    return PlayerbotAI::IsAssistTankOfIndex(bot, 0, true);
}

bool IsBlindeyeTank(Player* bot)
{
    return PlayerbotAI::IsAssistTankOfIndex(bot, 1, true);
}

//By leewheel 2026-09-04: 上游3b0ee7f7/93369f55——坦克选择拆为GUID查找+值缓存包装(全团每bot每秒一次遍历)
ObjectGuid FindKroshMageTankGuid(Player* bot)
{
    Group* group = bot->GetGroup();
    if (!group)
        return ObjectGuid::Empty;

    // 若找到助理法师(玩家或机器人)则立即返回, 否则回退到血量最高的机器人法师
    Player* highestHpBotMage = nullptr;
    uint32 highestHp = 0;

    for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
    {
        Player* member = ref->GetSource();
        if (!member || !member->IsAlive() || member->GetMapId() != GRUUL_MAP_ID ||
            member->getClass() != CLASS_MAGE)
        {
            continue;
        }

        if (group->IsAssistant(member->GetGUID()))
            return member->GetGUID();

        if (!GET_PLAYERBOT_AI(member))
            continue;

        uint32 const hp = member->GetMaxHealth();
        if (!highestHpBotMage || hp > highestHp)
        {
            highestHpBotMage = member;
            highestHp = hp;
        }
    }

    return highestHpBotMage ? highestHpBotMage->GetGUID() : ObjectGuid::Empty;
}

Player* GetKroshMageTank(Player* bot)
{
    // 团里所有bot算出的答案一致, 值系统按bot缓存——把每bot每tick一次的遍历收敛为每秒一次
    PlayerbotAI* botAI = GET_PLAYERBOT_AI(bot);
    ObjectGuid const guid = botAI
        ? botAI->GetAiObjectContext()->GetValue<ObjectGuid>("high king maulgar krosh mage tank")->Get()
        : FindKroshMageTankGuid(bot);

    // 每次调用都重新解析而不是持有指针: tank可能在缓存间隔内下线或离团, 缓存的Player*会悬垂
    return guid.IsEmpty() ? nullptr : ObjectAccessor::FindPlayer(guid);
}

bool IsKroshMageTank(Player* bot)
{
    return bot->getClass() == CLASS_MAGE && GetKroshMageTank(bot) == bot;
}

ObjectGuid FindKigglerMoonkinTankGuid(Player* bot)
{
    Group* group = bot->GetGroup();
    if (!group)
        return ObjectGuid::Empty;

    // 若找到助理平衡德(玩家或机器人)则立即返回, 否则回退到血量最高的机器人平衡德
    Player* highestHpBotMoonkin = nullptr;
    uint32 highestHp = 0;

    for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
    {
        Player* member = ref->GetSource();
        if (!member || !member->IsAlive() || member->GetMapId() != GRUUL_MAP_ID ||
            member->getClass() != CLASS_DRUID ||
            AiFactory::GetPlayerSpecTab(member) != DRUID_TAB_BALANCE)
        {
            continue;
        }

        if (group->IsAssistant(member->GetGUID()))
            return member->GetGUID();

        if (!GET_PLAYERBOT_AI(member))
            continue;

        uint32 const hp = member->GetMaxHealth();
        if (!highestHpBotMoonkin || hp > highestHp)
        {
            highestHpBotMoonkin = member;
            highestHp = hp;
        }
    }

    return highestHpBotMoonkin ? highestHpBotMoonkin->GetGUID() : ObjectGuid::Empty;
}

Player* GetKigglerMoonkinTank(Player* bot)
{
    PlayerbotAI* botAI = GET_PLAYERBOT_AI(bot);
    ObjectGuid const guid = botAI
        ? botAI->GetAiObjectContext()->GetValue<ObjectGuid>("high king maulgar kiggler moonkin tank")->Get()
        : FindKigglerMoonkinTankGuid(bot);

    return guid.IsEmpty() ? nullptr : ObjectAccessor::FindPlayer(guid);
}

bool IsKigglerMoonkinTank(Player* bot)
{
    return bot->getClass() == CLASS_DRUID && GetKigglerMoonkinTank(bot) == bot;
}

bool HasGroundSlam(Player* bot)
{
    return bot->HasAura(Id(GruulSpells::SPELL_GROUND_SLAM_1)) ||
        bot->HasAura(Id(GruulSpells::SPELL_GROUND_SLAM_2));
}

//By leewheel 2026-09-04: 上游3b0ee7f7——野性魔犬GUID搜索(按GUID排序使全团术士拿到一致列表)
GuidVector FindNearbyWildFelStalkerGuids(Player* bot)
{
    // 网格搜索是模块里最贵的检查, 且副本策略可能在离开副本后仍存活(如服务器重启), 用地图先行拦截
    if (bot->GetMapId() != GRUUL_MAP_ID)
        return {};

    std::list<Creature*> creatureList;
    bot->GetCreatureListWithEntryInGrid(
        creatureList, Id(GruulNpcs::NPC_WILD_FEL_STALKER), WILD_FEL_STALKER_SEARCH_RADIUS);

    GuidVector guids;
    guids.reserve(creatureList.size());
    for (Creature* creature : creatureList)
    {
        if (creature && creature->IsAlive())
            guids.push_back(creature->GetGUID());
    }

    // 搜索从搜索者自身格向外走格, 两个站位不同的bot返回同一批魔犬但顺序可能不同。
    // 排序使列表规范化, 术士i配魔犬i的全团配对才成立。
    std::sort(guids.begin(), guids.end(), [](ObjectGuid const& lhs, ObjectGuid const& rhs)
    {
        return lhs.GetCounter() < rhs.GetCounter();
    });

    return guids;
}

std::vector<Unit*> GetNearbyWildFelStalkers(PlayerbotAI* botAI)
{
    GuidVector const& guids =
        botAI->GetAiObjectContext()->GetValue<GuidVector>(
            "high king maulgar wild fel stalkers")->RefGet();

    // 列表最长可能滞后WILD_FEL_STALKER_CACHE_INTERVAL_MS, 魔犬可能已死。
    // 在此丢弃重新压实配对, 避免列表出现空洞。
    std::vector<Unit*> felStalkers;
    felStalkers.reserve(guids.size());
    for (ObjectGuid const& guid : guids)
    {
        Unit* felStalker = botAI->GetUnit(guid);
        if (felStalker && felStalker->IsAlive())
            felStalkers.push_back(felStalker);
    }

    return felStalkers;
}
//End By leewheel

}
