#include "CPlayer.h"

#include "../GameDefines.h"

CPlayer::CPlayer(int id, float x, float z)
	: CWinObject(id, x, z, PLAYER_SIZE, RGB(64, 140, 255))
{
}

void CPlayer::Render(HDC hdc, const RECT& mapRect)
{
	RECT playerRect = GetScreenRect(mapRect);
	FillRect(hdc, &playerRect, m_brush);
}
