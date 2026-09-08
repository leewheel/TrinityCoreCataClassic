#include "TradeStatusExtendedAction.h"
#include "Event.h"
#include "Item.h"
#include "Player.h"
#include "PlayerbotAI.h"
#include "TradeData.h"

bool TradeStatusExtendedAction::Execute(Event /*event*/)
{
    //By leewheel 2026-07-26: 重新实现扩展交易处理(此前因TC/AC包格式不同被stub)。
    //关键点：Playerbots跑在服务器端，交易数据直接存于TradeData对象，无需解析客户端封包。
    //AC原代码解析SMSG_TRADE_STATUS_EXTENDED字节流的唯一目的，就是拿到"不可交易槽"锁箱的锁信息，
    //随后它同样是从TradeData取回物品判定IsLocked——因此绕过封包、直接读TradeData完全等价且无越界崩溃风险。
    Player* trader = bot->GetTrader();
    if (!trader)
        return false;

    TradeData* tradeData = trader->GetTradeData();
    if (!tradeData)
        return false;

    // 检查对方放入"不可交易槽(TRADE_SLOT_NONTRADED)"的物品是否为需要开锁的锁箱
    Item* lockbox = tradeData->GetItem(TRADE_SLOT_NONTRADED);
    if (!lockbox)
        return false;

    ItemTemplate const* proto = lockbox->GetTemplate();
    if (!proto || proto->GetLockID() == 0 || !lockbox->IsLocked())
        return false;

    constexpr uint32 SPELL_PICK_LOCK = 1804;
    if (bot->getClass() == CLASS_ROGUE && bot->HasSpell(SPELL_PICK_LOCK))
    {
        // 交由"unlock traded item"动作执行开锁(其内部会再次校验职业/开锁技能与锁等级)。
        //By leewheel 2026-08-15: 补开锁后4秒延迟(对齐the-lab)——开锁是读条/施法，若对方立刻
        //点接受，交易可能在开锁完成前成交(锁箱以锁定状态交易)。延迟接受给开锁留出时间
        botAI->DoSpecificAction("unlock traded item");
        botAI->SetNextCheckDelay(4000);
        //End By leewheel
    }
    else
    {
        botAI->TellMaster("我无法解锁这个物品。");
    }

    return true;
    //End By leewheel
}
