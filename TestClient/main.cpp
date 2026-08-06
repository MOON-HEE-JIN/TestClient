#include <afxwin.h>

#include <process.h>
#include <chrono>

#include "CWinAPIThread.h"
#include "CMFCThread.h"
#include "NetWork/CBaseSelectNet.h"
#include "CWinAPIThread.h"
#include "CClient.h"
#include "CUtill/CUtill.h"
#include <timeapi.h>
#pragma comment(lib, "winmm.lib")
HANDLE g_hStopEvent;
#define FIXED_DELTA 0.01667f  
#define MAX_FRAME_LOOP_COUNT 5
unsigned __stdcall LogicThread(void*)
{
    timeBeginPeriod(1);
    double lasttime = CUtil::GetQPCNowTime();
	double accumulatedtime = 0.0f;
    while (true)
    {
        //1000 Frames 1초당 1000 처리
        DWORD ret = WaitForSingleObject(g_hStopEvent, 1);

        // 종료 이벤트
        if (ret == WAIT_OBJECT_0)
            break;

        // 지연 시간 누적
        double nowtime = CUtil::GetQPCNowTime();
        double frame = nowtime - lasttime;
        lasttime = nowtime;

        accumulatedtime += frame;

        int nLoop = 0;
        while (accumulatedtime >= FIXED_DELTA && nLoop < MAX_FRAME_LOOP_COUNT)
        {
            g_SelectNet.GetMainSocket()->Update(0);
            accumulatedtime -= FIXED_DELTA;
            nLoop++;
        }

        // 너무 많은 frame 이 쌓인다면 쌓인 frame 처리하느라 더 지연 최대 frame 만큼만 돌리기
        if (nLoop == MAX_FRAME_LOOP_COUNT)
        {
            accumulatedtime = 0.0f;
        }
    }
    timeEndPeriod(1);
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