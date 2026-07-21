#include "clogic.h"
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>
#include<QDebug>
#include"bcrypt/BCrypt.hpp"
void CLogic::setNetPackMap()
{
    NetPackMap(_DEF_PACK_REGISTER_RQ)    = &CLogic::RegisterRq;
    NetPackMap(_DEF_PACK_LOGIN_RQ)       = &CLogic::LoginRq;
    NetPackMap(DEF_PACK_JOIN_ZONE)       = &CLogic::JoinZoneRq;
    NetPackMap(DEF_PACK_LEAVE_ZONE)      = &CLogic::LeaveZoneRq;
    NetPackMap(DEF_JOIN_ROOM_RQ)         = &CLogic::JoinRoomRq;
    NetPackMap(DEF_LEAVE_ROOM_RQ)        = &CLogic::LeaveRoomRq;
    NetPackMap(DEF_FIL_ROOM_READY)       = &CLogic::FIL_MsgSendRq;
    NetPackMap(DEF_FIL_ROOM_NOTREADY)    = &CLogic::FIL_MsgSendRq;
    NetPackMap(DEF_FIL_GAME_START)       = &CLogic::FIL_GameStart;
    NetPackMap(DEF_FIL_AI_BEGIN)         = &CLogic::FIL_MsgSendRq;
    NetPackMap(DEF_FIL_AI_END)           = &CLogic::FIL_MsgSendRq;
    NetPackMap(DEF_FIL_DISCARD_THIS)     = &CLogic::FIL_MsgSendRq;
    NetPackMap(DEF_FIL_SURREND)          = &CLogic::FIL_MsgSendRq;
    NetPackMap(DEF_FIL_PIECEDOWN)        = &CLogic::FIL_PieceDownRq;
    NetPackMap(DEF_FIL_WIN)              = &CLogic::FIL_Gameover;
    NetPackMap(DEF_ZONE_INFO_RQ)         = &CLogic::zoneinfoRq;
    NetPackMap(DEF_FIL_ALLRECORD_RQ)     = &CLogic::FIL_AllRecordRq;
    NetPackMap(DEF_FIL_SINGLERECORD_RQ)     = &CLogic::FIL_SingleRecordRq;
    NetPackMap(DEF_FIL_HEARTBEAT)          = &CLogic::HeartBeatRq;
    NetPackMap(DEF_FIL_RECONNECT_RQ)       = &CLogic::ReconnectRq;
    NetPackMap(DEF_GAME_VERSION_RQ)        = &CLogic::GameVersionRq;
    NetPackMap(DEF_FIL_LOGOUT_RQ)          = &CLogic::LogoutRq;

}

#define _DEF_COUT_FUNC_    cout << "clientfd:"<< clientfd << __func__ << endl;

//注册
void CLogic::RegisterRq(sock_fd clientfd,char* szbuf,int )
{
    //cout << "clientfd:"<< clientfd << __func__ << endl;
    _DEF_COUT_FUNC_
   STRU_REGISTER_RQ *rq = (STRU_REGISTER_RQ*)szbuf;
    STRU_REGISTER_RS rs;
    list<string>result;
    char szsql[1024] = "";
    sprintf(szsql,"select tel from t_user where tel = '%s'",rq->tel);
    if(!m_sql->SelectMysql(szsql,1,result))
    {
        printf("sql select error\n");

    }
    if(result.size()>0)
    {
        rs.result = tel_is_exist;
        result.pop_front();
    }
    else
    {
        sprintf(szsql,"select name from t_user where name = '%s'",rq->name);
        if(!m_sql->SelectMysql(szsql,1,result))
        {
            printf("sql select error\n");

        }
        if(result.size()>0)
        {
            rs.result = name_is_exist;

        }
        else
        {
            rs.result = register_success;
            string bcstr = getbcrypt(rq->password, 10);
            sprintf(szsql,"insert into t_user(tel,password,name)values('%s','%s','%s')"
                    ,rq->tel,bcstr.c_str(),rq->name);
            if(!m_sql->UpdataMysql(szsql))
            {
                printf("update fail:%s\n",szsql);
                rs.result = 210317;
            }

        }

    }
    printf("%d\n",rs.result);
    SendData(clientfd,(char*)&rs,sizeof(rs));
    //get tel,pass,nick
    //select sql
    // not exit can register
    //has exit cant register
   
}

