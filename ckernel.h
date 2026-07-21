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
#include<QProcess>
#include<QProgressDialog>
#include<QMap>
#include<QList>
#include<QLabel>
#include<QPoint>
#include<QVector>
#include"gamepackageupdater.h"
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
    void slot_dealpiecedownRs(unsigned int lSendIP, char *buf, int nlen);
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
    // 外部游戏：版本校验
    void slot_sendGameVersionRq(int zoneid);
    void slot_gameVersionRs(unsigned int lSendIP, char *buf, int nlen);
    // 外部游戏：专区编排（阶段4）
    void slot_enterExternalZone(int zoneid);
    void slot_externalUpdateFinished(bool ok, const QString &error, const GameManifest &manifest);
    void slot_externalProcessFinished(int exitCode, QProcess::ExitStatus status);
public:
    void sendData(char*,int);
    void sendData(char* buf, int nlen, int expectPackType);
    void setnetpackmap();
    // 延迟探测：发包注册期望回包类型；分发命中后删除并算 RTT
    void registerLatencyExpect(int expectPackType);
    void removeLatencyExpect(int expectPackType);
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
    // 对局重连：成员列表结束前若落子包抢先到达则缓存，避免未 start 就落子导致回合错乱
    QVector<QPoint> m_reconnectMovePos;
    QVector<int> m_reconnectMoveColor;

    // 外部游戏专区
    GamePackageUpdater *m_gameUpdater = nullptr;
    QProcess *m_externalProcess = nullptr;
    QProgressDialog *m_externalProgress = nullptr;
    int m_pendingExternalZone = 0;
    QString m_pendingExeName;
    bool m_externalLaunching = false;
    void joinExternalZoneAndLaunch(const GameManifest &manifest);
    void abortExternalEnter(const QString &reason);
    QString gamesRootPath() const;

    // 网络延迟（仅 UI，不改房间逻辑）
    QMap<int, QList<qint64>> m_latencyExpectTicks; // expectPackType → 发送时刻队列
    qint64 m_lastRttMs = -1;
    QLabel *m_latencyLabel = nullptr; // 共享标签，挂在当前可见主窗口右下角
    int expectPackTypeForRequest(int requestPackType) const;
    void onLatencySample(qint64 rttMs);
    void updateNetLatencyUi();
    QWidget *currentLatencyHost() const;
    void sendLogoutHard(); // 主动退出：通知服务端硬断
signals:

};

#endif // CKERNEL_H
