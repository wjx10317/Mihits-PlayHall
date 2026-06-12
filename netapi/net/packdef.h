#pragma once

#include<memory.h>

#define _DEF_BUFFER         (4096)
#define _DEF_CONTENT_SIZE	(1024)
#define _MAX_SIZE           (40)
//自定义协议   先写协议头 再写协议结构
//登录 注册 获取好友信息 添加好友 聊天 发文件 下线请求
#define _DEF_PACK_BASE	(10000)
#define _DEF_PACK_COUNT (100)

//注册
#define _DEF_PACK_REGISTER_RQ	(_DEF_PACK_BASE + 0 )
#define _DEF_PACK_REGISTER_RS	(_DEF_PACK_BASE + 1 )
//登录
#define _DEF_PACK_LOGIN_RQ	(_DEF_PACK_BASE + 2 )
#define _DEF_PACK_LOGIN_RS	(_DEF_PACK_BASE + 3 )


//返回的结果
//注册请求的结果
#define tel_is_exist		(0)
#define register_success	(1)
#define name_is_exist       (2)
//登录请求的结果
#define user_not_exist		(0)
#define password_error		(1)
#define login_success		(2)


typedef int PackType;

//协议结构
//注册
typedef struct STRU_REGISTER_RQ
{
	STRU_REGISTER_RQ():type(_DEF_PACK_REGISTER_RQ)
	{
		memset( tel  , 0, sizeof(tel));
		memset( name  , 0, sizeof(name));
		memset( password , 0, sizeof(password) );
	}
	//需要手机号码 , 密码, 昵称
	PackType type;
	char tel[_MAX_SIZE];
	char name[_MAX_SIZE];
	char password[_MAX_SIZE];

}STRU_REGISTER_RQ;

typedef struct STRU_REGISTER_RS
{
	//回复结果
	STRU_REGISTER_RS(): type(_DEF_PACK_REGISTER_RS) , result(register_success)
	{
	}
	PackType type;
	int result;

}STRU_REGISTER_RS;

//登录
typedef struct STRU_LOGIN_RQ
{
	//登录需要: 手机号 密码 
	STRU_LOGIN_RQ():type(_DEF_PACK_LOGIN_RQ)
	{
		memset( tel , 0, sizeof(tel) );
		memset( password , 0, sizeof(password) );
	}
	PackType type;
	char tel[_MAX_SIZE];
	char password[_MAX_SIZE];

}STRU_LOGIN_RQ;

typedef struct STRU_LOGIN_RS
{
	// 需要 结果 , 用户的id
	STRU_LOGIN_RS(): type(_DEF_PACK_LOGIN_RS) , result(login_success),userid(0)
	{
        memset(name,0,sizeof(name));
	}
	PackType type;
	int result;
	int userid;
    char name[_MAX_SIZE];

}STRU_LOGIN_RS;
//游戏相关
#define DEF_PACK_JOIN_ZONE (_DEF_PACK_BASE + 4 )
#define DEF_PACK_LEAVE_ZONE (_DEF_PACK_BASE + 5 )
#define DEF_JOIN_ROOM_RQ (_DEF_PACK_BASE + 6 )
#define DEF_JOIN_ROOM_RS (_DEF_PACK_BASE + 7 )
#define DEF_ROOM_MEMBER (_DEF_PACK_BASE + 8 )
#define DEF_LEAVE_ROOM_RQ (_DEF_PACK_BASE + 9)
enum ENUM_PLAY_ZONE{Five_In_Line = 0x10,E_L_S,D_D_Z};
enum ENUM_ROOM_STATUS {_host,_player,_spec};
//加入专区
typedef struct STRU_JOIN_ZONE
{
    STRU_JOIN_ZONE():type(DEF_PACK_JOIN_ZONE),userid(0),zoneid(0)
    {}
    PackType type;
    int userid;
    int zoneid;
}STRU_JOIN_ZONE;

typedef struct STRU_LEAVE_ZONE
{
    STRU_LEAVE_ZONE():type(DEF_PACK_LEAVE_ZONE),userid(0)
    {}
    PackType type;
    int userid;
}STRU_LEAVE_ZONE;

typedef struct STRU_JOIN_ROOM_RQ
{
    STRU_JOIN_ROOM_RQ():type(DEF_JOIN_ROOM_RQ),userid(0),roomid(0)
    {}
    PackType type;
    int userid;
    int roomid;
}STRU_JOIN_ROOM_RQ;

typedef struct STRU_JOIN_ROOM_RS
{
    STRU_JOIN_ROOM_RS():type(DEF_JOIN_ROOM_RS),userid(0),roomid(0),status(_host),result(1)
    {}
    PackType type;
    int userid;
    int roomid;
    int status;
    int result;
}STRU_JOIN_ROOM_RS;

typedef struct STRU_ROOM_MENBER
{
    STRU_ROOM_MENBER():type(DEF_ROOM_MEMBER),userid(0),status(0)
    {
        memset(name,0,sizeof(_MAX_SIZE));
    }
    PackType type;
    int userid;
    int status;
    char name[_MAX_SIZE];

}STRU_ROOM_MENBER;

