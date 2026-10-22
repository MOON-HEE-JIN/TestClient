#include "CPacketChatProc.h"

#include "CClient.h"
#include "CUtill//CPacket.h"

int CPacketChatProc::DO_CHAT_GROUP(CClient* pTarget, CPacket& pReqPacket)
{
    return 0;
}

int CPacketChatProc::DO_CHAT_NORMAL(CClient* pTarget, CPacket& pReqPacket)
{
    return 0;
}

int CPacketChatProc::DO_CHAT_WHISPER(CClient* pTarget, CPacket& pReqPacket)
{
    return 0;
}

int CPacketChatProc::DO_ERROR_PACKET(CClient* pTarget, CPacket& pReqPacket)
{
    return 0;
}

int CPacketChatProc::DO_ERROR_RESULT(CClient* pTarget, int ret, int type)
{
    return 0;
}
