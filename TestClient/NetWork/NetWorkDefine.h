#pragma once
#include "../CUtill/CPacket.h"
#include <atomic>
#include <basetsd.h>

enum ESOCKET_TYPE
{
	ESOCKET_TYPE_MAIN_CLIENT,
};

typedef struct st_PACKET_JOB
{
	int type;
	CPacket packet;

	st_PACKET_JOB() : type(0) {};
	st_PACKET_JOB(int _type, CPacket& _packet) : type(_type), packet(_packet) {};
	st_PACKET_JOB& operator=(st_PACKET_JOB&& job) noexcept
	{
		if (this != &job)
		{
			type = job.type;
			//packet = std::move(job.packet);
			packet = job.packet;
		}
		return *this;
	}

	st_PACKET_JOB(const st_PACKET_JOB&) = default;
	st_PACKET_JOB& operator=(st_PACKET_JOB& job)
	{
		type = job.type;
		packet = job.packet;
		return *this;
	}

}PROC_MSG;