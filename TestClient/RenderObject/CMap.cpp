#include "CMap.h"

#include "../GameDefines.h"

CMap::CMap()
	: CWinObject(-1, 0.0f, 0.0f, 0, RGB(27, 27, 30))
	, m_aoiCenterX(140.0f)
	, m_aoiCenterZ(140.0f)
{
}

void CMap::SetAoiCenter(float x, float z) noexcept
{
	m_aoiCenterX = x;
	m_aoiCenterZ = z;
}

void CMap::Render(HDC hdc, const RECT& mapRect)
{
	FillRect(hdc, &mapRect, m_brush);

	int playerTileX = ClampTile(static_cast<int>(m_aoiCenterX) / TILE_SIZE, TILE_COUNT_X - 1);
	int playerTileY = ClampTile(static_cast<int>(m_aoiCenterZ) / TILE_SIZE, TILE_COUNT_Y - 1);

	int minTileX = ClampTile(playerTileX - AOI_TILE_RADIUS, TILE_COUNT_X - 1);
	int maxTileX = ClampTile(playerTileX + AOI_TILE_RADIUS, TILE_COUNT_X - 1);
	int minTileY = ClampTile(playerTileY - AOI_TILE_RADIUS, TILE_COUNT_Y - 1);
	int maxTileY = ClampTile(playerTileY + AOI_TILE_RADIUS, TILE_COUNT_Y - 1);

	RECT aoiRect{
		WorldToScreenX(static_cast<float>(minTileX * TILE_SIZE), mapRect),
		WorldToScreenY(static_cast<float>(minTileY * TILE_SIZE), mapRect),
		WorldToScreenX(static_cast<float>((maxTileX + 1) * TILE_SIZE), mapRect),
		WorldToScreenY(static_cast<float>((maxTileY + 1) * TILE_SIZE), mapRect)
	};

	HBRUSH aoiBrush = CreateSolidBrush(RGB(44, 78, 66));
	FillRect(hdc, &aoiRect, aoiBrush);
	DeleteObject(aoiBrush);

	HPEN aoiPen = CreatePen(PS_SOLID, 2, RGB(80, 220, 150));
	HGDIOBJ oldPen = SelectObject(hdc, aoiPen);
	HGDIOBJ oldBrush = SelectObject(hdc, GetStockObject(NULL_BRUSH));
	Rectangle(hdc, aoiRect.left, aoiRect.top, aoiRect.right, aoiRect.bottom);
	SelectObject(hdc, oldBrush);
	SelectObject(hdc, oldPen);
	DeleteObject(aoiPen);

	HPEN gridPen = CreatePen(PS_SOLID, 1, RGB(72, 72, 76));
	oldPen = SelectObject(hdc, gridPen);

	for (int x = 0; x <= TILE_COUNT_X; ++x)
	{
		int screenX = mapRect.left + ((mapRect.right - mapRect.left) * x / TILE_COUNT_X);
		MoveToEx(hdc, screenX, mapRect.top, nullptr);
		LineTo(hdc, screenX, mapRect.bottom);
	}

	for (int y = 0; y <= TILE_COUNT_Y; ++y)
	{
		int screenY = mapRect.top + ((mapRect.bottom - mapRect.top) * y / TILE_COUNT_Y);
		MoveToEx(hdc, mapRect.left, screenY, nullptr);
		LineTo(hdc, mapRect.right, screenY);
	}

	SelectObject(hdc, oldPen);
	DeleteObject(gridPen);
}

int CMap::ClampTile(int tile, int maxTile) const noexcept
{
	if (tile < 0)
	{
		return 0;
	}

	if (tile > maxTile)
	{
		return maxTile;
	}

	return tile;
}
