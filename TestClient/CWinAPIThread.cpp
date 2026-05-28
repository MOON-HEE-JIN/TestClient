#include "CWinAPIThread.h"

#include "CClient.h"

namespace
{
    constexpr wchar_t kWinClassName[] = L"WinApiRenderWindow";
    constexpr int kWindowSize = 300;

    CClient g_client;
    HWND g_hwnd = nullptr;

    void Render(HWND hwnd)
    {
        RECT rect{};
        GetClientRect(hwnd, &rect);

        PAINTSTRUCT ps{};
        HDC hdc = BeginPaint(hwnd, &ps);
        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP backBuffer = CreateCompatibleBitmap(hdc, rect.right, rect.bottom);
        HGDIOBJ oldBmp = SelectObject(memDC, backBuffer);

        HBRUSH bgBrush = CreateSolidBrush(RGB(128, 128, 128));
        FillRect(memDC, &rect, bgBrush);
        DeleteObject(bgBrush);

        RECT playerRect{
            g_client.GetX(),
            g_client.GetY(),
            g_client.GetX() + g_client.GetSize(),
            g_client.GetY() + g_client.GetSize()
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
            constexpr int speed = 3;
            RECT rect{};
            GetClientRect(hwnd, &rect);
            const int maxX = rect.right - g_client.GetSize();
            const int maxY = rect.bottom - g_client.GetSize();

            if (GetAsyncKeyState('W') & 0x8000) g_client.Move(0, -speed, 0, 0, maxX, maxY);
            if (GetAsyncKeyState('S') & 0x8000) g_client.Move(0, speed, 0, 0, maxX, maxY);
            if (GetAsyncKeyState('A') & 0x8000) g_client.Move(-speed, 0, 0, 0, maxX, maxY);
            if (GetAsyncKeyState('D') & 0x8000) g_client.Move(speed, 0, 0, 0, maxX, maxY);

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
