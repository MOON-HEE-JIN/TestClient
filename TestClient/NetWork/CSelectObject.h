#pragma once
#include <WS2tcpip.h>
#include <windows.h>

#include "../CUtill/CPacket.h"
#include "../CUtill/CRingBuffer.h"


class CSelectObject
{
public:
    CSelectObject(bool bMain = true);
    ~CSelectObject();

private:
    char m_strIP[16];
	unsigned short m_Port;
    std::atomic<bool> m_bKeyLock;

	CRingBuffer* m_recvBuffer;

	CRITICAL_SECTION m_sendLock;
protected:
    SOCKET m_socket;
protected:
    int InitSocket();
public:
    int Connect(const char IP[16], unsigned short Port);
    void Disconnect();

	SOCKET GetSocket() { return m_socket; }
	CRingBuffer* GetRecvBuffer() { return m_recvBuffer; }
public:
	template<typename T>
    void SendPacket(T& packet)
    {
        CPacket sendPacket;
        sendPacket << packet;
        SendPacket(&sendPacket);
	}
    void SendPacket(CPacket* pPacket);
    virtual void OnRecv(int type, CPacket* pPacket) = 0;
    virtual void Update(float delta) {};
	virtual void KeyProcess() {};

    void LockKey() { m_bKeyLock.store(true); }
    void UnLockKey() { m_bKeyLock.store(false); }
    bool GetKeyLock() { return m_bKeyLock.load(); }
};