#ifndef CLOGIC_H
#define CLOGIC_H

#include"TCPKernel.h"
#include<unordered_map>
#include<map>
#include<vector>
#include<QPair>
class CLogic
{
public:
    CLogic( TcpKernel* pkernel ):m_roomList(121)
    {
        m_pKernel = pkernel;
        m_sql = pkernel->m_sql;
        m_tcp = pkernel->m_tcp;
        pthread_mutex_init(&m_roomListlock,NULL);
        pthread_mutex_init(&m_roomcachelock,NULL);
        pthread_mutex_init(&m_heartbeatLock,NULL);
        pthread_mutex_init(&m_fdLock,NULL);
        m_heartbeatStop = false;
        // 注册 fd 关闭回调：recv_task 检测到自然断开时清理业务数据
        m_tcp->SetCloseCallback(CLogic::OnFdClosed);
        pthread_create(&m_heartbeatThread, NULL, HeartBeatThread, this);
    }
public:
    //设置协议映射
    void setNetPackMap();
    /************** 发送数据*********************/
    void SendData( sock_fd clientfd, char*szbuf, int nlen )
    {
        m_pKernel->SendData( clientfd ,szbuf , nlen );
    }
    /************** 网络处理 *********************/
    //注册
    void RegisterRq(sock_fd clientfd, char*szbuf, int nlen);
    //登录
    void LoginRq(sock_fd clientfd, char*szbuf, int nlen);
    void JoinZoneRq(sock_fd clientfd, char*szbuf, int nlen);
    void LeaveZoneRq(sock_fd clientfd, char *szbuf, int nlen);
    void JoinRoomRq(sock_fd clientfd, char *szbuf, int nlen);
    void zoneinfoRq(sock_fd clientfd, char *szbuf, int nlen);

    /*******************************************/

    void LeaveRoomRq(sock_fd clientfd, char *szbuf, int nlen);
    void FIL_MsgSendRq(sock_fd clientfd, char *szbuf, int nlen);
    void FIL_PieceDownRq(sock_fd clientfd, char *szbuf, int nlen);
    void FIL_Gameover(sock_fd clientfd, char *szbuf, int nlen);
    void FIL_GameStart(sock_fd clientfd, char *szbuf, int nlen);
    void FIL_AllRecordRq(sock_fd clientfd, char *szbuf, int nlen);
    void FIL_SingleRecordRq(sock_fd clientfd, char *szbuf, int nlen);
    void HeartBeatRq(sock_fd clientfd, char *szbuf, int nlen);
    // Phase1: 定时扫描心跳，清理超时用户
    void CheckHeartBeat();
    void HandleDisconnect(int userid);
    // 静态回调：recv_task 检测到 fd 断开时清理业务数据
    static void OnFdClosed(sock_fd fd);
    private:
    TcpKernel* m_pKernel;
    CMysql * m_sql;
    Block_Epoll_Net * m_tcp;
    MyMap<int ,USER_INFO*> m_mapfdtouserinfo;
    pthread_mutex_t m_roomListlock;
    pthread_mutex_t m_roomcachelock;
    vector<list<int>>m_roomList;
    unordered_map<int,vector<QPair<int,int>>>room_cache;
    // Phase1: 心跳超时检测
    std::map<int, time_t>  m_heartbeatMap;   // userid → last_heartbeat
    std::map<sock_fd, int> m_fdToUserid;     // fd → userid 反向索引
    pthread_mutex_t         m_heartbeatLock;
    pthread_mutex_t         m_fdLock;         // 保护 m_fdToUserid
    pthread_t               m_heartbeatThread;
    bool                    m_heartbeatStop;
    static void* HeartBeatThread(void* arg);
};


#endif // CLOGIC_H
