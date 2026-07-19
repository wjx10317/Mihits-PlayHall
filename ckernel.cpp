#include "ckernel.h"
#include"TcpClientMediator.h"
#include"netapi/net/packdef.h"
#include<QMessageBox>
#include<QDebug>
#include"md5/md5.h"
/*
#include<QMetaType>
qRegisterMetaType<classname>("classname");
*/
#include<QSettings>
#include<QCoreApplication>
#include<QFileInfo>
#include<QApplication>
#include<QKeyEvent>
#include<QDir>
#include<QProcess>
#include<QProgressDialog>
#include"gamepackageupdater.h"
void CKernel::ConfigSet()
{
    //获取配置文件的信息以及设置
    //.ini 配置文件
    //[net] 组名 GroupName
    //key=value

    //1.ip默认
    strcpy( m_serverIP , _DEF_SERVER_IP );
    //2.设置和获取配置文件  有还是没有 配置文件在哪里？ 设置和exe同一级的目录
    //exe的目录
    QString path =QCoreApplication::applicationDirPath()+"/config.ini";
    QFileInfo info(path);
    if(info.exists())
    {
        QSettings setting(path,QSettings::IniFormat,nullptr);
        setting.beginGroup("net");
        QVariant var =setting.value("ip");
        QString strip = var.toString();
        if(!strip.isEmpty())
        {
            strcpy(m_serverIP,strip.toStdString().c_str());

        }
        setting.endGroup();
    }
    else
    {
        QSettings setting(path,QSettings::IniFormat,nullptr);
        setting.beginGroup("net");
        setting.setValue("ip",QString::fromStdString(m_serverIP));
        setting.endGroup();
    }
}

/*static std::string getmd5(QString str)
{
    MD5 md(str.toStdString());
    return md.toString();
}*/
CKernel::CKernel(QObject *parent):
    QObject(parent),m_dialog(new Dialog),m_logindialog(new LoginDialog),
    m_roomdialog(new roomDialog),m_fiveinlinezone(new fiveinlinezone),m_client(new TcpClientMediator),
    m_id(0),m_roomid(0),m_zoneid(0),m_reconnecting(false),m_reconnectInGame(false)

