#pragma once

#include "./NetWork/CSelectObject.h"

class CClient : public CSelectObject
{
public:
    CClient(int x = 140, int y = 140, int size = 20) noexcept;

    void Move(int dx, int dy, int minX, int minY, int maxX, int maxY) noexcept;

    int GetX() const noexcept;
    int GetY() const noexcept;
    int GetSize() const noexcept;

private:
    int m_x;
    int m_y;
    int m_size;

public:
	virtual void OnRecv(CPacket* pPacket) override;

};
