#include "CPacketProc.h"
#include "Stub/EnumDef.h"


int CPacketProc::DO_GAME_LOOPBACK(CClient* pTarget, CPacket& pReqPacket)
{
	
	return 0;
}

int CPacketProc::DO_ERROR_PACKET(CClient* pTarget, CPacket& pReqPacket)
{
	return 0;
}
int CPacketProc::DO_ERROR_RESULT(CClient* pTarget, int ret, int type)
{

	return 0;
}

int CPacketProc::DO_GAME_CHANGEZONE(CClient* pTarget, CPacket& pReqPacket)
{
	
	return 0;
}

int CPacketProc::DO_GAME_CREATECHAR(CClient* pTarget, CPacket& pReqPacket)
{
	return 0;
}

int CPacketProc::DO_GAME_LEAVEZONE(CClient* pTarget, CPacket& pReqPacket)
{
	return 0;
}

int CPacketProc::DO_GAME_CONNECTINFO(CClient* pTarget, CPacket& pReqPacket)
{
	st_STC_ConnectInfo res;
	pReqPacket >> res;

	printf("ID : %d\n", res.info.ID);
	return 0;
}

int CPacketProc::DO_GAME_ENTERZONE(CClient* pTarget, CPacket& pReqPacket)
{
	return 0;
}

int CPacketProc::DO_GAME_MOVESTART(CClient* pTarget, CPacket& pReqPacket)
{
	return 0;
}

int CPacketProc::DO_GAME_MOVESTOP(CClient* pTarget, CPacket& pReqPacket)
{
	return 0;
}

int CPacketProc::DO_OBSERVER_CONNET_OBSERVER(CClient* pTarget, CPacket& pReqPacket)
{
	return 0;
}

int CPacketProc::DO_GAME_CHANGEINGZONE(CClient* pTarget, CPacket& pReqPacket)
{
	st_STC_ChangeingZone res;
	pReqPacket >> res;

	int ret = res.ret;
	int type = res.type;

	printf("ret : %d, type : %d\n", ret, type);
	return 0;
}

int CPacketProc::DO_GAME_TELEPORT(CClient* pTarget, CPacket& pReqPacket)
{
	st_STC_Teleport res;

	pReqPacket >> res;

	printf("[Teleport] ret : %d\n", res.ret);
	return 0;
}