//登录
void CLogic::LoginRq(sock_fd clientfd ,char* szbuf,int)
{
//    cout << "clientfd:"<< clientfd << __func__ << endl;
    _DEF_COUT_FUNC_
    STRU_LOGIN_RQ *rq = (STRU_LOGIN_RQ*)szbuf;
    STRU_LOGIN_RS rs;
    list<string>result;
    char szsql[1024] = "";
    sprintf(szsql,"select id,password,name from t_user where tel = '%s'",rq->tel);
    if(!m_sql->SelectMysql(szsql,3,result))
    {
        printf("sql select error\n");

    }
    if(result.size()==3)
    {
        int id = stoi(result.front());
        result.pop_front();
        string password = result.front();
        result.pop_front();
        string name = result.front();
        result.pop_front();
        if(!comparebcrypt(rq->password,password.c_str()))
        {
            rs.result = password_error;
        }
        else
        {
            rs.result = login_success;
            sprintf(rs.name,name.c_str());
            rs.userid = id;
            USER_INFO * info = nullptr;
            if (m_mapfdtouserinfo.find(id, info))
            {
                // 重新登录：清房间/旧 fd，复用同一 USER_INFO 再初始化
                NotifyRoomHardDisconnect(id, info);
                sock_fd old_fd = info->m_sockfd;
                pthread_mutex_lock(&m_fdLock);
                if (IS_ONLINE_SOCKFD(old_fd))
                    m_fdToUserid.erase(old_fd);
                pthread_mutex_unlock(&m_fdLock);
                if (IS_ONLINE_SOCKFD(old_fd) && old_fd != clientfd)
                    m_tcp->CloseFd(old_fd, false);
                info->reset();
            }
            else
            {
                info = new USER_INFO;
                m_mapfdtouserinfo.insert(id, info);
            }

            info->m_id = id;
            info->m_sockfd = clientfd;
            info->m_zoneid = 0;
            info->m_roomid = 0;
            strcpy(info->m_username, rs.name);

            // Phase1: 生成重连 token（rand_r 线程安全）
            {
                unsigned int seed = (unsigned int)(pthread_self() ^ time(NULL));
                int r1 = rand_r(&seed), r2 = rand_r(&seed);
                snprintf(info->m_token, _MAX_SIZE, "%08x%08x", r1, r2);
                strcpy(rs.token, info->m_token);
            }

            // 建立 fd→userid 反向索引
            pthread_mutex_lock(&m_fdLock);
            m_fdToUserid[clientfd] = id;
            pthread_mutex_unlock(&m_fdLock);
            // 初始化心跳时间，避免登录后首个 5s 窗口无记录
            pthread_mutex_lock(&m_heartbeatLock);
            m_heartbeatMap[id] = time(NULL);
            pthread_mutex_unlock(&m_heartbeatLock);

            SendData(clientfd,(char*)&rs,sizeof(rs));
            //send info data and friend data
            return ;
        }


    }
    else
    {
        rs.result = user_not_exist;
    }
    SendData(clientfd,(char*)&rs,sizeof(rs));
    //get tel,pass
    //select sql
    // not exist cant login
 //has exit but pas err cant login
    //else can login

}

void CLogic::JoinZoneRq(sock_fd clientfd, char *szbuf, int)
{
    _DEF_COUT_FUNC_
     STRU_JOIN_ZONE * rq = (STRU_JOIN_ZONE*)szbuf;
   USER_INFO* info =nullptr;
   if(!m_mapfdtouserinfo.find( rq->userid,info))
   {
       return;
   }
   info->m_zoneid = rq->zoneid;
}
void CLogic::LeaveZoneRq(sock_fd clientfd, char *szbuf, int)
{
    _DEF_COUT_FUNC_
     STRU_LEAVE_ZONE * rq = (STRU_LEAVE_ZONE*)szbuf;
    USER_INFO* info =nullptr;
    if(!m_mapfdtouserinfo.find( rq->userid,info))
    {
        return;
    }
    info->m_zoneid = 0;
}

void CLogic::JoinRoomRq(sock_fd clientfd, char *szbuf, int)
{
    _DEF_COUT_FUNC_
     STRU_JOIN_ROOM_RQ * rq = (STRU_JOIN_ROOM_RQ*)szbuf;
    STRU_JOIN_ROOM_RS rs;
    rs.userid = rq->userid;
    rs.roomid = rq->roomid;

    USER_INFO* info = nullptr;
    if (!m_mapfdtouserinfo.find(rq->userid, info))
    {
        rs.result = 0;
        rs.status = _spec;
        SendData(clientfd, (char*)&rs, sizeof(rs));
        return;
    }

    if (info->m_roomid != 0 && info->m_roomid != (int)rq->roomid)
    {
        rs.result = 0;
        rs.status = _spec;
        SendData(clientfd, (char*)&rs, sizeof(rs));
        return;
    }

    if (rq->roomid < 0 || rq->roomid >= static_cast<int>(m_roomList.size()))
    {
        rs.result = 0;
        rs.status = _spec;
        SendData(clientfd, (char*)&rs, sizeof(rs));
        return;
    }

    pthread_mutex_lock(&m_roomListlock);
    list<int>& usrlst = m_roomList[rq->roomid];

    int pos = 0;
    for (auto ite = usrlst.begin(); ite != usrlst.end(); ++ite, ++pos)
    {
        if (*ite != rq->userid)
            continue;
        rs.result = 1;
        if (pos == 0)
            rs.status = _host;
        else if (pos == 1)
            rs.status = _player;
        else
            rs.status = _spec;
        pthread_mutex_unlock(&m_roomListlock);
        SendData(clientfd, (char*)&rs, sizeof(rs));
        info->m_roomid = rq->roomid;
        return;
    }

    switch(usrlst.size())
    {
        case 0:
            rs.result =1;
            rs.roomid = rq->roomid;
            rs.status =_host;
            rs.userid = rq->userid;
            usrlst.push_back(rq->userid);

            break;
        case 1:
            rs.result =1;
            rs.roomid = rq->roomid;
            rs.status =_player;
            rs.userid = rq->userid;
            usrlst.push_back(rq->userid);



            break;
        default:
             rs.result = 1;
             rs.roomid = rq->roomid;
             rs.status =_spec;
             rs.userid = rq->userid;
             usrlst.push_back(rq->userid);
            break;
    }
    list<int> tmplist =usrlst;
    pthread_mutex_unlock(&m_roomListlock);

    // 是否已开局：用于 JOIN_RS.inGame，客户端立刻隐藏准备/开始
    vector<QPair<int,int>> moveSnapshot;
    pthread_mutex_lock(&m_roomcachelock);
    auto cit = room_cache.find(rq->roomid);
    const bool inGame = (cit != room_cache.end());
    if (inGame)
        moveSnapshot = cit->second;
    pthread_mutex_unlock(&m_roomcachelock);
    rs.inGame = inGame ? 1 : 0;

    SendData(clientfd,(char*)&rs,sizeof(rs));

    info->m_roomid = rq->roomid;

    STRU_ROOM_MENBER loginrq;
    loginrq.status = rs.status;
    loginrq.userid = rq->userid;
    strcpy(loginrq.name,info->m_username);
    int status =-1;
    for(auto ite = tmplist.begin();ite!=tmplist.end();ite++)
    {
        if(status<2)
        status++;
        int curid =*ite;
        if(curid!=rq->userid)
        {
            USER_INFO* meminfo = nullptr;
            if(!m_mapfdtouserinfo.find(curid,meminfo))
                continue;

            // 软离线成员也要同步座位；仅在线时再通知对方
            STRU_ROOM_MENBER memrq;
            memrq.userid = curid;
            memrq.status = status;
            strcpy(memrq.name,meminfo->m_username);
            SendData(info->m_sockfd,(char*)&memrq,sizeof(memrq));

            if (IS_ONLINE_SOCKFD(meminfo->m_sockfd))
                SendData(meminfo->m_sockfd,(char*)&loginrq,sizeof(loginrq));
        }
        else
        {
             loginrq.status = status;
            SendData(info->m_sockfd,(char*)&loginrq,sizeof(loginrq));
        }
    }

    // 对局中途加入：补发局面（开局态已由 JOIN_RS.inGame 触发）
    if (inGame)
    {
        int step = 0;
        for (auto& p : moveSnapshot)
        {
            STRU_FIL_PIECEDOWN pd;
            pd.zoneid = info->m_zoneid;
            pd.roomid = rq->roomid;
            pd.userid = rq->userid;
            pd.x = p.first;
            pd.y = p.second;
            pd.color = step;
            SendData(clientfd, (char*)&pd, sizeof(pd));
            step++;
        }
    }
}

