#include "clogic.h"
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>
#include<QDebug>
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



}

#define _DEF_COUT_FUNC_    cout << "clientfd:"<< clientfd << __func__ << endl;

//注册
void CLogic::RegisterRq(sock_fd clientfd,char* szbuf,int nlen)
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
            sprintf(szsql,"insert into t_user(tel,password,name)values('%s','%s','%s')"
                    ,rq->tel,rq->password,rq->name);
            if(!m_sql->UpdataMysql(szsql))
            {
                printf("update fail:%s,%s\n",szsql);           
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
void CLogic::LoginRq(sock_fd clientfd ,char* szbuf,int nlen)
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
        if(strcmp(password.c_str(),rq->password)!=0)
        {
            rs.result = password_error;
        }
        else
        {
            rs.result = login_success;
            sprintf(rs.name,name.c_str());
            rs.userid = id;
            USER_INFO * info = nullptr;
            if(m_mapfdtouserinfo.find(id,info))
            {
                m_mapfdtouserinfo.erase(id);
                delete info;
                info =nullptr;
            }
            info = new USER_INFO;
            info->m_id =id;
            info->m_sockfd =clientfd;
            strcpy(info->m_username,rs.name);

            // Phase1: 生成重连 token（rand_r 线程安全）
            {
                unsigned int seed = (unsigned int)(pthread_self() ^ time(NULL));
                int r1 = rand_r(&seed), r2 = rand_r(&seed);
                snprintf(info->m_token, _MAX_SIZE, "%08x%08x", r1, r2);
                strcpy(rs.token, info->m_token);
            }

            m_mapfdtouserinfo.insert(id,info);
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

void CLogic::JoinZoneRq(sock_fd clientfd, char *szbuf, int nlen)
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
void CLogic::LeaveZoneRq(sock_fd clientfd, char *szbuf, int nlen)
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

void CLogic::JoinRoomRq(sock_fd clientfd, char *szbuf, int nlen)
{
    _DEF_COUT_FUNC_
     STRU_JOIN_ROOM_RQ * rq = (STRU_JOIN_ROOM_RQ*)szbuf;
    STRU_JOIN_ROOM_RS rs;
    pthread_mutex_lock(&m_roomListlock);
    list<int>& usrlst = m_roomList[rq->roomid];
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
    printf("%d\n",tmplist.size());
    pthread_mutex_unlock(&m_roomListlock);
    SendData(clientfd,(char*)&rs,sizeof(rs));

    int loginid =rq->userid;
    USER_INFO* info =nullptr;
    if(!m_mapfdtouserinfo.find(loginid,info))
        return ;

    STRU_ROOM_MENBER loginrq;
    loginrq.status = rs.status;
    loginrq.userid =loginid;
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
                return;

            STRU_ROOM_MENBER memrq;
            memrq.userid = curid;
            memrq.status = status;

            strcpy(memrq.name,meminfo->m_username);
            SendData(info->m_sockfd,(char*)&memrq,sizeof(memrq));
            SendData(meminfo->m_sockfd,(char*)&loginrq,sizeof(loginrq));
        }
        else
        {
             loginrq.status = status;
            SendData(info->m_sockfd,(char*)&loginrq,sizeof(loginrq));
        }


    }

    //first 0-120 array look the numberof people
    // 0 is host add into the room list return
    // 1 player join suc and need info  player send to host and host send to player
    // 2 join fail
}

