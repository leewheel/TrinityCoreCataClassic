#include "Aq20Actions.h"

#include "Aq20Utils.h"
#include "GameObjectPackets.h"

bool Aq20UseCrystalAction::Execute(Event /*event*/)
{
    if (Unit* boss = AI_VALUE2(Unit*, "find target", "15339"))
    {
        if (GameObject* crystal = RaidAq20Utils::GetNearestCrystal(boss))
        {
            float botDist = bot->GetDistance(crystal);
            if (botDist > INTERACTION_DISTANCE)
                return MoveTo(bot->GetMapId(),
                    crystal->GetPositionX() + frand(-3.5f, 3.5f),
                    crystal->GetPositionY() + frand(-3.5f, 3.5f),
                    crystal->GetPositionZ());

            // if we're already in range just wait here until it's time to activate crystal
            SetNextMovementDelay(500);

            // don't activate crystal if boss too far or its already been activated
            if (boss->GetDistance(crystal) > 25.0f ||
                crystal->HasFlag(GO_FLAG_IN_USE))
                return false;

            // don't activate crystal if boss doesn't have buff yet AND isn't going to have it soon
            // (though ideally bot should activate it ~5 seconds early due to time it takes for
            // crystal to activate and remove buff)
            if (!RaidAq20Utils::IsOssirianBuffActive(boss) &&
                RaidAq20Utils::GetOssirianDebuffTimeRemaining(boss) > 5000)
                return false;

            // 让水晶播放动画（然后消失）
            WPPCompat::GameObjectUse(bot->GetSession(), crystal->GetGUID());

            // 让水晶真正移除增益并施加减益效果
            WPPCompat::GameObjectReportUse(bot->GetSession(), crystal->GetGUID());

            return true;
        }
    }
    return false;
}
