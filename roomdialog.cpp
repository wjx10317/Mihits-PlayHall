#include "roomdialog.h"
#include "ui_roomdialog.h"
#include"packdef.h"
#include<QMessageBox>
#include<QDebug>
roomDialog::roomDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::roomDialog),m_roomid(0)
{
    ui->setupUi(this);
    ui->pb_begin->setEnabled(false);
    connect(ui->widget,SIGNAL(SIG_PIECEDOWN(int ,int,int)),this,SIGNAL(SIG_PIECEDOWN(int ,int,int)));
    connect(ui->widget,SIGNAL(SIG_PLAYERWIN(int)),
            this,SLOT(slot_judgewin(int)));
}

roomDialog::~roomDialog()
{
    delete ui;
}
void roomDialog::setinfo(int roomid)
{
    m_roomid =roomid;
    QString txt = QString("五子棋-%1房").arg(roomid,2,10,QChar('0'));
    this->setWindowTitle(txt);
}

void roomDialog::setHostInfo(int id, QString name)
{
    ui->lb_player1_name ->setText(name);
    ui->lb_player1_icon->setPixmap(QPixmap(":/icon/avatar_12.png").copy(0,0,230,280));
}

void roomDialog::setPlayerInfo(int id, QString name)
{
    ui->lb_player2_name ->setText(name);
    ui->lb_player2_icon->setPixmap(QPixmap(":/icon/avatar_12.png").copy(0,0,230,280));

}

void roomDialog::setPlayerInfotoHost()
{
    auto it = userlist.begin();
    it++;
    setHostInfo((*it).first,(*it).second);
}

void roomDialog::setUserStatus(int status)
{
    m_status = status;
//需要修改
    ui->widget->setselfstatus( m_status == _host?FiveInLine::Black:FiveInLine::White );

}

void roomDialog::closeEvent(QCloseEvent *event)
{
    if( QMessageBox::question( this , "退出房间提示" , "是否退出房间?" )
                          == QMessageBox::Yes ){
        clear_pushbutton();
        if(!record_flag)
        Q_EMIT SIG_CLOSE();
        else
        {

            record_flag = false;
            Q_EMIT SIG_SHOWBACK();
        }
        event->accept();
    }else{
        event->ignore();
    }
}

void roomDialog::clearroom()
{
    //ui
    ui->lb_player1_name->setText("");
    ui->lb_player2_name->setText("");
    ui->lb_player1_icon->setPixmap(QPixmap(":/icon/slotwait.png"));
    ui->lb_player2_icon->setPixmap(QPixmap(":/icon/slotwait.png"));

    //游戏界面清空
    ui->pb_begin->setEnabled( false );
    ui->pb_player1_ready->setEnabled(true);
    ui->pb_player1_ready->setChecked( false );
    ui->pb_player2_ready->setEnabled(true);
    ui->pb_player2_ready->setChecked( false );
    ui->pb_player1_ready->setText("待准备");
    ui->pb_player2_ready->setText("待准备");
    ui->pb_player1_cpu->setChecked(false);
    ui->pb_player2_cpu->setChecked(false);

    ui->pb_player1_cpu->setText("待托管");
    ui->pb_player2_cpu->setText("待托管");
    ui->widget->clear();
    //聊天窗口清空

    //后台数据
    m_roomid = 0;
    userlist.clear();
    m_status = _player;
}

void roomDialog::clearPlayerInfo()
{
    ui->lb_player2_name->setText("");
    ui->lb_player2_icon->setPixmap(QPixmap(":/icon/slotwait.png"));
}

void roomDialog::setPlayerReady(int id)
{
    int status =0;
    for(auto it = userlist.begin();it!=userlist.end()&&status<2;it++,status++)
    {


        if( status==0 && (*it).first == id ){
            ui->pb_player1_ready->setChecked(true);
            ui->pb_player1_ready->setText("已准备");

        }
        else if( status==1 && (*it).first == id  ){
            ui->pb_player2_ready->setChecked(true);
            ui->pb_player2_ready->setText("已准备");

        }
    }

    if(  m_status==_host&&
            (ui->pb_player1_ready->isChecked() && ui->pb_player2_ready->isChecked()) )
    {
        //都准备
        ui->pb_begin->setEnabled( true );
    }
}

void roomDialog::delPlayerReady(int id)
{
    int status =0;
    for(auto it = userlist.begin();it!=userlist.end()&&status<2;it++,status++)
    {

        if( status==0 && (*it).first == id ){
            ui->pb_player1_ready->setChecked(false);
            ui->pb_player1_ready->setText("准备");

        }
        else if( status==1 && (*it).first == id  ){
            ui->pb_player2_ready->setChecked(false);
            ui->pb_player2_ready->setText("准备");

        }
    }
    if( m_status==_host&&
            (!ui->pb_player1_ready->isChecked() || !ui->pb_player2_ready->isChecked()) )
    {

        ui->pb_begin->setEnabled( false );
    }
}

void roomDialog::setGameStart()
{
     ui->pb_player1_ready->hide();
     ui->pb_player2_ready->hide();
     ui->pb_begin->hide();
     ui->widget->slot_startgame();
}

