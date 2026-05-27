#include <windows.h>
#include <process.h>

#include "CWinAPIThread.h"
#include "CMFCThread.h"

int wmain()
{
    unsigned int winApiThreadId = 0;
    unsigned int mfcThreadId = 0;

    HANDLE hWinApiThread = reinterpret_cast<HANDLE>(
        _beginthreadex(nullptr, 0, CWinAPIThread, nullptr, 0, &winApiThreadId));

    HANDLE hMfcThread = reinterpret_cast<HANDLE>(
        _beginthreadex(nullptr, 0, CMFCThread, nullptr, 0, &mfcThreadId));

    if (!hWinApiThread || !hMfcThread)
    {
        if (hWinApiThread) CloseHandle(hWinApiThread);
        if (hMfcThread) CloseHandle(hMfcThread);
        return 1;
    }

    HANDLE handles[2] = { hWinApiThread, hMfcThread };
    WaitForMultipleObjects(2, handles, TRUE, INFINITE);

    CloseHandle(hWinApiThread);
    CloseHandle(hMfcThread);

    return 0;
}
