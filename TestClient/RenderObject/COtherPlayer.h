#pragma once

#include "CWinObject.h"

class COtherPlayer : public CWinObject
{
public:
	COtherPlayer(int id, float x, float z);

	void Render(HDC hdc, const RECT& mapRect) override;
};
