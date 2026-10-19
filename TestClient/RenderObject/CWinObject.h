#pragma once

#include <windows.h>

class CWinObject
{
public:
	CWinObject(int id, float x, float z, int size, COLORREF brushColor);
	virtual ~CWinObject();

	CWinObject(const CWinObject&) = delete;
	CWinObject& operator=(const CWinObject&) = delete;

	virtual void Render(HDC hdc, const RECT& mapRect) = 0;

	int GetID() const noexcept;
	void SetID(int id) noexcept;
	float GetX() const noexcept;
	float GetZ() const noexcept;
	void SetPosition(float x, float z) noexcept;

protected:
	int WorldToScreenX(float x, const RECT& mapRect) const noexcept;
	int WorldToScreenY(float z, const RECT& mapRect) const noexcept;
	RECT GetScreenRect(const RECT& mapRect) const noexcept;

	int m_id;
	float m_x;
	float m_z;
	int m_size;
	HBRUSH m_brush;
};
