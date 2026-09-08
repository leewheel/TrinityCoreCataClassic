#ifndef PLAYERBOTS_RAIDSTRATEGYCONTEXT_H
#define PLAYERBOTS_RAIDSTRATEGYCONTEXT_H

#include "Strategy.h"
#include "Aq20Strategy.h"
#include "MCStrategy.h"
#include "BWLStrategy.h"
#include "KaraStrategy.h"
#include "GruulStrategy.h"
#include "MagStrategy.h"
#include "NaxxStrategy.h"
#include "SSCStrategy.h"
#include "TKStrategy.h"
#include "HyjalStrategy.h"
#include "BTStrategy.h"
#include "ZAStrategy.h"
#include "OSStrategy.h"
#include "EoEStrategy.h"
#include "VoAStrategy.h"
#include "UldStrategy.h"
#include "OnyStrategy.h"
#include "ICCStrategy.h"
#include "RSStrategy.h"
//By leewheel 2026-07-26: 移植brighton-chi太阳井(SWP)团本策略。
#include "SWPStrategy.h"
//End By leewheel

class RaidStrategyContext : public NamedObjectContext<Strategy>
{
public:
    RaidStrategyContext() : NamedObjectContext<Strategy>(false, true)
    {
        creators["aq20"] = &RaidStrategyContext::aq20;
        creators["moltencore"] = &RaidStrategyContext::moltencore;
        creators["bwl"] = &RaidStrategyContext::bwl;
        creators["karazhan"] = &RaidStrategyContext::karazhan;
        creators["gruulslair"] = &RaidStrategyContext::gruulslair;
        creators["magtheridon"] = &RaidStrategyContext::magtheridon;
        creators["naxx"] = &RaidStrategyContext::naxx;
        creators["ssc"] = &RaidStrategyContext::ssc;
        creators["tempestkeep"] = &RaidStrategyContext::tempestkeep;
        creators["hyjal"] = &RaidStrategyContext::hyjal;
        creators["blacktemple"] = &RaidStrategyContext::blacktemple;
        creators["zulaman"] = &RaidStrategyContext::zulaman;
        creators["wotlk-os"] = &RaidStrategyContext::wotlk_os;
        creators["wotlk-eoe"] = &RaidStrategyContext::wotlk_eoe;
        creators["voa"] = &RaidStrategyContext::voa;
        creators["ulduar"] = &RaidStrategyContext::ulduar;
        creators["onyxia"] = &RaidStrategyContext::onyxia;
        creators["icc"] = &RaidStrategyContext::icc;
        creators["rs"] = &RaidStrategyContext::rs;
        //By leewheel 2026-07-26: 注册太阳井策略。
        creators["sunwell"] = &RaidStrategyContext::sunwell;
        //End By leewheel
    }

private:
    static Strategy* aq20(PlayerbotAI* botAI) { return new RaidAq20Strategy(botAI); }
    static Strategy* moltencore(PlayerbotAI* botAI) { return new RaidMcStrategy(botAI); }
    static Strategy* bwl(PlayerbotAI* botAI) { return new RaidBwlStrategy(botAI); }
    static Strategy* karazhan(PlayerbotAI* botAI) { return new RaidKarazhanStrategy(botAI); }
    static Strategy* gruulslair(PlayerbotAI* botAI) { return new RaidGruulsLairStrategy(botAI); }
    static Strategy* magtheridon(PlayerbotAI* botAI) { return new RaidMagtheridonStrategy(botAI); }
    static Strategy* naxx(PlayerbotAI* botAI) { return new RaidNaxxStrategy(botAI); }
    //By leewheel 2026-09-04: 上游——RaidSSCStrategy 改名 RaidSscStrategy
    static Strategy* ssc(PlayerbotAI* botAI) { return new RaidSscStrategy(botAI); }
    //End By leewheel
    static Strategy* tempestkeep(PlayerbotAI* botAI) { return new RaidTempestKeepStrategy(botAI); }
    //By leewheel 2026-09-04: 上游——RaidHyjalSummitStrategy 改名 RaidHyjalStrategy
    static Strategy* hyjal(PlayerbotAI* botAI) { return new RaidHyjalStrategy(botAI); }
    //End By leewheel
    static Strategy* blacktemple(PlayerbotAI* botAI) { return new RaidBlackTempleStrategy(botAI); }
    static Strategy* zulaman(PlayerbotAI* botAI) { return new RaidZulAmanStrategy(botAI); }
    static Strategy* wotlk_os(PlayerbotAI* botAI) { return new RaidOsStrategy(botAI); }
    static Strategy* wotlk_eoe(PlayerbotAI* botAI) { return new RaidEoEStrategy(botAI); }
    static Strategy* voa(PlayerbotAI* botAI) { return new RaidVoAStrategy(botAI); }
    static Strategy* onyxia(PlayerbotAI* botAI) { return new RaidOnyxiaStrategy(botAI); }
    static Strategy* ulduar(PlayerbotAI* botAI) { return new RaidUlduarStrategy(botAI); }
    static Strategy* icc(PlayerbotAI* botAI) { return new RaidIccStrategy(botAI); }
    static Strategy* rs(PlayerbotAI* botAI) { return new RaidRsStrategy(botAI); }
    //By leewheel 2026-07-26: 太阳井策略工厂方法。
    //By leewheel 2026-09-04: 上游——RaidSunwellStrategy 改名 RaidSwpStrategy
    static Strategy* sunwell(PlayerbotAI* botAI) { return new RaidSwpStrategy(botAI); }
    //End By leewheel
};

#endif
