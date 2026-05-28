#pragma once
#include <WS2tcpip.h>
#include <windows.h>

#include "../CUtill/CPacket.h"
#include "../CUtill/CRingBuffer.h"


class CSelectObject
{
public:
    CSelectObject();
    ~CSelectObject();

private:
    char m_strIP[16];
	unsigned short m_Port;

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
    void SendPacket(CPacket* pPacket);
    virtual void OnRecv(CPacket* pPacket) = 0;
};