{
    m_logindialog->show();
    ConfigSet();
    m_client->OpenNet(m_serverIP,_DEF_TCP_PORT);
    connect(m_client,SIGNAL(SIG_ReadyData( unsigned int, char*, int)),
            this,SLOT(slot_deal_readydata(unsigned int, char*, int)));
    connect(m_dialog,SIGNAL(SIG_CLOSE()),
            this,SLOT(Des_Instance()));
    connect(m_logindialog,SIGNAL(SIG_CLOSE()),
            this,SLOT(Des_Instance()));
    connect(m_logindialog,SIGNAL(SIG_LOGIN_COMMIT(QString,QString)),
            this,SLOT(slot_loginRq(QString,QString)));
    connect(m_logindialog,SIGNAL(SIG_REGISTER_COMMIT(QString,QString,QString)),
            this,SLOT(slot_registerRq(QString,QString,QString)));
    connect(m_dialog,SIGNAL(SIG_JOINZONE(int)),
            this,SLOT(slot_joinzone(int)));
    connect(m_dialog,SIGNAL(SIG_ENTER_EXTERNAL_ZONE(int)),
            this,SLOT(slot_enterExternalZone(int)));
    connect(m_fiveinlinezone,SIGNAL(SIG_ZONE_JOINROOM(int)),
            this,SLOT(slot_joinroom(int)));
    connect(m_fiveinlinezone,SIGNAL(SIG_CLOSE()),
            this,SLOT(slot_leavezone()));
    connect(m_roomdialog,SIGNAL(SIG_CLOSE()),this,SLOT(slot_leaveroom()));
    connect(m_roomdialog,SIGNAL(SIG_gameReady(int,int,int)),
            this,SLOT(slot_sendgameready(int,int,int)));
    connect(m_roomdialog,SIGNAL(SIG_gameStart(int,int)),
            this,SLOT(slot_sendgamestart(int,int)));
    connect(m_roomdialog,SIGNAL(SIG_gamenotReady(int,int,int)),
            this,SLOT(slot_sendgamenotready(int,int,int)));
    connect(m_roomdialog,SIGNAL(SIG_PIECEDOWN(int ,int,int)),
            this,SLOT(slot_sendpiecedown(int,int,int) ));
    connect(m_roomdialog,SIGNAL( SIG_PLAYBYCPUBEGIN(int,int,int) ),
            this,SLOT(slot_PlayByCpuBegin(int,int,int)));
    connect(m_roomdialog,SIGNAL( SIG_PLAYBYCPUEND(int,int,int) ),
            this,SLOT(slot_PlayByCpuEnd(int,int,int)));
    connect(&m_rqtimer,SIGNAL(timeout()),this,SLOT( slot_roominfozone() ));
    connect(m_roomdialog,SIGNAL(SIG_GAMEOVER()),this,SLOT(slot_gameoverrq()));
    connect(m_fiveinlinezone,SIGNAL(SIG_ALLRECORD()),this,SLOT(slot_sendrecordrq()));
    connect(m_fiveinlinezone,SIGNAL(SIG_SINGLERECORD(QString)),this,SLOT(slot_sendsinglerecordrq(QString)));
    connect(m_roomdialog,SIGNAL(SIG_SHOWBACK()),this,SLOT(slot_reshowwindow()));
    // Phase1: 心跳 + 断线检测
    connect(&m_heartbeatTimer, SIGNAL(timeout()), this, SLOT(slot_sendHeartbeat()));
    connect(m_client, SIGNAL(SIG_disConnect()), this, SLOT(slot_disConnect()));
    qApp->installEventFilter(this);
    setnetpackmap();

    m_gameUpdater = new GamePackageUpdater(this);
    m_externalProcess = new QProcess(this);
    connect(m_gameUpdater, &GamePackageUpdater::finished,
            this, &CKernel::slot_externalUpdateFinished);
    connect(m_gameUpdater, &GamePackageUpdater::statusMessage, this, [this](const QString &msg) {
        if (m_externalProgress)
            m_externalProgress->setLabelText(msg);
    });
    connect(m_gameUpdater, &GamePackageUpdater::progress, this,
            [this](qint64 rec, qint64 total, const QString &) {
                if (!m_externalProgress)
                    return;
                m_externalProgress->setMaximum(total > 0 ? static_cast<int>(total) : 0);
                m_externalProgress->setValue(static_cast<int>(rec));
            });
    connect(m_externalProcess,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &CKernel::slot_externalProcessFinished);
}
bool CKernel::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress && m_id != 0)
    {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_F12 && keyEvent->modifiers() == Qt::NoModifier)
        {
            qDebug() << "[DEV] F12 simulate disconnect";
            m_client->blockSignals(true);
            m_client->CloseNet();
            m_client->blockSignals(false);
            QMetaObject::invokeMethod(this, "slot_disConnect", Qt::QueuedConnection);
            return true;
        }
    }
    return QObject::eventFilter(obj, event);
}
void CKernel::Des_Instance()
{
    qDebug()<<__func__;
    delete m_dialog;
    delete m_client;
    delete m_logindialog;
}
void CKernel::setnetpackmap()
{
    m_NetPackMap[ _DEF_PACK_REGISTER_RS - _DEF_PACK_BASE ]= &CKernel::slot_registerRs;
    m_NetPackMap[ _DEF_PACK_LOGIN_RS - _DEF_PACK_BASE ]= &CKernel::slot_loginRs;
    m_NetPackMap[ DEF_JOIN_ROOM_RS - _DEF_PACK_BASE ]= &CKernel::slot_joinroomrs;
    m_NetPackMap[ DEF_ROOM_MEMBER - _DEF_PACK_BASE ]= &CKernel::slot_roommemberrq;
    m_NetPackMap[ DEF_LEAVE_ROOM_RQ - _DEF_PACK_BASE ]= &CKernel::slot_leaveroomrq;
    m_NetPackMap[ DEF_ZONE_ROOM_INFO - _DEF_PACK_BASE ]= &CKernel::slot_dealzoneroominfo;
    m_NetPackMap[ DEF_FIL_ROOM_READY  -_DEF_PACK_BASE]=&CKernel::slot_dealroomready;
    m_NetPackMap[ DEF_FIL_ROOM_NOTREADY  -_DEF_PACK_BASE]=&CKernel::slot_dealroomnotready;
    m_NetPackMap[ DEF_FIL_GAME_START  -_DEF_PACK_BASE]=&CKernel::slot_dealroomstart;
    m_NetPackMap[ DEF_FIL_AI_BEGIN    -_DEF_PACK_BASE]=&CKernel::slot_dealaibegin;
    m_NetPackMap[ DEF_FIL_AI_END      -_DEF_PACK_BASE]=&CKernel::slot_dealaiend;
    //m_NetPackMap[ DEF_FIL_DISCARD_THIS-_DEF_PACK_BASE]=&CKernel::slot_
    //m_NetPackMap[ DEF_FIL_SURREND     -_DEF_PACK_BASE]=&CKernel::slot_
    m_NetPackMap[ DEF_FIL_PIECEDOWN   -_DEF_PACK_BASE]=&CKernel::slot_dealpiecedown;
    m_NetPackMap[DEF_FIL_ALLRECORD_RS - _DEF_PACK_BASE] = &CKernel::slot_deal_allrecordrs;
    m_NetPackMap[DEF_FIL_SINGLERECORD_RS - _DEF_PACK_BASE] = &CKernel::slot_deal_singlerecordrs;
    m_NetPackMap[DEF_FIL_RECONNECT_RS - _DEF_PACK_BASE] = &CKernel::slot_reconnectRs;
    m_NetPackMap[DEF_FIL_OPPONENT_DISCONNECT - _DEF_PACK_BASE] = &CKernel::slot_opponentDisconnect;
    m_NetPackMap[DEF_GAME_VERSION_RS - _DEF_PACK_BASE] = &CKernel::slot_gameVersionRs;

}
void CKernel::sendData(char* buf , int nlen )
{
    m_client->SendData(0,buf,nlen);
}
void CKernel::slot_deal_readydata(unsigned int lSendIP , char* buf , int nlen)
{
    int type = *(int*)buf;
    if(type>=_DEF_PACK_BASE&&type<_DEF_PACK_BASE+_DEF_PACK_COUNT)
    {
        PFUN pf = NetPackMap(type);
        if(pf==nullptr)
        {

            qDebug()<<"fun ptr not inititalized:"<<type;
            return ;
        }
        (this->*pf)(lSendIP,buf,nlen);
    }
    else
    {
            qDebug()<<"protocol types err";
    }
    delete [] buf;
}
void CKernel::slot_registerRq(QString tel,QString password,QString nick)
{
    STRU_REGISTER_RQ rq;
    strcpy_s(rq.tel,tel.toStdString().c_str());
     strcpy_s(rq.password,password.toStdString().c_str());
    //兼容中文
    std::string strname = nick.toStdString();
    strcpy_s(rq.name,strname.c_str());
    qDebug()<<"send register request";
    sendData((char*)&rq,sizeof(rq));
}
void CKernel::slot_loginRq(QString tel,QString password)
{
    STRU_LOGIN_RQ rq;
    strcpy_s(rq.tel,tel.toStdString().c_str());
    strcpy_s(rq.password,password.toStdString().c_str());
     sendData((char*)&rq,sizeof(rq));

}
void CKernel::slot_registerRs(unsigned int , char* buf , int )
{
    STRU_REGISTER_RS rs = *(STRU_REGISTER_RS*)buf;
    switch(rs.result)
    {
        case tel_is_exist:
        QMessageBox::about(this->m_logindialog,"提示","电话号码已存在");
        break;
        case name_is_exist:
        QMessageBox::about(this->m_logindialog,"提示","昵称已存在");
        break;
        case register_success:
         QMessageBox::about(this->m_logindialog,"提示","注册成功");
        break;
        default:
        QMessageBox::about(this->m_logindialog,"提示","注册异常");
        break;
    }


}
void CKernel::slot_loginRs(unsigned int , char* buf , int)
{
    STRU_LOGIN_RS rs = *(STRU_LOGIN_RS*)buf;
    switch(rs.result)
    {
        case user_not_exist:
        QMessageBox::about(this->m_logindialog,"提示","用户不存在");
        break;
        case password_error:
        QMessageBox::about(this->m_logindialog,"提示","密码错误");
        break;
        case login_success:
        m_logindialog->hide();
        m_dialog->show();
        m_id = rs.userid;
        m_username = QString::fromStdString(rs.name);
        strcpy_s(m_reconnectToken, rs.token);
        // 启动心跳（每 5 秒发一次）
        m_heartbeatTimer.start(_DEF_HEARTBEAT_INTERVAL_MS);
        break;
        default:
        QMessageBox::about(this->m_logindialog,"提示","登录异常");
        break;
    }

}

