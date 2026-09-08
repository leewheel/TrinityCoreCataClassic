/*
 * 构建共享动作上下文
 * 从 AzerothCore mod-playerbots 移植到 TrinityCore
 */

#include "AiObjectContext.h"
#include "ActionContext.h"
#include "ChatActionContext.h"
#include "WorldPacketActionContext.h"
#include "Aq20ActionContext.h"
#include "MCActionContext.h"
#include "BWLActionContext.h"
#include "KaraActionContext.h"
#include "GruulActionContext.h"
#include "NaxxActionContext.h"
#include "MagActionContext.h"
#include "SSCActionContext.h"
#include "TKActionContext.h"
#include "HyjalActionContext.h"
#include "BTActionContext.h"
#include "ZAActionContext.h"
#include "OSActionContext.h"
#include "EoEActionContext.h"
#include "VoAActionContext.h"
#include "UldActionContext.h"
#include "OnyActionContext.h"
#include "ICCActionContext.h"
#include "RSActionContext.h"
#include "SWPActionContext.h" //By leewheel 2026-08-14: 补注册SWP(此前漏注册导致sunwell action不生效)
#include "Ai/Dungeon/TbcDungeonActionContext.h"
#include "Ai/Dungeon/WotlkDungeonActionContext.h"

void AiObjectContext::BuildSharedActionContexts(SharedNamedObjectContextList<Action>& actionContexts)
{
    actionContexts.Add(new ActionContext());
    actionContexts.Add(new ChatActionContext());
    actionContexts.Add(new WorldPacketActionContext());
    actionContexts.Add(new RaidAq20ActionContext());
    actionContexts.Add(new RaidMcActionContext());
    actionContexts.Add(new RaidBwlActionContext());
    actionContexts.Add(new RaidKarazhanActionContext());
    actionContexts.Add(new RaidGruulsLairActionContext());
    actionContexts.Add(new RaidMagtheridonActionContext());
    actionContexts.Add(new RaidSscActionContext()); //By leewheel 2026-09-04: 上游——RaidSSCActionContext 改名 RaidSscActionContext
    actionContexts.Add(new RaidTempestKeepActionContext());
    actionContexts.Add(new RaidHyjalActionContext()); //By leewheel 2026-09-04: 上游——RaidHyjalSummitActionContext 改名 RaidHyjalActionContext
    actionContexts.Add(new RaidBlackTempleActionContext());
    actionContexts.Add(new RaidZulAmanActionContext());
    actionContexts.Add(new RaidNaxxActionContext());
    actionContexts.Add(new RaidOsActionContext());
    actionContexts.Add(new RaidEoEActionContext());
    actionContexts.Add(new RaidVoAActionContext());
    actionContexts.Add(new RaidUlduarActionContext());
    actionContexts.Add(new RaidOnyxiaActionContext());
    actionContexts.Add(new RaidIccActionContext());
    actionContexts.Add(new RaidRsActionContext());
    //By leewheel 2026-08-14: 补注册SWP action上下文
    //By leewheel 2026-09-04: 上游——RaidSunwellActionContext 改名 RaidSwpActionContext
    actionContexts.Add(new RaidSwpActionContext());
    //End By leewheel
    actionContexts.Add(new TbcDungeonHellfireRampartsActionContext());
    actionContexts.Add(new TbcDungeonUnderbogActionContext());
    actionContexts.Add(new TbcDungeonAuchenaiCryptsActionContext());
    actionContexts.Add(new TbcDungeonSethekkHallsActionContext());
    actionContexts.Add(new TbcDungeonMechanarActionContext());
    actionContexts.Add(new TbcDungeonUnderbogActionContext());
    //By leewheel 2026-09-05: 上游8c000dfc——注册MgT action上下文
    actionContexts.Add(new TbcDungeonMagistersTerraceActionContext());
    //End By leewheel
    actionContexts.Add(new WotlkDungeonUKActionContext());
    actionContexts.Add(new WotlkDungeonNexActionContext());
    actionContexts.Add(new WotlkDungeonANActionContext());
    actionContexts.Add(new WotlkDungeonOKActionContext());
    actionContexts.Add(new WotlkDungeonDTKActionContext());
    actionContexts.Add(new WotlkDungeonVHActionContext());
    actionContexts.Add(new WotlkDungeonGDActionContext());
    actionContexts.Add(new WotlkDungeonHoSActionContext());
    actionContexts.Add(new WotlkDungeonHoLActionContext());
    actionContexts.Add(new WotlkDungeonOccActionContext());
    actionContexts.Add(new WotlkDungeonUPActionContext());
    actionContexts.Add(new WotlkDungeonCoSActionContext());
    actionContexts.Add(new WotlkDungeonFoSActionContext());
    actionContexts.Add(new WotlkDungeonPoSActionContext());
    actionContexts.Add(new WotlkDungeonToCActionContext());
}
