#include <afxwin.h>

#include <process.h>
#include <chrono>

#include "CWinAPIThread.h"
#include "CMFCThread.h"
#include "NetWork/CBaseSelectNet.h"
#include "CClient.h"

HANDLE g_hStopEvent;

unsigned __stdcall LogicThread(void*)
{
    using namespace std::chrono;

    auto prev = steady_clock::now();

    while (true)
    {
        auto now = steady_clock::now();

        float delta =
            duration<float>(now - prev).count();

        prev = now;

        g_SelectNet.GetMainSocket()->Update(delta);

        DWORD ret = WaitForSingleObject(g_hStopEvent, 16);

        if (ret == WAIT_OBJECT_0)
        {
            break;
        }
    }

    return 0;
}

int wmain()
{
    /*
    * 생성 된 Thread 목록
	* WinAPIThread : WinAPI로 렌더링을 담당하는 Thread
    * MFCThread : MFC 기반의 UI 처리를 담당하는 Thread
    * LogicThread : 게임 로직 처리를 담당하는 Thread
    * NetworkThread : 네트워크 처리를 담당하는 Thread
    */
    HINSTANCE hInst = ::GetModuleHandle(nullptr);

    if (!AfxWinInit(hInst, nullptr, ::GetCommandLine(), SW_SHOW))
        return 1;

    unsigned int winApiThreadId = 0;

    HANDLE hWinApiThread = reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, CWinAPIThread, nullptr, 0, &winApiThreadId));
    
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

    g_hStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	HANDLE hLogicThread = reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, LogicThread, nullptr, 0, nullptr));

    HANDLE handles[2] =
    {
        hWinApiThread,
        pMfcThread->m_hThread
    };

    WaitForMultipleObjects(2, handles, TRUE, INFINITE);
	g_SelectNet.WaitStopServer();
	SetEvent(g_hStopEvent);
    CloseHandle(hWinApiThread);

    delete pMfcThread;

    return 0;
}