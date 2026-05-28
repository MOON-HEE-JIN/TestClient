#include <afxwin.h>

#include <process.h>

#include "CWinAPIThread.h"
#include "CMFCThread.h"
#include "NetWork/CBaseSelectNet.h"
#include "CClient.h"

int wmain()
{
    HINSTANCE hInst = ::GetModuleHandle(nullptr);

    if (!AfxWinInit(hInst, nullptr, ::GetCommandLine(), SW_SHOW))
        return 1;

    unsigned int winApiThreadId = 0;

    HANDLE hWinApiThread = reinterpret_cast<HANDLE>(
        _beginthreadex(nullptr, 0, CWinAPIThread, nullptr, 0, &winApiThreadId)
        );

    CWinThread* pMfcThread = StartMFCThread();

    if (!hWinApiThread || !pMfcThread)
    {
        if (hWinApiThread)
            CloseHandle(hWinApiThread);

        if (pMfcThread)
            delete pMfcThread;

        return 1;
    }

	CClient client;
    int ret = client.Connect("127.0.0.1", 7799);
	g_SelectNet.StartServer(&g_SelectNet, &client);

    HANDLE handles[2] =
    {
        hWinApiThread,
        pMfcThread->m_hThread
    };

    WaitForMultipleObjects(2, handles, TRUE, INFINITE);
	g_SelectNet.WaitStopServer();
    CloseHandle(hWinApiThread);

    delete pMfcThread;

    return 0;
}