void CLogic::zoneinfoRq(sock_fd clientfd, char *szbuf, int)
{
    _DEF_COUT_FUNC_
    STRU_ZONE_INFO_RQ *rq = (STRU_ZONE_INFO_RQ*)szbuf;
    STRU_ZONE_ROOM_INFO rs;
    for(int i=1;i<static_cast<int>(m_roomList.size());i++)
    {
        list<int>&lst = m_roomList[i];
        rs.roomInfo[i] =lst.size();
    }
    rs.zoneid =rq->zoneid;
    SendData(clientfd,(char*)&rs,sizeof(rs));
}
void CLogic::LeaveRoomRq(sock_fd clientfd, char *szbuf, int nlen)
{
    _DEF_COUT_FUNC_
    STRU_LEAVE_ROOM_RQ* rq = (STRU_LEAVE_ROOM_RQ*) szbuf;
    int id = rq->userid;
    const int roomid = rq->roomid;

    // 离房前取身份：对局中 host/player 离房需清 room_cache
    const int status = GetRoomMemberStatus(roomid, id);

    pthread_mutex_lock(&m_roomListlock);
    list<int>& usrlst = m_roomList[roomid];
    bool found = false;
    for (auto ite = usrlst.begin(); ite != usrlst.end(); ++ite)
    {
        if (*ite == id)
        {
            found = true;
            continue;
        }
        USER_INFO* peer = nullptr;
        if (!m_mapfdtouserinfo.find(*ite, peer))
            continue;
        if (!IS_ONLINE_SOCKFD(peer->m_sockfd))
            continue;
        SendData(peer->m_sockfd, szbuf, nlen);
    }
    if (found)
        usrlst.remove(id);
    pthread_mutex_unlock(&m_roomListlock);

    if (status <= _player)
        ClearRoomGameCache(roomid);

    USER_INFO* info = nullptr;
    if (m_mapfdtouserinfo.find(id, info))
        info->m_roomid = 0;
}
void CLogic::FIL_MsgSendRq(sock_fd clientfd, char *szbuf, int nlen)
{

       _DEF_COUT_FUNC_
        //拆包
        STRU_FIL_RQ* rq = (STRU_FIL_RQ*)szbuf;
        //什么专区 什么房间 谁 发了什么
        //根据专区 拿到房间列表  根据房间 拿到房间内成员  转发给房间里所有人
        //rq->zoneid; // 根据专区 拿到房间列表
        pthread_mutex_lock(&m_roomListlock);
        list<int> lstRes = m_roomList[ rq->roomid ];
        pthread_mutex_unlock(&m_roomListlock);
        for( auto ite = lstRes.begin() ; ite != lstRes.end(); ++ite ){
            int userid = *ite;
            USER_INFO* info = nullptr;
            if( !m_mapfdtouserinfo.find(userid , info ) ) continue;
            if( !IS_ONLINE_SOCKFD(info->m_sockfd) ) continue;
            SendData( info->m_sockfd , szbuf, nlen );
        }

}
//五子棋落子转发
void CLogic::FIL_PieceDownRq(sock_fd clientfd, char *szbuf, int nlen)
{
    _DEF_COUT_FUNC_
    STRU_FIL_PIECEDOWN* rq = (STRU_FIL_PIECEDOWN*)szbuf;
    STRU_FIL_PIECEDOWN_RS rs;
    rs.userid = rq->userid;
    rs.roomid = rq->roomid;
    rs.color = rq->color;
    rs.x = rq->x;
    rs.y = rq->y;

    auto fail = [&](int status, const char* why) {
        rs.result = piecedown_fail;
        rs.status = status;
        strncpy(rs.reason, why, sizeof(rs.reason) - 1);
        SendData(clientfd, (char*)&rs, sizeof(rs));
    };

    int roomid = rq->roomid;
    if (roomid <= 0 || roomid >= (int)m_roomList.size())
    {
        fail(_spec, "房间无效");
        return;
    }
    if (rq->x < 0 || rq->x >= 15 || rq->y < 0 || rq->y >= 15)
    {
        fail(GetRoomMemberStatus(roomid, rq->userid), "落子坐标越界");
        return;
    }

    int status = GetRoomMemberStatus(roomid, rq->userid);
    rs.status = status;
    if (status > _player)
    {
        fail(status, "观战身份不能落子");
        return;
    }

    // host=黑=偶数步, player=白=奇数步
    const int expectColor = (status == _host) ? 0 : 1;

    pthread_mutex_lock(&m_roomcachelock);
    auto it = room_cache.find(roomid);
    if (it == room_cache.end())
    {
        pthread_mutex_unlock(&m_roomcachelock);
        fail(status, "对局未开始或已结束");
        return;
    }
    const int step = (int)it->second.size();
    // UI 用 color%2 判回合，客户端步号可能漂移；以奇偶 + 身份校验，入队时规范化为 step
    if ((rq->color % 2) != (step % 2) || (rq->color % 2) != expectColor)
    {
        pthread_mutex_unlock(&m_roomcachelock);
        char tip[80];
        snprintf(tip, sizeof(tip),
                 "非你的回合(你是%s, 服务器下一步应走%s, color=%d, step=%d)",
                 status == _host ? "黑/房主" : "白/玩家",
                 (step % 2) == 0 ? "黑" : "白",
                 rq->color, step);
        fail(status, tip);
        return;
    }
    for (const auto& pos : it->second)
    {
        if (pos.first == rq->x && pos.second == rq->y)
        {
            pthread_mutex_unlock(&m_roomcachelock);
            fail(status, "该位置已有棋子");
            return;
        }
    }
    it->second.push_back(QPair<int,int>(rq->x, rq->y));
    pthread_mutex_unlock(&m_roomcachelock);

    // 广播规范化后的步序号，保证各端 m_turns 可对齐
    STRU_FIL_PIECEDOWN broadcast = *rq;
    broadcast.color = step;

    pthread_mutex_lock(&m_roomListlock);
    list<int> lstRes = m_roomList[roomid];
    pthread_mutex_unlock(&m_roomListlock);
    for (auto ite = lstRes.begin(); ite != lstRes.end(); ++ite)
    {
        int userid = *ite;
        USER_INFO* info = nullptr;
        if (!m_mapfdtouserinfo.find(userid, info)) continue;
        if (!IS_ONLINE_SOCKFD(info->m_sockfd)) continue;
        SendData(info->m_sockfd, (char*)&broadcast, sizeof(broadcast));
    }

    rs.result = piecedown_ok;
    rs.color = step;
    rs.reason[0] = '\0';
    SendData(clientfd, (char*)&rs, sizeof(rs));
    (void)nlen;
}

