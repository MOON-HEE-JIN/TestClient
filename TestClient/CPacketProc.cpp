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
	st_STC_ChangeZone res;
	pReqPacket >> res;

	if (res.ret == 0)
	{
		if (res.channel == 0 && res.zone == 0)
		{
			printf("로비로 이동\n");
		}
		else
		{
			printf("채널 : %d, 존 : %d\n", res.channel, res.zone);
			printf("게임존으로 이동\n");
		}
	
		CPacket clear;
		g_renderQueue.Enqueue(PROC_MSG(ERENDER_CLEAR_ENTITIES, clear));
	}

	return 0;
}

int CPacketProc::DO_GAME_CONNECTINFO(CClient* pTarget, CPacket& pReqPacket)
{
	st_STC_ConnectInfo res;
	pReqPacket >> res;

	printf("나의 ID : %d\n", res.info.ID);

	pTarget->SetID(res.info.ID);

	CPacket render;
	render << res.info.ID;
	g_renderQueue.Enqueue(PROC_MSG(ERENDER_SET_LOCAL_PLAYER_ID, render));
	return 0;
}


int CPacketProc::DO_GAME_MOVESTART(CClient* pTarget, CPacket& pReqPacket)
{
	st_STC_MoveStart res;
	pReqPacket >> res;
	printf("서버 이동 시작 패킷 수신 %d\n", res.ret);
	return 0;
}

int CPacketProc::DO_GAME_MOVESTOP(CClient* pTarget, CPacket& pReqPacket)
{
	st_STC_MoveStop res;
	pReqPacket >> res;

	if (res.ret != 0)
	{
		printf("서버 이동 중지 실패 ret : %d 서버 좌표 (%.2f, %.2f)\n", res.ret, res.pos.X, res.pos.Z);
	}
	else
	{
		printf("서버 이동 중지 ID : %d Pos : (%.2f, %.2f, %.2f)\n", res.ID, res.pos.X, res.pos.Y, res.pos.Z);
	}
	
	CPacket logic;
	int logictype = 0;
	if (res.ID == pTarget->GetID())
	{
		logic << (int)ELOGIC_MSG::ELOGIC_MOVE_STOP;
		logictype = 1;
	}
	else
	{
		logic << (int)ELOGIC_MSG::ELOGIC_MOVE_STOP_OTHER_PLAYER;
		logictype = 2;

	}
	logic << res.ID << res.pos;
	pTarget->PushLogicPacket(PROC_MSG(logictype, logic));
	
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

	if (res.ret == 0)
	{
		printf("Teleport 완료 이동 좌표 : (%.2f, %.2f)\n", res.pos.X, res.pos.Z);

		CPacket logic;
		logic << (int)ELOGIC_MSG::ELOGIC_TELEPORT << res.pos.X << res.pos.Z;
		pTarget->PushLogicPacket(PROC_MSG(1, logic));

		CPacket render;
		render << res.pos.X << res.pos.Z;
		g_renderQueue.Enqueue(PROC_MSG(ERENDER_LOCAL_PLAYER_POS, render));

		CPacket clear;
		g_renderQueue.Enqueue(PROC_MSG(ERENDER_CLEAR_ENTITIES, clear));
	}

	return 0;
}

int CPacketProc::DO_GAME_AOI_IN_PLAYER(CClient* pTarget, CPacket& pReqPacket)
{
	st_STC_AoiInPlayer res;

	pReqPacket >> res;
	
	if (pTarget->GetID() == res.info.ID)
	{
		printf("자신의 ID 를 받음\n");
		return 0;
	}

	CPacket logic;
	logic << (int)ELOGIC_MSG::ELOGIC_CREATE_OTHER_PLAYER;
	logic << res.info;
	
	pTarget->PushLogicPacket({ 2 , logic });
	
	EnqueueEntity(res.info.ID, res.info.pos.X, res.info.pos.Z);
	
	return 0;
}

int CPacketProc::DO_GAME_AOI_IN_PLAYERS(CClient* pTarget, CPacket& pReqPacket)
{
	st_STC_AoiInPlayers res;
	pReqPacket >> res;
	
	for (int i = 0; i < res.Loop1; ++i)
	{
		if (pTarget->GetID() == res.info[i].ID)
		{
			printf("자신의 ID 를 받음\n");
			continue;
		}

		CPacket logic;
		logic << (int)ELOGIC_MSG::ELOGIC_CREATE_OTHER_PLAYER;
		logic << res.info[i];

		pTarget->PushLogicPacket({ 2 , logic });

		EnqueueEntity(res.info[i].ID, res.info[i].pos.X, res.info[i].pos.Z);

		printf("AOI 범위 안 ID : %d, Pos : (%.2f, %.2f)\n", res.info[i].ID, res.info[i].pos.X, res.info[i].pos.Z);
	}
	return 0;
}

int CPacketProc::DO_GAME_AOI_OUT_PLAYER(CClient* pTarget, CPacket& pReqPacket)
{
	st_STC_AoiOutPlayer res;
	pReqPacket >> res;
	
	CPacket logic;
	logic << (int)ELOGIC_MSG::ELOGIC_REMOVE_OTHER_PLAYER;
	logic << res.ID;

	pTarget->PushLogicPacket({ 2 , logic });

	CPacket render;
	render << res.ID;
	g_renderQueue.Enqueue(PROC_MSG(ERENDER_REMOVE_ENTITY, render));
	printf("AOI 범위 밖 ID : %d\n", res.ID);
	return 0;
}

int CPacketProc::DO_GAME_AOI_OUT_PLAYERS(CClient* pTarget, CPacket& pReqPacket)
{
	st_STC_AoiOutPlayers res;
	pReqPacket >> res;

	for (int i = 0; i < res.Loop1; ++i)
	{
		if (pTarget->GetID() == res.info[i].ID)
		{
			printf("자신의 ID 를 받음\n");
			continue;
		}

		CPacket logic;
		logic << (int)ELOGIC_MSG::ELOGIC_REMOVE_OTHER_PLAYER;
		logic << res.info[i].ID;

		pTarget->PushLogicPacket({ 2 , logic });

		CPacket render;
		render << res.info[i].ID;
		g_renderQueue.Enqueue(PROC_MSG(ERENDER_REMOVE_ENTITY, render));
		printf("AOI 범위 밖 ID : %d\n", res.info[i].ID);
	}
	return 0;
}

int CPacketProc::DO_GAME_OTHERMOVESTART(CClient* pTarget, CPacket& pReqPacket)
{
	st_STC_OtherMoveStart res;
	pReqPacket >> res;

	if (pTarget->GetID() == res.ID)
	{
		printf("이동 시작 잘못된 아이디 나의 아이디 를 받음\n");
		return 0;
	}

	CPacket logic;
	logic << (int)ELOGIC_MSG::ELOGIC_MOVE_START_OTHER_PLAYER;
	logic << res.ID << res.pos << res.dir;

	pTarget->PushLogicPacket({ 2, logic });

	return 0;
}
