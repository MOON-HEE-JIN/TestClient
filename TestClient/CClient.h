#pragma once

#include "./NetWork/CSelectObject.h"
#include "./NetWork/NetWorkDefine.h"
#include "./CUtill/CLockFreeQueue_SPSC.h"
#include "GameDefines.h"
#include <map>

class COtherClient;

class CClient : public CSelectObject
{
public:
    CClient(int x = 0, int y = 0, int size = PLAYER_SIZE) noexcept;

    int GetSize() const noexcept;

private:
    enum class EAutoAoiState
    {
        Disabled,
        WaitConnect,
        WaitZone,
        WaitTeleport,
        Warmup,
        GridMove,
        GridStopWait,
        SeparateMove,
        SeparateStopWait,
        ReturnMove,
        FinalWait,
        Finished
    };

    int m_iID;
    st_Vector3F m_pos;
    st_Vector3F m_dir;

    int m_eAction;
    int m_size;
    int m_iSpeed;

    CLockFreeQueue_SPSC<PACKET_MSG> m_queue;
	CLockFreeQueue_SPSC<PROC_MSG> m_logicqueue;

    // OtherPlayer 를 Client 안에 넣은 이유는 따로 관리하는 컨테이너 로 생성하면
    // 컨테이너 에 접근 하는 Queue 를 넣거나 Lock 을 걸어야한다
    // 해당 환경을 피하기 위해서 Client 에서 관리하도록 설정
    std::map<int, COtherClient*> m_mapOtherPlayer;

    bool m_autoTestEnabled = false;
    bool m_autoZoneEntered = false;
    bool m_autoTeleportCompleted = false;
    int m_autoClientIndex = -1;
    double m_autoStartDelaySeconds = 0.0;
    double m_autoTestBeginTime = 0.0;
    double m_autoStateBeginTime = 0.0;
    EAutoAoiState m_autoState = EAutoAoiState::Disabled;
private:
    void ProcessPacket();
	void ProcessLogicPacket();
    void ProcessAction();
    void ProcessOtherPlayer();
    void UpdateAutoAoiTest();
    void SetAutoState(EAutoAoiState state, double now);
    void StartAutoMove(const st_Vector3F& dir);
    void StopAutoMove();
    void FinishAutoAoiTest();
public:
	virtual void OnRecv(int sock, int type, CPacket* pPacket) override;
	virtual void Update(float delta) override;
	virtual void KeyProcess() override;

public:
    void PushLogicPacket(PROC_MSG&& msg)
    {
        m_logicqueue.Enqueue(std::move(msg));
	}

    void SetID(int id) { m_iID = id; }
    int GetID() { return m_iID; }
    st_Vector3F GetPos() const { return m_pos; }    

    void EnableAutoAoiTest(int clientIndex, double startDelaySeconds);
    void NotifyZoneEntered(int channel, int zone, st_Vector3F pos);
    void NotifyTeleportCompleted();
};

extern std::atomic<bool> g_isTextInputFocused;
