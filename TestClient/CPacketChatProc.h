#pragma once
#include "Stub/ChatClient.ClientToChatServerStub.g.h"
#include "Stub/ChatClient.ClientToChatServerStub.g.cpp"

class CClient;
class CPacket;

using namespace Rpc::ChatClient;

class CPacketChatProc : public ClientToChatServerStub<CClient, CPacket>
{
	public:
		CPacketChatProc() {}
private:
	// ClientToChatServerStub을(를) 통해 상속됨
	int DO_CHAT_GROUP(CClient* pTarget, CPacket& pReqPacket) override;
	int DO_CHAT_NORMAL(CClient* pTarget, CPacket& pReqPacket) override;
	int DO_CHAT_WHISPER(CClient* pTarget, CPacket& pReqPacket) override;
	int DO_ERROR_PACKET(CClient* pTarget, CPacket& pReqPacket) override;
	int DO_ERROR_RESULT(CClient* pTarget, int ret, int type) override;
};