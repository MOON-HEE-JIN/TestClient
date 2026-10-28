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
    CRingBuffer* m_recvChatBuffer;

	CRITICAL_SECTION m_sendLock;
protected:
    SOCKET m_socket;
    SOCKET m_chatsocket;
protected:
    int InitSocket();
public:
    int Connect(const char IP[16], unsigned short Port);
    int ChatConnect(const char IP[16], unsigned short Port);
    void Disconnect();

	SOCKET GetSocket() { return m_socket; }
    SOCKET GetChatSocket() { return m_chatsocket; }
	CRingBuffer* GetRecvBuffer() { return m_recvBuffer; }
    CRingBuffer* GetRecvChatBuffer() { return m_recvChatBuffer; }
public:
	template<typename T>
    void SendPacket(T& packet)
    {
        CPacket sendPacket;
        sendPacket << packet;
        SendPacket(&sendPacket);
	}
    template<typename T>
    void ChatSendPacket(T& packet)
    {
        CPacket sendPacket;
        sendPacket << packet;
        ChatSendPacket(&sendPacket);
    }
    void SendPacket(CPacket* pPacket);
    void ChatSendPacket(CPacket* pPacket);
    virtual void OnRecv(int sock, int type, CPacket* pPacket) = 0;
    virtual void Update(float delta) {};
	virtual void KeyProcess() {};

    void LockKey() { m_bKeyLock.store(true); }
    void UnLockKey() { m_bKeyLock.store(false); }
    bool GetKeyLock() { return m_bKeyLock.load(); }
};