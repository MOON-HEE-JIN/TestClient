#include "CClient.h"

#include "CPacketProc.h"
#include "GameDefines.h"
#include "CWinAPIThread.h"

static CPacketProc Proc;

std::atomic<bool> g_isTextInputFocused = false;

CClient::CClient(int x, int y, int size) noexcept
    : m_size(size)
{
    InitSocket();
    m_pos = st_Vector3F(static_cast<float>(x), 0, static_cast<float>(y));
    m_dir = st_Vector3F(0, 0, 0);
    m_eAction = EACTION_IDLE;
    m_iSpeed = 3;

	CPacket render;
	render << m_pos.X << m_pos.Z;
	g_renderQueue.Enqueue(PROC_MSG(ERENDER_LOCAL_PLAYER_POS, render));
}

int CClient::GetSize() const noexcept { return m_size; }

void CClient::ProcessPacket()
{
    PROC_MSG msg;
    while (m_queue.TryDequeue(msg))
    {
        Proc.DO_GAME_Proc(msg.type, this, msg.packet);
	}
}

void CClient::ProcessLogicPacket()
{
    PROC_MSG msg;
    while (m_logicqueue.TryDequeue(msg))
    {
        // TODO : Logic Packet 처리
        if(msg.type == 0)
        {
            msg.packet >> m_eAction;
		}
	}
}

void CClient::ProcessAction()
{
    switch (m_eAction)
    {
    case EACTION::EACTION_IDLE:
        break;
    case EACTION::EACTION_MOVE:
    {
		m_pos += m_dir * static_cast<float>(m_iSpeed);
		if (m_pos.X < 0) m_pos.X = 0;
		if (m_pos.Z < 0) m_pos.Z = 0;
		if (m_pos.X > MAP_WIDTH) m_pos.X = static_cast<float>(MAP_WIDTH);
		if (m_pos.Z > MAP_HEIGHT) m_pos.Z = static_cast<float>(MAP_HEIGHT);

        CPacket render;
		render << m_pos.X << m_pos.Z;
		g_renderQueue.Enqueue(PROC_MSG(ERENDER_LOCAL_PLAYER_POS, render));
    }
        break;
    default:
        break;
    }
}

void CClient::OnRecv(int type, CPacket* pPacket)
{
	m_queue.Enqueue(PROC_MSG(type, *pPacket));
}

void CClient::Update(float delta)
{
	ProcessPacket();
    ProcessLogicPacket();
	ProcessAction();
}

// WINAPI Thread 에서 키입력을 받는다
// 키입력을 받으면 해당 동작을 Logic Thread로 전달한다
void CClient::KeyProcess()
{
    constexpr int speed = 3;

	st_Vector3F newDir{ 0, 0, 0 };

    if (GetAsyncKeyState('W') & 0x8000) newDir.Z = -1;
    else if (GetAsyncKeyState('S') & 0x8000) newDir.Z = 1;
    if (GetAsyncKeyState('A') & 0x8000) newDir.X = -1;
    else if (GetAsyncKeyState('D') & 0x8000) newDir.X = 1;

    // 이동 방향이 설정되면 이동
    if (newDir != m_dir)
    {
        CPacket logic;
		if (newDir == st_Vector3F(0, 0, 0))
        {
            logic << (int)EACTION::EACTION_IDLE;
            st_CTS_MoveStop req;
            SendPacket(req);
        }
        else
        {
            logic << (int)EACTION::EACTION_MOVE;
            st_CTS_MoveStart req;
            req.dir = newDir;
            SendPacket(req);
        }
		m_logicqueue.Enqueue(PROC_MSG(0, logic));
        m_dir = newDir;
        //Move(newDir.X * speed, newDir.Y * speed, 0, 0, 100, 100);
    }
}

