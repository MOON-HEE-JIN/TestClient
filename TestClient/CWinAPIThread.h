#pragma once
#include <WS2tcpip.h>
#include <windows.h>
#include <vector>
#include "./NetWork/NetWorkDefine.h"
#include "./CUtill/CLockFreeQueue_SPSC.h"

class CWinObject;

unsigned __stdcall CWinAPIThread(void* arg);
HWND GetWinApiWindowHandle();
extern CLockFreeQueue_SPSC<PROC_MSG> g_renderQueue;
extern std::vector<std::vector<CWinObject*>> g_RenderObjects;