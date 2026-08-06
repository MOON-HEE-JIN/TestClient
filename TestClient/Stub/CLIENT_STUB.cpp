#include "PacketEnumDef.h"
#include "CLIENT_STUB.h"
template<typename Object, typename Packet>
void Stub<Object,Packet>::InitRegisterFuncPointer()
{
	m_mapGAMEProc[GAME::LOOPBACK] = std::bind(&Stub::DO_GAME_LOOPBACK, this, std::placeholders::_1, std::placeholders::_2);
	m_mapGAMEProc[GAME::CONNECTINFO] = std::bind(&Stub::DO_GAME_CONNECTINFO, this, std::placeholders::_1, std::placeholders::_2);
	m_mapGAMEProc[GAME::CHANGEZONE] = std::bind(&Stub::DO_GAME_CHANGEZONE, this, std::placeholders::_1, std::placeholders::_2);
	m_mapGAMEProc[GAME::AOI_IN_PLAYER] = std::bind(&Stub::DO_GAME_AOI_IN_PLAYER, this, std::placeholders::_1, std::placeholders::_2);
	m_mapGAMEProc[GAME::AOI_IN_PLAYERS] = std::bind(&Stub::DO_GAME_AOI_IN_PLAYERS, this, std::placeholders::_1, std::placeholders::_2);
	m_mapGAMEProc[GAME::AOI_OUT_PLAYER] = std::bind(&Stub::DO_GAME_AOI_OUT_PLAYER, this, std::placeholders::_1, std::placeholders::_2);
	m_mapGAMEProc[GAME::AOI_OUT_PLAYERS] = std::bind(&Stub::DO_GAME_AOI_OUT_PLAYERS, this, std::placeholders::_1, std::placeholders::_2);
	m_mapGAMEProc[GAME::MOVESTART] = std::bind(&Stub::DO_GAME_MOVESTART, this, std::placeholders::_1, std::placeholders::_2);
	m_mapGAMEProc[GAME::MOVESTOP] = std::bind(&Stub::DO_GAME_MOVESTOP, this, std::placeholders::_1, std::placeholders::_2);
	m_mapGAMEProc[GAME::CHANGEINGZONE] = std::bind(&Stub::DO_GAME_CHANGEINGZONE, this, std::placeholders::_1, std::placeholders::_2);
	m_mapGAMEProc[GAME::TELEPORT] = std::bind(&Stub::DO_GAME_TELEPORT, this, std::placeholders::_1, std::placeholders::_2);
	m_mapGAMEProc[GAME::OTHERMOVESTART] = std::bind(&Stub::DO_GAME_OTHERMOVESTART, this, std::placeholders::_1, std::placeholders::_2);
	m_mapOBSERVERProc[OBSERVER::CONNET_OBSERVER] = std::bind(&Stub::DO_OBSERVER_CONNET_OBSERVER, this, std::placeholders::_1, std::placeholders::_2);
}
template<typename Object, typename Packet>
void Stub<Object,Packet>::DO_GAME_Proc(int type, Object* pTarget, Packet& cPacket)
{
	int ret = 0;
	if(m_mapGAMEProc.find(type) == m_mapGAMEProc.end())
	{
		ret = DO_ERROR_PACKET(pTarget, cPacket);
		return;
	}
	ret = m_mapGAMEProc[type](pTarget, cPacket);
	if(ret != 0)		DO_ERROR_RESULT(pTarget, ret, type);
}
template<typename Object, typename Packet>
void Stub<Object,Packet>::DO_OBSERVER_Proc(int type, Object* pTarget, Packet& cPacket)
{
	int ret = 0;
	if(m_mapOBSERVERProc.find(type) == m_mapOBSERVERProc.end())
	{
		ret = DO_ERROR_PACKET(pTarget, cPacket);
		return;
	}
	ret = m_mapOBSERVERProc[type](pTarget, cPacket);
	if(ret != 0)		DO_ERROR_RESULT(pTarget, ret, type);
}
