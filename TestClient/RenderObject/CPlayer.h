#pragma once

#include "CWinObject.h"

class CPlayer : public CWinObject
{
public:
	CPlayer(int id, float x, float z);

	void Render(HDC hdc, const RECT& mapRect) override;
};