void CLogic::FIL_Gameover(sock_fd clientfd, char *szbuf, int)
{
    _DEF_COUT_FUNC_
    STRU_FIL_RQ* rq = (STRU_FIL_RQ*)szbuf;
    //depend on zoneid to find which room is over
    int zoneid = rq->zoneid;
    int roomid = rq->roomid;
    pthread_mutex_lock(&m_roomcachelock);
    auto it = room_cache.find(roomid);
    if(it==room_cache.end())
    {
        pthread_mutex_unlock(&m_roomcachelock);
        return;
    }
    auto tmp = (*it).second;
    QJsonArray dataArr;
    for(auto i:tmp)
    {
        QJsonObject o;
        o["x"] = i.first;
        o["y"] = i.second;
        dataArr.append(o);
    }
    room_cache.erase(it);
    pthread_mutex_unlock(&m_roomcachelock);
    QJsonDocument doc(dataArr);
    char szsql[4096]="";
    int count =0;
    int id[2];
    for(auto ite = m_roomList[roomid].begin()
        ;count<2&&ite!=m_roomList[roomid].end();ite++,count++)
    {
        id[count] = (*ite);
    }

    sprintf(szsql,"insert into t_records(idA,idB,zoneid,roomid,data) values(%d,%d,%d,%d,'%s')"
              ,id[0],id[1],zoneid,roomid,doc.toJson(QJsonDocument::Compact).constData());
    if(!m_sql->UpdataMysql(szsql))
    {
        printf("data json insert error\n");
    }

}

void CLogic::FIL_GameStart(sock_fd clientfd, char *szbuf, int nlen)
{
    _DEF_COUT_FUNC_
    STRU_FIL_RQ* rq = (STRU_FIL_RQ*)szbuf;
    //
    int roomid = rq->roomid;
    pthread_mutex_lock(&m_roomcachelock);
    room_cache[roomid] = {};
    pthread_mutex_unlock(&m_roomcachelock);
    FIL_MsgSendRq(clientfd,szbuf,nlen);
}

