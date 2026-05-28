#pragma once



#include <map>
#include <set>
#include <vector>
#include <atomic>

#include "CSelectObject.h"

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
	bool m_bRun;
	SOCKET m_slisten;
	HANDLE m_hAceeptThread;
	HANDLE m_hWorkerThread;
	unsigned short m_Port;

	std::vector<CSelectObject*> m_vecSelectSocket;
	int m_iMaxClients;

private:
	//static unsigned __stdcall AceeptThread(void* arg);		// accept() Thread
	//virtual int AcceptRun();
	static unsigned __stdcall WorkerThread(void* arg);		// recv, send Thread
	virtual int WorkerRun();

public:
	void StartServer(CBaseSelectNet* ptr, CSelectObject* pSelectObject);
	void WaitStopServer();
	void ServerShutDown();
};

extern CBaseSelectNet g_SelectNet;