#include "CClient.h"

CClient::CClient(int x, int y, int size) noexcept
    : m_x(x), m_y(y), m_size(size)
{
}

void CClient::Move(int dx, int dy, int minX, int minY, int maxX, int maxY) noexcept
{
    m_x += dx;
    m_y += dy;

    if (m_x < minX) m_x = minX;
    if (m_y < minY) m_y = minY;
    if (m_x > maxX) m_x = maxX;
    if (m_y > maxY) m_y = maxY;
}

int CClient::GetX() const noexcept { return m_x; }
int CClient::GetY() const noexcept { return m_y; }
int CClient::GetSize() const noexcept { return m_size; }