void CLogic::zoneinfoRq(sock_fd clientfd, char *szbuf, int nlen)
{
    _DEF_COUT_FUNC_
    STRU_ZONE_INFO_RQ *rq = (STRU_ZONE_INFO_RQ*)szbuf;
    STRU_ZONE_ROOM_INFO rs;
    for(int i=1;i<m_roomList.size();i++)
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
    list<int>::iterator it;
    list<int>& usrlst = m_roomList[rq->roomid];
    for(auto ite = usrlst.begin();ite!=usrlst.end();ite++)
    {
        if(*ite!=id)
        {
            USER_INFO* info =nullptr;
            if(!m_mapfdtouserinfo.find(*ite,info))
                continue;
            SendData(info->m_sockfd,szbuf,nlen);
        }
        else
        {
            it =ite;
        }
    }
    pthread_mutex_lock(&m_roomListlock);
    usrlst.erase(it);
    pthread_mutex_unlock(&m_roomListlock);
}
void CLogic::FIL_MsgSendRq(sock_fd clientfd, char *szbuf, int nlen)
{

       _DEF_COUT_FUNC_
        //拆包
        STRU_FIL_RQ* rq = (STRU_FIL_RQ*)szbuf;
        //什么专区 什么房间 谁 发了什么
        //根据专区 拿到房间列表  根据房间 拿到房间内成员  转发给房间里所有人
        //rq->zoneid; // 根据专区 拿到房间列表
        list<int>& lstRes = m_roomList[ rq->roomid ];
        for( auto ite = lstRes.begin() ; ite != lstRes.end(); ++ite ){
            int userid = *ite;
            USER_INFO* info = nullptr;
            if( !m_mapfdtouserinfo.find(userid , info ) ) continue;
            SendData( info->m_sockfd , szbuf, nlen );
        }

}
//五子棋落子转发
void CLogic::FIL_PieceDownRq(sock_fd clientfd, char *szbuf, int nlen)
{

_DEF_COUT_FUNC_
    //拆包
    STRU_FIL_PIECEDOWN* rq = (STRU_FIL_PIECEDOWN*)szbuf;
    //什么专区 什么房间 谁 发了什么
    //根据专区 拿到房间列表  根据房间 拿到房间内成员  转发给房间里所有人
    //rq->zoneid; // 根据专区 拿到房间列表
    list<int>& lstRes = m_roomList[ rq->roomid ];
    for( auto ite = lstRes.begin() ; ite != lstRes.end(); ++ite ){
        int userid = *ite;
        USER_INFO* info = nullptr;
        if( !m_mapfdtouserinfo.find(userid , info ) ) continue;
        SendData( info->m_sockfd , szbuf, nlen );
    }
    int roomid = rq->roomid;
    pthread_mutex_lock(&m_roomcachelock);
    auto it = room_cache.find(roomid);
    if(it!=room_cache.end())
    {
        (*it).second.push_back(QPair<int,int>(rq->x,rq->y));
    }
    pthread_mutex_unlock(&m_roomcachelock);
}

void CLogic::FIL_Gameover(sock_fd clientfd, char *szbuf, int nlen)
{
    _DEF_COUT_FUNC_
    STRU_FIL_RQ* rq = (STRU_FIL_RQ*)szbuf;
    //depend on zoneid to find which room is over
    //int zoneid = rq->zoneid;
    int roomid = rq->roomid;
    pthread_mutex_lock(&m_roomcachelock);
    auto it = room_cache.find(rq->roomid);
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
    for(auto ite = m_roomList[rq->roomid].begin()
        ;count<2&&ite!=m_roomList[rq->roomid].end();ite++,count++)
    {
        id[count] = (*ite);
    }

    sprintf(szsql,"insert into t_records(idA,idB,zoneid,roomid,data) values(%d,%d,%d,%d,'%s')"
              ,id[0],id[1],rq->zoneid,rq->roomid,doc.toJson(QJsonDocument::Compact).constData());
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
    room_cache[roomid] = {};
    FIL_MsgSendRq(clientfd,szbuf,nlen);
}

void CLogic::FIL_AllRecordRq(sock_fd clientfd, char *szbuf, int nlen)
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
        if(!m_sql->SelectMysql(szsql,1,nameresult))
        {
            printf("sql select error\n");
        }
        hostname = nameresult.front();
        nameresult.pop_front();
        sprintf(szsql,"select name from t_user where id = %d ",playerid);
        if(!m_sql->SelectMysql(szsql,1,nameresult))
        {
            printf("sql select error\n");
        }
        playername = nameresult.front();
        nameresult.pop_front();
        STRU_FIL_ALLRECORD_RS rs;
        strcpy(rs.c_time,time.c_str());
        strcpy(rs.hostid,hostname.c_str());
        strcpy(rs.playerid,playername.c_str());
        rs.zoneid = zoneid;
        SendData(clientfd,(char*)&rs,sizeof(rs));
    }
}

