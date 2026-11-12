#pragma once

#include "Stub/ChatClient.ChatServerToClientStub.g.h"
#include "Stub/ChatClient.ChatServerToClientStub.g.cpp"

class CClient;
class CPacket;

using namespace Rpc::ChatClient;

class CPacketChatProc : public ChatServerToClientStub<CClient, CPacket>
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

	// ChatServerToClientStub을(를) 통해 상속됨
	int DO_CHAT_CHATAUTH(CClient* pTarget, CPacket& pReqPacket) override;
};