void CLogic::FIL_AllRecordRq(sock_fd clientfd, char *szbuf, int)
{
    _DEF_COUT_FUNC_
    STRU_FIL_ALLRECORD_RQ* rq = (STRU_FIL_ALLRECORD_RQ*)szbuf;
    int id = rq->userid;
    char szsql[1024];
    list<string>result;
    sprintf(szsql,"select idA,idB,zoneid,end_time from t_records where idA = %d or idB = %d order by end_time desc",id,id);
    if(!m_sql->SelectMysql(szsql,4,result))
    {
        printf("sql select error\n");
    }
    while(1)
    {   int hostid;
        int playerid;
        int zoneid;
        string time;
        if(result.size()>=4)
        {
            hostid = stoi(result.front());
            result.pop_front();
            playerid = stoi(result.front());
            result.pop_front();
            zoneid = stoi(result.front());
            result.pop_front();
            time = result.front();
            result.pop_front();

        }
        else
        {
            break;
        }
        string hostname;
        string playername;
        list<string>nameresult;
        sprintf(szsql,"select name from t_user where id = %d ",hostid);
        if(!m_sql->SelectMysql(szsql,1,nameresult) || nameresult.empty())
        {
            printf("sql select hostname error\n");
            continue;
        }
        hostname = nameresult.front();
        nameresult.pop_front();
        sprintf(szsql,"select name from t_user where id = %d ",playerid);
        if(!m_sql->SelectMysql(szsql,1,nameresult) || nameresult.empty())
        {
            printf("sql select playername error\n");
            continue;
        }
        playername = nameresult.front();
        nameresult.pop_front();
        STRU_FIL_ALLRECORD_RS rs;
        strncpy(rs.c_time,time.c_str(), sizeof(rs.c_time) - 1);
        strncpy(rs.hostid,hostname.c_str(), sizeof(rs.hostid) - 1);
        strncpy(rs.playerid,playername.c_str(), sizeof(rs.playerid) - 1);
        rs.zoneid = zoneid;
        SendData(clientfd,(char*)&rs,sizeof(rs));
    }
}

