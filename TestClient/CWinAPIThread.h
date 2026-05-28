#pragma once
#include <WS2tcpip.h>
#include <windows.h>

unsigned __stdcall CWinAPIThread(void* arg);
HWND GetWinApiWindowHandle();
