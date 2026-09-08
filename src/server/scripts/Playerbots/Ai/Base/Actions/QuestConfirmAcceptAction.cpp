#include "QuestConfirmAcceptAction.h"

#include "QuestPackets.h"
#include "WorldPacket.h"

bool QuestConfirmAcceptAction::Execute(Event event)
{
    WorldPacket packet(event.getPacket());
    uint32 questId;
    //By leewheel 2026-08-04: 修复rpos(0)+长度检查
    if (packet.wpos() < packet.rpos() || (packet.wpos() - packet.rpos()) < 4)
        return false;
    //End By leewheel
    packet >> questId;

    WorldPacket sendPacket(CMSG_QUEST_CONFIRM_ACCEPT);
    sendPacket << questId;
    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest || !bot->CanAddQuest(quest, true))
    {
        return false;
    }
    std::ostringstream out;
    //By leewheel 2026-08-01: 玩家可见文本中文化
    out << "任务: " << chat->FormatQuest(quest) << " 确认接受";
    //End By leewheel
    botAI->TellMaster(out);
    // By leewheel 2026-07-09: 修复类型名称，TC中使用QuestConfirmAccept而非QuestConfirmAcceptClient
    WorldPackets::Quest::QuestConfirmAccept confirmAccept(std::move(sendPacket));
    confirmAccept.Read();
    bot->GetSession()->HandleQuestConfirmAccept(confirmAccept);
    // End By leewheel
    return true;
}
