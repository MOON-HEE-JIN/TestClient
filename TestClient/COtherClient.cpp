#include "COtherClient.h"

#include "AoiTestRecorder.h"
#include "GameDefines.h"
#include "CWinAPIThread.h"

COtherClient::COtherClient(int id, st_Vector3F pos, int speed)
	: m_iID(id), m_pos(pos), m_dir(), m_eAction(EACTION::EACTION_IDLE), m_iSpeed(speed)
{
}

void COtherClient::MoveStart(st_Vector3F pos, st_Vector3F dir)
{
	m_eAction = EACTION::EACTION_MOVE;
	m_pos = pos;
	m_dir = dir;
}

void COtherClient::MoveStop(st_Vector3F pos)
{
	m_eAction = EACTION::EACTION_IDLE;
	m_pos = pos;
}

void COtherClient::Update()
{
	switch (m_eAction)
	{
	case EACTION::EACTION_IDLE:
		break;
	case EACTION::EACTION_MOVE:
	{
		m_pos += m_dir * static_cast<float>(m_iSpeed) * 0.01667f;
		CAoiTestRecorder::Get().RecordOtherPosition(m_iID, m_pos);

		CPacket render;
		render << m_iID << m_pos.X << m_pos.Z;
		g_renderQueue.Enqueue(PROC_MSG(ERENDER_SET_ENTITY, render));
	}
		break;
	}
}
