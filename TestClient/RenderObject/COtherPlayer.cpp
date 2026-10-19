#include "COtherPlayer.h"

COtherPlayer::COtherPlayer(int id, float x, float z)
	: CWinObject(id, x, z, 12, RGB(236, 92, 86))
{
}

void COtherPlayer::Render(HDC hdc, const RECT& mapRect)
{
	RECT playerRect = GetScreenRect(mapRect);
	FillRect(hdc, &playerRect, m_brush);
}
