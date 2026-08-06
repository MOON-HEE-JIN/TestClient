#pragma once
#include "./NetWork/NetWorkDefine.h"
class COtherClient
{
public:
    COtherClient(int id, st_Vector3F pos, int speed);
private:
    int m_iID;
    st_Vector3F m_pos;
    st_Vector3F m_dir;

    int m_eAction;
    int m_iSpeed;

public:
    void MoveStart(st_Vector3F pos, st_Vector3F dir);
    void MoveStop(st_Vector3F pos);
public:
    void Update();
};