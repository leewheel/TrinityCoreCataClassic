/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "EmoteAction.h"

#include "Event.h"
#include "Playerbots.h"
#include "ServerFacade.h"

std::map<std::string, uint32> EmoteActionBase::emotes;
std::map<std::string, uint32> EmoteActionBase::textEmotes;
char* strstri(char const* haystack, char const* needle);

EmoteActionBase::EmoteActionBase(PlayerbotAI* botAI, std::string const name) : Action(botAI, name)
{
    if (emotes.empty())
        InitEmotes();
}

EmoteAction::EmoteAction(PlayerbotAI* botAI) : EmoteActionBase(botAI, "emote"), Qualified() {}

void EmoteActionBase::InitEmotes()
{
    emotes["dance"] = EMOTE_ONESHOT_DANCE;
    emotes["drown"] = EMOTE_ONESHOT_DROWN;
    emotes["land"] = EMOTE_ONESHOT_LAND;
    emotes["liftoff"] = EMOTE_ONESHOT_LIFTOFF;
    emotes["loot"] = EMOTE_ONESHOT_LOOT;
    emotes["no"] = EMOTE_ONESHOT_NO;
    emotes["roar"] = EMOTE_STATE_ROAR;
    emotes["salute"] = EMOTE_ONESHOT_SALUTE;
    emotes["stomp"] = EMOTE_ONESHOT_STOMP;
    emotes["train"] = EMOTE_ONESHOT_TRAIN;
    emotes["yes"] = EMOTE_ONESHOT_YES;
    emotes["applaud"] = EMOTE_ONESHOT_APPLAUD;
    emotes["beg"] = EMOTE_ONESHOT_BEG;
    emotes["bow"] = EMOTE_ONESHOT_BOW;
    emotes["cheer"] = EMOTE_ONESHOT_CHEER;
    emotes["chicken"] = EMOTE_ONESHOT_CHICKEN;
    emotes["cry"] = EMOTE_ONESHOT_CRY;
    emotes["dance"] = EMOTE_STATE_DANCE;
    emotes["eat"] = EMOTE_ONESHOT_EAT;
    emotes["exclamation"] = EMOTE_ONESHOT_EXCLAMATION;
    emotes["flex"] = EMOTE_ONESHOT_FLEX;
    emotes["kick"] = EMOTE_ONESHOT_KICK;
    emotes["kiss"] = EMOTE_ONESHOT_KISS;
    emotes["kneel"] = EMOTE_ONESHOT_KNEEL;
    emotes["laugh"] = EMOTE_ONESHOT_LAUGH;
    emotes["point"] = EMOTE_ONESHOT_POINT;
    emotes["question"] = EMOTE_ONESHOT_QUESTION;
    emotes["ready1h"] = EMOTE_ONESHOT_READY1H;
    emotes["roar"] = EMOTE_ONESHOT_ROAR;
    emotes["rude"] = EMOTE_ONESHOT_RUDE;
    emotes["shout"] = EMOTE_ONESHOT_SHOUT;
    emotes["shy"] = EMOTE_ONESHOT_SHY;
    emotes["sleep"] = EMOTE_STATE_SLEEP;
    emotes["talk"] = EMOTE_ONESHOT_TALK;
    emotes["wave"] = EMOTE_ONESHOT_WAVE;
    emotes["wound"] = EMOTE_ONESHOT_WOUND;

    textEmotes["bored"] = TEXT_EMOTE_BORED;
    textEmotes["bye"] = TEXT_EMOTE_BYE;
    textEmotes["cheer"] = TEXT_EMOTE_CHEER;
    textEmotes["congratulate"] = TEXT_EMOTE_CONGRATULATE;
    textEmotes["hello"] = TEXT_EMOTE_HELLO;
    textEmotes["no"] = TEXT_EMOTE_NO;
    textEmotes["nod"] = TEXT_EMOTE_NOD;  // yes
    textEmotes["sigh"] = TEXT_EMOTE_SIGH;
    textEmotes["thank"] = TEXT_EMOTE_THANK;
    textEmotes["welcome"] = TEXT_EMOTE_WELCOME;  // you are welcome
    textEmotes["whistle"] = TEXT_EMOTE_WHISTLE;
    textEmotes["yawn"] = TEXT_EMOTE_YAWN;
    textEmotes["oom"] = 323;
    textEmotes["follow"] = 324;
    textEmotes["wait"] = 325;
    textEmotes["healme"] = 326;
    textEmotes["openfire"] = 327;
    textEmotes["helpme"] = 303;
    textEmotes["flee"] = 306;
    textEmotes["danger"] = 304;
    textEmotes["charge"] = 305;
    textEmotes["help"] = 307;
    textEmotes["train"] = 264;
}

bool EmoteActionBase::Emote(Unit* target, uint32 type, bool textEmote)
{
    if (target && !bot->HasInArc(static_cast<float>(M_PI), target, sPlayerbotAIConfig.sightDistance))
        bot->SetFacingToObject(target);

    ObjectGuid oldSelection = bot->GetTarget();
    if (target)
    {
        bot->SetSelection(target->GetGUID());

        Player* player = dynamic_cast<Player*>(target);
        if (player)
        {
            PlayerbotAI* playerBotAI = GET_PLAYERBOT_AI(player);
            if (playerBotAI && !player->HasInArc(static_cast<float>(M_PI), bot, sPlayerbotAIConfig.sightDistance))
            {
                player->SetFacingToObject(bot);
            }
        }
    }

    if (textEmote)
    {
        WPPCompat::DoTextEmote(bot->GetSession(), bot->GetTarget(), type, GetNumberOfEmoteVariants((TextEmotes)type, bot->getRace(), bot->getGender()));
    }
    else
        //By leewheel 2026-07-10: Emote类型与方法名冲突，使用::Emote指定全局枚举
        bot->HandleEmoteCommand((::Emote)type);
        //End By leewheel

    if (oldSelection)
        bot->SetTarget(oldSelection);

    return true;
}

Unit* EmoteActionBase::GetTarget()
{
    Unit* target = nullptr;

    GuidVector nfp = *context->GetValue<GuidVector>("nearest friendly players");
    std::vector<Unit*> targets;
    for (GuidVector::iterator i = nfp.begin(); i != nfp.end(); ++i)
    {
        Unit* unit = botAI->GetUnit(*i);
        if (unit && ServerFacade::instance().GetDistance2d(bot, unit) < sPlayerbotAIConfig.tooCloseDistance)
            targets.push_back(unit);
    }

    if (!targets.empty())
        target = targets[urand(0, targets.size() - 1)];

    return target;
}

