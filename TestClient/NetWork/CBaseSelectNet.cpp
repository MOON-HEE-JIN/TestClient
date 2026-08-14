#include "CBaseSelectNet.h"
#include "../Stub/ProjectDefineStruct.h"
#include "../CUtill/CPacket.h"

#include <process.h>

CBaseSelectNet g_SelectNet;

CBaseSelectNet::CBaseSelectNet()
{
	m_bRun.store(false);
	m_slisten = INVALID_SOCKET;
}

int CBaseSelectNet::Init(int Port, int MaxClients)
{
	int ret = 0;

	m_Port = Port;
	m_iMaxClients = MaxClients;

	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		return WSAGetLastError();
	}
	
	
	ret = ListenSocket(m_Port, m_slisten);
	if (ret != 0)
		return ret;
	

	return ret;
}

int CBaseSelectNet::ListenSocket(unsigned short _port, SOCKET& out)
{
	int ret = 0;
	out = socket(AF_INET, SOCK_STREAM, 0);
	return 0;
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

	serveraddr.sin_port = htons(_port);
	
	ret = bind(out, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (ret == SOCKET_ERROR)
		return ret;

	ret = listen(out, SOMAXCONN);
	if (ret == SOCKET_ERROR)
		return ret;

	int optval = 0;
	int optlen = sizeof(optval);
	int tmep = setsockopt(out, SOL_SOCKET, SO_SNDBUF, (char*)&optval, sizeof(optval));

	linger _linger;
	_linger.l_onoff = 1;
	_linger.l_linger = 0;
	setsockopt(out, SOL_SOCKET, SO_LINGER, (char*)&_linger, sizeof(linger));

	int nValue = 1;
	setsockopt(out, SOL_SOCKET, TCP_NODELAY, (char*)&nValue, sizeof(nValue));

	//u_long nonBlocking = 1;
	//ioctlsocket(out, FIONBIO, &nonBlocking);

	return ret;
}

unsigned __stdcall CBaseSelectNet::WorkerThread(void* arg)
{
	CBaseSelectNet* p = static_cast<CBaseSelectNet*>(arg);

	p->WorkerRun();
	return 0;
}

int CBaseSelectNet::WorkerRun()
{
	FD_SET baseSet;
	FD_ZERO(&baseSet);

	//FD_SET(m_slisten, &baseSet);
	for (CSelectObject* pObj : m_vecSelectSocket)
	{
		FD_SET(pObj->GetSocket(), &baseSet);
	}

	FD_SET rSet;
	memcpy(&rSet, &baseSet, sizeof(FD_SET));

	int ret;
	while (m_bRun.load())
	{
		timeval timeout{ 0, 100 * 1000 };
		ret = select(0, &rSet, NULL, NULL, &timeout);
		if (ret > 0)
		{
			int Loop = m_vecSelectSocket.size();
			for (int i = 0; i < Loop; i++)
			{

				if (!FD_ISSET(m_vecSelectSocket[i]->GetSocket(), &rSet))
					continue;

				char buff[4096];
				ZeroMemory(buff, sizeof(buff));

				int recvRet = recv(m_vecSelectSocket[i]->GetSocket(), buff, sizeof(buff), 0);
				if (recvRet <= 0)
				{
					// 종료 처리
					continue;
				}
				m_vecSelectSocket[i]->GetRecvBuffer()->Enqueue(buff, recvRet);
				while (1)
				{
					if (sizeof(st_Header) > m_vecSelectSocket[i]->GetRecvBuffer()->GetUseSize())
						break;

					st_Header header;
					m_vecSelectSocket[i]->GetRecvBuffer()->Peek((char*)&header, sizeof(st_Header));

					if (sizeof(st_Header) + header.size > m_vecSelectSocket[i]->GetRecvBuffer()->GetUseSize())
						break;

					CPacket cPacket;
					m_vecSelectSocket[i]->GetRecvBuffer()->MoveReadPointer(sizeof(st_Header));
					m_vecSelectSocket[i]->GetRecvBuffer()->Dequeue(cPacket.GetBufferPtr(), header.size);
					cPacket.MoveWritePos(header.size);

					m_vecSelectSocket[i]->OnRecv(header.type, &cPacket);
				}
			}
		}
		//Sleep(10);
		memcpy(&rSet, &baseSet, sizeof(FD_SET));
	}
	return 0;
}

void CBaseSelectNet::StartServer(CBaseSelectNet* ptr, CSelectObject* pSelectObject)
{
	if (pSelectObject == nullptr)
		return;

	m_bRun.store(true);
	m_vecSelectSocket.push_back(pSelectObject);
	m_hWorkerThread = (HANDLE)_beginthreadex(NULL, 0, WorkerThread, ptr, 0, NULL);
}

void CBaseSelectNet::WaitStopServer()
{
	if (m_hWorkerThread != NULL)
	{
		WaitForSingleObject(m_hWorkerThread, INFINITE);
		CloseHandle(m_hWorkerThread);
		m_hWorkerThread = NULL;
	}
}

void CBaseSelectNet::ServerShutDown()
{
	if (!m_bRun.exchange(false))
		return;

	// select()를 즉시 깨워 WorkerThread가 종료 조건을 확인하게 한다.
	for (CSelectObject* pObject : m_vecSelectSocket)
	{
		if (pObject != nullptr && pObject->GetSocket() != INVALID_SOCKET)
			shutdown(pObject->GetSocket(), SD_BOTH);
	}
}
