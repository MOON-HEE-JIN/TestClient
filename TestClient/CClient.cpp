#include "CClient.h"

#include "CPacketProc.h"

static CPacketProc Proc;

CClient::CClient(int x, int y, int size) noexcept
    : m_x(x), m_y(y), m_size(size)
{
    InitSocket();
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

void CClient::ProcessPacket()
{
    PROC_MSG msg;
    while (m_queue.TryDequeue(msg))
    {
        Proc.DO_GAME_Proc(msg.type, this, msg.packet);
	}
}

void CClient::OnRecv(int type, CPacket* pPacket)
{
	m_queue.Enqueue(PROC_MSG(type, *pPacket));
}

void CClient::Update(float delta)
{
	ProcessPacket();
}