bool EmoteActionBase::ReceiveEmote(Player* source, uint32 emote, bool verbal)
{
    uint32 emoteId = 0;
    uint32 textEmote = 0;
    std::string emoteText;
    std::string emoteYell;

    switch (emote)
    {
        case TEXT_EMOTE_BONK:
            emoteId = EMOTE_ONESHOT_CRY;
            textEmote = TEXT_EMOTE_CRY;
            break;
        case TEXT_EMOTE_SALUTE:
            emoteId = EMOTE_ONESHOT_SALUTE;
            textEmote = TEXT_EMOTE_SALUTE;
            break;
        case 325:
            if (botAI->GetMaster() == source)
            {
                botAI->ChangeStrategy("-follow,+stay", BOT_STATE_NON_COMBAT);
                //By leewheel 2026-08-01: 玩家可见文本中文化
                botAI->TellMasterNoFacing("好吧……我就待在这儿。");
                //End By leewheel
            }
            break;
        case TEXT_EMOTE_BECKON:
        case 324:
            if (botAI->GetMaster() == source)
            {
                botAI->ChangeStrategy("+follow", BOT_STATE_NON_COMBAT);
                //By leewheel 2026-08-01: 玩家可见文本中文化
                botAI->TellMasterNoFacing("无论你去哪，我都跟着。");
                //End By leewheel
            }
            break;
        case TEXT_EMOTE_WAVE:
        case TEXT_EMOTE_GREET:
        case TEXT_EMOTE_HAIL:
        case TEXT_EMOTE_HELLO:
        case TEXT_EMOTE_WELCOME:
        case TEXT_EMOTE_INTRODUCE:
            emoteText = "嗨，你好！";
            emoteId = EMOTE_ONESHOT_WAVE;
            textEmote = TEXT_EMOTE_HELLO;
            break;
        case TEXT_EMOTE_DANCE:
            emoteText = "扭起来，让你老妈给的都派上用场！";
            emoteId = EMOTE_ONESHOT_DANCE;
            textEmote = TEXT_EMOTE_DANCE;
            break;
        case TEXT_EMOTE_FLIRT:
        case TEXT_EMOTE_KISS:
        case TEXT_EMOTE_HUG:
        case TEXT_EMOTE_BLUSH:
        case TEXT_EMOTE_SMILE:
        case TEXT_EMOTE_LOVE:
            // case TEXT_EMOTE_HOLDHAND:
            emoteText = "哎呀呀……";
            emoteId = EMOTE_ONESHOT_SHY;
            textEmote = TEXT_EMOTE_SHY;
            break;
        case TEXT_EMOTE_FLEX:
            emoteText = "大力士！大力士！";
            emoteId = EMOTE_ONESHOT_APPLAUD;
            textEmote = TEXT_EMOTE_APPLAUD;
            break;
        case TEXT_EMOTE_ANGRY:
            // case TEXT_EMOTE_FACEPALM:
        case TEXT_EMOTE_GLARE:
        case TEXT_EMOTE_BLAME:
            // case TEXT_EMOTE_FAIL:
            // case TEXT_EMOTE_REGRET:
            // case TEXT_EMOTE_SCOLD:
            // case TEXT_EMOTE_CROSSARMS:
            emoteText = "是我干的吗——？";
            emoteId = EMOTE_ONESHOT_QUESTION;
            textEmote = TEXT_EMOTE_SHRUG;
            break;
        case TEXT_EMOTE_FART:
        case TEXT_EMOTE_BURP:
        case TEXT_EMOTE_GASP:
        case TEXT_EMOTE_NOSEPICK:
        case TEXT_EMOTE_SNIFF:
        case TEXT_EMOTE_STINK:
            emoteText = "不是我！就说说而已……";
            emoteId = EMOTE_ONESHOT_POINT;
            textEmote = TEXT_EMOTE_POINT;
            break;
        case TEXT_EMOTE_JOKE:
            emoteId = EMOTE_ONESHOT_LAUGH;
            textEmote = TEXT_EMOTE_LAUGH;
            emoteText = "哦……我是不是不该笑这么早？";
            break;
        case TEXT_EMOTE_CHICKEN:
            emoteText = "很快就能看到谁才是胆小鬼！";
            emoteId = EMOTE_ONESHOT_RUDE;
            textEmote = TEXT_EMOTE_RUDE;
            break;
        case TEXT_EMOTE_APOLOGIZE:
            emoteId = EMOTE_ONESHOT_POINT;
            textEmote = TEXT_EMOTE_APOLOGIZE;
            emoteText = "你确实该道歉！";
            break;
        case TEXT_EMOTE_APPLAUD:
        case TEXT_EMOTE_CLAP:
        case TEXT_EMOTE_CONGRATULATE:
        case TEXT_EMOTE_HAPPY:
            // case TEXT_EMOTE_GOLFCLAP:
            emoteId = EMOTE_ONESHOT_BOW;
            textEmote = TEXT_EMOTE_BOW;
            emoteText = "谢谢……谢谢……我整周都在这里。";
            break;
        case TEXT_EMOTE_BEG:
        case TEXT_EMOTE_GROVEL:
        case TEXT_EMOTE_PLEAD:
            emoteId = EMOTE_ONESHOT_NO;
            textEmote = TEXT_EMOTE_NO;
            emoteText = "尽管求我吧……我可什么都不会给你。";
            break;
        case TEXT_EMOTE_BITE:
        case TEXT_EMOTE_POKE:
        case TEXT_EMOTE_SCRATCH:
            // case TEXT_EMOTE_PINCH:
            // case TEXT_EMOTE_PUNCH:
            emoteId = EMOTE_ONESHOT_ROAR;
            textEmote = TEXT_EMOTE_ROAR;
            emoteYell = "OUCH! Dammit, that hurt!";
            break;
        case TEXT_EMOTE_BORED:
            emoteId = EMOTE_ONESHOT_NO;
            textEmote = TEXT_EMOTE_NO;
            emoteText = "我的工作职责可不包括逗你开心……";
            break;
        case TEXT_EMOTE_BOW:
        case TEXT_EMOTE_CURTSEY:
            emoteId = EMOTE_ONESHOT_BOW;
            textEmote = TEXT_EMOTE_BOW;
            break;
        case TEXT_EMOTE_BRB:
        case TEXT_EMOTE_SIT:
            emoteId = EMOTE_ONESHOT_EAT;
            textEmote = TEXT_EMOTE_EAT;
            emoteText = "看来该挂机休息一下了……";
            break;
        case TEXT_EMOTE_AGREE:
        case TEXT_EMOTE_NOD:
            emoteId = EMOTE_ONESHOT_EXCLAMATION;
            textEmote = TEXT_EMOTE_NOD;
            emoteText = "总算有人同意我了！";
            break;
        case TEXT_EMOTE_AMAZE:
        case TEXT_EMOTE_COWER:
        case TEXT_EMOTE_CRINGE:
        case TEXT_EMOTE_EYE:
        case TEXT_EMOTE_KNEEL:
        case TEXT_EMOTE_PEER:
        case TEXT_EMOTE_SURRENDER:
        case TEXT_EMOTE_PRAISE:
        case TEXT_EMOTE_SCARED:
        case TEXT_EMOTE_COMMEND:
            // case TEXT_EMOTE_AWE:
            // case TEXT_EMOTE_JEALOUS:
            // case TEXT_EMOTE_PROUD:
            emoteId = EMOTE_ONESHOT_FLEX;
            textEmote = TEXT_EMOTE_FLEX;
            emoteText = "是的，是的，我知道我很厉害……";
            break;
        case TEXT_EMOTE_BLEED:
        case TEXT_EMOTE_MOURN:
        case TEXT_EMOTE_FLOP:
            // case TEXT_EMOTE_FAINT:
            // case TEXT_EMOTE_PULSE:
            emoteId = EMOTE_ONESHOT_KNEEL;
            textEmote = TEXT_EMOTE_KNEEL;
            emoteText = "军医！快来！";
            break;
        case TEXT_EMOTE_BLINK:
            emoteId = EMOTE_ONESHOT_KICK;
            emoteText = "怎么？你眼睛进东西了？";
            break;
        case TEXT_EMOTE_BOUNCE:
        case TEXT_EMOTE_BARK:
            emoteId = EMOTE_ONESHOT_POINT;
            textEmote = TEXT_EMOTE_POINT;
            emoteText = "谁是好狗狗？你是好狗狗！";
            break;
        case TEXT_EMOTE_BYE:
            emoteId = EMOTE_ONESHOT_WAVE;
            textEmote = TEXT_EMOTE_WAVE;
            emoteText = "嗯……等等！你要去哪？";
            break;
        case TEXT_EMOTE_CACKLE:
        case TEXT_EMOTE_LAUGH:
        case TEXT_EMOTE_CHUCKLE:
        case TEXT_EMOTE_GIGGLE:
        case TEXT_EMOTE_GUFFAW:
        case TEXT_EMOTE_ROFL:
        case TEXT_EMOTE_SNICKER:
            // case TEXT_EMOTE_SNORT:
            emoteId = EMOTE_ONESHOT_LAUGH;
            textEmote = TEXT_EMOTE_LAUGH;
            emoteText = "等等……我们到底在笑什么来着？";
            break;
        case TEXT_EMOTE_CONFUSED:
        case TEXT_EMOTE_CURIOUS:
        case TEXT_EMOTE_FIDGET:
        case TEXT_EMOTE_FROWN:
        case TEXT_EMOTE_SHRUG:
        case TEXT_EMOTE_SIGH:
        case TEXT_EMOTE_STARE:
        case TEXT_EMOTE_TAP:
        case TEXT_EMOTE_SURPRISED:
        case TEXT_EMOTE_WHINE:
        case TEXT_EMOTE_BOGGLE:
        case TEXT_EMOTE_LOST:
        case TEXT_EMOTE_PONDER:
        case TEXT_EMOTE_SNUB:
        case TEXT_EMOTE_SERIOUS:
        case TEXT_EMOTE_EYEBROW:
            emoteId = EMOTE_ONESHOT_QUESTION;
            textEmote = TEXT_EMOTE_SHRUG;
            emoteText = "别看我……我只是在这打工的";
            break;
        case TEXT_EMOTE_COUGH:
        case TEXT_EMOTE_DROOL:
        case TEXT_EMOTE_SPIT:
        case TEXT_EMOTE_LICK:
        case TEXT_EMOTE_BREATH:
            // case TEXT_EMOTE_SNEEZE:
            // case TEXT_EMOTE_SWEAT:
            emoteId = EMOTE_ONESHOT_POINT;
            textEmote = TEXT_EMOTE_POINT;
            emoteText = "咦！把你的脏细菌放那边去！";
            break;
        case TEXT_EMOTE_CRY:
            emoteId = EMOTE_ONESHOT_CRY;
            textEmote = TEXT_EMOTE_CRY;
            emoteText = "你可别先哭，不然我也要哭了！";
            break;
        case TEXT_EMOTE_CRACK:
            emoteId = EMOTE_ONESHOT_ROAR;
            textEmote = TEXT_EMOTE_ROAR;
            emoteText = "揍人时间到！";
            break;
        case TEXT_EMOTE_EAT:
        case TEXT_EMOTE_DRINK:
            emoteId = EMOTE_ONESHOT_EAT;
            textEmote = TEXT_EMOTE_EAT;
            emoteText = "我希望你带够了全班人的份……";
            break;
        case TEXT_EMOTE_GLOAT:
        case TEXT_EMOTE_MOCK:
        case TEXT_EMOTE_TEASE:
        case TEXT_EMOTE_EMBARRASS:
            emoteId = EMOTE_ONESHOT_CRY;
            textEmote = TEXT_EMOTE_CRY;
            emoteText = "但这不意味着你要这么混蛋吧……";
            break;
        case TEXT_EMOTE_HUNGRY:
            emoteId = EMOTE_ONESHOT_EAT;
            textEmote = TEXT_EMOTE_EAT;
            emoteText = "怎么？你想尝尝这个？";
            break;
        case TEXT_EMOTE_LAYDOWN:
        case TEXT_EMOTE_TIRED:
        case TEXT_EMOTE_YAWN:
            emoteId = EMOTE_ONESHOT_KNEEL;
            textEmote = TEXT_EMOTE_KNEEL;
            emoteText = "已经到休息时间了？";
            break;
        case TEXT_EMOTE_MOAN:
        case TEXT_EMOTE_MOON:
        case TEXT_EMOTE_SEXY:
        case TEXT_EMOTE_SHAKE:
        case TEXT_EMOTE_WHISTLE:
        case TEXT_EMOTE_CUDDLE:
        case TEXT_EMOTE_PURR:
        case TEXT_EMOTE_SHIMMY:
        case TEXT_EMOTE_SMIRK:
        case TEXT_EMOTE_WINK:
            // case TEXT_EMOTE_CHARM:
            emoteId = EMOTE_ONESHOT_NO;
            textEmote = TEXT_EMOTE_NO;
            emoteText = "老板，悠着点……";
            break;
        case TEXT_EMOTE_NO:
        case TEXT_EMOTE_VETO:
        case TEXT_EMOTE_DISAGREE:
        case TEXT_EMOTE_DOUBT:
            emoteId = EMOTE_ONESHOT_QUESTION;
            textEmote = TEXT_EMOTE_SHRUG;
            emoteText = "哎呀……为什么不行？！";
            break;
        case TEXT_EMOTE_PANIC:
            emoteId = EMOTE_ONESHOT_EXCLAMATION;
            textEmote = TEXT_EMOTE_CALM;
            emoteText = "现在可不是惊慌的时候！";
            break;
        case TEXT_EMOTE_POINT:
            emoteId = EMOTE_ONESHOT_POINT;
            textEmote = TEXT_EMOTE_POINT;
            emoteText = "什么？！这个我也会！";
            break;
        case TEXT_EMOTE_RUDE:
        case TEXT_EMOTE_RASP:
            emoteId = EMOTE_ONESHOT_RUDE;
            textEmote = TEXT_EMOTE_RASP;
            emoteText = "原话奉还，伙计！";  // , LANG_UNIVERSAL;
            break;
        case TEXT_EMOTE_ROAR:
        case TEXT_EMOTE_THREATEN:
        case TEXT_EMOTE_CALM:
        case TEXT_EMOTE_DUCK:
        case TEXT_EMOTE_TAUNT:
        case TEXT_EMOTE_PITY:
        case TEXT_EMOTE_GROWL:
            // case TEXT_EMOTE_TRAIN:
            // case TEXT_EMOTE_INCOMING:
            // case TEXT_EMOTE_CHARGE:
            // case TEXT_EMOTE_FLEE:
            // case TEXT_EMOTE_ATTACKMYTARGET:
        case TEXT_EMOTE_OPENFIRE:
        case TEXT_EMOTE_ENCOURAGE:
        case TEXT_EMOTE_ENEMY:
            // case TEXT_EMOTE_CHALLENGE:
            // case TEXT_EMOTE_REVENGE:
            // case TEXT_EMOTE_SHAKEFIST:
            emoteId = EMOTE_ONESHOT_ROAR;
            textEmote = TEXT_EMOTE_ROAR;
            emoteYell = "RAWR!";
            break;
        case TEXT_EMOTE_TALK:
            emoteId = EMOTE_ONESHOT_TALK;
            textEmote = TEXT_EMOTE_LISTEN;
            break;
        case TEXT_EMOTE_TALKEX:
            emoteId = EMOTE_ONESHOT_YES;
            textEmote = TEXT_EMOTE_AGREE;
            break;
        case TEXT_EMOTE_TALKQ:
        case TEXT_EMOTE_LISTEN:
            emoteId = EMOTE_ONESHOT_TALK;
            textEmote = TEXT_EMOTE_TALKQ;
            emoteText = "叽里呱啦叽里呱啦……";
            break;
        case TEXT_EMOTE_THANK:
            emoteId = EMOTE_ONESHOT_BOW;
            textEmote = TEXT_EMOTE_BOW;
            emoteText = "你真不用客气！";
            break;
        case TEXT_EMOTE_VICTORY:
        case TEXT_EMOTE_CHEER:
        case TEXT_EMOTE_TOAST:
            // case TEXT_EMOTE_HIGHFIVE:
            // case TEXT_EMOTE_DING:
            emoteId = EMOTE_ONESHOT_CHEER;
            textEmote = TEXT_EMOTE_CHEER;
            emoteText = "耶！";
            break;
        case TEXT_EMOTE_COLD:
        case TEXT_EMOTE_SHIVER:
        case TEXT_EMOTE_THIRSTY:
            // case TEXT_EMOTE_OOM:
            // case TEXT_EMOTE_HEALME:
            // case TEXT_EMOTE_POUT:
            emoteId = EMOTE_ONESHOT_QUESTION;
            textEmote = TEXT_EMOTE_PUZZLE;
            emoteText = "那我到底该怎么做？";
            break;
        case TEXT_EMOTE_COMFORT:
        case TEXT_EMOTE_SOOTHE:
        case TEXT_EMOTE_PAT:
            emoteId = EMOTE_ONESHOT_CRY;
            textEmote = TEXT_EMOTE_CRY;
            emoteText = "谢谢……";
            break;
        case TEXT_EMOTE_INSULT:
            emoteId = EMOTE_ONESHOT_CRY;
            textEmote = TEXT_EMOTE_CRY;
            emoteText = "你伤到我的心了……";
            break;
        case TEXT_EMOTE_JK:
            emoteId = EMOTE_ONESHOT_POINT;
            textEmote = TEXT_EMOTE_POINT;
            emoteText = "你……";
            break;
        case TEXT_EMOTE_RAISE:
            emoteId = EMOTE_ONESHOT_POINT;
            textEmote = TEXT_EMOTE_POINT;
            emoteText = "对……就是你……教室后排那位……";
            break;
        case TEXT_EMOTE_READY:
            emoteId = EMOTE_ONESHOT_SALUTE;
            textEmote = TEXT_EMOTE_SALUTE;
            emoteText = "我也准备好了！";
            break;
        case TEXT_EMOTE_SHOO:
            emoteId = EMOTE_ONESHOT_KICK;
            textEmote = TEXT_EMOTE_SHOO;
            emoteText = "你自己一边去！";
            break;
        case TEXT_EMOTE_SLAP:
            // case TEXT_EMOTE_SMACK:
            emoteId = EMOTE_ONESHOT_CRY;
            textEmote = TEXT_EMOTE_CRY;
            emoteText = "我做了什么要遭这报应？";
            break;
        case TEXT_EMOTE_STAND:
            emoteId = EMOTE_ONESHOT_NONE;
            textEmote = TEXT_EMOTE_STAND;
            emoteText = "什么？休息时间结束了？好吧……";
            break;
        case TEXT_EMOTE_TICKLE:
            emoteId = EMOTE_ONESHOT_LAUGH;
            textEmote = TEXT_EMOTE_GIGGLE;
            emoteText = "嘿！住手！";
            break;
        case TEXT_EMOTE_VIOLIN:
            emoteId = EMOTE_ONESHOT_TALK;
            textEmote = TEXT_EMOTE_SIGH;
            emoteText = "哈哈哈……真好笑……";
            break;
            // case TEXT_EMOTE_HELPME:
            //     bot->HandleEmoteCommand(EMOTE_ONESHOT_POINT);
            //     bot->Yell("Quick! Someone HELP!", LANG_UNIVERSAL);
            //     break;
        case TEXT_EMOTE_GOODLUCK:
            // case TEXT_EMOTE_LUCK:
            emoteId = EMOTE_ONESHOT_TALK;
            textEmote = TEXT_EMOTE_THANK;
            emoteText = "谢谢……我确实需要这个……";
            break;
        case TEXT_EMOTE_BRANDISH:
            // case TEXT_EMOTE_MERCY:
            emoteId = EMOTE_ONESHOT_BEG;
            textEmote = TEXT_EMOTE_BEG;
            emoteText = "求求你别杀我！";
            break;
        /*case TEXT_EMOTE_BADFEELING:
            bot->HandleEmoteCommand(EMOTE_ONESHOT_QUESTION);
            bot->Say("I'm just waiting for the ominous music now...", LANG_UNIVERSAL);
            break;
        case TEXT_EMOTE_MAP:
            bot->HandleEmoteCommand(EMOTE_ONESHOT_NO);
            bot->Say("Noooooooo.. you just couldn't ask for directions, huh?", LANG_UNIVERSAL);
            break;
        case TEXT_EMOTE_IDEA:
        case TEXT_EMOTE_THINK:
            bot->HandleEmoteCommand(EMOTE_ONESHOT_NO);
            bot->Say("Oh boy.. another genius idea...", LANG_UNIVERSAL);
            break;
        case TEXT_EMOTE_OFFER:
            bot->HandleEmoteCommand(EMOTE_ONESHOT_NO);
            bot->Say("No thanks.. I had some back at the last village", LANG_UNIVERSAL);
            break;
        case TEXT_EMOTE_PET:
            bot->HandleEmoteCommand(EMOTE_ONESHOT_ROAR);
            bot->Say("Do I look like a dog to you?!", LANG_UNIVERSAL);
            break;
        case TEXT_EMOTE_ROLLEYES:
            bot->HandleEmoteCommand(EMOTE_ONESHOT_POINT);
            bot->Say("Keep doing that and I'll roll those eyes right out of your head..", LANG_UNIVERSAL);
            break;
        case TEXT_EMOTE_SING:
            bot->HandleEmoteCommand(EMOTE_ONESHOT_APPLAUD);
            bot->Say("Lovely... just lovely..", LANG_UNIVERSAL);
            break;
        case TEXT_EMOTE_COVEREARS:
            bot->HandleEmoteCommand(EMOTE_ONESHOT_EXCLAMATION);
            bot->Yell("You think that's going to help you?!", LANG_UNIVERSAL);
            break;*/
        default:
            // return false;
            // bot->HandleEmoteCommand(EMOTE_ONESHOT_QUESTION);
            // bot->Say("Mmmmmkaaaaaay...", LANG_UNIVERSAL);
            break;
    }

    if (source && !bot->isMoving() && !bot->HasInArc(static_cast<float>(M_PI), source, sPlayerbotAIConfig.farDistance))
        ServerFacade::instance().SetFacingTo(bot, source);

    if (verbal)
    {
        if (emoteText.size())
            bot->Say(emoteText, (bot->GetTeamId() == TEAM_ALLIANCE ? LANG_COMMON : LANG_ORCISH));

        if (emoteYell.size())
            bot->Yell(emoteYell, (bot->GetTeamId() == TEAM_ALLIANCE ? LANG_COMMON : LANG_ORCISH));
    }

    //By leewheel 2026-07-10: 修复type未定义问题，使用textEmote变量
    if (textEmote)
    {
        WPPCompat::DoTextEmote(bot->GetSession(), bot->GetTarget(), textEmote, GetNumberOfEmoteVariants((TextEmotes)textEmote, bot->getRace(), bot->getGender()));
    }
    else
    {
        if (emoteId)
            bot->HandleEmoteCommand((::Emote)emoteId);
    }
    //End By leewheel

    return true;
}

