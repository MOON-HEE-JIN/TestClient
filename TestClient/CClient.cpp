#include "CClient.h"

#include "CPacketProc.h"
#include "GameDefines.h"
#include "CWinAPIThread.h"
#include "COtherClient.h"

static CPacketProc Proc;

std::atomic<bool> g_isTextInputFocused = false;

CClient::CClient(int x, int y, int size) noexcept
    : m_size(size)
{
    InitSocket();
    m_pos = st_Vector3F(static_cast<float>(x), 0, static_cast<float>(y));
    m_dir = st_Vector3F(0, 0, 0);
    m_eAction = EACTION_IDLE;
    m_iSpeed = 5;

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
        else if(msg.type == 1)
        {
			int logicMsg;
			msg.packet >> logicMsg;
            switch (logicMsg)
            {
                case ELOGIC_MSG::ELOGIC_TELEPORT:
                    msg.packet >> m_pos.X >> m_pos.Z;
				break;
                case ELOGIC_MSG::ELOGIC_MOVE_STOP:
                {
                    int id = 0;
                    st_Vector3F pos;
					msg.packet >> id >> pos;
					float diff = m_pos.DistanceToSquared(pos);

					printf("Logic 멈추기 Pos : (%.2f, %.2f) Diff : %.f\n", m_pos.X, m_pos.Z, diff);
                    m_pos.X = pos.X;
                    m_pos.Z = pos.Z;
                }
                break;
                default:
                    break;
            }
        }
        else if (msg.type == 2)
        {
            int logicMsg;
            msg.packet >> logicMsg;
            switch (logicMsg)
            {
            case ELOGIC_MSG::ELOGIC_CREATE_OTHER_PLAYER:
            {
                st_PlayerInfo info;
                msg.packet >> info;
                if (m_mapOtherPlayer.find(info.ID) != m_mapOtherPlayer.end())
                {
                    printf("이미 존재 하는 OtherPlayer[%d]\n", info.ID);
                    break;
                }
                COtherClient* pOther = new COtherClient(info.ID, info.pos, info.speed);
                m_mapOtherPlayer[info.ID] = pOther;
                printf("새로운 플레이어 추가 %d\n", info.ID);
            }
            break;
            case ELOGIC_MSG::ELOGIC_REMOVE_OTHER_PLAYER:
            {
                int id;
                msg.packet >> id;
                if (m_mapOtherPlayer.find(id) == m_mapOtherPlayer.end())
                {
                    printf("잘못된 삭제 아이디 \n");
                    break;
                }

                COtherClient* pOther = m_mapOtherPlayer[id];
                m_mapOtherPlayer.erase(id);
                delete pOther;
            }
                break;
            case ELOGIC_MSG::ELOGIC_MOVE_START_OTHER_PLAYER:
            {
                int id;
                st_Vector3F pos;
                st_Vector3F dir;
                msg.packet >> id >> pos >> dir;

                if (m_mapOtherPlayer.find(id) == m_mapOtherPlayer.end())
                {
                    printf("다른 플레이어 이동 시작 실패 %d\n", id);
                    break;
                }
				printf("다른 플레이어 이동 시작 %d\n", id);
                COtherClient* pOther = m_mapOtherPlayer[id];
                pOther->MoveStart(pos, dir);
            }
                break;
            case ELOGIC_MSG::ELOGIC_MOVE_STOP_OTHER_PLAYER:
            {
                int id;
                st_Vector3F pos;
                msg.packet >> id >> pos;
            
                if (m_mapOtherPlayer.find(id) == m_mapOtherPlayer.end())
                {
                    printf("다른 플레이어 이동 중지 찾기 실패 %d\n", id);
                    break;
                }

                COtherClient* pOther = m_mapOtherPlayer[id];
                pOther->MoveStop(pos);
            }
                break;
            default:
                break;
            }
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
        m_pos += m_dir * static_cast<float>(m_iSpeed) * 0.01667f;
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

void CClient::ProcessOtherPlayer()
{
    std::map<int, COtherClient*>::iterator biter = m_mapOtherPlayer.begin();
    std::map<int, COtherClient*>::iterator eiter = m_mapOtherPlayer.end();

    for (biter; biter != eiter; biter++)
    {
        biter->second->Update();
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
    ProcessOtherPlayer();
}

// WINAPI Thread 에서 키입력을 받는다
// 키입력을 받으면 해당 동작을 Logic Thread로 전달한다
void CClient::KeyProcess()
{
    constexpr int speed = 5;

	st_Vector3F newDir{ 0, 0, 0 };

    if (GetAsyncKeyState('W') & 0x8000) newDir.Z = -1;
    else if (GetAsyncKeyState('S') & 0x8000) newDir.Z = 1;
    if (GetAsyncKeyState('A') & 0x8000) newDir.X = -1;
    else if (GetAsyncKeyState('D') & 0x8000) newDir.X = 1;

    newDir.Normalization();
	
    // 이동 방향이 설정되면 이동
    if (newDir != m_dir)
    {
        CPacket logic;
		if (newDir == st_Vector3F(0, 0, 0))
        {
            logic << (int)EACTION::EACTION_IDLE;
            st_CTS_MoveStop req;
			req.pos = m_pos;
            SendPacket(req);
        }
        else
        {
            logic << (int)EACTION::EACTION_MOVE;
            st_CTS_MoveStart req;
            req.dir = newDir;
			req.pos = m_pos;
			req.goal = m_pos + req.dir * 100; // 목표 지점은 현재 위치에서 방향으로 100만큼 떨어진 지점
            SendPacket(req);
        }
		m_logicqueue.Enqueue(PROC_MSG(0, logic));
        m_dir = newDir;
        //Move(newDir.X * speed, newDir.Y * speed, 0, 0, 100, 100);
    }
}