void CLogic::FIL_SingleRecordRq(sock_fd clientfd, char *szbuf, int)
{
    STRU_FIL_SINGLERECORD_RQ* rq = (STRU_FIL_SINGLERECORD_RQ*)szbuf;
    STRU_FIL_SINGLERECORD_RS rs;
    char szsql[1024];
    sprintf(szsql,"select idA,idB,data from t_records where (idA = %d or idB = %d) and end_time = '%s' ",rq->userid,rq->userid,rq->time);
    list<string>result;
    if(!m_sql->SelectMysql(szsql,3,result) || result.size() < 3)
    {
        printf("sql select error or empty record\n");
        rs.result =0;
        SendData(clientfd,(char*)&rs,sizeof(rs));
        return;
    }
    rs.result =1;

    int hostid = 0;
    int playerid = 0;
    try
    {
        hostid = stoi(result.front());
        result.pop_front();
        playerid = stoi(result.front());
        result.pop_front();
    }
    catch (...)
    {
        rs.result = 0;
        SendData(clientfd,(char*)&rs,sizeof(rs));
        return;
    }
    string jsonstr = result.front();
    result.pop_front();

    string hostname;
    string playername;

    sprintf(szsql,"select name from t_user where id = %d ",hostid);
    if(!m_sql->SelectMysql(szsql,1,result) || result.empty())
    {
        printf("sql select hostname error\n");
        rs.result = 0;
        SendData(clientfd,(char*)&rs,sizeof(rs));
        return;
    }
    hostname = result.front();
    result.pop_front();
    sprintf(szsql,"select name from t_user where id = %d ",playerid);
    if(!m_sql->SelectMysql(szsql,1,result) || result.empty())
    {
        printf("sql select playername error\n");
        rs.result = 0;
        SendData(clientfd,(char*)&rs,sizeof(rs));
        return;
    }
    playername = result.front();
    result.pop_front();
    strncpy(rs.hostname,hostname.c_str(), sizeof(rs.hostname) - 1);
    strncpy(rs.playername,playername.c_str(), sizeof(rs.playername) - 1);
    rs.hostid = hostid;
    rs.playerid = playerid;

    SendData(clientfd,(char*)&rs,sizeof(rs));

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(
           QByteArray::fromStdString(jsonstr),
           &parseError
       );
    if (parseError.error != QJsonParseError::NoError) {
           printf("JSON parse fail: %s\n", parseError.errorString().toUtf8().constData());
           return ;
       }

       if (!doc.isArray()) {
           printf("JSON is not array\n");
           return ;
       }

       QJsonArray arr = doc.array();
       int count =0;
       for (const QJsonValue &val : arr) {
           if (!val.isObject()) continue;

           QJsonObject obj = val.toObject();
           STRU_FIL_PIECEDOWN piece;
           piece.x = obj["x"].toInt(0);
           piece.y = obj["y"].toInt(0);
           // 与对局一致：步序号；客户端 slot_piecedown 内 %2 上色
           piece.color = count;
           if (m_tcp->SendData(clientfd,(char*)&piece,sizeof(piece)) < 0)
               break;
           count++;
       }
}
// ============================================================
// 断线重连
// ============================================================
void CLogic::ReconnectRq(sock_fd clientfd, char *szbuf, int)
{
    STRU_FIL_RECONNECT_RQ* rq = (STRU_FIL_RECONNECT_RQ*)szbuf;
    STRU_FIL_RECONNECT_RS rs;

    USER_INFO* info = nullptr;
    if (!m_mapfdtouserinfo.find(rq->userid, info))
    {
        rs.result = 0;
        SendData(clientfd, (char*)&rs, sizeof(rs));
        return;
    }
    if (strcmp(info->m_token, rq->token) != 0)
    {
        rs.result = 0;
        SendData(clientfd, (char*)&rs, sizeof(rs));
        return;
    }

    sock_fd old_fd = info->m_sockfd;
    if (IS_ONLINE_SOCKFD(old_fd) && old_fd != clientfd)
    {
        pthread_mutex_lock(&m_fdLock);
        m_fdToUserid.erase(old_fd);
        pthread_mutex_unlock(&m_fdLock);
        m_tcp->CloseFd(old_fd, false);
    }
    // 先标记离线：重放完成前不挂新 sockfd，避免对局转发插入重放序列导致客户端回合/颜色错乱
    info->m_sockfd = -1;
    if (IS_ONLINE_SOCKFD(old_fd))
    {
        pthread_mutex_lock(&m_fdLock);
        m_fdToUserid.erase(old_fd);
        pthread_mutex_unlock(&m_fdLock);
    }

    pthread_mutex_lock(&m_heartbeatLock);
    m_heartbeatMap[rq->userid] = time(NULL);
    pthread_mutex_unlock(&m_heartbeatLock);

    int zoneid = info->m_zoneid;
    int roomid = info->m_roomid;
    int inGame = 0;
    vector<QPair<int, int>> moveSnapshot;
    pthread_mutex_lock(&m_roomcachelock);
    if (roomid > 0 && room_cache.count(roomid) > 0)
    {
        inGame = 1;
        moveSnapshot = room_cache[roomid];
    }
    pthread_mutex_unlock(&m_roomcachelock);

    rs.result = 1;
    rs.zoneid = zoneid;
    rs.roomid = roomid;
    rs.inGame = inGame;
    SendData(clientfd, (char*)&rs, sizeof(rs));

    if (roomid <= 0)
    {
        info->m_sockfd = clientfd;
        pthread_mutex_lock(&m_fdLock);
        m_fdToUserid[clientfd] = rq->userid;
        pthread_mutex_unlock(&m_fdLock);
        return;
    }

    // 非法 roomid 会越界访问 m_roomList，直接按无房间恢复，避免整进程崩溃
    if (roomid >= (int)m_roomList.size())
    {
        printf("ReconnectRq: invalid roomid=%d, force roomid=0\n", roomid);
        info->m_roomid = 0;
        rs.roomid = 0;
        rs.inGame = 0;
        // 已发过 rs（带旧 roomid）；再发一包校正意义有限，仅挂 fd 返回
        info->m_sockfd = clientfd;
        pthread_mutex_lock(&m_fdLock);
        m_fdToUserid[clientfd] = rq->userid;
        pthread_mutex_unlock(&m_fdLock);
        return;
    }

    pthread_mutex_lock(&m_roomListlock);
    list<int> tmplist = m_roomList[roomid];
    pthread_mutex_unlock(&m_roomListlock);

    int reconnectStatus = _spec;
    int status = -1;
    for (auto ite = tmplist.begin(); ite != tmplist.end(); ++ite)
    {
        if (status < 2)
            status++;
        if (*ite == rq->userid)
        {
            reconnectStatus = status;
            break;
        }
    }

    status = -1;
    for (auto ite = tmplist.begin(); ite != tmplist.end(); ++ite)
    {
        if (status < 2)
            status++;
        int curid = *ite;
        USER_INFO* meminfo = nullptr;
        if (!m_mapfdtouserinfo.find(curid, meminfo))
            continue;

        STRU_ROOM_MENBER memrq;
        memrq.userid = curid;
        memrq.status = status;
        strcpy(memrq.name, meminfo->m_username);
        SendData(clientfd, (char*)&memrq, sizeof(memrq));
    }

    STRU_ROOM_MENBER endrq;
    endrq.userid = -1;
    SendData(clientfd, (char*)&endrq, sizeof(endrq));

    if (inGame == 0 && reconnectStatus <= _player)
    {
        STRU_FIL_RQ notready(DEF_FIL_ROOM_NOTREADY);
        notready.zoneid = zoneid;
        notready.roomid = roomid;
        notready.userid = rq->userid;

        status = -1;
        for (auto ite = tmplist.begin(); ite != tmplist.end(); ++ite)
        {
            if (status < 2)
                status++;
            int curid = *ite;
            if (curid == rq->userid)
                continue;
            if (status > _player)
                continue;

            USER_INFO* meminfo = nullptr;
            if (!m_mapfdtouserinfo.find(curid, meminfo))
                continue;
            if (!IS_ONLINE_SOCKFD(meminfo->m_sockfd))
                continue;
            SendData(meminfo->m_sockfd, (char*)&notready, sizeof(notready));
        }
    }

    if (reconnectStatus <= _player)
    {
        STRU_FIL_OPPONENT_DISCONNECT online;
        online.userid = rq->userid;
        online.roomid = roomid;
        online.status = reconnectStatus;
        online.kind = DEF_DISCONNECT_REONLINE;
        SendToRoomOnline(roomid, rq->userid, (char*)&online, sizeof(online));
    }

    if (inGame == 1)
    {
        // color 用步序号（与正常对局 getTurns() 一致），客户端 slot_piecedown 内再 %2
        int step = 0;
        for (auto& p : moveSnapshot)
        {
            STRU_FIL_PIECEDOWN pd;
            pd.zoneid = zoneid;
            pd.roomid = roomid;
            pd.userid = rq->userid;
            pd.x = p.first;
            pd.y = p.second;
            pd.color = step;
            SendData(clientfd, (char*)&pd, sizeof(pd));
            step++;
        }
    }

    // 重放完成后再挂 sockfd，并补发快照之后新产生的落子
    info->m_sockfd = clientfd;
    pthread_mutex_lock(&m_fdLock);
    m_fdToUserid[clientfd] = rq->userid;
    pthread_mutex_unlock(&m_fdLock);

    if (inGame == 1)
    {
        pthread_mutex_lock(&m_roomcachelock);
        vector<QPair<int, int>> tail;
        auto it = room_cache.find(roomid);
        if (it != room_cache.end() && it->second.size() > moveSnapshot.size())
            tail.assign(it->second.begin() + static_cast<long>(moveSnapshot.size()),
                        it->second.end());
        pthread_mutex_unlock(&m_roomcachelock);

        int step = (int)moveSnapshot.size();
        for (auto& p : tail)
        {
            STRU_FIL_PIECEDOWN pd;
            pd.zoneid = zoneid;
            pd.roomid = roomid;
            pd.userid = rq->userid;
            pd.x = p.first;
            pd.y = p.second;
            pd.color = step;
            SendData(clientfd, (char*)&pd, sizeof(pd));
            step++;
        }
    }
}