typedef struct STRU_LEAVE_ROOM_RQ
{
    STRU_LEAVE_ROOM_RQ():type(DEF_LEAVE_ROOM_RQ),userid(0),status(0),roomid(0)
    {

    }
    PackType type;
    int userid;
    int status;
    int roomid;

}STRU_LEAVE_ROOM_RQ;

#define DEF_ZONE_ROOM_INFO (_DEF_PACK_BASE + 10 )
#define DEF_ZONE_INFO_RQ (_DEF_PACK_BASE + 11 )
#define DEF_ZONE_ROOM_COUNT 121

typedef struct STRU_ZONE_ROOM_INFO // 专区每个房间的人数
{
    STRU_ZONE_ROOM_INFO():type(DEF_ZONE_ROOM_INFO),zoneid(0){
        memset( roomInfo , 0 , sizeof(roomInfo));
    }
    PackType type;
    int zoneid;
    int roomInfo[DEF_ZONE_ROOM_COUNT];
}STRU_ZONE_ROOM_INFO;
typedef struct STRU_ZONE_INFO_RQ // 请求专区每个房间人数
{
    STRU_ZONE_INFO_RQ():type(DEF_ZONE_INFO_RQ),zoneid(0){

    }
    PackType type;
    int zoneid;
}STRU_ZONE_INFO_RQ;


#define DEF_FIL_ROOM_READY      (_DEF_PACK_BASE + 12)
#define DEF_FIL_ROOM_NOTREADY   (_DEF_PACK_BASE + 13)
#define DEF_FIL_GAME_START      (_DEF_PACK_BASE + 14)
#define DEF_FIL_AI_BEGIN        (_DEF_PACK_BASE + 15)
#define DEF_FIL_AI_END          (_DEF_PACK_BASE + 16)
#define DEF_FIL_DISCARD_THIS    (_DEF_PACK_BASE + 17)
#define DEF_FIL_SURREND         (_DEF_PACK_BASE + 18)
#define DEF_FIL_PIECEDOWN       (_DEF_PACK_BASE + 19)
#define DEF_FIL_WIN             (_DEF_PACK_BASE + 20)

//游戏的准备
//准备 开始 胜利 托管 弃权(当前一次) 投降 落子(谁在什么位置下了一个什么子)
struct STRU_FIL_RQ
{
    STRU_FIL_RQ( PackType _type ) : type( _type ),userid(0), zoneid(0) , roomid(0){
    }
    PackType type; // 准备 开始 胜利 托管 弃权(当前一次) 投降 复用
    int userid;
    int zoneid;
    int roomid;
};//只有知道 什么专区 什么房间 才能找到响应的人

struct STRU_FIL_PIECEDOWN //什么专区的什么房间 谁在xy位置放了一个什么子
{
    STRU_FIL_PIECEDOWN() : type( DEF_FIL_PIECEDOWN ),userid(0), zoneid(0) , roomid(0)
    ,color(0) , x(-1),y(-1){
    }

    PackType type; // 落子
    int userid;
    int zoneid;
    int roomid;
    int color;
    int x;
    int y;
};
#define DEF_FIL_ALLRECORD_RQ             (_DEF_PACK_BASE + 21)
#define DEF_FIL_ALLRECORD_RS             (_DEF_PACK_BASE + 22)
#define DEF_FIL_SINGLERECORD_RQ             (_DEF_PACK_BASE + 23)
#define DEF_FIL_SINGLERECORD_RS             (_DEF_PACK_BASE + 24)
struct STRU_FIL_ALLRECORD_RQ
{
    STRU_FIL_ALLRECORD_RQ():type(DEF_FIL_ALLRECORD_RQ),userid(0)
    {

    }
    PackType type;
    int userid;
    //加上分区，可以找不同游戏模块的记录

};
struct STRU_FIL_ALLRECORD_RS
{
    STRU_FIL_ALLRECORD_RS():type(DEF_FIL_ALLRECORD_RS),zoneid(0)
    {
        memset(c_time,0,sizeof(c_time));
        memset(hostid,0,_MAX_SIZE);
        memset(playerid,0,_MAX_SIZE);
    }
    PackType type;
    int zoneid;
    char hostid[_MAX_SIZE];
    char playerid[_MAX_SIZE];
    char c_time[32];

};
struct STRU_FIL_SINGLERECORD_RQ
{
    STRU_FIL_SINGLERECORD_RQ():type(DEF_FIL_SINGLERECORD_RQ),userid(0)
    {

    }
    PackType type;
    int userid;
    char time[32];

};
struct STRU_FIL_SINGLERECORD_RS
{
    STRU_FIL_SINGLERECORD_RS():type(DEF_FIL_SINGLERECORD_RS),hostid(0),playerid(0),result(0)
    {
        memset(hostname,0,_MAX_SIZE);
        memset(playername,0,_MAX_SIZE);
    }
    PackType type;
    char hostname[_MAX_SIZE];
    char playername[_MAX_SIZE];
    int hostid;
    int playerid;
    int result;

};
