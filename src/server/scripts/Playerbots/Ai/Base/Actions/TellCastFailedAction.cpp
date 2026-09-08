/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "TellCastFailedAction.h"

#include "ChatHelper.h"
#include "Event.h"
#include "Playerbots.h"

bool TellCastFailedAction::Execute(Event event)
{
    WorldPacket p(event.getPacket());
    //By leewheel 2026-08-04: 修复——TC的SMSG_CAST_FAILED(CastFailed包)格式与AC完全不同
    //AC格式: uint8 castCount + uint32 spellId + uint8 result = 6字节
    //TC格式: ObjectGuid CastID(packed) + int32 SpellID + SpellCastVisual(8) + int32 Reason + int32 FailedArg1 + int32 FailedArg2
    //原代码按AC读castCount(1)+spellId(4)+result(1),在TC下castCount读到CastID的lowMask字节,
    //spellId读到CastID中间字节(垃圾值),result读到SpellID末尾字节→法术失败提示完全错乱
    //修复: 按TC格式读CastID(ObjectGuid)→SpellID(int32)→跳过Visual(8)→Reason(int32)
    //包体最小: CastID(2,全0极端)+SpellID(4)+Visual(8)+Reason(4)=18字节
    if (p.wpos() < p.rpos() || (p.wpos() - p.rpos()) < 18)
        return false;
    //End By leewheel

    ObjectGuid castId;
    int32 spellId;
    p >> castId >> spellId;

    // 跳过 SpellCastVisual(SpellXSpellVisualID(4) + MissReason(4) = 8字节)
    int32 spellXSpellVisualId, missReason;
    p >> spellXSpellVisualId >> missReason;

    int32 result;
    p >> result;
    //End By leewheel

    botAI->SpellInterrupted(spellId);

    if (result == SPELL_CAST_OK)
        return false;

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);

    std::ostringstream out;
    out << chat->FormatSpell(spellInfo) << ": ";
    switch (result)
    {
        //By leewheel 2026-08-01: 玩家可见文本中文化
        case SPELL_FAILED_NOT_READY:
            out << "未就绪";
            break;
        case SPELL_FAILED_REQUIRES_SPELL_FOCUS:
            out << "需要法术聚焦";
            break;
        case SPELL_FAILED_REQUIRES_AREA:
            out << "不能在此处施放";
            break;
        case SPELL_FAILED_EQUIPPED_ITEM_CLASS:
            out << "需要物品";
            break;
        case SPELL_FAILED_EQUIPPED_ITEM_CLASS_MAINHAND:
        case SPELL_FAILED_EQUIPPED_ITEM_CLASS_OFFHAND:
            out << "需要武器";
            break;
        case SPELL_FAILED_PREVENTED_BY_MECHANIC:
            out << "被打断";
            break;
        default:
            out << "无法施放";
        //End By leewheel
    }

    if (spellInfo->CalcCastTime() >= 2000)
        botAI->TellError(out.str());

    return true;
}

bool TellSpellAction::Execute(Event event)
{
    std::string const spell = event.getParam();
    uint32 spellId = AI_VALUE2(uint32, "spell id", spell);
    if (!spellId)
        return false;

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);
    if (!spellInfo)
        return false;

    std::ostringstream out;
    out << chat->FormatSpell(spellInfo);
    botAI->TellError(out.str());
    return true;
}
