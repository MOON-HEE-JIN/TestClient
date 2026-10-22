#pragma once
#include "../CUtill/CPacket.h"
#include <atomic>
#include <basetsd.h>

typedef struct st_JOB
{
	int type;
	CPacket packet;

	st_JOB() : type(0) {};
	st_JOB(int _type, CPacket& _packet) : type(_type), packet(_packet) {};
	st_JOB& operator=(st_JOB&& job) noexcept
	{
		if (this != &job)
		{
			type = job.type;
			//packet = std::move(job.packet);
			packet = job.packet;
		}
		return *this;
	}

	st_JOB(const st_JOB&) = default;
	st_JOB& operator=(st_JOB& job)
	{
		type = job.type;
		packet = job.packet;
		return *this;
	}

}PROC_MSG;

typedef struct st_PACKET
{
	int sock;
	st_JOB job;
}PACKET_MSG;