/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "MailAction.h"

#include "ChatHelper.h"
#include "Event.h"
#include "Mail.h"
#include "Playerbots.h"

std::map<std::string, MailProcessor*> MailAction::processors;

class TellMailProcessor : public MailProcessor
{
public:
    bool Before(PlayerbotAI* botAI) override
    {
        //By leewheel 2026-08-01: 玩家可见文本中文化
        botAI->TellMaster("=== 邮箱 ===");
        //End By leewheel
        tells.clear();
        return true;
    }

    bool Process(uint32 index, Mail* mail, PlayerbotAI* botAI) override
    {
        Player* bot = botAI->GetBot();
        time_t cur_time = time(nullptr);
        uint32 days = (cur_time - mail->deliver_time) / 3600 / 24;

        std::ostringstream out;
        out << "#" << (index + 1) << " ";
        if (!mail->money && !mail->HasItems())
            out << "|cffffffff" << mail->subject;

        if (mail->money)
        {
            out << "|cffffff00" << ChatHelper::formatMoney(mail->money);
            if (!mail->subject.empty())
                out << " |cffa0a0a0(" << mail->subject << ")";
        }

        if (mail->HasItems())
        {
            for (MailItemInfoVec::iterator i = mail->items.begin(); i != mail->items.end(); ++i)
            {
                Item* item = bot->GetMItem(i->item_guid);
                uint32 count = item ? item->GetCount() : 1;

                if (ItemTemplate const* proto = sObjectMgr->GetItemTemplate(i->item_template))
                {
                    out << ChatHelper::FormatItem(proto, count);
                    if (!mail->subject.empty())
                        out << " |cffa0a0a0(" << mail->subject << ")";
                }
            }
        }

        //By leewheel 2026-08-01: 玩家可见文本中文化
        out << ", |cff00ff00" << days << " 天前";
        //End By leewheel
        tells.push_front(out.str());
        return true;
    }

    bool After(PlayerbotAI* botAI) override
    {
        for (std::list<std::string>::iterator i = tells.begin(); i != tells.end(); ++i)
            botAI->TellMaster(*i);

        return true;
    }

    static TellMailProcessor instance;

private:
    std::list<std::string> tells;
};

class TakeMailProcessor : public MailProcessor
{
public:
    bool Process(uint32 /*index*/, Mail* mail, PlayerbotAI* botAI) override
    {
        Player* bot = botAI->GetBot();
        if (!CheckBagSpace(bot))
        {
            //By leewheel 2026-08-01: 玩家可见文本中文化
            botAI->TellError("背包空间不足");
            //End By leewheel
            return false;
        }

        ObjectGuid mailbox = FindMailbox(botAI);
        if (mail->money)
        {
            std::ostringstream out;
            //By leewheel 2026-08-01: 玩家可见文本中文化
            out << mail->subject << ", |cffffff00" << ChatHelper::formatMoney(mail->money) << "|cff00ff00 已处理";
            //End By leewheel
            botAI->TellMaster(out.str());

            //By leewheel 2026-08-15: 修复——原扁平包经HandleMailTakeMoney按uint64错位读取(mailID错位+12字节包读16字节越界)，
            //邮件取钱永远失败。改用typed MailTakeMoney直接填Mailbox/MailID/Money
            WPPCompat::MailTakeMoney(bot->GetSession(), mailbox, mail->messageID, mail->money);
            //End By leewheel
            RemoveMail(bot, mail->messageID, mailbox);
        }
        else if (!mail->items.empty())
        {
            std::vector<uint32> guids;
            for (MailItemInfoVec::iterator i = mail->items.begin(); i != mail->items.end(); ++i)
                if (sObjectMgr->GetItemTemplate(i->item_template))
                    guids.push_back(i->item_guid);

            for (std::vector<uint32>::iterator i = guids.begin(); i != guids.end(); ++i)
            {
                //By leewheel 2026-08-15: 修复——原扁平包经HandleMailTakeItem只读mailbox+mailID(attachID恒0)，
                //TC的find_if(item_guid==0)永不匹配导致取物品永远失败。改用typed MailTakeItem直接填Mailbox/MailID/AttachID
                WPPCompat::MailTakeItem(bot->GetSession(), mailbox, mail->messageID, *i);
                //End By leewheel

                Item* item = bot->GetMItem(*i);

                std::ostringstream out;
                //By leewheel 2026-08-01: 玩家可见文本中文化
                out << mail->subject << ", " << ChatHelper::FormatItem(item->GetTemplate()) << "|cff00ff00 已处理";
                //End By leewheel

                // By leewheel 2026-07-09: 已在上面的WPPCompat调用中处理，移除重复调用
                botAI->TellMaster(out.str());
            }

            RemoveMail(bot, mail->messageID, mailbox);
        }

        return true;
    }