void CLogic::FIL_SingleRecordRq(sock_fd clientfd, char *szbuf, int nlen)
{
    STRU_FIL_SINGLERECORD_RQ* rq = (STRU_FIL_SINGLERECORD_RQ*)szbuf;
    STRU_FIL_SINGLERECORD_RS rs;
    char szsql[1024];
    sprintf(szsql,"select idA,idB,data from t_records where (idA = %d or idB = %d) and end_time = '%s' ",rq->userid,rq->userid,rq->time);
    list<string>result;
    if(!m_sql->SelectMysql(szsql,3,result))
    {
        printf("sql select error\n");

        rs.result =0;
        SendData(clientfd,(char*)&rs,sizeof(rs));

    }
    else
        rs.result =1;


    int hostid = stoi(result.front());
    result.pop_front();
    int playerid = stoi(result.front());
    result.pop_front();
    string jsonstr = result.front();
    result.pop_front();

    string hostname;
    string playername;

    sprintf(szsql,"select name from t_user where id = %d ",hostid);//sql can be better
    if(!m_sql->SelectMysql(szsql,1,result))
    {
        printf("sql select error\n");
    }
    hostname = result.front();
    result.pop_front();
    sprintf(szsql,"select name from t_user where id = %d ",playerid);
    if(!m_sql->SelectMysql(szsql,1,result))
    {
        printf("sql select error\n");
    }
    playername = result.front();
    result.pop_front();
    strcpy(rs.hostname,hostname.c_str());
    strcpy(rs.playername,playername.c_str());
    rs.hostid = hostid;
    rs.playerid = playerid;

    SendData(clientfd,(char*)&rs,sizeof(rs));

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(
           QByteArray::fromStdString(jsonstr),
           &parseError
       );
    if (parseError.error != QJsonParseError::NoError) {
           qDebug() << "JSON 解析失败：" << parseError.errorString();
           return ; // 解析失败返回空
       }

       // 2. 必须是数组类型
       if (!doc.isArray()) {
           qDebug() << "JSON 不是数组格式";
           return ;
       }

       // 3. 遍历数组，逐个解析对象
       QJsonArray arr = doc.array();
       int count =0;
       for (const QJsonValue &val : arr) {
           if (!val.isObject()) continue; // 跳过非对象元素

           QJsonObject obj = val.toObject();
           int x,y;
           x = obj["x"].toInt(0); // 默认值0，防止字段不存在
           y = obj["y"].toInt(0);
           STRU_FIL_PIECEDOWN rq;
           rq.x =x;
           rq.y =y;
           rq.color = count;
           SendData(clientfd,(char*)&rq,sizeof(rq));
           count++;
       }
}
// ============================================================
// Phase1: 心跳 + 掉线处理
// ============================================================
void CLogic::HeartBeatRq(sock_fd clientfd, char *szbuf, int nlen)
{
    STRU_FIL_HEARTBEAT* rq = (STRU_FIL_HEARTBEAT*)szbuf;
    pthread_mutex_lock(&m_heartbeatLock);
    m_heartbeatMap[rq->userid] = time(NULL);
    pthread_mutex_unlock(&m_heartbeatLock);
    // 纯保活，不回复
}

void CLogic::CheckHeartBeat()
{
    time_t now = time(NULL);
    vector<int> timeoutUsers;

    // 1. 遍历心跳表，找出超时用户
    pthread_mutex_lock(&m_heartbeatLock);
    for (auto& kv : m_heartbeatMap)
    {
        if (now - kv.second > 15)
            timeoutUsers.push_back(kv.first);
    }
    pthread_mutex_unlock(&m_heartbeatLock);

    // 2. 逐个处理
    for (int userid : timeoutUsers)
    {
        printf("User %d heartbeat timeout, disconnecting...\n", userid);
        HandleDisconnect(userid);
    }
}

void CLogic::HandleDisconnect(int userid)
{
    USER_INFO* info = nullptr;
    if (!m_mapfdtouserinfo.find(userid, info))
        return;

    sock_fd old_fd = info->m_sockfd;

    // 清理业务数据
    m_mapfdtouserinfo.erase(userid);
    pthread_mutex_lock(&m_fdLock);
    m_fdToUserid.erase(old_fd);
    pthread_mutex_unlock(&m_fdLock);
    pthread_mutex_lock(&m_heartbeatLock);
    m_heartbeatMap.erase(userid);
    pthread_mutex_unlock(&m_heartbeatLock);
    delete info;

    // 网络层关闭 fd（摘 epoll + close + 清 myevent_s）
    m_tcp->CloseFd(old_fd);
}

// 静态回调：recv_task 检测到 fd 自然断开时，清理该用户的业务数据
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
        logic->m_mapfdtouserinfo.erase(userid);
        delete info;
    }

    pthread_mutex_lock(&logic->m_heartbeatLock);
    logic->m_heartbeatMap.erase(userid);
    pthread_mutex_unlock(&logic->m_heartbeatLock);
}

void* CLogic::HeartBeatThread(void* arg)
{
    CLogic* pthis = (CLogic*)arg;
    while (!pthis->m_heartbeatStop)
    {
        sleep(10);  // 每 10 秒检查一次
        pthis->CheckHeartBeat();
    }
    return NULL;
}
