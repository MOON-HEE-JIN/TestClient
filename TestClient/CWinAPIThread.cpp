#include "CWinAPIThread.h"

#include <map>
#include <string>

#include "CClient.h"
#include "GameDefines.h"
#include "RenderObject/CMap.h"
#include "RenderObject/COtherPlayer.h"
#include "RenderObject/CPlayer.h"
#include "RenderObject/CWinObject.h"
#include "./NetWork/CBaseSelectNet.h"

CLockFreeQueue_SPSC<PROC_MSG> g_renderQueue;
std::vector<std::vector<CWinObject*>> g_RenderObjects;

namespace
{
    constexpr wchar_t kWinClassName[] = L"WinApiRenderWindow";
    constexpr int kWindowWidth = 1280;
    constexpr int kWindowHeight = 1100;
    constexpr int kMapMargin = 16;
    constexpr int kHudWidth = 220;

    enum ERenderLayer
    {
        ERENDER_LAYER_MAP,
        ERENDER_LAYER_OTHER_PLAYER,
        ERENDER_LAYER_PLAYER,
        ERENDER_LAYER_COUNT
    };

    CClient g_client;
    HWND g_hwnd = nullptr;

    int g_localPlayerId = -1;
    CMap* g_map = nullptr;
    CPlayer* g_player = nullptr;
    std::map<int, COtherPlayer*> g_otherPlayers;

    int ClampInt(int value, int minValue, int maxValue)
    {
        if (value < minValue)
        {
            return minValue;
        }

        if (value > maxValue)
        {
            return maxValue;
        }

        return value;
    }

    int GetTileIndex(float worldValue, int tileCount)
    {
        int tile = static_cast<int>(worldValue) / TILE_SIZE;
        return ClampInt(tile, 0, tileCount - 1);
    }

    RECT GetMapRect(const RECT& clientRect)
    {
        int availableWidth = clientRect.right - clientRect.left - kHudWidth - (kMapMargin * 3);
        int availableHeight = clientRect.bottom - clientRect.top - (kMapMargin * 2);
        if (availableWidth < 1)
        {
            availableWidth = 1;
        }

        if (availableHeight < 1)
        {
            availableHeight = 1;
        }

        int mapPixels = availableWidth < availableHeight ? availableWidth : availableHeight;

        return RECT{
            kMapMargin,
            kMapMargin,
            kMapMargin + mapPixels,
            kMapMargin + mapPixels
        };
    }

    void DrawTextLine(HDC hdc, int x, int y, const std::wstring& text)
    {
        TextOutW(hdc, x, y, text.c_str(), static_cast<int>(text.length()));
    }

    void AddRenderObject(ERenderLayer layer, CWinObject* object)
    {
        if (g_RenderObjects.size() < ERENDER_LAYER_COUNT)
        {
            g_RenderObjects.resize(ERENDER_LAYER_COUNT);
        }

        g_RenderObjects[layer].push_back(object);
    }

    void RemoveRenderObject(CWinObject* object)
    {
        for (auto& layer : g_RenderObjects)
        {
            for (auto it = layer.begin(); it != layer.end(); ++it)
            {
                if (*it == object)
                {
                    layer.erase(it);
                    return;
                }
            }
        }
    }

    void DeleteRenderObject(CWinObject* object)
    {
        if (!object)
        {
            return;
        }

        RemoveRenderObject(object);
        delete object;
    }

    void ClearOtherPlayers()
    {
        for (auto& otherPlayer : g_otherPlayers)
        {
            DeleteRenderObject(otherPlayer.second);
        }

        g_otherPlayers.clear();
    }

    void ClearRenderObjects()
    {
        ClearOtherPlayers();
        DeleteRenderObject(g_player);
        g_player = nullptr;
        DeleteRenderObject(g_map);
        g_map = nullptr;
        g_RenderObjects.clear();
    }

    void EnsureRenderObjects()
    {
        if (g_RenderObjects.size() < ERENDER_LAYER_COUNT)
        {
            g_RenderObjects.resize(ERENDER_LAYER_COUNT);
        }

        if (!g_map)
        {
            g_map = new CMap();
            AddRenderObject(ERENDER_LAYER_MAP, g_map);
        }

        if (!g_player)
        {
            g_player = new CPlayer(g_localPlayerId, 140.0f, 140.0f);
            AddRenderObject(ERENDER_LAYER_PLAYER, g_player);
            g_map->SetAoiCenter(g_player->GetX(), g_player->GetZ());
        }
    }

    void SetLocalPlayerPosition(float x, float z)
    {
        EnsureRenderObjects();
        g_player->SetPosition(x, z);
        g_map->SetAoiCenter(x, z);
    }

    void SetLocalPlayerId(int id)
    {
        g_localPlayerId = id;
        EnsureRenderObjects();
        g_player->SetID(id);

        auto otherIt = g_otherPlayers.find(id);
        if (otherIt != g_otherPlayers.end())
        {
            g_player->SetPosition(otherIt->second->GetX(), otherIt->second->GetZ());
            g_map->SetAoiCenter(g_player->GetX(), g_player->GetZ());
            DeleteRenderObject(otherIt->second);
            g_otherPlayers.erase(otherIt);
        }
    }

