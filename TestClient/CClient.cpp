#include "CClient.h"

#include "CPacketProc.h"
#include "CPacketChatProc.h"
#include "AoiTestRecorder.h"
#include "GameDefines.h"
#include "CWinAPIThread.h"
#include "COtherClient.h"
#include "CUtill/CUtill.h"

static CPacketProc Proc;
static CPacketChatProc ChatProc;

std::atomic<bool> g_isTextInputFocused = false;

CClient::CClient(int x, int y, int size) noexcept
    : m_iID(-1), m_size(size)
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
    PACKET_MSG msg;
    while (m_queue.TryDequeue(msg))
    {
        switch (msg.sock)
        {
        case eKIND_SOCKET::Game:
            Proc.DO_GAME_Proc(msg.job.type, this, msg.job.packet);
            break;
        case eKIND_SOCKET::Chat:
            ChatProc.DO_CHAT_Proc(msg.job.type, this, msg.job.packet);
            break;
        }
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
					CAoiTestRecorder::Get().SetLocalPosition(m_pos);
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
                    CAoiTestRecorder::Get().RecordLogicError(
                        "duplicate other-player create id=" + std::to_string(info.ID));
                    printf("이미 존재 하는 OtherPlayer[%d]\n", info.ID);
                    break;
                }
                COtherClient* pOther = new COtherClient(info.ID, info.pos, info.speed);
                m_mapOtherPlayer[info.ID] = pOther;
                CAoiTestRecorder::Get().RecordOtherCreated(info.ID, info.pos);
                printf("새로운 플레이어 추가 %d\n", info.ID);
            }
            break;
            case ELOGIC_MSG::ELOGIC_REMOVE_OTHER_PLAYER:
            {
                int id;
                msg.packet >> id;
                if (m_mapOtherPlayer.find(id) == m_mapOtherPlayer.end())
                {
                    CAoiTestRecorder::Get().RecordLogicError(
                        "remove missing other-player id=" + std::to_string(id));
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
                    CAoiTestRecorder::Get().RecordLogicError(
                        "move-start missing other-player id=" + std::to_string(id));
                    printf("다른 플레이어 이동 시작 실패 %d\n", id);
                    break;
                }
				printf("다른 플레이어 이동 시작 %d\n", id);
                COtherClient* pOther = m_mapOtherPlayer[id];
                pOther->MoveStart(pos, dir);
                CAoiTestRecorder::Get().RecordOtherMoveStartApplied(id);
            }
                break;
            case ELOGIC_MSG::ELOGIC_MOVE_STOP_OTHER_PLAYER:
            {
                int id;
                st_Vector3F pos;
                msg.packet >> id >> pos;
            
                if (m_mapOtherPlayer.find(id) == m_mapOtherPlayer.end())
                {
                    CAoiTestRecorder::Get().RecordLogicError(
                        "move-stop missing other-player id=" + std::to_string(id));
                    printf("다른 플레이어 이동 중지 찾기 실패 %d\n", id);
                    break;
                }

                COtherClient* pOther = m_mapOtherPlayer[id];
                pOther->MoveStop(pos);
                CAoiTestRecorder::Get().RecordOtherMoveStopApplied(id);
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
		CAoiTestRecorder::Get().RecordLocalPosition(m_pos);
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

void CClient::OnRecv(int sock, int type, CPacket* pPacket)
{
	m_queue.Enqueue(PACKET_MSG(sock, PROC_MSG(type, *pPacket)));
}

void CClient::Update(float delta)
{
	ProcessPacket();
    ProcessLogicPacket();
	ProcessAction();
    ProcessOtherPlayer();
    UpdateAutoAoiTest();
}

void CClient::EnableAutoAoiTest(int clientIndex, double startDelaySeconds)
{
    m_autoTestEnabled = true;
    m_autoClientIndex = clientIndex;
    m_autoStartDelaySeconds = startDelaySeconds;
    m_autoTestBeginTime = CUtil::GetQPCNowTime();
    m_autoStateBeginTime = m_autoTestBeginTime;
    m_autoState = EAutoAoiState::WaitConnect;
    LockKey();
}

void CClient::NotifyZoneEntered(int channel, int zone, st_Vector3F pos)
{
    if (m_autoTestEnabled && channel == 0 && zone == 1)
        m_autoZoneEntered = true;
    m_pos = pos;
}

void CClient::NotifyTeleportCompleted()
{
    if (m_autoTestEnabled)
        m_autoTeleportCompleted = true;
}

void CClient::SetAutoState(EAutoAoiState state, double now)
{
    m_autoState = state;
    m_autoStateBeginTime = now;
}

void CClient::StartAutoMove(const st_Vector3F& direction)
{
    st_Vector3F dir = direction;
    dir.Normalization();

    st_CTS_MoveStart req;
    req.dir = dir;
    req.pos = m_pos;
    req.goal = m_pos + dir * 100.0f;
    SendPacket(req);
    CAoiTestRecorder::Get().RecordLocalMoveStartRequest(req.pos, req.dir);

    m_dir = dir;
    m_eAction = EACTION::EACTION_MOVE;
}

void CClient::StopAutoMove()
{
    if (m_eAction != EACTION::EACTION_MOVE)
        return;

    st_CTS_MoveStop req;
    req.pos = m_pos;
    SendPacket(req);
    CAoiTestRecorder::Get().RecordLocalMoveStopRequest(req.pos);

    m_dir = st_Vector3F(0.0f, 0.0f, 0.0f);
    m_eAction = EACTION::EACTION_IDLE;
}

void CClient::FinishAutoAoiTest()
{
    if (m_autoState == EAutoAoiState::Finished)
        return;

    StopAutoMove();
    m_autoState = EAutoAoiState::Finished;
    HWND hwnd = GetWinApiWindowHandle();
    if (hwnd != nullptr)
        PostMessage(hwnd, WM_CLOSE, 0, 0);
}

void CClient::UpdateAutoAoiTest()
{
    if (!m_autoTestEnabled || m_autoState == EAutoAoiState::Finished)
        return;

    const double now = CUtil::GetQPCNowTime();
    const double stateElapsed = now - m_autoStateBeginTime;

    // 연결 또는 서버 오류가 있어도 테스트 프로세스가 무한 대기하지 않도록 종료한다.
    if (now - m_autoTestBeginTime > 90.0)
    {
        FinishAutoAoiTest();
        return;
    }

    switch (m_autoState)
    {
    case EAutoAoiState::WaitConnect:
        if (m_iID >= 0)
        {
            st_CTS_ChangeZone req;
            req.channel = 0;
            req.zone = 1;
            SendPacket(req);
            SetAutoState(EAutoAoiState::WaitZone, now);
        }
        break;

    case EAutoAoiState::WaitZone:
        if (m_autoZoneEntered)
        {
            st_CTS_Teleport req;
            req.pos = st_Vector3F(500.0f + static_cast<float>(m_autoClientIndex * 2), 0.0f, 250.0f);
            CAoiTestRecorder::Get().BeginLocalTeleport(req.pos);
            SendPacket(req);
            SetAutoState(EAutoAoiState::WaitTeleport, now);
        }
        break;

    case EAutoAoiState::WaitTeleport:
        if (m_autoTeleportCompleted)
            SetAutoState(EAutoAoiState::Warmup, now);
        break;

    case EAutoAoiState::Warmup:
        if (stateElapsed >= m_autoStartDelaySeconds)
        {
            StartAutoMove(st_Vector3F(0.0f, 0.0f, 1.0f));
            SetAutoState(EAutoAoiState::GridMove, now);
        }
        break;

    case EAutoAoiState::GridMove:
        if (stateElapsed >= 4.0)
        {
            StopAutoMove();
            SetAutoState(EAutoAoiState::GridStopWait, now);
        }
        break;

    case EAutoAoiState::GridStopWait:
        if (stateElapsed >= 2.0)
        {
            const float directionX = (m_autoClientIndex % 2 == 0) ? 1.0f : -1.0f;
            StartAutoMove(st_Vector3F(directionX, 0.0f, 0.0f));
            SetAutoState(EAutoAoiState::SeparateMove, now);
        }
        break;

    case EAutoAoiState::SeparateMove:
        if (stateElapsed >= 17.0)
        {
            StopAutoMove();
            SetAutoState(EAutoAoiState::SeparateStopWait, now);
        }
        break;

    case EAutoAoiState::SeparateStopWait:
        if (stateElapsed >= 3.0)
        {
            const float directionX = (m_autoClientIndex % 2 == 0) ? -1.0f : 1.0f;
            StartAutoMove(st_Vector3F(directionX, 0.0f, 0.0f));
            SetAutoState(EAutoAoiState::ReturnMove, now);
        }
        break;

    case EAutoAoiState::ReturnMove:
        if (stateElapsed >= 17.0)
        {
            StopAutoMove();
            SetAutoState(EAutoAoiState::FinalWait, now);
        }
        break;

    case EAutoAoiState::FinalWait:
        if (stateElapsed >= 3.0)
            FinishAutoAoiTest();
        break;

    default:
        break;
    }
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
            CAoiTestRecorder::Get().RecordLocalMoveStopRequest(req.pos);
        }
        else
        {
            logic << (int)EACTION::EACTION_MOVE;
            st_CTS_MoveStart req;
            req.dir = newDir;
			req.pos = m_pos;
			req.goal = m_pos + req.dir * 100; // 목표 지점은 현재 위치에서 방향으로 100만큼 떨어진 지점
            SendPacket(req);
            CAoiTestRecorder::Get().RecordLocalMoveStartRequest(req.pos, req.dir);
        }
		m_logicqueue.Enqueue(PROC_MSG(0, logic));
        m_dir = newDir;
        //Move(newDir.X * speed, newDir.Y * speed, 0, 0, 100, 100);
    }
}

