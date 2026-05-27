#include <afxwin.h>

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include <process.h>

#include "CWinAPIThread.h"
#include "CMFCThread.h"

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

    HANDLE handles[2] =
    {
        hWinApiThread,
        pMfcThread->m_hThread
    };

    WaitForMultipleObjects(2, handles, TRUE, INFINITE);

    CloseHandle(hWinApiThread);

    delete pMfcThread;

    return 0;
}