    static TakeMailProcessor instance;

private:
    bool CheckBagSpace(Player* bot)
    {
        uint32 totalused = 0;
        for (uint8 slot = INVENTORY_SLOT_ITEM_START; slot < INVENTORY_SLOT_ITEM_END; slot++)
            if (bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
                ++totalused;

        uint32 totalfree = 16 - totalused;
        for (uint8 bag = INVENTORY_SLOT_BAG_START; bag < INVENTORY_SLOT_BAG_END; ++bag)
        {
            if (Bag const* pBag = (Bag*)bot->GetItemByPos(INVENTORY_SLOT_BAG_0, bag))
            {
                ItemTemplate const* pBagProto = pBag->GetTemplate();
                //By leewheel 2025-01-16
                // TC中使用GetClass()方法而不是直接访问Class成员
                if (pBagProto->GetClass() == ITEM_CLASS_CONTAINER && pBagProto->GetSubClass() == ITEM_SUBCLASS_CONTAINER)
                    totalfree += pBag->GetFreeSlots();
                //End By leewheel 2025-01-16
            }
        }

        return totalfree >= 2;
    }
};

class DeleteMailProcessor : public MailProcessor
{
public:
    bool Process(uint32 /*index*/, Mail* mail, PlayerbotAI* botAI) override
    {
        std::ostringstream out;
        //By leewheel 2026-08-01: 玩家可见文本中文化
        out << "|cffffffff" << mail->subject << "|cffff0000 已删除";
        //End By leewheel
        RemoveMail(botAI->GetBot(), mail->messageID, FindMailbox(botAI));
        botAI->TellMaster(out.str());
        return true;
    }

    static DeleteMailProcessor instance;
};

class ReadMailProcessor : public MailProcessor
{
public:
    bool Process(uint32 /*index*/, Mail* mail, PlayerbotAI* botAI) override
    {
        std::ostringstream out, body;
        out << "|cffffffff" << mail->subject;
        botAI->TellMaster(out.str());

        return true;
    }

