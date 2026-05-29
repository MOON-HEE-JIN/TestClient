#pragma once

#include <afxwin.h>

CWinThread* StartMFCThread();

void PostCheatCommand(const CString& cmd);