bool EmoteAction::Execute(Event event)
{
    WorldPacket p(event.getPacket());
    uint32 emote = 0;

    Player* pSource = nullptr;
    bool isReact = false;
    if (!p.empty() && p.GetOpcode() == SMSG_TEXT_EMOTE)
    {
        isReact = true;
        //By leewheel 2026-08-04: 适配TC的SMSG_TEXT_EMOTE格式
        //AC格式: source(8)+text_emote(4)+emote_num(4)+namlen(4)+nam(namlen)
        //TC格式: SourceGUID(8)+SourceAccountGUID(8)+EmoteID(4)+SoundIndex(4)+TargetGUID(8)=32字节
        //AC原代码用rpos(0)把opcode当包体读,且按AC格式解析namlen=EmoteID可能为垃圾值导致ReadString越界崩溃
        if (p.wpos() < p.rpos() || (p.wpos() - p.rpos()) < 32)
            return false;

        ObjectGuid source;
        ObjectGuid sourceAccountGuid;
        uint32 text_emote;
        uint32 soundIndex;
        ObjectGuid targetGuid;
        p >> source >> sourceAccountGuid >> text_emote >> soundIndex >> targetGuid;
        //End By leewheel

        pSource = ObjectAccessor::FindPlayer(source);
        if (pSource && (pSource->GetGUID() != bot->GetGUID()) &&
            (urand(0, 1) && bot->HasInArc(static_cast<float>(M_PI), pSource, 10.0f)))
        {
            emote = text_emote;
        }
    }

    if (!p.empty() && p.GetOpcode() == SMSG_EMOTE)
    {
        isReact = true;
        //By leewheel 2026-08-04: 适配TC的SMSG_EMOTE格式
        //AC格式: emoteId(4)+source(8)=12字节
        //TC格式: Guid(8)+EmoteID(4)+SpellVisualKitIDs.size(4)+SequenceVariation(4)=20字节(最小)
        //AC原代码用rpos(0)且字段顺序相反,会导致source读到垃圾值
        if (p.wpos() < p.rpos() || (p.wpos() - p.rpos()) < 20)
            return false;

        ObjectGuid source;
        uint32 emoteId;
        uint32 spellVisualKitCount;
        int32 sequenceVariation;
        p >> source >> emoteId >> spellVisualKitCount >> sequenceVariation;
        //End By leewheel

        pSource = ObjectAccessor::FindPlayer(source);
        if (pSource && pSource != bot && ServerFacade::instance().GetDistance2d(bot, pSource) < sPlayerbotAIConfig.farDistance &&
            emoteId != EMOTE_ONESHOT_NONE)
        {
            if ((pSource->GetGUID() != bot->GetGUID()) &&
                (pSource->GetTarget() == bot->GetGUID() ||
                 (urand(0, 1) && bot->HasInArc(static_cast<float>(M_PI), pSource, 10.0f))))
            {
                /*TC_LOG_INFO("playerbots", "Bot {} {}:{} <{}> received SMSG_EMOTE {} from player {} <{}>",
                    bot->GetGUID().ToString().c_str(), bot->GetTeamId() == TEAM_ALLIANCE ? "A" : "H", bot->GetLevel(),
                   bot->GetName(), emoteId, pSource->GetGUID().ToString().c_str(), pSource->GetName());*/

                std::vector<uint32> types;
                for (int32 i = sEmotesTextStore.GetNumRows(); i >= 0; --i)
                {
                    EmotesTextEntry const* em = sEmotesTextStore.LookupEntry(uint32(i));
                    if (!em)
                        continue;

                    //By leewheel 2026-09-09: 移植到TrinityCore-Cata，EmotesTextEntry兼容——
                    //Cata的EmotesTextEntry无textid()方法，用EmoteID字段(经EmotesTextEntry_GetTextId兼容函数)
                    if (EmotesTextEntry_GetTextId(em) == static_cast<uint16>(EMOTE_ONESHOT_TALK))
                        continue;

                    if (EmotesTextEntry_GetTextId(em) == static_cast<uint16>(EMOTE_ONESHOT_QUESTION))
                        continue;

                    if (EmotesTextEntry_GetTextId(em) == static_cast<uint16>(EMOTE_ONESHOT_EXCLAMATION))
                        continue;

                    if (EmotesTextEntry_GetTextId(em) == emoteId)
                    {
                        types.push_back(em->ID);
                    }
                    //End By leewheel
                }

                if (types.size())
                    emote = types[urand(0, types.size() - 1)];
            }
        }
    }

    if (isReact && !emote)
        return false;

    std::string param = event.getParam();
    if ((!isReact && param.empty()) || emote)
    {
        // time_t lastEmote = AI_VALUE2(time_t, "last emote", qualifier); //not used, line marked for removal.
        botAI->GetAiObjectContext()
            ->GetValue<time_t>("last emote", qualifier)
            ->Set(time(nullptr) + urand(1000, sPlayerbotAIConfig.repeatDelay) / 1000);
        param = qualifier;
    }

    if (emote)
        return ReceiveEmote(pSource, emote, bot->InBattleground() ? false : urand(0, 1));

    if (param.find("sound") == 0)
    {
        return botAI->PlaySound(atoi(param.substr(5).c_str()));
    }

    //By leewheel 2026-07-10: 修复type未定义问题，使用textEmotes[param]
    if (!param.empty() && textEmotes.find(param) != textEmotes.end())
    {
        WPPCompat::DoTextEmote(bot->GetSession(), bot->GetTarget(), textEmotes[param], GetNumberOfEmoteVariants((TextEmotes)textEmotes[param], bot->getRace(), bot->getGender()));
        return true;
    }
    //End By leewheel

    if (param.empty() || emotes.find(param) == emotes.end())
    {
        uint32 index = rand() % emotes.size();
        for (std::map<std::string, uint32>::iterator i = emotes.begin(); i != emotes.end() && index; ++i, --index)
            emote = i->second;
    }
    else
    {
        emote = emotes[param];
    }

    if (param.find("text") == 0)
    {
        emote = atoi(param.substr(4).c_str());
    }

    return Emote(GetTarget(), emote);
}