    void SetEntity(int id, float x, float z)
    {
        EnsureRenderObjects();

        if (id == g_localPlayerId)
        {
            SetLocalPlayerPosition(x, z);
            return;
        }

        auto it = g_otherPlayers.find(id);
        if (it != g_otherPlayers.end())
        {
            it->second->SetPosition(x, z);
            return;
        }

        COtherPlayer* otherPlayer = new COtherPlayer(id, x, z);
        g_otherPlayers[id] = otherPlayer;
        AddRenderObject(ERENDER_LAYER_OTHER_PLAYER, otherPlayer);
    }

    void RemoveEntity(int id)
    {
        auto it = g_otherPlayers.find(id);
        if (it == g_otherPlayers.end())
        {
			printf("RemoveEntity 찾지 못함 ID : %d\n", id);
            return;
        }

        DeleteRenderObject(it->second);
        g_otherPlayers.erase(it);
    }

    void DrawHud(HDC hdc, const RECT& clientRect, const RECT& mapRect)
    {
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(220, 220, 220));

        EnsureRenderObjects();

        int x = mapRect.right + kMapMargin;
        int y = kMapMargin;
        int tileX = GetTileIndex(g_player->GetX(), TILE_COUNT_X);
        int tileY = GetTileIndex(g_player->GetZ(), TILE_COUNT_Y);

        DrawTextLine(hdc, x, y, L"AOI TestClient");
        y += 28;
        DrawTextLine(hdc, x, y, L"Map: 1024 x 1024");
        y += 20;
        DrawTextLine(hdc, x, y, L"Tile: 64 / Grid: 16 x 16");
        y += 20;
        DrawTextLine(hdc, x, y, L"Layer: Map / Other / Player");
        y += 32;
        DrawTextLine(hdc, x, y, L"Player ID: " + std::to_wstring(g_localPlayerId));
        y += 20;
        DrawTextLine(hdc, x, y, L"Pos: " + std::to_wstring(static_cast<int>(g_player->GetX())) + L", " + std::to_wstring(static_cast<int>(g_player->GetZ())));
        y += 20;
        DrawTextLine(hdc, x, y, L"Tile: " + std::to_wstring(tileX) + L", " + std::to_wstring(tileY));
        y += 20;
        DrawTextLine(hdc, x, y, L"Other: " + std::to_wstring(g_otherPlayers.size()));

        RECT hudLine{ x, y + 34, clientRect.right - kMapMargin, y + 35 };
        HBRUSH lineBrush = CreateSolidBrush(RGB(72, 72, 76));
        FillRect(hdc, &hudLine, lineBrush);
        DeleteObject(lineBrush);
    }

    void ProcessRenderQueue()
    {
        EnsureRenderObjects();

        PROC_MSG msg;
        while (g_renderQueue.TryDequeue(msg))
        {
            if (msg.type == ERENDER_LOCAL_PLAYER_POS)
            {
                float x = 0.0f;
                float z = 0.0f;
                msg.packet >> x >> z;
                SetLocalPlayerPosition(x, z);
            }
            else if (msg.type == ERENDER_SET_LOCAL_PLAYER_ID)
            {
                int id = -1;
                msg.packet >> id;
                SetLocalPlayerId(id);
            }
            else if (msg.type == ERENDER_SET_ENTITY)
            {
                int id = -1;
                float x = 0.0f;
                float z = 0.0f;
                msg.packet >> id >> x >> z;
                SetEntity(id, x, z);
            }
            else if (msg.type == ERENDER_REMOVE_ENTITY)
            {
                int id = -1;
                msg.packet >> id;
                RemoveEntity(id);
            }
            else if (msg.type == ERENDER_CLEAR_ENTITIES)
            {
                ClearOtherPlayers();
            }
        }
    }

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

        EnsureRenderObjects();

        RECT mapRect = GetMapRect(rect);
        for (const auto& layer : g_RenderObjects)
        {
            for (CWinObject* object : layer)
            {
                if (object)
                {
                    object->Render(memDC, mapRect);
                }
            }
        }

        DrawHud(memDC, rect, mapRect);

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
            EnsureRenderObjects();
            SetTimer(hwnd, 1, 16, nullptr);
            return 0;

        case WM_TIMER:
        {
			if (g_isTextInputFocused.load())
                return 0;

			g_SelectNet.GetMainSocket()->KeyProcess();
            ProcessRenderQueue();

            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }

        case WM_PAINT:
            Render(hwnd);
            return 0;

        case WM_DESTROY:
            KillTimer(hwnd, 1);
            ClearRenderObjects();
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
        L"AOI TestClient",
        WS_OVERLAPPEDWINDOW,
        100, 100,
        kWindowWidth, kWindowHeight,
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
