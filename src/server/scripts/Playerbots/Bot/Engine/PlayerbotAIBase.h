/*
 * 机器人AI基类
 * 从 AzerothCore mod-playerbots 移植到 TrinityCore
 */

#ifndef PLAYERBOTS_PLAYERBOTAIBASE_H
#define PLAYERBOTS_PLAYERBOTAIBASE_H

#include "Define.h"
#include "PlayerbotAIConfig.h"
#include "Player.h"

class PerfMonitorOperation;

class PlayerbotAIBase
{
public:
    PlayerbotAIBase(bool isBotAI);

    bool CanUpdateAI();
    uint32 GetNextAICheckDelay() const { return nextAICheckDelay; }
    void SetNextCheckDelay(uint32 const delay);
    void IncreaseNextCheckDelay(uint32 delay);
    void YieldThread(Player* bot, uint32 delay = sPlayerbotAIConfig.reactDelay);
    virtual void UpdateAI(uint32 elapsed, bool minimal = false);
    virtual void UpdateAIInternal(uint32 elapsed, bool minimal = false) = 0;
    bool IsActive();
    bool IsBotAI() const;

protected:
    uint32 nextAICheckDelay = 0;
    PerfMonitorOperation* totalPmo = nullptr;

private:
    bool _isBotAI;
};

#endif // PLAYERBOTS_PLAYERBOTAIBASE_H
