/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "Playerbots.h"

#include "CheckMailAction.h"
#include "Event.h"
#include "GuildTaskMgr.h"
#include "PlayerbotAIConfig.h"
#include "Mail.h"
#include "MailPackets.h"
#include "Opcodes.h"

bool CheckMailAction::Execute(Event /*event*/)
{
    // By leewheel 2026-07-09: 修复最令人头疼的解析，使用花括号初始化
    WorldPackets::Mail::MailQueryNextMailTime queryNextMailTime{WorldPacket(CMSG_QUERY_NEXT_MAIL_TIME, 0)};
    queryNextMailTime.Read();
    bot->GetSession()->HandleQueryNextMailTime(queryNextMailTime);
    // End By leewheel

    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();

    std::vector<uint32> ids;
    for (PlayerMails::const_iterator i = bot->GetMails().begin(); i != bot->GetMails().end(); ++i)
    {
        Mail* mail = *i;
        if (!mail || mail->state == MAIL_STATE_DELETED)
            continue;

        Player* owner = ObjectAccessor::FindConnectedPlayer(ObjectGuid::Create<HighGuid::Player>(mail->sender));
        if (!owner)
            continue;

        uint32 account = owner->GetSession()->GetAccountId();
        if (PlayerbotAIConfig::instance().IsInRandomAccountList(account))
            continue;

        ProcessMail(mail, owner, trans);
        ids.push_back(mail->messageID);
        mail->state = MAIL_STATE_DELETED;
    }

    for (uint32 id : ids)
    {
        bot->SendMailResult(static_cast<uint64>(id), MAIL_DELETED, MAIL_OK);

        CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_MAIL_BY_ID);
        stmt->SetData(0, id);
        trans->Append(stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_MAIL_ITEM_BY_ID);
        stmt->SetData(0, id);
        trans->Append(stmt);

        bot->RemoveMail(id);
    }

    CharacterDatabase.CommitTransaction(trans);

    return true;
}

bool CheckMailAction::isUseful()
{
    if (botAI->GetMaster() || !bot->GetMailSize() || bot->InBattleground())
        return false;

    return true;
}

void CheckMailAction::ProcessMail(Mail* mail, Player* owner, CharacterDatabaseTransaction trans)
{
    if (mail->items.empty())
    {
        return;
    }

    if (mail->subject.find("你请求的物品") != std::string::npos)
        return;

    for (MailItemInfoVec::iterator i = mail->items.begin(); i != mail->items.end(); ++i)
    {
        Item* item = bot->GetMItem(i->item_guid);
        if (!item)
            continue;

        if (!GuildTaskMgr::instance().CheckItemTask(i->item_template, item->GetCount(), owner, bot, true))
        {
            std::ostringstream body;
            body << "你好，" << owner->GetName() << "，\n";
            body << "\n";
            body << "这是你误发给我的物品";
            body << "\n";
            body << "谢谢，\n";
            body << bot->GetName() << "\n";

            MailDraft draft("你误发给我的物品", body.str());
            draft.AddItem(item);
            bot->RemoveMItem(i->item_guid);
            draft.SendMailTo(trans, MailReceiver(owner), MailSender(bot));
            return;
        }

        bot->RemoveMItem(i->item_guid);
        item->DestroyForPlayer(bot);
    }
}
