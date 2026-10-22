#pragma once

#include <map>
#include <set>
#include <vector>
#include <atomic>

#include "CSelectObject.h"
#include "NetWorkDefine.h"


class CBaseSelectNet
{
public:
	CBaseSelectNet();
	~CBaseSelectNet() {};

public:
	int Init(int Port, int MaxClients);
private:
	int ListenSocket(unsigned short _port, SOCKET& out);
private:
	std::atomic<bool> m_bRun;
	SOCKET m_slisten;
	HANDLE m_hWorkerThread;
	unsigned short m_Port;

	CSelectObject* pSelectSocket;
	std::vector<SOCKET> m_vecSocket;
	int m_iMaxClients;

private:
	static unsigned __stdcall WorkerThread(void* arg);		// recv, send Thread
	virtual int WorkerRun();

public:
	// main 이 되는 소켓
	CSelectObject* GetMainSocket() { return pSelectSocket; }
	void StartServer(CBaseSelectNet* ptr, CSelectObject* pSelectObject);
	void WaitStopServer();
	void ServerShutDown();
};

extern CBaseSelectNet g_SelectNet;