void CKernel::slot_joinzone(int zoneid)
{
    m_zoneid =zoneid;
    STRU_JOIN_ZONE rq;
    rq.userid = m_id;
    rq.zoneid = zoneid;
    sendData((char*)&rq,sizeof(rq));
    switch(zoneid)
    {
        case Five_In_Line:
            m_rqtimer.start(1000);
            m_fiveinlinezone->show();
        break;
        default:
        break;
    }



}

void CKernel::slot_joinroom(int roomid)
{
    //m_fiveinlinezone->hide();
    //加入成功后隐藏
    if(m_roomid!=0)
    {
        QMessageBox::about(nullptr,"提示","已经在房间，无法重复加入");
        return;
    }

    STRU_JOIN_ROOM_RQ rq;
    rq.userid = m_id;
    rq.roomid =roomid;
    sendData((char*)&rq,sizeof(rq));
}
void CKernel::slot_leavezone()
{
    m_rqtimer.stop();
    //成员属性修改
    m_zoneid =0;

    //请求
    STRU_LEAVE_ZONE rq;
    rq.userid = m_id;
    sendData((char*)&rq,sizeof(rq));
    //ui跳转
    m_dialog->show();
}
void CKernel::slot_joinroomrs( unsigned int , char* buf , int )
{
    STRU_JOIN_ROOM_RS* rs = (STRU_JOIN_ROOM_RS*)buf;
    if(rs->result==0)
    {
        QMessageBox::about(m_fiveinlinezone,"提示","加入房间失败");
        return;
    }
    if(rs->status==_host)
    {
        m_ishost =true;
    }
    m_roomid = rs->roomid;

//根据zoneid找到需要隐藏的分区
    m_fiveinlinezone->hide();
    //同理
    m_roomdialog->show();
    m_roomdialog->setinfo(m_roomid);


}
void CKernel::slot_roommemberrq( unsigned int  , char* buf , int )
{
    STRU_ROOM_MENBER* rq = (STRU_ROOM_MENBER*)buf;
    if (rq->userid == -1)
    {
        m_reconnecting = false;
        if (m_reconnectInGame)
        {
            m_roomdialog->setGameStart();
            m_reconnectInGame = false;
        }
        return;
    }
    if(rq->status==_host)
    {
        m_roomdialog->setHostInfo(rq->userid,QString::fromStdString(rq->name));
        m_roomdialog->addmem(rq->userid,QString::fromStdString(rq->name));
    }
    else if(rq->status==_player)
    {
        m_roomdialog->setPlayerInfo(rq->userid,QString::fromStdString(rq->name));
        m_roomdialog->addmem(rq->userid,QString::fromStdString(rq->name));
    }
    else
    {
        m_roomdialog->addmem(rq->userid,QString::fromStdString(rq->name));
    }

    if(rq->userid==m_id)
    {
        m_roomdialog->setUserStatus(rq->status);
        m_ishost = (rq->status == _host);
    }
}
void CKernel::slot_leaveroom()
{
    //这个人主动离开
    STRU_LEAVE_ROOM_RQ rq;
    rq.status = m_roomdialog->getStatus();
    rq.userid = m_id;
    rq.roomid = m_roomid;
    sendData((char*)&rq , sizeof(rq));

    //界面
    m_roomdialog->clearroom();
    m_roomdialog->hide();
    m_fiveinlinezone->show();

    //后台数据
    m_roomid = 0;
    m_ishost = false;
}
void CKernel::slot_leaveroomrq(unsigned int, char* buf , int)
{//补加判断游戏是否开始，否则直接判负结束游戏，不保存

    STRU_LEAVE_ROOM_RQ* rq =(STRU_LEAVE_ROOM_RQ*)buf;
    if (rq->status <= _player)
        m_roomdialog->clear_pushbutton(true);
    if(rq->status==_host)
    {
        if(m_roomdialog->getStatus()==_player)
        {
            m_ishost =true;
            m_roomdialog->setUserStatus(_host);
            m_roomdialog->setHostInfo(m_id,m_username);
            m_roomdialog->clearPlayerInfo();
            m_roomdialog->rmmem(rq->userid);
            m_roomdialog->loadnewPlayerInfo();

        }
        else if(m_roomdialog->getStatus()==_spec)
        {
            if(m_roomdialog->isfirstspec(m_id))
            {
                m_roomdialog->setUserStatus(_player);
            }
            m_roomdialog->setPlayerInfotoHost();
            m_roomdialog->clearPlayerInfo();
            m_roomdialog->rmmem(rq->userid);
            m_roomdialog->loadnewPlayerInfo();


        }
    }
    else if(rq->status==_player)
    {
        if(m_roomdialog->getStatus()== _host)
        {
            m_roomdialog->clearPlayerInfo();
            m_roomdialog->rmmem(rq->userid);
            m_roomdialog->loadnewPlayerInfo();
        }
        else if(m_roomdialog->getStatus()== _spec)
        {
            if(m_roomdialog->isfirstspec(m_id))
            {
                m_roomdialog->setUserStatus(_player);
            }
             m_roomdialog->clearPlayerInfo();
             m_roomdialog->rmmem(rq->userid);
             m_roomdialog->loadnewPlayerInfo();
        }

    }
    else
    {
        m_roomdialog->rmmem(rq->userid);
    }




}
void CKernel::slot_sendgameready(int zoneid,int roomid,int userid)
{
    STRU_FIL_RQ rq(DEF_FIL_ROOM_READY);
    rq.zoneid = zoneid;
    rq.roomid =roomid;
    rq.userid =userid;
    sendData((char*)&rq,sizeof(rq));
}
void CKernel::slot_sendgamestart(int zoneid, int roomid)
{
    STRU_FIL_RQ rq(DEF_FIL_GAME_START);
    rq.zoneid = zoneid;
    rq.roomid =roomid;
    sendData((char*)&rq,sizeof(rq));
}
void CKernel::slot_sendgamenotready(int zoneid,int roomid,int userid)
{
    STRU_FIL_RQ rq(DEF_FIL_ROOM_NOTREADY);
    rq.zoneid = zoneid;
    rq.roomid =roomid;
    rq.userid =userid;
    sendData((char*)&rq,sizeof(rq));
}
void CKernel::slot_dealroomready(unsigned int , char* buf , int )
{
    // 拆包
    qDebug()<<__func__;
    STRU_FIL_RQ* rq = (STRU_FIL_RQ*)buf;
    // 什么专区 什么房间 谁 做了什么事
    if( rq->roomid == m_roomid ){
        m_roomdialog->setPlayerReady( rq->userid );
    }
}
void CKernel::slot_dealroomnotready(unsigned int, char* buf , int)
{
    // 拆包
    STRU_FIL_RQ* rq = (STRU_FIL_RQ*)buf;
    // 什么专区 什么房间 谁 做了什么事
    if( rq->roomid == m_roomid ){
        m_roomdialog->delPlayerReady( rq->userid );
    }
}
void CKernel::slot_dealroomstart(unsigned int, char* buf , int)
{
    STRU_FIL_RQ* rq = (STRU_FIL_RQ*)buf;
    // 什么专区 什么房间 谁 做了什么事
    if( rq->roomid == m_roomid ){
        m_roomdialog->setGameStart();
    }
}
void CKernel::slot_dealpiecedown(unsigned int, char* buf , int)
{
    STRU_FIL_PIECEDOWN* rq = (STRU_FIL_PIECEDOWN*)buf;
    m_roomdialog->slot_piecedown(rq->color,rq->x,rq->y);
}

