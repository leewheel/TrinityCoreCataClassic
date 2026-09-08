/*
 * 构建共享值上下文
 * 从 AzerothCore mod-playerbots 移植到 TrinityCore
 */

#include "AiObjectContext.h"
#include "ValueContext.h"
//By leewheel 2026-08-01: 移植brighton-chi接线——注册Mech/UB的值上下文(此前漏注册导致其值不生效)
#include "MechValueContext.h"
#include "UBValueContext.h"
//End By leewheel
//By leewheel 2026-08-14: 补注册Hyjal值上下文(地狱火GUID缓存)，对齐brighton-chi the-lab
#include "ZAValueContext.h"
#include "HyjalValueContext.h"
//End By leewheel
//By leewheel 2026-08-21: 移植brighton-chi 77ff8ec4——补注册TK值上下文(凯尔萨斯神器武器GUID缓存)
#include "TKValueContext.h"
//End By leewheel
//By leewheel 2026-08-21: 移植 brighton-chi cbfc7ea0——补注册SWP值上下文(双子烈焰位置缓存)
#include "SWPValueContext.h"
//End By leewheel
//By leewheel 2026-09-04: 上游(HEAD 7df92554)——补注册Gruul/SSC值上下文(魔犬与法系坦克缓存/毒池位置缓存)
#include "GruulValueContext.h"
#include "SSCValueContext.h"
//End By leewheel
//By leewheel 2026-09-05: 上游 feat tbc-mgt(8c000dfc)——接入MgT值上下文
#include "MgTValueContext.h"
//End By leewheel

void AiObjectContext::BuildSharedValueContexts(SharedNamedObjectContextList<UntypedValue>& valueContexts)
{
    valueContexts.Add(new ValueContext());
    //By leewheel 2026-08-30: 修复——此前 patch 把 RaidZulAmanValueContext 注册行错误插到函数签名与{之间导致语法错误，
    //  已移回函数体内（对齐上游 61b6e532 的注册顺序）
    valueContexts.Add(new RaidZulAmanValueContext());
    //End By leewheel
    //By leewheel 2026-08-01: 补注册Mech/UB值上下文，对齐brighton-chi
    valueContexts.Add(new TbcDungeonMechValueContext());
    valueContexts.Add(new TbcDungeonUnderbogValueContext());
    //End By leewheel
    //By leewheel 2026-09-05: 上游 feat tbc-mgt(8c000dfc)——注册MgT值上下文
    valueContexts.Add(new TbcDungeonMgTValueContext());
    //End By leewheel
    //By leewheel 2026-09-04: 上游d86674ef——RaidHyjalSummitValueContext 改名 RaidHyjalValueContext
    valueContexts.Add(new RaidHyjalValueContext());
    //End By leewheel
    //By leewheel 2026-09-04: 上游d86674ef——新增SSC值上下文
    valueContexts.Add(new RaidSscValueContext());
    //End By leewheel
    //By leewheel 2026-08-21: 补注册TK值上下文
    valueContexts.Add(new RaidTempestKeepValueContext());
    //End By leewheel
    //By leewheel 2026-09-04: 上游——RaidSunwellValueContext 改名 RaidSwpValueContext
    valueContexts.Add(new RaidSwpValueContext());
    //End By leewheel
    //By leewheel 2026-09-04: 上游52e295d2——新增格鲁尔巢穴值上下文
    valueContexts.Add(new RaidGruulsLairValueContext());
    //End By leewheel
}
