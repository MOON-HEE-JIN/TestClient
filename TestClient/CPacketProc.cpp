#include "CPacketProc.h"
#include "Stub/EnumDef.h"
#include "CWinAPIThread.h"
#include "GameDefines.h"

namespace
{
	void EnqueueEntity(int id, float x, float z)
	{
		CPacket render;
		render << id << x << z;
		g_renderQueue.Enqueue(PROC_MSG(ERENDER_SET_ENTITY, render));
	}
}

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
	st_STC_CreateChar res;
	pReqPacket >> res;

	EnqueueEntity(res.ID, res.pos.X, res.pos.Z);
	return 0;
}

int CPacketProc::DO_GAME_LEAVEZONE(CClient* pTarget, CPacket& pReqPacket)
{
	st_STC_LeaveZone res;
	pReqPacket >> res;

	CPacket render;
	render << res.ID;
	g_renderQueue.Enqueue(PROC_MSG(ERENDER_REMOVE_ENTITY, render));
	return 0;
}

int CPacketProc::DO_GAME_CONNECTINFO(CClient* pTarget, CPacket& pReqPacket)
{
	st_STC_ConnectInfo res;
	pReqPacket >> res;

	printf("ID : %d\n", res.info.ID);

	CPacket render;
	render << res.info.ID;
	g_renderQueue.Enqueue(PROC_MSG(ERENDER_SET_LOCAL_PLAYER_ID, render));
	return 0;
}

int CPacketProc::DO_GAME_ENTERZONE(CClient* pTarget, CPacket& pReqPacket)
{
	st_STC_EnterZone res;
	pReqPacket >> res;

	CPacket clear;
	g_renderQueue.Enqueue(PROC_MSG(ERENDER_CLEAR_ENTITIES, clear));

	for (int i = 0; i < res.Loop1; ++i)
	{
		EnqueueEntity(res.info[i].ID, res.info[i].pos.X, res.info[i].pos.Z);
	}

	return 0;
}

int CPacketProc::DO_GAME_MOVESTART(CClient* pTarget, CPacket& pReqPacket)
{
	return 0;
}

int CPacketProc::DO_GAME_MOVESTOP(CClient* pTarget, CPacket& pReqPacket)
{
	st_STC_MoveStop res;
	pReqPacket >> res;

	EnqueueEntity(res.ID, res.pos.X, res.pos.Z);
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
