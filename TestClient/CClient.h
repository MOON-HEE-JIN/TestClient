#pragma once

#include "./NetWork/CSelectObject.h"
#include "./NetWork/NetWorkDefine.h"
#include "./CUtill/CLockFreeQueue_SPSC.h"

class CClient : public CSelectObject
{
public:
    CClient(int x = 140, int y = 140, int size = 20) noexcept;

    int GetSize() const noexcept;

private:
    st_Vector3F m_pos;
    st_Vector3F m_dir;

    int m_eAction;
    int m_size;

    int m_iSpeed;

    CLockFreeQueue_SPSC<PROC_MSG> m_queue;

	CLockFreeQueue_SPSC<PROC_MSG> m_logicqueue;
private:
    void ProcessPacket();
	void ProcessLogicPacket();
    void ProcessAction();
public:
	virtual void OnRecv(int type, CPacket* pPacket) override;
	virtual void Update(float delta) override;
	virtual void KeyProcess() override;
};

extern std::atomic<bool> g_isTextInputFocused;