bool EmoteAction::isUseful()
{
    if (!botAI->AllowActivity())
        return false;

    time_t lastEmote = AI_VALUE2(time_t, "last emote", qualifier);
    return time(nullptr) >= lastEmote;
}

bool TalkAction::Execute(Event /*event*/)
{
    Unit* target = botAI->GetUnit(AI_VALUE(ObjectGuid, "talk target"));
    if (!target)
        target = GetTarget();

    if (!urand(0, 100))
    {
        target = nullptr;
        context->GetValue<ObjectGuid>("talk target")->Set(ObjectGuid::Empty);
        return true;
    }

    if (target)
    {
        if (Player* player = dynamic_cast<Player*>(target))
            if (PlayerbotAI* playerBotAI = GET_PLAYERBOT_AI(player))
                playerBotAI->GetAiObjectContext()->GetValue<ObjectGuid>("talk target")->Set(bot->GetGUID());

        context->GetValue<ObjectGuid>("talk target")->Set(target->GetGUID());
        return Emote(target, GetRandomEmote(target, true), true);
    }

    return false;
}

uint32 TalkAction::GetRandomEmote(Unit* unit, bool textEmote)
{
    std::vector<uint32> types;
    if (textEmote)
    {
        if (!urand(0, 20))
        {
            // expressions
            types.push_back(TEXT_EMOTE_BOW);
            types.push_back(TEXT_EMOTE_RUDE);
            types.push_back(TEXT_EMOTE_CRY);
            types.push_back(TEXT_EMOTE_LAUGH);
            types.push_back(TEXT_EMOTE_POINT);
            types.push_back(TEXT_EMOTE_CHEER);
            types.push_back(TEXT_EMOTE_SHY);
            types.push_back(TEXT_EMOTE_JOKE);
        }
        else
        {
            // talk
            types.push_back(TEXT_EMOTE_TALK);
            types.push_back(TEXT_EMOTE_TALKEX);
            types.push_back(TEXT_EMOTE_TALKQ);

            if (unit && (unit->HasNpcFlag(UNIT_NPC_FLAG_TRAINER) ||
                         unit->HasNpcFlag(UNIT_NPC_FLAG_QUESTGIVER)))
            {
                types.push_back(TEXT_EMOTE_SALUTE);
            }
        }
        return types[urand(0, types.size() - 1)];
    }

    if (!urand(0, 20))
    {
        // expressions
        types.push_back(EMOTE_ONESHOT_BOW);
        types.push_back(EMOTE_ONESHOT_RUDE);
        types.push_back(EMOTE_ONESHOT_CRY);
        types.push_back(EMOTE_ONESHOT_LAUGH);
        types.push_back(EMOTE_ONESHOT_POINT);
        types.push_back(EMOTE_ONESHOT_CHEER);
        types.push_back(EMOTE_ONESHOT_SHY);
    }
    else
    {
        // talk
        types.push_back(EMOTE_ONESHOT_TALK);
        types.push_back(EMOTE_ONESHOT_EXCLAMATION);
        types.push_back(EMOTE_ONESHOT_QUESTION);

        if (unit && (unit->HasNpcFlag(UNIT_NPC_FLAG_TRAINER) ||
                     unit->HasNpcFlag(UNIT_NPC_FLAG_QUESTGIVER)))
        {
            types.push_back(EMOTE_ONESHOT_SALUTE);
        }
    }

    return types[urand(0, types.size() - 1)];
}

