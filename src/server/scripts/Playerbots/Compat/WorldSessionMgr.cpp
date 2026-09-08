//By leewheel 2026-07-12
// WorldSessionMgr兼容层实现
// TC使用sWorld->FindSession管理会话，AC使用sWorldSessionMgr
// 此垫片将AC的sWorldSessionMgr调用转发到TC的sWorld

#include "WorldSessionMgr.h"
#include "World/World.h"

WorldSession* WorldSessionMgr::FindSession(uint32 id) const
{
    return sWorld->FindSession(id);
}

//By leewheel 2026-09-04: 修 C4100——TC 的 World::Update 内部已自动更新会话, 参数显式消引
void WorldSessionMgr::UpdateSessions(uint32 /*diff*/)
{
    // TC的World::Update内部会自动更新会话，这里不需要做任何事
}
//End By leewheel
