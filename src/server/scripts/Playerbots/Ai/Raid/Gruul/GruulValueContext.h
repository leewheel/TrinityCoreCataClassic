/*
 * 移植来源: AC mod-playerbots GruulValueContext.h 提交 3b0ee7f7(野性魔犬值)/93369f55(坦克缓存值)
 * 移植适配 TC 框架
 * 业务对标: AC azerothcore-wotlk mod-playerbots
 * 作者: leewheel
 */

#ifndef PLAYERBOTS_GRUULVALUECONTEXT_H
#define PLAYERBOTS_GRUULVALUECONTEXT_H

#include "GruulHelpers.h"
#include "NamedObjectContext.h"
#include "ObjectGuid.h"
#include "Value.h"

//By leewheel 2026-09-04: 上游3b0ee7f7/93369f55——Olm每48.5秒召唤一只野性魔犬(实际最多1-2只), 网格搜索照样缓存
class HighKingMaulgarWildFelStalkersValue : public CalculatedValue<GuidVector>
{
public:
    HighKingMaulgarWildFelStalkersValue(PlayerbotAI* botAI)
        : CalculatedValue<GuidVector>(
              botAI, "high king maulgar wild fel stalkers",
              GruulHelpers::WILD_FEL_STALKER_CACHE_INTERVAL_MS) {}

protected:
    GuidVector Calculate() override { return GruulHelpers::FindNearbyWildFelStalkerGuids(bot); }
};

//By leewheel 2026-09-04: 上游93369f55——两个食人魔法系坦克由遍历全团选出, 值缓存避免每bot每tick重复遍历
class HighKingMaulgarKroshMageTankValue : public ObjectGuidCalculatedValue
{
public:
    HighKingMaulgarKroshMageTankValue(PlayerbotAI* botAI)
        : ObjectGuidCalculatedValue(
              botAI, "high king maulgar krosh mage tank",
              GruulHelpers::CASTER_TANK_CACHE_INTERVAL_MS) {}

protected:
    ObjectGuid Calculate() override { return GruulHelpers::FindKroshMageTankGuid(bot); }
};

class HighKingMaulgarKigglerMoonkinTankValue : public ObjectGuidCalculatedValue
{
public:
    HighKingMaulgarKigglerMoonkinTankValue(PlayerbotAI* botAI)
        : ObjectGuidCalculatedValue(
              botAI, "high king maulgar kiggler moonkin tank",
              GruulHelpers::CASTER_TANK_CACHE_INTERVAL_MS) {}

protected:
    ObjectGuid Calculate() override { return GruulHelpers::FindKigglerMoonkinTankGuid(bot); }
};
//End By leewheel

class RaidGruulsLairValueContext : public NamedObjectContext<UntypedValue>
{
public:
    RaidGruulsLairValueContext()
    {
        creators["high king maulgar wild fel stalkers"] =
            &RaidGruulsLairValueContext::high_king_maulgar_wild_fel_stalkers;
        creators["high king maulgar krosh mage tank"] =
            &RaidGruulsLairValueContext::high_king_maulgar_krosh_mage_tank;
        creators["high king maulgar kiggler moonkin tank"] =
            &RaidGruulsLairValueContext::high_king_maulgar_kiggler_moonkin_tank;
    }

private:
    static UntypedValue* high_king_maulgar_wild_fel_stalkers(PlayerbotAI* botAI) {
        return new HighKingMaulgarWildFelStalkersValue(botAI);
    }

    static UntypedValue* high_king_maulgar_krosh_mage_tank(PlayerbotAI* botAI) {
        return new HighKingMaulgarKroshMageTankValue(botAI);
    }

    static UntypedValue* high_king_maulgar_kiggler_moonkin_tank(PlayerbotAI* botAI) {
        return new HighKingMaulgarKigglerMoonkinTankValue(botAI);
    }
};

#endif