// ============================================================
// 房间内断线通知
// ============================================================
bool CLogic::GetUserIdByFd(sock_fd clientfd, int& userid)
{
    pthread_mutex_lock(&m_fdLock);
    auto it = m_fdToUserid.find(clientfd);
    if (it == m_fdToUserid.end())
    {
        pthread_mutex_unlock(&m_fdLock);
        return false;
    }
    userid = it->second;
    pthread_mutex_unlock(&m_fdLock);
    return true;
}

int CLogic::GetRoomMemberStatus(int roomid, int userid)
{
    pthread_mutex_lock(&m_roomListlock);
    list<int>& lst = m_roomList[roomid];
    int status = -1;
    for (auto ite = lst.begin(); ite != lst.end(); ++ite)
    {
        if (status < 2)
            status++;
        if (*ite == userid)
        {
            pthread_mutex_unlock(&m_roomListlock);
            return status;
        }
    }
    pthread_mutex_unlock(&m_roomListlock);
    return _spec;
}

void CLogic::SendToRoomOnline(int roomid, int excludeUserid, char* buf, int nlen)
{
    pthread_mutex_lock(&m_roomListlock);
    list<int> copy = m_roomList[roomid];
    pthread_mutex_unlock(&m_roomListlock);

    for (int uid : copy)
    {
        if (uid == excludeUserid)
            continue;
        USER_INFO* mem = nullptr;
        if (!m_mapfdtouserinfo.find(uid, mem))
            continue;
        if (!IS_ONLINE_SOCKFD(mem->m_sockfd))
            continue;
        SendData(mem->m_sockfd, buf, nlen);
    }
}

void CLogic::RemoveUserFromRoomList(int roomid, int userid)
{
    pthread_mutex_lock(&m_roomListlock);
    list<int>& lst = m_roomList[roomid];
    for (auto ite = lst.begin(); ite != lst.end(); ++ite)
    {
        if (*ite == userid)
        {
            lst.erase(ite);
            break;
        }
    }
    pthread_mutex_unlock(&m_roomListlock);
}

void CLogic::ClearRoomGameCache(int roomid)
{
    pthread_mutex_lock(&m_roomcachelock);
    room_cache.erase(roomid);
    pthread_mutex_unlock(&m_roomcachelock);
}

void CLogic::NotifyRoomSoftDisconnect(int userid, USER_INFO* info)
{
    int roomid = info->m_roomid;
    if (roomid <= 0)
        return;

    int status = GetRoomMemberStatus(roomid, userid);
    // spec 软断不广播（仍留在房间，等重连或硬断）
    if (status > _player)
        return;

    STRU_FIL_OPPONENT_DISCONNECT pkt;
    pkt.userid = userid;
    pkt.roomid = roomid;
    pkt.status = status;
    pkt.kind = DEF_DISCONNECT_SOFT;
    SendToRoomOnline(roomid, userid, (char*)&pkt, sizeof(pkt));
}

void CLogic::NotifyRoomHardDisconnect(int userid, USER_INFO* info)
{
    int roomid = info->m_roomid;
    if (roomid <= 0)
        return;

    int status = GetRoomMemberStatus(roomid, userid);

    STRU_FIL_OPPONENT_DISCONNECT pkt;
    pkt.userid = userid;
    pkt.roomid = roomid;
    pkt.status = status;
    pkt.kind = DEF_DISCONNECT_HARD;
    SendToRoomOnline(roomid, userid, (char*)&pkt, sizeof(pkt));

    // 仅 host/player 硬断才清对局缓存；spec 硬断不影响棋局
    if (status <= _player)
        ClearRoomGameCache(roomid);
    RemoveUserFromRoomList(roomid, userid);
    info->m_roomid = 0;
}

void CLogic::GameVersionRq(sock_fd clientfd, char *szbuf, int)
{
    _DEF_COUT_FUNC_
    STRU_GAME_VERSION_RQ* rq = (STRU_GAME_VERSION_RQ*)szbuf;
    STRU_GAME_VERSION_RS rs;
    rs.zoneid = rq->zoneid;

    int owner = 0;
    if (!GetUserIdByFd(clientfd, owner) || owner != rq->userid)
    {
        rs.result = 0;
        SendData(clientfd, (char*)&rs, sizeof(rs));
        return;
    }

    list<string> result;
    vector<SqlParam> params;
    params.push_back(SqlParam::FromInt(rq->zoneid));
    const char* sql =
        "select version, exe_name, manifest_url, release_note from t_game_pkg where zoneid = ?";
    if (!m_sql->SelectPrepared(sql, params, 4, result) || result.size() < 4)
    {
        rs.result = 0;
        SendData(clientfd, (char*)&rs, sizeof(rs));
        return;
    }

    string version = result.front(); result.pop_front();
    string exeName = result.front(); result.pop_front();
    string manifestUrl = result.front(); result.pop_front();
    string note = result.front(); result.pop_front();

    rs.result = 1;
    strncpy(rs.serverVersion, version.c_str(), sizeof(rs.serverVersion) - 1);
    strncpy(rs.exe_name, exeName.c_str(), sizeof(rs.exe_name) - 1);
    strncpy(rs.manifest_url, manifestUrl.c_str(), sizeof(rs.manifest_url) - 1);
    strncpy(rs.release_note, note.c_str(), sizeof(rs.release_note) - 1);
    SendData(clientfd, (char*)&rs, sizeof(rs));
}