void CKernel::slot_gameoverrq()
{
    STRU_FIL_RQ rq(DEF_FIL_WIN);
    rq.roomid = m_roomid;
    rq.userid = m_id;
    rq.zoneid = m_zoneid;
    sendData((char*)&rq,sizeof(rq));
}


void CKernel::slot_sendpiecedown(int blackorwhite, int x , int y)
{
    STRU_FIL_PIECEDOWN rq;
    rq.color =blackorwhite;
    rq.roomid =m_roomid;
    rq.userid = m_id;
    rq.x =x;
    rq.y =y;
    rq.zoneid = m_zoneid;
    sendData((char*)&rq,sizeof(rq));
}

void CKernel::slot_PlayByCpuBegin(int zoneid, int roomid, int userid)
{
    //发包
    STRU_FIL_RQ rq(DEF_FIL_AI_BEGIN);
    rq.roomid = roomid;
    rq.userid = userid;
    rq.zoneid = zoneid;

    sendData( (char*)&rq , sizeof(rq) );
}

void CKernel::slot_PlayByCpuEnd(int zoneid, int roomid, int userid)
{
    STRU_FIL_RQ rq(DEF_FIL_AI_END);
    rq.roomid = roomid;
    rq.userid = userid;
    rq.zoneid = zoneid;

    sendData( (char*)&rq , sizeof(rq) );
}

