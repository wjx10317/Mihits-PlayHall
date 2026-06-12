#ifndef ROOMDIALOG_H
#define ROOMDIALOG_H

#include <QDialog>
#include<QCloseEvent>
#include<QPair>
namespace Ui {
class roomDialog;
}

class roomDialog : public QDialog
{
    Q_OBJECT
signals:
    void SIG_CLOSE();
    void SIG_gameReady(int,int,int);
    void SIG_gameStart(int,int);
    void SIG_gamenotReady(int,int,int);
    void SIG_PIECEDOWN(int ,int,int);
    void SIG_PLAYBYCPUBEGIN(int,int,int);
    void SIG_PLAYBYCPUEND(int,int,int);
    void SIG_GAMEOVER();
    void SIG_SHOWBACK();

public:
    explicit roomDialog(QWidget *parent = nullptr);

    ~roomDialog();
    void setinfo(int );
    void setHostInfo(int ,QString);
    void setPlayerInfo(int ,QString);
    void setPlayerInfotoHost();
    void setUserStatus(int);
    void closeEvent(QCloseEvent* );
    void clearroom();
    void clearPlayerInfo();
    void loadnewPlayerInfo();
    void setPlayerReady(int);
    void delPlayerReady(int);
    void setGameStart();
    void setPlayerPlayByCpu(bool yes);
    void setHostPlayByCpu(bool yes);
    int getStatus();
    bool isfirstspec(int id);
    void rmmem(int id);
    void setrecordstatus(QString,QString,int,int);




    void clear_pushbutton();

    void addmem(int id,QString name);
private slots:
    void on_pb_player1_ready_clicked(bool checked);

    void on_pb_player2_ready_clicked(bool checked);

    void on_pb_begin_clicked();
    void on_pb_player1_cpu_clicked(bool checked);

    void on_pb_player2_cpu_clicked(bool checked);

public slots:
    void slot_piecedown(int,int,int);
    void slot_judgewin(int color);

private:
    Ui::roomDialog *ui;
    int m_roomid;
    int m_status;
    int record_flag;
    std::list<QPair<int,QString>>userlist;
};

#endif // ROOMDIALOG_H
