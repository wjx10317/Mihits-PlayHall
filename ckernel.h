#ifndef CKERNEL_H
#define CKERNEL_H

#include <QObject>
#include <QApplication>
#include <QKeyEvent>
#include"dialog.h"
#include"INetMediator.h"
#include"logindialog.h"
#include"netapi/net/packdef.h"
#include"fiveinlinezone.h"
#include"roomdialog.h"
#include"roomitem.h"
#include<QTimer>
class CKernel;
typedef void (CKernel::*PFUN)(unsigned int,char*,int);
#define NetPackMap(a)  CKernel::Get_Instance()->m_NetPackMap[ a - _DEF_PACK_BASE ]
class CKernel : public QObject
{
    Q_OBJECT
public:
    static CKernel* Get_Instance()
    {
        static CKernel kernel;
        return &kernel;
    }

private:
    explicit CKernel(QObject *parent = nullptr);
    ~CKernel(){}
    CKernel(const CKernel&) = delete;
    CKernel& operator=(const CKernel&)
    {return *this;}
    void ConfigSet();
    bool eventFilter(QObject *obj, QEvent *event) override;

public slots:
    void Des_Instance();
    void slot_deal_readydata(unsigned int lSendIP, char *buf, int nlen);
    void slot_registerRq(QString ,QString,QString);
    void slot_loginRq(QString ,QString);
    void slot_registerRs(unsigned int lSendIP, char *buf, int nlen);
    void slot_loginRs(unsigned int lSendIP, char *buf, int nlen);
    void slot_joinzone(int);
    void slot_joinroom(int);
    void slot_leavezone();
    void slot_joinroomrs(unsigned int lSendIP, char *buf, int nlen);
    void slot_roommemberrq(unsigned int lSendIP, char *buf, int nlen);
    void slot_leaveroomrq(unsigned int lSendIP, char *buf, int nlen);
    void slot_leaveroom();
    void slot_sendgameready(int zoneid, int roomid, int userid);
    void slot_sendgamenotready(int zoneid, int roomid, int userid);
    void slot_sendgamestart(int zoneid, int roomid);
    void slot_dealroomstart(unsigned int lSendIP, char *buf, int nlen);
    void slot_dealroomnotready(unsigned int lSendIP, char *buf, int nlen);
    void slot_dealroomready(unsigned int lSendIP, char *buf, int nlen);
    void slot_dealpiecedown(unsigned int lSendIP, char *buf, int nlen);
    void slot_gameoverrq();
    void slot_sendpiecedown(int,int,int);
    void slot_PlayByCpuBegin(int,int,int);
    void slot_PlayByCpuEnd(int,int,int);
    void slot_dealaibegin(unsigned int lSendIP, char *buf, int nlen);
    void slot_dealaiend(unsigned int lSendIP, char *buf, int nlen);
    void slot_roominfozone();
    void slot_sendrecordrq();
    void slot_sendsinglerecordrq(QString);
    void slot_deal_allrecordrs(unsigned int lSendIP, char *buf, int nlen);
    void slot_deal_singlerecordrs(unsigned int lSendIP, char *buf, int nlen);
    void slot_reshowwindow();
    void slot_dealzoneroominfo(unsigned int lSendIP, char *buf, int nlen);
    // Phase1: 心跳 + 断线处理
    void slot_sendHeartbeat();
    void slot_disConnect();
    void slot_reconnectRs(unsigned int lSendIP, char *buf, int nlen);
    void slot_opponentDisconnect(unsigned int lSendIP, char *buf, int nlen);
public:
    void sendData(char*,int);
    void setnetpackmap();
public:
    //窗体成员
    Dialog* m_dialog;
    LoginDialog* m_logindialog;
    roomDialog* m_roomdialog;
    roomitem* m_roomitem;
    fiveinlinezone* m_fiveinlinezone;



    INetMediator* m_client;
    PFUN m_NetPackMap[_DEF_PACK_COUNT];
    int m_id;
    int m_roomid;
    int m_zoneid;
    QString m_username;
    char m_serverIP[20];
    bool m_ishost;
    QTimer m_rqtimer;

    QTimer m_heartbeatTimer;
    char m_reconnectToken[_MAX_SIZE];
    bool m_reconnecting;
    bool m_reconnectInGame;

signals:

};

#endif // CKERNEL_H
