#include "CWinAPIThread.h"

#include "CClient.h"
#include "./NetWork/CBaseSelectNet.h"

CLockFreeQueue_SPSC<PROC_MSG> g_renderQueue;
namespace
{
    constexpr wchar_t kWinClassName[] = L"WinApiRenderWindow";
    constexpr int kWindowSize = 300;

    CClient g_client;
    HWND g_hwnd = nullptr;

	int testX = 140;
    int testY = 140;

    void Render(HWND hwnd)
    {
        RECT rect{};
        GetClientRect(hwnd, &rect);

        PAINTSTRUCT ps{};
        HDC hdc = BeginPaint(hwnd, &ps);
        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP backBuffer = CreateCompatibleBitmap(hdc, rect.right, rect.bottom);
        HGDIOBJ oldBmp = SelectObject(memDC, backBuffer);

        HBRUSH bgBrush = CreateSolidBrush(RGB(37, 37, 38));
        FillRect(memDC, &rect, bgBrush);
        DeleteObject(bgBrush);


        // Render
        
        RECT playerRect{
            testX,
            testY,
            testX + 20,
            testY + 20
        };

        HBRUSH clientBrush = CreateSolidBrush(RGB(0, 0, 0));
        FillRect(memDC, &playerRect, clientBrush);
        DeleteObject(clientBrush);
        


        BitBlt(hdc, 0, 0, rect.right, rect.bottom, memDC, 0, 0, SRCCOPY);

        SelectObject(memDC, oldBmp);
        DeleteObject(backBuffer);
        DeleteDC(memDC);
        EndPaint(hwnd, &ps);
    }

    LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg)
        {
        case WM_CREATE:
            SetTimer(hwnd, 1, 16, nullptr);
            return 0;

        case WM_TIMER:
        {
			if (g_isTextInputFocused.load())
                return 0;

			g_SelectNet.GetMainSocket()->KeyProcess();
            
            PROC_MSG msg;
            while (g_renderQueue.TryDequeue(msg))
            {
                // TODO : Render Packet 처리
                if (msg.type == 0)
                {
                    float x, y;
                    msg.packet >> x >> y;
					testX = static_cast<int>(x);
					testY = static_cast<int>(y);
                }
                
			}

            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }

        case WM_PAINT:
            Render(hwnd);
            return 0;

        case WM_DESTROY:
            KillTimer(hwnd, 1);
            PostQuitMessage(0);
            return 0;
        }

        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}

HWND GetWinApiWindowHandle()
{
    return g_hwnd;
}

unsigned __stdcall CWinAPIThread(void*)
{
    HINSTANCE hInst = GetModuleHandle(nullptr);

    WNDCLASS wc{};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(GRAY_BRUSH));
    wc.lpszClassName = kWinClassName;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

    RegisterClass(&wc);

    g_hwnd = CreateWindowEx(
        0,
        kWinClassName,
        L"CWinAPI Render",
        WS_OVERLAPPEDWINDOW,
        100, 100,
        kWindowSize, kWindowSize,
        nullptr,
        nullptr,
        hInst,
        nullptr
    );

    if (!g_hwnd)
    {
        return 1;
    }

    ShowWindow(g_hwnd, SW_SHOW);
    UpdateWindow(g_hwnd);

    MSG msg{};
    while (GetMessage(&msg, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