void CKernel::slot_dealaibegin(unsigned int, char *buf, int)
{
    //拆包
    qDebug()<<__func__;
    STRU_FIL_RQ * rq = (STRU_FIL_RQ *)buf;
    //查看身份
    //rq->zoneid; // 根据专区 看房间
   // rq->roomid; // 根据房间看ui
    if( m_id == rq->userid ){
        if( m_ishost ){
            m_roomdialog->setHostPlayByCpu( true );
        }else{
            m_roomdialog->setPlayerPlayByCpu( true );
        }
    }else{
        if( m_ishost ){
            m_roomdialog->setPlayerPlayByCpu( true );
        }else{
            m_roomdialog->setHostPlayByCpu( true );
        }
    }
}

void CKernel::slot_dealaiend(unsigned int, char *buf, int)
{
    //拆包
    STRU_FIL_RQ * rq = (STRU_FIL_RQ *)buf;
    //查看身份
    //rq->zoneid; // 根据专区 看房间
    //rq->roomid; // 根据房间看ui
    if( m_id == rq->userid ){
        if( m_ishost ){
            m_roomdialog->setHostPlayByCpu( false );
        }else{
            m_roomdialog->setPlayerPlayByCpu( false );
        }
    }else{
        if( m_ishost ){
            m_roomdialog->setPlayerPlayByCpu( false );
        }else{
            m_roomdialog->setHostPlayByCpu( false );
        }
    }
}

