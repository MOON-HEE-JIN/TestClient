#pragma once

namespace Rpc::ChatClient
{

namespace ERROR_CODE 
 {
	enum ERROR_CODE
	{
		NOT_ERROR,					//
		LOGIN_CONNECT,					//로그인 실패
		SECELT_CHANNEL,					//채널 선택 실패
		MAX_CHANNEL,					//채널 인원 많음
		LOGIN_ERROR,					//채널 입장 실패
		SELECT_CHARID,					//캐릭터 선택 실패
		ATTACK_FAIL,					//
		NOT_FIND_PATH,					//
		NOT_MOVE_BY_ATTACK,					//
		NOT_MOVE_BY_WALL,					//
		NOT_FIND_MAP,					//map 찾을수가 없음
		FAILD_ATTACK,					//공격 실패
	};
};

namespace OBJECT 
 {
	enum OBJECT
	{
		PLAYER = 111,			//
		MONSTER = 222,			//
		DROPITEM = 333,			//
	};
};

} // namespace Rpc::ChatClient