// ============================================================
// Phase1: 心跳 + 掉线处理
// ============================================================
void CLogic::HeartBeatRq(sock_fd clientfd, char *szbuf, int)
{
    STRU_FIL_HEARTBEAT* rq = (STRU_FIL_HEARTBEAT*)szbuf;
    int owner = 0;
    if (!GetUserIdByFd(clientfd, owner) || owner != rq->userid)
        return;
    pthread_mutex_lock(&m_heartbeatLock);
    m_heartbeatMap[rq->userid] = time(NULL);
    pthread_mutex_unlock(&m_heartbeatLock);
    // 纯保活，不回复
}

void CLogic::LogoutRq(sock_fd clientfd, char *szbuf, int)
{
    STRU_FIL_LOGOUT_RQ* rq = (STRU_FIL_LOGOUT_RQ*)szbuf;
    int owner = 0;
    if (!GetUserIdByFd(clientfd, owner) || owner != rq->userid)
        return;
    // 主动退出：与心跳超时相同，硬断清会话（不可再 RECONNECT）
    HandleDisconnect(rq->userid);
}

void CLogic::CheckHeartBeat()
{
    time_t now = time(NULL);
    vector<pair<int, time_t>> overdue; // userid, last_heartbeat

    // 一次加锁、一次遍历：筛出满基础超时(30s)的用户
    pthread_mutex_lock(&m_heartbeatLock);
    for (auto& kv : m_heartbeatMap)
    {
        if (now - kv.second >= _DEF_HEARTBEAT_TIMEOUT_SEC)
            overdue.emplace_back(kv.first, kv.second);
    }
    pthread_mutex_unlock(&m_heartbeatLock);

    // 释放心跳锁后再查 USER_INFO（MyMap 自带锁；避免 heartbeat→userinfo 锁顺序死锁）
    for (auto& item : overdue)
    {
        const int userid = item.first;
        const time_t elapsed = now - item.second;
        if (elapsed < _DEF_HEARTBEAT_TIMEOUT_SEC)
            continue;

        USER_INFO* info = nullptr;
        const bool fdAlive = m_mapfdtouserinfo.find(userid, info)
                             && IS_ONLINE_SOCKFD(info->m_sockfd);

        if (fdAlive)
        {
            if (elapsed < _DEF_HEARTBEAT_WEAKNET_TIMEOUT_SEC)
                continue; // 弱网：未满 60s，下一位
            printf("User %d weak-net timeout (%lds >= %ds), disconnecting...\n",
                   userid, (long)elapsed, _DEF_HEARTBEAT_WEAKNET_TIMEOUT_SEC);
        }
        else
        {
            printf("User %d heartbeat timeout (fd offline, %lds), disconnecting...\n",
                   userid, (long)elapsed);
        }
        HandleDisconnect(userid);
    }
}

void CLogic::HandleDisconnect(int userid)
{
    USER_INFO* info = nullptr;
    if (!m_mapfdtouserinfo.find(userid, info))
        return;

    NotifyRoomHardDisconnect(userid, info);

    sock_fd old_fd = info->m_sockfd;

    // 清理业务数据
    m_mapfdtouserinfo.erase(userid);
    pthread_mutex_lock(&m_fdLock);
    if (IS_ONLINE_SOCKFD(old_fd))
        m_fdToUserid.erase(old_fd);
    pthread_mutex_unlock(&m_fdLock);
    pthread_mutex_lock(&m_heartbeatLock);
    m_heartbeatMap.erase(userid);
    pthread_mutex_unlock(&m_heartbeatLock);
    delete info;

    if (IS_ONLINE_SOCKFD(old_fd))
        m_tcp->CloseFd(old_fd, false);
}

// 静态回调：recv_task 检测到 fd 自然断开时，标记用户离线（保留 USER_INFO 供重连）
void CLogic::OnFdClosed(sock_fd fd)
{
    CLogic* logic = TcpKernel::GetInstance()->m_logic;

    pthread_mutex_lock(&logic->m_fdLock);
    auto it = logic->m_fdToUserid.find(fd);
    if (it == logic->m_fdToUserid.end())
    {
        pthread_mutex_unlock(&logic->m_fdLock);
        return;  // 已被 HandleDisconnect 清理
    }
    int userid = it->second;
    logic->m_fdToUserid.erase(it);
    pthread_mutex_unlock(&logic->m_fdLock);

    USER_INFO* info = nullptr;
    if (logic->m_mapfdtouserinfo.find(userid, info))
    {
        info->m_sockfd = _DEF_OFFLINE_SOCKFD;
        logic->NotifyRoomSoftDisconnect(userid, info);
    }
}
string CLogic::getbcrypt(char* password, int cost)
{
    return BCrypt::generateHash(password, cost);
}

bool CLogic::comparebcrypt(const char* src, const char* dst)
{
    return BCrypt::validatePassword(src, dst);
}
void* CLogic::HeartBeatThread(void* arg)
{
    CLogic* pthis = (CLogic*)arg;
    while (!pthis->m_heartbeatStop)
    {
        sleep(_DEF_HEARTBEAT_CHECK_INTERVAL_SEC);  // 每 10 秒检查一次
        pthis->CheckHeartBeat();
    }
    return NULL;
}