    static ReadMailProcessor instance;
};

TellMailProcessor TellMailProcessor::instance;
TakeMailProcessor TakeMailProcessor::instance;
DeleteMailProcessor DeleteMailProcessor::instance;
ReadMailProcessor ReadMailProcessor::instance;

std::map<uint32, Mail*> filterList(std::vector<Mail*> src, std::string const filter)
{
    std::map<uint32, Mail*> result;

    if (src.empty())
        return result;
    if (filter.empty() || filter == "*")
    {
        uint32 idx = 0;
        for (std::vector<Mail*>::iterator i = src.begin(); i != src.end(); ++i)
            result[idx++] = *i;

        return result;
    }

    if (filter.find("-") != std::string::npos)
    {
        std::vector<std::string> ss = split(filter, '-');
        int32 from = 0;
        int32 to = static_cast<int32>(src.size()) - 1;

        if (!ss[0].empty())
            from = atoi(ss[0].c_str()) - 1;

        if (ss.size() > 1 && !ss[1].empty())
            to = atoi(ss[1].c_str()) - 1;

        if (from < 0)
            from = 0;

        if (from > static_cast<int32>(src.size()) - 1)
            from = static_cast<int32>(src.size()) - 1;

        if (to < 0)
            to = 0;

        if (to > static_cast<int32>(src.size()) - 1)
            to = static_cast<int32>(src.size()) - 1;

        for (int32 i = from; i <= to; ++i)
            result[static_cast<uint32>(i)] = src[i];

        return result;
    }

    std::vector<std::string> ss = split(filter, ',');
    for (std::vector<std::string>::iterator i = ss.begin(); i != ss.end(); ++i)
    {
        int32 idx = atoi(i->c_str()) - 1;

        if (idx < 0)
            idx = 0;

        if (idx > static_cast<int32>(src.size()) - 1)
            idx = static_cast<int32>(src.size()) - 1;

        result[static_cast<uint32>(idx)] = src[idx];
    }

    return result;
}

bool MailAction::Execute(Event event)
{
    Player* master = GetMaster();
    if (!master)
        return false;

    if (!MailProcessor::FindMailbox(botAI))
    {
        //By leewheel 2026-08-01: 玩家可见文本中文化
        botAI->TellError("附近没有邮箱");
        //End By leewheel
        return false;
    }

    if (processors.empty())
    {
        processors["?"] = &TellMailProcessor::instance;
        processors["take"] = &TakeMailProcessor::instance;
        processors["delete"] = &DeleteMailProcessor::instance;
        processors["read"] = &ReadMailProcessor::instance;
    }

    std::string const text = event.getParam();
    if (text.empty())
    {
        //By leewheel 2026-08-01: 玩家可见文本中文化
        botAI->TellMaster(
            "密语 'mail ?' 查询邮箱，'mail take/delete/read 过滤器' 按过滤器领取/删除/阅读邮件");
        //End By leewheel
        return false;
    }

    std::vector<std::string> ss = split(text, ' ');
    std::string const action = ss[0];
    std::string const filter = ss.size() > 1 ? ss[1] : "";

    MailProcessor* processor = processors[action];
    if (!processor)
    {
        std::ostringstream out;
        //By leewheel 2026-08-01: 玩家可见文本中文化
        out << action << ": 我不知道该怎么做";
        //End By leewheel
        botAI->TellMaster(out.str());
        return false;
    }

    if (!processor->Before(botAI))
        return false;

    std::vector<Mail*> mailList;
    time_t cur_time = time(nullptr);
    for (PlayerMails::const_iterator itr = bot->GetMails().begin(); itr != bot->GetMails().end(); ++itr)
    {
        if ((*itr)->state == MAIL_STATE_DELETED || cur_time < (*itr)->deliver_time)
            continue;

        Mail* mail = *itr;
        mailList.push_back(mail);
    }

    std::map<uint32, Mail*> filtered = filterList(mailList, filter);
    for (std::map<uint32, Mail*>::iterator i = filtered.begin(); i != filtered.end(); ++i)
    {
        if (!processor->Process(i->first, i->second, botAI))
            break;
    }

    return processor->After(botAI);
}

//By leewheel 2026-09-03 修复C4100警告：TC用typed MailDelete只传mailID，mailbox参数不再使用，显式省略参数名
void MailProcessor::RemoveMail(Player* bot, uint32 id, ObjectGuid /*mailbox*/)
//End By leewheel
{
    //By leewheel 2026-08-15: 修复——原扁平包[mailbox][id][0]经HandleMailDelete从流首读uint64把mailbox GUID当mailID，
    //TC的GetMail(垃圾mailID)返回null不删除(还无条件回MAIL_OK让bot误以为已删除)，邮件永远删不掉。
    //改用typed MailDelete直接传真实mailID
    WPPCompat::MailDelete(bot->GetSession(), id);
    //End By leewheel
}

ObjectGuid MailProcessor::FindMailbox(PlayerbotAI* botAI)
{
    GuidVector gos = *botAI->GetAiObjectContext()->GetValue<GuidVector>("nearest game objects");
    ObjectGuid mailbox;
    for (ObjectGuid const guid : gos)
    {
        if (GameObject* go = botAI->GetGameObject(guid))
            if (go->GetGoType() == GAMEOBJECT_TYPE_MAILBOX)
            {
                mailbox = go->GetGUID();
                break;
            }
    }

    return mailbox;
}