void CKernel::slot_roominfozone()
{
    STRU_ZONE_INFO_RQ rq;
    rq.zoneid = m_zoneid;
    sendData((char*)&rq,sizeof(rq));
}

void CKernel::slot_sendrecordrq()
{
    STRU_FIL_ALLRECORD_RQ rq;
    rq.userid = m_id;
    sendData((char*)&rq,sizeof(rq));
}

void CKernel::slot_sendsinglerecordrq(QString time)
{
    STRU_FIL_SINGLERECORD_RQ rq;
    sprintf_s(rq.time,time.toStdString().c_str());
    rq.userid = m_id;
    sendData((char*)&rq,sizeof(rq));
}

void CKernel::slot_deal_allrecordrs(unsigned int, char *buf, int)
{
    STRU_FIL_ALLRECORD_RS* rs = (STRU_FIL_ALLRECORD_RS*)buf;

    m_fiveinlinezone->addrecord(QString(rs->c_time),QString(rs->hostid),QString(rs->playerid));
}

void CKernel::slot_deal_singlerecordrs(unsigned int, char *buf, int)
{
    STRU_FIL_SINGLERECORD_RS* rs = (STRU_FIL_SINGLERECORD_RS*)buf;
    if(rs->result)
    {
        m_roomdialog->show();
        m_roomdialog->setrecordstatus(QString(rs->hostname),QString(rs->playername),rs->hostid,rs->playerid);
        m_fiveinlinezone->hide();
        m_fiveinlinezone->hidelist();


    }
    else
       QMessageBox::information(nullptr, "提示", "对局数据异常，暂不支持查看");


}

void CKernel::slot_reshowwindow()
{
    m_fiveinlinezone->show();
}

void CKernel::slot_dealzoneroominfo(unsigned int, char *buf, int)
{
    STRU_ZONE_ROOM_INFO * rq = (STRU_ZONE_ROOM_INFO*)buf;
    std::vector<roomitem*>&vec = m_fiveinlinezone->getvecroomitem();
    for(int i=1;i<DEF_ZONE_ROOM_COUNT;i++)
    {
        vec[i]->setroomitem(rq->roomInfo[i]);
    }
}

// ============================================================
// Phase1: 心跳 + 断线处理
// ============================================================
void CKernel::slot_sendHeartbeat()
{
    if (m_id == 0) return;  // 未登录
    STRU_FIL_HEARTBEAT hb;
    hb.userid = m_id;
    hb.zoneid = m_zoneid;
    hb.roomid = m_roomid;
    sendData((char*)&hb, sizeof(hb));
}

