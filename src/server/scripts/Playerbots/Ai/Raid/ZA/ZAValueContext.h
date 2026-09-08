/*
 * 移植来源: AC azerothcore-wotlk mod-playerbots 提交 61b6e532 (ZA 炸弹值缓存)
 * 移植适配 TC 框架
 * 业务对标: AC azerothcore-wotlk mod-playerbots
 * 作者: leewheel
 */

#ifndef PLAYERBOTS_ZAVALUECONTEXT_H
#define PLAYERBOTS_ZAVALUECONTEXT_H

#include "NamedObjectContext.h"
#include "ObjectGuid.h"
#include "Value.h"
#include "ZAHelpers.h"

// Jan'alai 一次释放40个炸弹，四个地方（三个触发器+一个移动乘数）每tick都要查询，各自做一次网格搜索
// 缓存功能将多次搜索合并为一次
class JanalaiFireBombsValue : public CalculatedValue<GuidVector>
{
public:
    JanalaiFireBombsValue(PlayerbotAI* botAI)
        : CalculatedValue<GuidVector>(
              botAI, "jan'alai fire bombs", ZaHelpers::FIRE_BOMB_CACHE_INTERVAL_MS) {}

protected:
    GuidVector Calculate() override { return ZaHelpers::FindNearbyFireBombGuids(bot); }
};

//By leewheel 2026-09-04: 上游14413ee2——冰霜陷阱GUID缓存值(触发器判定+逃跑动作每tick各查一次, 一次网格搜索服务两处)
class HexLordMalacrassFreezingTrapValue : public ObjectGuidCalculatedValue
{
public:
    HexLordMalacrassFreezingTrapValue(PlayerbotAI* botAI)
        : ObjectGuidCalculatedValue(
              botAI, "hex lord malacrass freezing trap",
              ZaHelpers::FREEZING_TRAP_CACHE_INTERVAL_MS) {}

protected:
    ObjectGuid Calculate() override { return ZaHelpers::FindNearbyFreezingTrapGuid(bot); }
};
//End By leewheel

class RaidZulAmanValueContext : public NamedObjectContext<UntypedValue>
{
public:
    RaidZulAmanValueContext()
    {
        creators["jan'alai fire bombs"] = &RaidZulAmanValueContext::janalai_fire_bombs;
        //By leewheel 2026-09-04: 上游14413ee2——注册冰霜陷阱缓存值
        creators["hex lord malacrass freezing trap"] =
            &RaidZulAmanValueContext::hex_lord_malacrass_freezing_trap;
        //End By leewheel
    }

private:
    static UntypedValue* janalai_fire_bombs(PlayerbotAI* botAI) {
        return new JanalaiFireBombsValue(botAI);
    }

    //By leewheel 2026-09-04: 上游14413ee2——冰霜陷阱值工厂
    static UntypedValue* hex_lord_malacrass_freezing_trap(PlayerbotAI* botAI) {
        return new HexLordMalacrassFreezingTrapValue(botAI);
    }
    //End By leewheel
};

#endif