#include "NetWork/CSelectObject.h"
#include "../../LoginServer/Native/LoginTransport.h"
#include <cstdio>
namespace {
bool Ready(SOCKET sock,bool write,ULONGLONG deadline) {
    auto now=GetTickCount64(); if(now>=deadline) return false;
    auto left=deadline-now; timeval timeout{static_cast<long>(left/1000),static_cast<long>((left%1000)*1000)};
    fd_set set; FD_ZERO(&set); FD_SET(sock,&set);
    return select(0,write?nullptr:&set,write?&set:nullptr,nullptr,&timeout)>0;
}
bool Transfer(SOCKET sock,char* data,int length,bool write,ULONGLONG deadline) {
    int offset=0;
    while(offset<length) { if(!Ready(sock,write,deadline)) return false;
        int n=write?send(sock,data+offset,length-offset,0):recv(sock,data+offset,length-offset,0);
        if(n==SOCKET_ERROR && WSAGetLastError()==WSAEWOULDBLOCK) continue;
        if(n<=0) return false; offset+=n;
    } return true;
}
bool AuthenticateSocket(SOCKET sock,int connectResult,int type,const char (&value)[32],long long userId) {
    if(connectResult!=0 && connectResult!=WSAEWOULDBLOCK) return false;
    auto deadline=GetTickCount64()+LoginTransport::Number("LOGIN_AUTH_TIMEOUT_MS",10000);
    if(!Ready(sock,true,deadline)) return false;
    int error=0,len=sizeof(error); if(getsockopt(sock,SOL_SOCKET,SO_ERROR,reinterpret_cast<char*>(&error),&len)!=0 || error) return false;
    std::vector<char> request;
    if(type==Rpc::Login::AUTH::GAME_AUTH) { Rpc::Login::st_CTS_GAME_AUTH p{}; memcpy(p.value,value,32); request=LoginTransport::Encode(p); }
    else { Rpc::Login::st_CTS_CHAT_AUTH p{}; memcpy(p.value,value,32); request=LoginTransport::Encode(p); }
    std::vector<char> response(28);
    if(!Transfer(sock,request.data(),static_cast<int>(request.size()),true,deadline) || !Transfer(sock,response.data(),28,false,deadline) || !LoginTransport::Check(response,type,20)) return false;
    Rpc::Login::st_STC_GAME_AUTH result{}; Rpc::Login::UnSerialization(response.data()+8,result);
    printf("%s authentication result=%d\n",type==Rpc::Login::AUTH::GAME_AUTH?"GameServer":"ChatServer",result.result);
    return result.result==0 && result.userId==userId && result.expiresAt>LoginTransport::Now();
}
}
bool CSelectObject::LoginAndAuthenticate() {
    try {
        LoginTransport::Config config;
        auto login=LoginTransport::Login(config);
        printf("LoginServer login result=%d\n",login.result);
        if(login.result!=0) return false;
        if(login.userId<=0 || login.expiresAt<=LoginTransport::Now()) return false;
        memcpy(m_loginValue,login.value,32); SecureZeroMemory(login.value,32);
        printf("Session issued, user=%lld, expiresAt=%lld (token hidden)\n",login.userId,login.expiresAt);
        auto gameHost=LoginTransport::Env("GAME_HOST","127.0.0.1");
        auto chatHost=LoginTransport::Env("CHAT_HOST","127.0.0.1");
        int gameResult=Connect(gameHost.c_str(),static_cast<unsigned short>(LoginTransport::Number("GAME_PORT",7799,1,65535)));
        if(!AuthenticateSocket(m_socket,gameResult,Rpc::Login::AUTH::GAME_AUTH,m_loginValue,login.userId)) { puts("GameServer authentication failed"); return false; }
        int chatResult=ChatConnect(chatHost.c_str(),static_cast<unsigned short>(LoginTransport::Number("CHAT_PORT",4499,1,65535)));
        if(!AuthenticateSocket(m_chatsocket,chatResult,Rpc::Login::AUTH::CHAT_AUTH,m_loginValue,login.userId)) { puts("ChatServer authentication failed"); return false; }
        puts("LOGIN FLOW PASS: both servers authenticated with the same value"); return true;
    } catch(const std::exception& e) { printf("Login configuration/transport failed: %s\n",e.what()); return false; }
}
