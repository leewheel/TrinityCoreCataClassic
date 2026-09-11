/*
 * By leewheel 2026-09-06
 * 移植到TrinityCore-Cata：Cata核心移除了各战场Zone类头文件(BattlegroundAB/WS/EY/IC.h)，
 * 模块战术代码引用的战场游戏对象entry常量在此按WotLK定义补齐
 * 这些entry对应客户端游戏对象数据，WotLK与Cata的战场对象entry保持一致
 */
#ifndef PLAYERBOTS_BATTLEGROUNDENTRIESCOMPAT_H
#define PLAYERBOTS_BATTLEGROUNDENTRIESCOMPAT_H

// --- 阿拉希盆地(AB)横幅 ---
enum BG_AB_ObjectIdsCompat
{
    BG_AB_OBJECTID_BANNER_A             = 180058, // 联盟横幅
    BG_AB_OBJECTID_BANNER_CONT_A        = 180059, // 联盟受击横幅
    BG_AB_OBJECTID_BANNER_H             = 180060, // 部落横幅
    BG_AB_OBJECTID_BANNER_CONT_H        = 180061, // 部落受击横幅

    BG_AB_OBJECTID_NODE_BANNER_0        = 180087, // 马厩横幅
    BG_AB_OBJECTID_NODE_BANNER_1        = 180088, // 铁匠铺横幅
    BG_AB_OBJECTID_NODE_BANNER_2        = 180089, // 农场横幅
    BG_AB_OBJECTID_NODE_BANNER_3        = 180090, // 伐木场横幅
    BG_AB_OBJECTID_NODE_BANNER_4        = 180091  // 金矿横幅
};

// --- 战歌峡谷(WS)旗帜 ---
enum BG_WS_ObjectEntriesCompat
{
    BG_OBJECT_A_FLAG_WS_ENTRY           = 179830, // 联盟银翼旗帜(基座)
    BG_OBJECT_H_FLAG_WS_ENTRY           = 179831, // 部落战歌旗帜(基座)
    BG_OBJECT_A_FLAG_GROUND_WS_ENTRY    = 179785, // 联盟旗帜(掉落地面)
    BG_OBJECT_H_FLAG_GROUND_WS_ENTRY    = 179786  // 部落旗帜(掉落地面)
};

// --- 风暴之眼(EY)旗帜 ---
enum BG_EY_ObjectEntriesCompat
{
    BG_OBJECT_A_FLAG_EY_ENTRY           = 184141, // 联盟旗帜(旗座)
    BG_OBJECT_H_FLAG_EY_ENTRY           = 184142, // 部落旗帜(旗座)
    BG_OBJECT_FLAG_EY_ENTRY             = 184143, // 中立旗帜(掉落)
    BG_OBJECT_FLAG2_EY_ENTRY            = 208977  // 风暴之眼旗帜(旗座)
};

// --- 奥特兰克盆地(IoC)车间/码头/机库/采石场/精炼厂横幅 ---
enum BG_IC_ObjectEntriesCompat
{
    GO_ALLIANCE_BANNER                  = 195396, // 联盟横幅
    GO_HORDE_BANNER                     = 195393, // 部落横幅
    GO_WORKSHOP_BANNER                  = 195133, // 车间横幅
    GO_DOCKS_BANNER                     = 195157, // 码头横幅
    GO_HANGAR_BANNER                    = 195158, // 机库横幅
    GO_QUARRY_BANNER                    = 195338, // 采石场横幅
    GO_REFINERY_BANNER                  = 195343, // 精炼厂横幅

    GO_ALLIANCE_BANNER_WORKSHOP         = 195149, // 联盟车间横幅
    GO_ALLIANCE_BANNER_WORKSHOP_CONT    = 195150, // 联盟车间受击横幅
    GO_HORDE_BANNER_WORKSHOP            = 195151, // 部落车间横幅
    GO_HORDE_BANNER_WORKSHOP_CONT       = 195152, // 部落车间受击横幅

    GO_ALLIANCE_BANNER_DOCK             = 195153, // 联盟码头横幅
    GO_ALLIANCE_BANNER_DOCK_CONT        = 195154, // 联盟码头受击横幅
    GO_HORDE_BANNER_DOCK                = 195155, // 部落码头横幅
    GO_HORDE_BANNER_DOCK_CONT           = 195156, // 部落码头受击横幅

    GO_ALLIANCE_BANNER_HANGAR           = 195132, // 联盟机库横幅
    GO_ALLIANCE_BANNER_HANGAR_CONT      = 195144, // 联盟机库受击横幅
    GO_HORDE_BANNER_HANGAR              = 195130, // 部落机库横幅
    GO_HORDE_BANNER_HANGAR_CONT         = 195145, // 部落机库受击横幅

    GO_ALLIANCE_BANNER_QUARRY           = 195334, // 联盟采石场横幅
    GO_ALLIANCE_BANNER_QUARRY_CONT      = 195335, // 联盟采石场受击横幅
    GO_HORDE_BANNER_QUARRY              = 195336, // 部落采石场横幅
    GO_HORDE_BANNER_QUARRY_CONT         = 195337, // 部落采石场受击横幅

    GO_ALLIANCE_BANNER_REFINERY         = 195339, // 联盟精炼厂横幅
    GO_ALLIANCE_BANNER_REFINERY_CONT    = 195340, // 联盟精炼厂受击横幅
    GO_HORDE_BANNER_REFINERY            = 195341, // 部落精炼厂横幅
    GO_HORDE_BANNER_REFINERY_CONT       = 195342, // 部落精炼厂受击横幅

    //By leewheel 2026-09-08: IoC墓地横幅(WotLK与Cata一致)
    GO_ALLIANCE_BANNER_GRAVEYARD_A      = 195699, // 联盟墓地(联盟侧)横幅
    GO_ALLIANCE_BANNER_GRAVEYARD_A_CONT = 195700, // 联盟墓地(联盟侧)受击横幅
    GO_HORDE_BANNER_GRAVEYARD_A         = 195701, // 部落墓地(联盟侧)横幅
    GO_HORDE_BANNER_GRAVEYARD_A_CONT    = 195702, // 部落墓地(联盟侧)受击横幅
    GO_ALLIANCE_BANNER_GRAVEYARD_H      = 195703, // 联盟墓地(部落侧)横幅
    GO_ALLIANCE_BANNER_GRAVEYARD_H_CONT = 195704, // 联盟墓地(部落侧)受击横幅
    GO_HORDE_BANNER_GRAVEYARD_H         = 195705, // 部落墓地(部落侧)横幅
    GO_HORDE_BANNER_GRAVEYARD_H_CONT    = 195706  // 部落墓地(部落侧)受击横幅
};

#endif