void roomDialog::clear_pushbutton()
{
    ui->pb_begin->setEnabled( false );
    ui->pb_player1_ready->setEnabled(true);
    ui->pb_player1_ready->setChecked( false );
    ui->pb_player2_ready->setEnabled(true);
    ui->pb_player2_ready->setChecked( false );
    ui->pb_player1_ready->setText("待准备");
    ui->pb_player2_ready->setText("待准备");
    ui->pb_player1_ready->show();
    ui->pb_player2_ready->show();
    ui->pb_begin->show();

    ui->pb_player1_cpu->setChecked(false);
    ui->pb_player2_cpu->setChecked(false);

    ui->pb_player1_cpu->setText("待托管");
    ui->pb_player2_cpu->setText("待托管");
}

void roomDialog::slot_piecedown(int color, int x, int y)
{
    ui->widget->slot_piecedown(color,x,y);
}

void roomDialog::slot_judgewin(int color)
{
    QString res;
    if(m_status == _host&&color==FiveInLine::Black)
    {
        res = QString("你赢了");
    }
    else if(m_status == _player&&color==FiveInLine::White)
    {
        res = QString("你赢了");
    }
    else if(m_status == _spec)
    {
         res = QString("游戏结束");
    }
    else
    {
        res =  QString("你输了");
    }
    QMessageBox::about(this,"提示",res);
    clear_pushbutton();

    if(m_status==_host)
    {
        emit SIG_GAMEOVER();
    }
}

void roomDialog::on_pb_player1_ready_clicked(bool checked)
{
    if(m_status!=_host)return;
    if(userlist.size()==1)return;
    auto it = userlist.begin();
    if( ui->pb_player1_ready->isChecked() ){

        //发送信号
        Q_EMIT SIG_gameReady( 0x10 , m_roomid , (*it).first  );
    }else{

        Q_EMIT SIG_gamenotReady( 0x10 , m_roomid , (*it).first  );
    }
}

void roomDialog::on_pb_player2_ready_clicked(bool checked)
{
    if(m_status!=_player)return;
    auto it = userlist.begin();
    it++;
    if( ui->pb_player2_ready->isChecked() ){
        //发送信号
        Q_EMIT SIG_gameReady( 0x10 , m_roomid , (*it).first );
    }else{
        Q_EMIT SIG_gamenotReady( 0x10 , m_roomid , (*it).first );
    }
}

void roomDialog::on_pb_begin_clicked()
{
    Q_EMIT SIG_gameStart(0x10,m_roomid);
}

void roomDialog::on_pb_player1_cpu_clicked(bool checked)
{
    if(m_status ==_host)
    {
        auto it = userlist.begin();

        if(ui->pb_player1_cpu->isChecked())
        {
            Q_EMIT SIG_PLAYBYCPUBEGIN(0x10,m_roomid,(*it).first);

            ui->widget->setcpucolor(FiveInLine::Black);

            ui->widget->piecedownbycpu();
        }
        else
        {
            Q_EMIT SIG_PLAYBYCPUEND(0x10,m_roomid,(*it).first);
            ui->widget->setcpucolor(FiveInLine::None);
        }
    }
}

void roomDialog::on_pb_player2_cpu_clicked(bool checked)
{
    if(m_status ==_player)
    {
        auto it = userlist.begin();
        it++;
        if(ui->pb_player2_cpu->isChecked())
        {
            Q_EMIT SIG_PLAYBYCPUBEGIN(0x10,m_roomid,(*it).first);

            ui->widget->setcpucolor(FiveInLine::White);

            ui->widget->piecedownbycpu();
        }
        else
        {
            Q_EMIT SIG_PLAYBYCPUEND(0x10,m_roomid,(*it).first);
            ui->widget->setcpucolor(FiveInLine::None);
        }
    }
}

void roomDialog::setHostPlayByCpu(bool yes)
{
    if( yes ){
        ui->pb_player1_cpu->setText("托管中");
    }else{
        ui->pb_player1_cpu->setText("待托管");
    }
}

int roomDialog::getStatus()
{
    return m_status;
}

bool roomDialog::isfirstspec(int id)//理论不会越界出现迭代器失效问题
{
    auto it = userlist.begin();
    int count =0;
    while(count<2)
    {
        it++;
        count++;
        if(it==userlist.end())
            return false;
    }

    if((*it).first == id)
    {
        return true;
    }
    else
        return false;
}

void roomDialog::rmmem(int id)
{
    for(auto ite = userlist.begin();ite!=userlist.end();ite++)
    {

        int curid = (*ite).first;
        if(curid==id)
        {
            ite = userlist.erase(ite);
            break;
        }
    }
}

void roomDialog::setrecordstatus(QString a, QString b ,int c,int d)
{
    setHostInfo(c,a);
    setPlayerInfo(d,b);
    record_flag = true;
    m_status = _spec;
}

void roomDialog::addmem(int id,QString name)
{
   userlist.push_back(QPair<int,QString>(id,name));
}

void roomDialog::loadnewPlayerInfo()
{
    if(userlist.empty())
        return;
    auto ite = userlist.begin();
    ite++;
    if(ite==userlist.end())
        return;

    ui->lb_player2_name->setText(((*ite).second));
    ui->lb_player2_icon->setPixmap(QPixmap(":/icon/avatar_12.png").copy(0,0,230,280));


}

void roomDialog::setPlayerPlayByCpu(bool yes)
{
    if( yes ){
        ui->pb_player2_cpu->setText("托管中");
    }else{
        ui->pb_player2_cpu->setText("待托管");
    }
}