void CKernel::slot_opponentDisconnect(unsigned int, char *buf, int)
{
    STRU_FIL_OPPONENT_DISCONNECT* rq = (STRU_FIL_OPPONENT_DISCONNECT*)buf;
    if (rq->roomid != m_roomid)
        return;

    if (rq->kind == DEF_DISCONNECT_SOFT)
    {
        if (rq->status <= _player)
            m_roomdialog->setMemberOffline(rq->userid);
        return;
    }

    if (rq->kind == DEF_DISCONNECT_REONLINE)
    {
        if (rq->status <= _player)
            m_roomdialog->setMemberOnline(rq->userid);
        return;
    }

    if (rq->kind == DEF_DISCONNECT_HARD && rq->status <= _player)
    {
        STRU_LEAVE_ROOM_RQ leave;
        leave.userid = rq->userid;
        leave.status = rq->status;
        leave.roomid = rq->roomid;
        slot_leaveroomrq(0, (char*)&leave, sizeof(leave));
    }
}

void CKernel::slot_reconnectRs(unsigned int, char *buf, int)
{
    STRU_FIL_RECONNECT_RS* rs = (STRU_FIL_RECONNECT_RS*)buf;

    if (rs->result == 0)
    {
        QMessageBox::warning(m_logindialog, "提示", "重连失败，请重新登录");
        m_heartbeatTimer.stop();
        m_roomdialog->hide();
        m_fiveinlinezone->hide();
        m_dialog->hide();
        m_logindialog->show();
        m_id = 0;
        m_roomid = 0;
        m_zoneid = 0;
        return;
    }

    m_zoneid = rs->zoneid;
    m_roomid = rs->roomid;

    if (rs->zoneid == 0)
    {
        m_roomdialog->hide();
        m_fiveinlinezone->hide();
        m_dialog->show();
        m_rqtimer.stop();
        return;
    }

    if (rs->roomid == 0)
    {
        m_dialog->hide();
        m_roomdialog->hide();
        m_fiveinlinezone->show();
        m_rqtimer.start(1000);
        slot_roominfozone();
        return;
    }

    m_dialog->hide();
    m_fiveinlinezone->hide();
    m_reconnectInGame = (rs->inGame == 1);
    m_roomdialog->clearroom();
    m_roomdialog->setinfo(rs->roomid);
    m_roomdialog->show();
    m_reconnecting = true;
}

void CKernel::slot_disConnect()
{
    // 停止心跳，避免向死连接发包
    m_heartbeatTimer.stop();

    // 弹窗提示
    QMessageBox msg;
    msg.setWindowTitle("连接断开");
    msg.setText("与服务器的连接已断开，是否尝试重连？");
    msg.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msg.setDefaultButton(QMessageBox::Yes);

    if (msg.exec() == QMessageBox::Yes)
    {
        // 重建 TCP 连接
        m_client->CloseNet();
        // 重新 OpenNet，用缓存的服务器 IP 和端口
        if (m_client->OpenNet(m_serverIP, _DEF_TCP_PORT))
        {
            // 重连成功，发重连请求
            STRU_FIL_RECONNECT_RQ rq;
            rq.userid = m_id;
            strcpy(rq.token, m_reconnectToken);
            sendData((char*)&rq, sizeof(rq));
            // 重新启动心跳
            m_heartbeatTimer.start(_DEF_HEARTBEAT_INTERVAL_MS);
        }
        else
        {
            QMessageBox::warning(nullptr, "重连失败", "无法连接到服务器，请重启程序");
            exit(0);
        }
    }
    else
    {
        exit(0);
    }
}

void CKernel::slot_sendGameVersionRq(int zoneid)
{
    if (m_id == 0)
        return;
    STRU_GAME_VERSION_RQ rq;
    rq.userid = m_id;
    rq.zoneid = zoneid;
    sendData((char*)&rq, sizeof(rq));
}

void CKernel::slot_gameVersionRs(unsigned int, char *buf, int)
{
    STRU_GAME_VERSION_RS* rs = (STRU_GAME_VERSION_RS*)buf;
    if (!m_externalLaunching || rs->zoneid != m_pendingExternalZone)
        return;

    if (rs->result != 1)
    {
        abortExternalEnter(QString("版本查询失败 zoneid=%1").arg(rs->zoneid));
        return;
    }

    m_pendingExeName = QString::fromLocal8Bit(rs->exe_name);
    const QString manifestUrl = QString::fromLocal8Bit(rs->manifest_url);
    if (m_externalProgress)
        m_externalProgress->setLabelText(QString("更新中: %1").arg(rs->serverVersion));
    m_gameUpdater->startUpdate(manifestUrl, gamesRootPath());
}

