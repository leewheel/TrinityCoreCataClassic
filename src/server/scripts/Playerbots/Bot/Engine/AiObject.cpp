/*
 * AI对象基类
 * 从 AzerothCore mod-playerbots 移植到 TrinityCore
 */

#include "AiObject.h"

#include "PlayerbotAI.h"
#include "Playerbots.h"

AiObject::AiObject(PlayerbotAI* botAI)
    : PlayerbotAIAware(botAI), bot(botAI->GetBot()), context(botAI->GetAiObjectContext()), chat(botAI->GetChatHelper())
{
}

Player* AiObject::GetMaster() { return botAI->GetMaster(); }
