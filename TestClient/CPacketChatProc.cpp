#include "CPacketChatProc.h"

#include "CClient.h"
#include "CUtill//CPacket.h"

#include "Stub/ChatClient.StructDef.g.h"

template<typename T>
void DeserializeChatPayload(T& value, CPacket& packet)
{
    int ret = Rpc::ChatClient::UnSerialization(packet.GetReadBuffPtr(), value);
}

int CPacketChatProc::DO_CHAT_GROUP(CClient* pTarget, CPacket& pReqPacket)
{
    return 0;
}

int CPacketChatProc::DO_CHAT_NORMAL(CClient* pTarget, CPacket& pReqPacket)
{
    st_STC_NORMAL res;
    DeserializeChatPayload(res, pReqPacket);

    printf("loop back %s\n", res.msg.comment.c_str());

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
