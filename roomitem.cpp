#include "roomitem.h"
#include "ui_roomitem.h"

roomitem::roomitem(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::roomitem),m_roomid(0)
{
    ui->setupUi(this);

}
void roomitem::setinfo(int roomid)
{
    m_roomid =roomid;
    QString txt = QString("五子棋-%1房").arg(roomid,2,10,QChar('0'));
    ui->lb_title->setText(txt);
}

void roomitem::setroomitem(int num)
{
    QPixmap ready = QPixmap(":/icon/avatar_12.png");
    QPixmap wait = QPixmap(":/icon/slotwait.png");
    switch(num)
    {
    case 0:
        ui->lb_player1->setPixmap(wait);
        ui->lb_player2->setPixmap(wait);
        break;
    case 1:
        ui->lb_player1->setPixmap(ready);
        ui->lb_player2->setPixmap(wait);
        break;
    case 2:
        ui->lb_player1->setPixmap(ready);
        ui->lb_player2->setPixmap(ready);
        break;
    default:
        break;
    }
}
roomitem::~roomitem()
{
    delete ui;
}


void roomitem::on_pb_join_clicked()
{
    emit SIG_JOINROOM(m_roomid);
}

