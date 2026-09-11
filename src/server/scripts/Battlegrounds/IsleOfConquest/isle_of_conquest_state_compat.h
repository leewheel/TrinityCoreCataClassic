/*
 * By leewheel 2026-09-06
 * IC(奥特兰克盆地,Isle of Conquest)节点状态查询兼容接口
 *
 * 与AV同理：Cata的IC战场脚本(battlefield_isle_of_conquest)是脚本内部类，
 * Playerbots无法直接访问其节点状态；通过本注册表把各IC实例的节点状态暴露给战术模块。
 * 节点类型ID(NODE_TYPE_*)与状态值在Cata的IC脚本中与模块使用的语义一致。
 */
#ifndef IOC_STATE_COMPAT_H
#define IOC_STATE_COMPAT_H

#include <cstdint>
#include <unordered_map>

// 节点状态值(与Cata IC脚本的IsleOfConquestNodeState枚举一致)
namespace IoCCompat
{
    enum IoCNodeState : uint8
    {
        NODE_STATE_NEUTRAL      = 0, // 中立
        NODE_STATE_CONFLICT_A   = 1, // 联盟争夺中
        NODE_STATE_CONFLICT_H   = 2, // 部落争夺中
        NODE_STATE_CONTROLLED_A = 3, // 联盟已占领
        NODE_STATE_CONTROLLED_H = 4  // 部落已占领
    };

    // 兼容别名：WotLK的ICNodePoint.nodeState在模块中的其他称谓，语义与上述状态一一对应
    constexpr uint8 NODE_STATE_ALLY_CONTESTED  = NODE_STATE_CONFLICT_A;
    constexpr uint8 NODE_STATE_ALLY_OCCUPIED   = NODE_STATE_CONTROLLED_A;
    constexpr uint8 NODE_STATE_HORDE_CONTESTED = NODE_STATE_CONFLICT_H;
    constexpr uint8 NODE_STATE_HORDE_OCCUPIED  = NODE_STATE_CONTROLLED_H;
}

// 节点类型ID(与Cata IC脚本的ICNodePointType一致: 0精炼厂/1采石场/2码头/3机库/4车间/5联盟墓地/6部落墓地)
enum IoCCompatNodeType : uint8
{
    NODE_TYPE_REFINERY    = 0,
    NODE_TYPE_QUARRY      = 1,
    NODE_TYPE_DOCKS       = 2,
    NODE_TYPE_HANGAR      = 3,
    NODE_TYPE_WORKSHOP    = 4,
    NODE_TYPE_GRAVEYARD_A = 5,
    NODE_TYPE_GRAVEYARD_H = 6,
    MAX_IC_NODE_TYPES     = 7
};

// 城门索引(与Cata IC脚本的BG_IC_H_FRONT..BG_IC_A_EAST顺序一致)
enum IoCCompatGateId : uint8
{
    IOCGATE_H_FRONT = 0,
    IOCGATE_H_WEST  = 1,
    IOCGATE_H_EAST  = 2,
    IOCGATE_A_FRONT = 3,
    IOCGATE_A_WEST  = 4,
    IOCGATE_A_EAST  = 5
};

// 门状态值(与Cata IC脚本的BG_IC_GateState一致)
enum IoCCompatGateState : uint8
{
    IOCGATE_OK        = 1,
    IOCGATE_DAMAGED   = 2,
    IOCGATE_DESTROYED = 3
};

// By leewheel 2026-09-06: IC战场游戏对象/生物entry(客户端数据两代一致，取自Cata的IC脚本头)
//By leewheel 2026-09-08: 移除GO_*_BANNER定义,与BattlegroundEntriesCompat.h重复导致C2872歧义
namespace IoCCompat
{
    constexpr uint32 NPC_HIGH_COMMANDER_HALFORD_WYRMBANE = 34924; // 联盟首领
    constexpr uint32 NPC_OVERLORD_AGMAR                  = 34922; // 部落首领
    constexpr uint32 NPC_SIEGE_ENGINE_H                  = 35069; // 部落攻城车
    constexpr uint32 NPC_SIEGE_ENGINE_A                  = 34776; // 联盟攻城车

    constexpr uint32 GO_ALLIANCE_GATE_3                  = 195698; // 联盟前门
    constexpr uint32 GO_HORDE_GATE_1                     = 195494; // 部落前门

    // 传送器 entry ID
    constexpr uint32 GO_TELEPORTER_1                     = 195314; // 部落外侧
    constexpr uint32 GO_TELEPORTER_2                     = 195313; // 部落内侧
    constexpr uint32 GO_TELEPORTER_3                     = 195315; // 联盟外侧
    constexpr uint32 GO_TELEPORTER_4                     = 195316; // 联盟内侧

    // 城门/吊闸 GameObject entry ID
    constexpr uint32 GO_DOODAD_PORTCULLISACTIVE02        = 195452; // 联盟要塞内吊闸
    constexpr uint32 GO_DOODAD_HU_PORTCULLIS01           = 195436; // 部落要塞吊闸
}

// By leewheel 2026-09-09: AC兼容常量 - BG_IC_GO_* 用于GetBGObject(类型索引)
// 注意：TC的IC战场不使用GetBGObject模式，此处定义为entry ID仅供编译通过
//       实际调用GetBGObject可能返回nullptr，调用方需做判空处理
constexpr uint32 BG_IC_GO_DOODAD_PORTCULLISACTIVE02 = IoCCompat::GO_DOODAD_PORTCULLISACTIVE02;
constexpr uint32 BG_IC_GO_HORDE_KEEP_PORTCULLIS     = IoCCompat::GO_DOODAD_HU_PORTCULLIS01;
constexpr uint32 GO_TELEPORTER_2                     = IoCCompat::GO_TELEPORTER_2;
constexpr uint32 GO_TELEPORTER_4                     = IoCCompat::GO_TELEPORTER_4;
//End By leewheel

// 节点状态查询抽象接口(由Cata的IC战场脚本实现)
struct BattlegroundIsleOfConquestAccess
{
    virtual ~BattlegroundIsleOfConquestAccess() = default;

    // 查询节点状态: nodeType=节点类型，state=IoCCompat::IoCNodeState值
    virtual bool GetNodeStateCompat(uint8 nodeType, uint8& state) = 0;

    // 查询城门状态: gateId=0部落前门/1部落西门/2部落东门/3联盟前门/4联盟西门/5联盟东门
    // 返回值语义: 1完好 / 2受损 / 3被摧毁
    virtual bool GetGateStateCompat(uint8 gateId, uint8& state) = 0;
};

// 注册表：战场实例ID → IC脚本实例(inline变量，链接器合并)
inline std::unordered_map<uint32, BattlegroundIsleOfConquestAccess*>& IoCCompatInstanceRegistry()
{
    static std::unordered_map<uint32, BattlegroundIsleOfConquestAccess*> registry;
    return registry;
}

// 查询入口：返回false表示该实例不存在或不是IC
inline bool IoCCompatQueryNodeState(uint32 instanceId, uint8 nodeType, uint8& state)
{
    auto const& registry = IoCCompatInstanceRegistry();
    auto itr = registry.find(instanceId);
    if (itr == registry.end())
        return false;
    return itr->second->GetNodeStateCompat(nodeType, state);
}

inline bool IoCCompatQueryGateState(uint32 instanceId, uint8 gateId, uint8& state)
{
    auto const& registry = IoCCompatInstanceRegistry();
    auto itr = registry.find(instanceId);
    if (itr == registry.end())
        return false;
    return itr->second->GetGateStateCompat(gateId, state);
}

#endif
