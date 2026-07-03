#include "roomdialog.h"
#include "ui_roomdialog.h"
#include"packdef.h"
#include<QMessageBox>
#include<QDebug>
#include<QImage>
roomDialog::roomDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::roomDialog),m_roomid(0)
{
    ui->setupUi(this);
    ui->pb_begin->setEnabled(false);
    connect(ui->widget,SIGNAL(SIG_PIECEDOWN(int ,int,int)),this,SIGNAL(SIG_PIECEDOWN(int ,int,int)));
    connect(ui->widget,SIGNAL(SIG_PLAYERWIN(int)),
            this,SLOT(slot_judgewin(int)));
    // 连接AI最佳位置信号
    connect(ui->widget,SIGNAL(SIG_AI_BEST_MOVES(int,int,int,int,int,int,int)),
            this,SLOT(slot_updateAIBestMoves(int,int,int,int,int,int,int)));
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
    ui->widget->setSpectating( m_status == _spec );

    // 玩家身份进入时隐藏AI推荐标签，观战时显示
    bool isSpectator = (m_status == _spec);
    ui->lb_ai_best1->setVisible(isSpectator);
    ui->lb_ai_best2->setVisible(isSpectator);
}

void roomDialog::closeEvent(QCloseEvent *event)
{
    if( QMessageBox::question( this , "退出房间提示" , "是否退出房间?" )
                          == QMessageBox::Yes ){
        clear_pushbutton();
        if(!record_flag)
        emit SIG_CLOSE();
        else
        {

            record_flag = false;
            emit SIG_SHOWBACK();
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

    clear_pushbutton();
    ui->widget->clear();
    // 重置时隐藏AI推荐
    ui->lb_ai_best1->setVisible(false);
    ui->lb_ai_best2->setVisible(false);
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

void roomDialog::clear_pushbutton(bool clearBoard)
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

    if (clearBoard && ui->widget->isGameActive())
        ui->widget->clear();
}

void roomDialog::slot_piecedown(int color, int x, int y)
{
    ui->widget->slot_piecedown(color,x,y);
}

void roomDialog::slot_updateAIBestMoves(int nextPlayer, int best1_x, int best1_y, int best1_score,
                                            int best2_x, int best2_y, int best2_score)
{
    // 只有在回放模式下才显示AI推荐
    if(m_status != _spec)
        return;

    // 显示AI首选位置
    QString playerColor = (nextPlayer == FiveInLine::Black) ? "黑棋" : "白棋";
    QString best1 = QString("AI推荐%1: (%2,%3) 分数:%4")
                    .arg(playerColor)
                    .arg(best1_x).arg(best1_y).arg(best1_score);
    ui->lb_ai_best1->setText(best1);

    // 显示AI次选位置（用哨兵坐标判断是否有次选，而非分数）
    if(best2_x != -1 || best2_y != -1)
    {
        QString best2 = QString("AI推荐%1: (%2,%3) 分数:%4")
                        .arg(playerColor)
                        .arg(best2_x).arg(best2_y).arg(best2_score);
        ui->lb_ai_best2->setText(best2);
    }
    else
    {
        ui->lb_ai_best2->setText("AI次选: 无");
    }
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
        emit SIG_gameReady( 0x10 , m_roomid , (*it).first  );
    }else{

        emit SIG_gamenotReady( 0x10 , m_roomid , (*it).first  );
    }
}

void roomDialog::on_pb_player2_ready_clicked(bool checked)
{
    if(m_status!=_player)return;
    auto it = userlist.begin();
    it++;
    if( ui->pb_player2_ready->isChecked() ){
        //发送信号
        emit SIG_gameReady( 0x10 , m_roomid , (*it).first );
    }else{
        emit SIG_gamenotReady( 0x10 , m_roomid , (*it).first );
    }
}

void roomDialog::on_pb_begin_clicked()
{
    emit SIG_gameStart(0x10,m_roomid);
}

void roomDialog::on_pb_player1_cpu_clicked(bool )
{
    if(m_status ==_host)
    {
        auto it = userlist.begin();

        if(ui->pb_player1_cpu->isChecked())
        {
            emit SIG_PLAYBYCPUBEGIN(0x10,m_roomid,(*it).first);

            ui->widget->setcpucolor(FiveInLine::Black);

            ui->widget->piecedownbycpu();
        }
        else
        {
            emit SIG_PLAYBYCPUEND(0x10,m_roomid,(*it).first);
            ui->widget->setcpucolor(FiveInLine::None);
        }
    }
}

void roomDialog::on_pb_player2_cpu_clicked(bool )
{
    if(m_status ==_player)
    {
        auto it = userlist.begin();
        it++;
        if(ui->pb_player2_cpu->isChecked())
        {
            emit SIG_PLAYBYCPUBEGIN(0x10,m_roomid,(*it).first);

            ui->widget->setcpucolor(FiveInLine::White);

            ui->widget->piecedownbycpu();
        }
        else
        {
            emit SIG_PLAYBYCPUEND(0x10,m_roomid,(*it).first);
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
    ui->widget->clear();
    ui->widget->setSpectating(true);
    // 回看模式显示AI推荐
    ui->lb_ai_best1->setVisible(true);
    ui->lb_ai_best2->setVisible(true);
}

void roomDialog::addmem(int id,QString name)
{
   userlist.push_back(QPair<int,QString>(id,name));
}

void roomDialog::setMemberOffline(int id)
{
    int pos = 0;
    for (auto it = userlist.begin(); it != userlist.end() && pos < 2; ++it, ++pos)
    {
        if ((*it).first != id)
            continue;
        QPixmap pm(":/icon/avatar_12.png");
        QImage gray = pm.toImage().convertToFormat(QImage::Format_Grayscale8);
        QPixmap offline = QPixmap::fromImage(gray);
        if (pos == 0)
            ui->lb_player1_icon->setPixmap(offline);
        else
            ui->lb_player2_icon->setPixmap(offline);
        delPlayerReady(id);
        break;
    }
}

void roomDialog::setMemberOnline(int id)
{
    int pos = 0;
    for (auto it = userlist.begin(); it != userlist.end() && pos < 2; ++it, ++pos)
    {
        if ((*it).first != id)
            continue;
        QPixmap pm(":/icon/avatar_12.png");
        if (pos == 0)
            ui->lb_player1_icon->setPixmap(pm.copy(0, 0, 230, 280));
        else
            ui->lb_player2_icon->setPixmap(pm.copy(0, 0, 230, 280));
        break;
    }
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

