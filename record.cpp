#include "record.h"
#include "ui_record.h"
#include<QDebug>
record::record(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::record),m_time("null"),hostid(),playerid()
{
    ui->setupUi(this);
}
void record::setbaseinfo(QString time, QString idA, QString idB)
{
    m_time = time;
    hostid = idA;
    playerid =idB;
    ui->lb_host->setText ( idA);
    ui->lb_player->setText ( idB);
    ui->lb_time->setText(m_time);

}
record::~record()
{
    delete ui;
}




void record::on_pb_select_clicked()
{
    emit SIG_SELECT(m_time);
}

