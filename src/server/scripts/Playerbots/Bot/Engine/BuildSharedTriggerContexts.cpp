/*
 * 构建共享触发器上下文
 * 从 AzerothCore mod-playerbots 移植到 TrinityCore
 */

#include "AiObjectContext.h"
#include "TriggerContext.h"
#include "ChatTriggerContext.h"
#include "WorldPacketTriggerContext.h"
#include "Aq20TriggerContext.h"
#include "MCTriggerContext.h"
#include "BWLTriggerContext.h"
#include "KaraTriggerContext.h"
#include "GruulTriggerContext.h"
#include "MagTriggerContext.h"
#include "NaxxTriggerContext.h"
#include "SSCTriggerContext.h"
#include "TKTriggerContext.h"
#include "HyjalTriggerContext.h"
#include "BTTriggerContext.h"
#include "ZATriggerContext.h"
#include "OSTriggerContext.h"
#include "EoETriggerContext.h"
#include "VoATriggerContext.h"
#include "UldTriggerContext.h"
#include "OnyTriggerContext.h"
#include "ICCTriggerContext.h"
#include "RSTriggerContext.h"
#include "SWPTriggerContext.h" //By leewheel 2026-08-14: 补注册SWP(此前漏注册导致sunwell trigger不生效)
#include "Ai/Dungeon/TbcDungeonTriggerContext.h"
#include "Ai/Dungeon/WotlkDungeonTriggerContext.h"

void AiObjectContext::BuildSharedTriggerContexts(SharedNamedObjectContextList<Trigger>& triggerContexts)
{
    triggerContexts.Add(new TriggerContext());
    triggerContexts.Add(new ChatTriggerContext());
    triggerContexts.Add(new WorldPacketTriggerContext());
    triggerContexts.Add(new RaidAq20TriggerContext());
    triggerContexts.Add(new RaidMcTriggerContext());
    triggerContexts.Add(new RaidBwlTriggerContext());
    triggerContexts.Add(new RaidKarazhanTriggerContext());
    triggerContexts.Add(new RaidGruulsLairTriggerContext());
    triggerContexts.Add(new RaidMagtheridonTriggerContext());
    triggerContexts.Add(new RaidNaxxTriggerContext());
    //By leewheel 2026-09-04: 上游——RaidSSCTriggerContext 改名 RaidSscTriggerContext / RaidHyjalSummitTriggerContext 改名 RaidHyjalTriggerContext / RaidSunwellTriggerContext 改名 RaidSwpTriggerContext
    triggerContexts.Add(new RaidSscTriggerContext());
    triggerContexts.Add(new RaidTempestKeepTriggerContext());
    triggerContexts.Add(new RaidHyjalTriggerContext());
    triggerContexts.Add(new RaidBlackTempleTriggerContext());
    triggerContexts.Add(new RaidZulAmanTriggerContext());
    triggerContexts.Add(new RaidOsTriggerContext());
    triggerContexts.Add(new RaidEoETriggerContext());
    triggerContexts.Add(new RaidVoATriggerContext());
    triggerContexts.Add(new RaidUlduarTriggerContext());
    triggerContexts.Add(new RaidOnyxiaTriggerContext());
    triggerContexts.Add(new RaidIccTriggerContext());
    triggerContexts.Add(new RaidRsTriggerContext());
    //By leewheel 2026-08-14: 补注册SWP trigger上下文
    triggerContexts.Add(new RaidSwpTriggerContext());
    //End By leewheel
    triggerContexts.Add(new TbcDungeonHellfireRampartsTriggerContext());
    triggerContexts.Add(new TbcDungeonUnderbogTriggerContext());
    triggerContexts.Add(new TbcDungeonAuchenaiCryptsTriggerContext());
    triggerContexts.Add(new TbcDungeonSethekkHallsTriggerContext());
    triggerContexts.Add(new TbcDungeonMechanarTriggerContext());
    triggerContexts.Add(new TbcDungeonUnderbogTriggerContext());
    //By leewheel 2026-09-05: 上游8c000dfc——注册MgT trigger上下文
    triggerContexts.Add(new TbcDungeonMagistersTerraceTriggerContext());
    //End By leewheel
    triggerContexts.Add(new WotlkDungeonUKTriggerContext());
    triggerContexts.Add(new WotlkDungeonNexTriggerContext());
    triggerContexts.Add(new WotlkDungeonANTriggerContext());
    triggerContexts.Add(new WotlkDungeonOKTriggerContext());
    triggerContexts.Add(new WotlkDungeonDTKTriggerContext());
    triggerContexts.Add(new WotlkDungeonVHTriggerContext());
    triggerContexts.Add(new WotlkDungeonGDTriggerContext());
    triggerContexts.Add(new WotlkDungeonHoSTriggerContext());
    triggerContexts.Add(new WotlkDungeonHoLTriggerContext());
    triggerContexts.Add(new WotlkDungeonOccTriggerContext());
    triggerContexts.Add(new WotlkDungeonUPTriggerContext());
    triggerContexts.Add(new WotlkDungeonCoSTriggerContext());
    triggerContexts.Add(new WotlkDungeonFoSTriggerContext());
    triggerContexts.Add(new WotlkDungeonPoSTriggerContext());
    triggerContexts.Add(new WotlkDungeonToCTriggerContext());
}