QString CKernel::gamesRootPath() const
{
    return QDir(QCoreApplication::applicationDirPath()).filePath("games");
}

void CKernel::slot_enterExternalZone(int zoneid)
{
    if (m_id == 0)
        return;
    if (m_externalLaunching || (m_externalProcess && m_externalProcess->state() != QProcess::NotRunning))
    {
        QMessageBox::about(m_dialog, "提示", "外部游戏正在运行或更新中");
        return;
    }
    if (m_zoneid != 0)
    {
        QMessageBox::about(m_dialog, "提示", "已在其他专区，请先返回大厅");
        return;
    }

    m_externalLaunching = true;
    m_pendingExternalZone = zoneid;
    m_pendingExeName.clear();

    if (!m_externalProgress)
    {
        m_externalProgress = new QProgressDialog("准备更新…", "取消", 0, 0, m_dialog);
        m_externalProgress->setWindowModality(Qt::ApplicationModal);
        m_externalProgress->setMinimumDuration(0);
        connect(m_externalProgress, &QProgressDialog::canceled, this, [this]() {
            if (m_gameUpdater)
                m_gameUpdater->cancel();
            abortExternalEnter("已取消");
        });
    }
    m_externalProgress->setMaximum(0);
    m_externalProgress->setValue(0);
    m_externalProgress->setLabelText("查询版本…");
    m_externalProgress->show();

    slot_sendGameVersionRq(zoneid);
}

void CKernel::abortExternalEnter(const QString &reason)
{
    m_externalLaunching = false;
    m_pendingExternalZone = 0;
    if (m_externalProgress)
    {
        m_externalProgress->hide();
    }
    if (!reason.isEmpty())
        QMessageBox::warning(m_dialog, "外部游戏", reason);
}

void CKernel::slot_externalUpdateFinished(bool ok, const QString &error, const GameManifest &manifest)
{
    if (!m_externalLaunching)
        return;

    if (!ok)
    {
        const QString exePath = GamePackageUpdater::installDir(gamesRootPath(),
                m_pendingExeName.isEmpty() ? manifest.exeName : m_pendingExeName)
                + "/" + (m_pendingExeName.isEmpty() ? manifest.exeName : m_pendingExeName)
                + ".exe";
        const bool hasOld = QFileInfo::exists(exePath);
        if (hasOld)
        {
            if (m_externalProgress)
                m_externalProgress->hide();
            const auto ret = QMessageBox::question(
                m_dialog, "更新失败",
                error + "\n是否仍使用本地旧版进入？",
                QMessageBox::Yes | QMessageBox::No);
            if (ret == QMessageBox::Yes)
            {
                GameManifest local = manifest;
                if (local.exeName.isEmpty())
                    local.exeName = m_pendingExeName;
                joinExternalZoneAndLaunch(local);
                return;
            }
        }
        abortExternalEnter(error.isEmpty() ? "更新失败" : error);
        return;
    }

    joinExternalZoneAndLaunch(manifest);
}

void CKernel::joinExternalZoneAndLaunch(const GameManifest &manifest)
{
    const QString exeName = manifest.exeName.isEmpty() ? m_pendingExeName : manifest.exeName;
    const QString dir = GamePackageUpdater::installDir(gamesRootPath(), exeName);
    const QString exePath = QDir(dir).filePath(exeName + ".exe");
    if (!QFileInfo::exists(exePath))
    {
        abortExternalEnter(QString("找不到可执行文件: %1").arg(exePath));
        return;
    }

    // 先 JoinZone，再藏大厅并启动
    slot_joinzone(m_pendingExternalZone);
    m_dialog->hide();
    if (m_externalProgress)
        m_externalProgress->hide();

    m_externalProcess->setWorkingDirectory(dir);
    m_externalProcess->start(exePath, QStringList());
    if (!m_externalProcess->waitForStarted(5000))
    {
        slot_leavezone();
        abortExternalEnter(QString("启动失败: %1").arg(m_externalProcess->errorString()));
        return;
    }

    m_externalLaunching = false;
    m_pendingExeName = exeName;
}

void CKernel::slot_externalProcessFinished(int, QProcess::ExitStatus)
{
    // 外部游戏进程结束：离开专区并显示大厅
    if (m_zoneid != 0 && m_zoneid != Five_In_Line)
        slot_leavezone();
    else if (!m_dialog->isVisible())
        m_dialog->show();
    m_pendingExternalZone = 0;
    m_externalLaunching = false;
}