uint32 EmoteActionBase::GetNumberOfEmoteVariants(TextEmotes emote, uint8 Race, uint8 Gender)
{
    if (emote == 304)
    {
        switch (Race)
        {
            case RACE_HUMAN:
            case RACE_DWARF:
            case RACE_NIGHTELF:
            case RACE_UNDEAD_PLAYER:
            case RACE_TROLL:
            {
                if (Gender == GENDER_MALE)
                    return 2;

                return 2;
            }
            case RACE_GNOME:
            {
                if (Gender == GENDER_MALE)
                    return 1;

                return 1;
            }
            case RACE_ORC:
            {
                if (Gender == GENDER_MALE)
                    return 3;

                return 2;
            }
            case RACE_TAUREN:
            {
                if (Gender == GENDER_MALE)
                    return 2;

                return 3;
            }
        }
    }
    else if (emote == 305)
    {
        switch (Race)
        {
            case RACE_HUMAN:
            case RACE_DWARF:
            case RACE_UNDEAD_PLAYER:
            case RACE_TROLL:
            {
                if (Gender == GENDER_MALE)
                    return 2;

                return 2;
            }
            case RACE_NIGHTELF:
            {
                if (Gender == GENDER_MALE)
                    return 2;

                return 3;
            }
            case RACE_GNOME:
            case RACE_TAUREN:
            {
                if (Gender == GENDER_MALE)
                    return 3;

                return 2;
            }
            case RACE_ORC:
            {
                if (Gender == GENDER_MALE)
                    return 3;

                return 3;
            }
        }
    }
    else if (emote == 306)
    {
        switch (Race)
        {
            case RACE_HUMAN:
            case RACE_NIGHTELF:
            case RACE_ORC:
            case RACE_UNDEAD_PLAYER:
            case RACE_TAUREN:
            case RACE_TROLL:
            {
                if (Gender == GENDER_MALE)
                    return 2;

                return 2;
            }
            case RACE_DWARF:
            {
                if (Gender == GENDER_MALE)
                    return 3;

                return 2;
            }
            case RACE_GNOME:
            {
                if (Gender == GENDER_MALE)
                    return 3;

                return 3;
            }
        }
    }
    else if (emote == TEXT_EMOTE_HELLO)
    {
        switch (Race)
        {
            case RACE_HUMAN:
            case RACE_DWARF:
            case RACE_GNOME:
            {
                if (Gender == GENDER_MALE)
                    return 4;

                return 3;
            }
            case RACE_NIGHTELF:
            {
                if (Gender == GENDER_MALE)
                    return 3;

                return 4;
            }
            case RACE_ORC:
            case RACE_UNDEAD_PLAYER:
            case RACE_TAUREN:
            case RACE_TROLL:
            {
                if (Gender == GENDER_MALE)
                    return 3;

                return 3;
            }
        }
    }
    else if (emote == 323)
    {
        return 2;
    }
    else if (emote == 324)
    {
        switch (Race)
        {
            case RACE_HUMAN:
            case RACE_NIGHTELF:
            case RACE_ORC:
            case RACE_UNDEAD_PLAYER:
            case RACE_TAUREN:
            case RACE_TROLL:
            {
                if (Gender == GENDER_MALE)
                    return 2;

                return 2;
            }
            case RACE_DWARF:
            {
                if (Gender == GENDER_MALE)
                    return 3;

                return 2;
            }
            case RACE_GNOME:
            {
                if (Gender == GENDER_MALE)
                    return 2;

                return 1;
            }
        }
    }
    else if (emote == 325)
    {
        switch (Race)
        {
            case RACE_HUMAN:
            case RACE_TROLL:
            {
                if (Gender == GENDER_MALE)
                    return 3;

                return 3;
            }
            case RACE_DWARF:
            case RACE_TAUREN:
            {
                if (Gender == GENDER_MALE)
                    return 3;

                return 2;
            }
            case RACE_NIGHTELF:
            case RACE_GNOME:
            case RACE_ORC:
            case RACE_UNDEAD_PLAYER:
            {
                if (Gender == GENDER_MALE)
                    return 2;

                return 2;
            }
        }
    }
    else if (emote == 326)
    {
        switch (Race)
        {
            case RACE_HUMAN:
            case RACE_NIGHTELF:
            case RACE_GNOME:
            case RACE_ORC:
            case RACE_UNDEAD_PLAYER:
            case RACE_TAUREN:
            {
                if (Gender == GENDER_MALE)
                    return 2;

                return 2;
            }
            case RACE_DWARF:
            case RACE_TROLL:
            {
                if (Gender == GENDER_MALE)
                    return 3;

                return 3;
            }
        }
    }
    else if (emote == TEXT_EMOTE_CHEER)
    {
        switch (Race)
        {
            case RACE_HUMAN:
            case RACE_NIGHTELF:
            case RACE_GNOME:
            case RACE_ORC:
            case RACE_UNDEAD_PLAYER:
            case RACE_TAUREN:
            {
                if (Gender == GENDER_MALE)
                    return 2;

                return 2;
            }
            case RACE_DWARF:
            case RACE_TROLL:
            {
                if (Gender == GENDER_MALE)
                    return 3;

                return 2;
            }
        }
    }
    else if (emote == TEXT_EMOTE_OPENFIRE)
    {
        switch (Race)
        {
            case RACE_HUMAN:
            case RACE_DWARF:
            case RACE_NIGHTELF:
            case RACE_UNDEAD_PLAYER:
            case RACE_TAUREN:
            case RACE_TROLL:
            case RACE_GNOME:
            {
                return 2;
            }
            case RACE_ORC:
            {
                if (Gender == GENDER_MALE)
                    return 2;

                return 3;
            }
        }
    }
    else if (emote == TEXT_EMOTE_BYE)
    {
        switch (Race)
        {
            case RACE_HUMAN:
            case RACE_DWARF:
            case RACE_NIGHTELF:
            case RACE_ORC:
            case RACE_UNDEAD_PLAYER:
            case RACE_TAUREN:
            case RACE_TROLL:
            {
                if (Gender == GENDER_MALE)
                    return 3;

                return 3;
            }
            case RACE_GNOME:
            {
                if (Gender == GENDER_MALE)
                    return 4;

                return 4;
            }
        }
    }
    else if (emote == TEXT_EMOTE_NOD)
    {
        switch (Race)
        {
            case RACE_HUMAN:
            case RACE_NIGHTELF:
            case RACE_GNOME:
            case RACE_UNDEAD_PLAYER:
            case RACE_TAUREN:
            {
                if (Gender == GENDER_MALE)
                    return 3;

                return 3;
            }
            case RACE_DWARF:
            case RACE_TROLL:
            {
                if (Gender == GENDER_MALE)
                    return 4;

                return 3;
            }
            case RACE_ORC:
            {
                if (Gender == GENDER_MALE)
                    return 4;

                return 4;
            }
        }
    }
    else if (emote == TEXT_EMOTE_NO)
    {
        switch (Race)
        {
            case RACE_HUMAN:
            case RACE_DWARF:
            case RACE_TROLL:
            {
                if (Gender == GENDER_MALE)
                    return 4;

                return 3;
            }
            case RACE_NIGHTELF:
            case RACE_GNOME:
            case RACE_ORC:
            case RACE_UNDEAD_PLAYER:
            case RACE_TAUREN:
            {
                if (Gender == GENDER_MALE)
                    return 3;

                return 3;
            }
        }
    }
    else if (emote == TEXT_EMOTE_THANK)
    {
        switch (Race)
        {
            case RACE_HUMAN:
            case RACE_NIGHTELF:
            case RACE_GNOME:
            case RACE_ORC:
            case RACE_UNDEAD_PLAYER:
            case RACE_TROLL:
            {
                if (Gender == GENDER_MALE)
                    return 3;

                return 3;
            }
            case RACE_DWARF:
            {
                if (Gender == GENDER_MALE)
                    return 4;

                return 4;
            }
            case RACE_TAUREN:
            {
                if (Gender == GENDER_MALE)
                    return 4;

                return 3;
            }
        }
    }
    else if (emote == TEXT_EMOTE_WELCOME)
    {
        switch (Race)
        {
            case RACE_HUMAN:
            case RACE_DWARF:
            case RACE_NIGHTELF:
            case RACE_GNOME:
            case RACE_ORC:
            case RACE_TAUREN:
            case RACE_TROLL:
            {
                if (Gender == GENDER_MALE)
                    return 3;
                return 3;
            }
            case RACE_UNDEAD_PLAYER:
            {
                if (Gender == GENDER_MALE)
                    return 2;
                return 3;
            }
        }
    }
    else if (emote == TEXT_EMOTE_CONGRATULATE)
    {
        switch (Race)
        {
            case RACE_HUMAN:
            case RACE_NIGHTELF:
            case RACE_ORC:
            case RACE_TAUREN:
            case RACE_TROLL:
            {
                if (Gender == GENDER_MALE)
                    return 3;

                return 3;
            }
            case RACE_DWARF:
            {
                if (Gender == GENDER_MALE)
                    return 5;

                return 4;
            }
            case RACE_GNOME:
            case RACE_UNDEAD_PLAYER:
            {
                if (Gender == GENDER_MALE)
                    return 3;

                return 4;
            }
        }
    }
    else if (emote == TEXT_EMOTE_FLIRT)
    {
        switch (Race)
        {
            case RACE_HUMAN:
            {
                if (Gender == GENDER_MALE)
                    return 6;
                return 3;
            }
            case RACE_DWARF:
            case RACE_TAUREN:
            {
                if (Gender == GENDER_MALE)
                    return 6;

                return 5;
            }
            case RACE_NIGHTELF:
            {
                if (Gender == GENDER_MALE)
                    return 5;

                return 4;
            }
            case RACE_GNOME:
            case RACE_TROLL:
            {
                if (Gender == GENDER_MALE)
                    return 4;

                return 5;
            }
            case RACE_ORC:
            case RACE_UNDEAD_PLAYER:
            {
                if (Gender == GENDER_MALE)
                    return 6;

                return 6;
            }
        }
    }
    else if (emote == TEXT_EMOTE_JOKE)
    {
        switch (Race)
        {
            case RACE_HUMAN:
            {
                if (Gender == GENDER_MALE)
                    return 5;

                return 6;
            }
            case RACE_DWARF:
            {
                if (Gender == GENDER_MALE)
                    return 6;

                return 5;
            }
            case RACE_NIGHTELF:
            {
                if (Gender == GENDER_MALE)
                    return 7;

                return 4;
            }
            case RACE_GNOME:
            {
                if (Gender == GENDER_MALE)
                    return 5;

                return 3;
            }
            case RACE_ORC:
            {
                if (Gender == GENDER_MALE)
                    return 5;

                return 5;
            }
            case RACE_TAUREN:
            {
                if (Gender == GENDER_MALE)
                    return 4;

                return 3;
            }
            case RACE_TROLL:
            {
                if (Gender == GENDER_MALE)
                    return 5;

                return 4;
            }
            case RACE_UNDEAD_PLAYER:
            {
                if (Gender == GENDER_MALE)
                    return 4;

                return 7;
            }
        }
    }

    return 1;
}
