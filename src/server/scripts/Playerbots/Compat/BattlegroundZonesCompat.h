/*
 * By leewheel 2026-09-08
 * TC-Cata移除了BattlegroundAB/EY等战场Zone子类
 * 这些stub提供编译兼容,返回默认值的CapturePointInfo
 * TODO: 后续通过BattlegroundScript系统实现真实节点状态查询
 */
#ifndef PLAYERBOTS_BATTLEGROUNDZONESCOMPAT_H
#define PLAYERBOTS_BATTLEGROUNDZONESCOMPAT_H

#include "Battleground.h"

struct CapturePointInfo
{
    uint8 _state = 0;
    TeamId _ownerTeamId = TEAM_NEUTRAL;
};

class BattlegroundAB : public Battleground
{
public:
    CapturePointInfo GetCapturePointInfo(uint32 /*nodeId*/) const { return {}; }
};

class BattlegroundEY : public Battleground
{
public:
    CapturePointInfo GetCapturePointInfo(uint32 /*nodeId*/) const { return {}; }
};

#endif
