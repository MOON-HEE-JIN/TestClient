#include "ChatClient.PacketEnumDef.g.h"
#include "ChatClient.ClientToChatServerStub.g.h"

namespace Rpc::ChatClient
{

template<typename Object, typename Packet>
void ClientToChatServerStub<Object,Packet>::InitRegisterFuncPointer()
{
	m_mapCHATProc[CHAT::NORMAL] = std::bind(&ClientToChatServerStub::DO_CHAT_NORMAL, this, std::placeholders::_1, std::placeholders::_2);
	m_mapCHATProc[CHAT::WHISPER] = std::bind(&ClientToChatServerStub::DO_CHAT_WHISPER, this, std::placeholders::_1, std::placeholders::_2);
	m_mapCHATProc[CHAT::GROUP] = std::bind(&ClientToChatServerStub::DO_CHAT_GROUP, this, std::placeholders::_1, std::placeholders::_2);
}
template<typename Object, typename Packet>
void ClientToChatServerStub<Object,Packet>::DO_CHAT_Proc(int type, Object* pTarget, Packet& cPacket)
{
	int ret = 0;
	if(m_mapCHATProc.find(type) == m_mapCHATProc.end())
	{
		ret = DO_ERROR_PACKET(pTarget, cPacket);
		return;
	}
	ret = m_mapCHATProc[type](pTarget, cPacket);
	if(ret != 0)		DO_ERROR_RESULT(pTarget, ret, type);
}

} // namespace Rpc::ChatClient
