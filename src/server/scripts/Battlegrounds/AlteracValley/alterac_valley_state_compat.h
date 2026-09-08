/*
 * By leewheel 2026-09-06
 * AV(奥特兰克山谷)节点状态查询兼容接口
 *
 * 背景：老大要求机器人基础功能必须从AzerothCore的mod-playerbots移植。
 * AC版的AV战术依赖WotLK的BattlegroundAV::GetAVNodeInfo/GetMineOwner查询节点归属与状态；
 * Cata核心把AV改为数据驱动脚本(src/server/scripts/Battlegrounds/AlteracValley)，
 * 其状态类battleground_alterac_valley是脚本内部结构，Playerbots无法直接访问。
 *
 * 方案：AV脚本实例通过本注册表把自身暴露出来，Playerbots按实例ID查询节点/矿洞状态。
 * 节点ID(0-14)与状态值(0中性/1被攻击/2被摧毁/3被控制)在Cata的AV脚本中与WotLK一致。
 */
#ifndef AV_STATE_COMPAT_H
#define AV_STATE_COMPAT_H

#include <cstdint>
#include <unordered_map>

// AV节点状态值(与Cata AV脚本的BG_AV_States一致)
enum AVCompatNodeState : uint8
{
    AV_COMPAT_POINT_NEUTRAL   = 0, // 中立
    AV_COMPAT_POINT_ASSAULTED = 1, // 被进攻(未占领)
    AV_COMPAT_POINT_DESTROYED = 2, // 被摧毁(哨塔)
    AV_COMPAT_POINT_CONTROLED = 3  // 被控制(已占领)
};

// AV节点状态查询抽象接口(由Cata的AV战场脚本实现)
struct BattlegroundAlteracValleyAccess
{
    virtual ~BattlegroundAlteracValleyAccess() = default;

    // 查询节点状态: node=节点ID(0-14)，state=AV状态值，owner=归属Team，tower=是否哨塔
    virtual bool GetNodeInfoCompat(uint8 node, uint8& state, uint32& owner, bool& tower) = 0;
    // 查询矿洞归属: mine=0北矿/1南矿，owner=归属Team
    virtual bool GetMineOwnerCompat(uint8 mine, uint32& owner) = 0;
    // 查询队长存活: team=TEAM_ALLIANCE(巴林达)/TEAM_HORDE(加尔范上尉)
    virtual bool IsCaptainAliveCompat(uint8 team) = 0;
};

// 注册表：战场实例ID → AV脚本实例(inline变量，链接器合并)
inline std::unordered_map<uint32, BattlegroundAlteracValleyAccess*>& AVCompatInstanceRegistry()
{
    static std::unordered_map<uint32, BattlegroundAlteracValleyAccess*> registry;
    return registry;
}

// 查询入口：返回false表示该实例不存在或不是AV
inline bool AVCompatQueryNode(uint32 instanceId, uint8 node, uint8& state, uint32& owner, bool& tower)
{
    auto const& registry = AVCompatInstanceRegistry();
    auto itr = registry.find(instanceId);
    if (itr == registry.end())
        return false;
    return itr->second->GetNodeInfoCompat(node, state, owner, tower);
}

inline bool AVCompatQueryMineOwner(uint32 instanceId, uint8 mine, uint32& owner)
{
    auto const& registry = AVCompatInstanceRegistry();
    auto itr = registry.find(instanceId);
    if (itr == registry.end())
        return false;
    return itr->second->GetMineOwnerCompat(mine, owner);
}

#endif
