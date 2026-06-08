#include "CWinObject.h"

#include "../GameDefines.h"

CWinObject::CWinObject(int id, float x, float z, int size, COLORREF brushColor)
	: m_id(id)
	, m_x(x)
	, m_z(z)
	, m_size(size)
	, m_brush(CreateSolidBrush(brushColor))
{
}

CWinObject::~CWinObject()
{
	if (m_brush)
	{
		DeleteObject(m_brush);
		m_brush = nullptr;
	}
}

int CWinObject::GetID() const noexcept
{
	return m_id;
}

void CWinObject::SetID(int id) noexcept
{
	m_id = id;
}

float CWinObject::GetX() const noexcept
{
	return m_x;
}

float CWinObject::GetZ() const noexcept
{
	return m_z;
}

void CWinObject::SetPosition(float x, float z) noexcept
{
	m_x = x;
	m_z = z;
}

int CWinObject::WorldToScreenX(float x, const RECT& mapRect) const noexcept
{
	float scale = static_cast<float>(mapRect.right - mapRect.left) / MAP_WIDTH;
	return mapRect.left + static_cast<int>(x * scale);
}

int CWinObject::WorldToScreenY(float z, const RECT& mapRect) const noexcept
{
	float scale = static_cast<float>(mapRect.bottom - mapRect.top) / MAP_HEIGHT;
	return mapRect.top + static_cast<int>(z * scale);
}

RECT CWinObject::GetScreenRect(const RECT& mapRect) const noexcept
{
	int screenX = WorldToScreenX(m_x, mapRect);
	int screenY = WorldToScreenY(m_z, mapRect);

	return RECT{
		screenX - (m_size / 2),
		screenY - (m_size / 2),
		screenX + (m_size / 2),
		screenY + (m_size / 2)
	};
}
