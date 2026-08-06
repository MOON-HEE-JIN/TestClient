#pragma once

#include "CWinObject.h"

class CMap : public CWinObject
{
public:
	CMap();

	void SetAoiCenter(float x, float z) noexcept;
	void Render(HDC hdc, const RECT& mapRect) override;
private:
	int ClampTile(int tile, int maxTile) const noexcept;

	float m_aoiCenterX;
	float m_aoiCenterZ;
};
