#include "CSelectObject.h"


CSelectObject::CSelectObject()
{
	InitSocket();

	m_recvBuffer = new CRingBuffer();
	InitializeCriticalSection(&m_sendLock);
}

CSelectObject::~CSelectObject()
{
	delete m_recvBuffer;
}

int CSelectObject::InitSocket()
{
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		int error = WSAGetLastError();
		return error;
	}

	m_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (m_socket == INVALID_SOCKET)
	{
		int error = WSAGetLastError();
		return error;
	}

	u_long on = 1;
	ioctlsocket(m_socket, FIONBIO, &on);

	return 0;
}

int CSelectObject::Connect(const char IP[16], unsigned short Port)
{
	//connect
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	
	inet_pton(AF_INET, IP, &serveraddr.sin_addr);

	serveraddr.sin_port = htons(Port);

	int retval = connect(m_socket, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR)
	{
		int error = WSAGetLastError();
		return error;
	}

	memcpy(m_strIP, IP, 16);
	m_Port = Port;

	return 0;
}

void CSelectObject::SendPacket(CPacket* pPacket)
{
	//EnterCriticalSection(&m_sendLock);
	send(m_socket, pPacket->GetBufferPtr(), pPacket->GetDataSize(), 0);
	//LeaveCriticalSection(&m_sendLock);
}
