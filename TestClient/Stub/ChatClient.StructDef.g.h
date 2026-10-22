#pragma once

#include <string>
#include "ProjectDefineStruct.h"

namespace Rpc::ChatClient
{

struct st_String
{
	__int16		length;
	std::string		msg;
};
struct st_Msg
{
	__int32		MsgID;
	st_String		Message;
};
struct st_CTS_GROUP
{
	__int32		group_ID;
	st_Msg		msg;
};
struct st_CTS_NORMAL
{
	st_Msg		msg;
};
struct st_CTS_WHISPER
{
	__int32		whisper_ID;
	st_Msg		msg;
};
struct st_STC_GROUP
{
	__int32		group_ID;
	st_Msg		msg;
};
struct st_STC_NORMAL
{
	st_Msg		msg;
};
struct st_STC_WHISPER
{
	__int32		whisper_ID;
	st_Msg		msg;
};

} // namespace Rpc::ChatClient
