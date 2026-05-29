#pragma once
#include <WS2tcpip.h>
#include <windows.h>
#include "./NetWork/NetWorkDefine.h"
#include "./CUtill/CLockFreeQueue_SPSC.h"
unsigned __stdcall CWinAPIThread(void* arg);
HWND GetWinApiWindowHandle();
extern CLockFreeQueue_SPSC<PROC_MSG> g_renderQueue;