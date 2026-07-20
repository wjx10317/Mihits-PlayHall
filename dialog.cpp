#include "dialog.h"
#include "ui_dialog.h"
#include<QMessageBox>
#include<QDebug>
Dialog::Dialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Dialog)
{
    ui->setupUi(this);
}

Dialog::~Dialog()
{
    delete ui;
}
void Dialog::closeEvent(QCloseEvent* e)
{
    if(QMessageBox::question(this,"退出","是否退出")==QMessageBox::Yes)
    {
       // qDebug()<<__func__;
        emit SIG_CLOSE();
        e->accept();
    }
    else
    {
        e->ignore();
    }
}
#include"netapi/net/packdef.h"

void Dialog::on_pb_fiveinline_clicked()
{
    //发送信号

    emit SIG_JOINZONE(Five_In_Line);
    this->hide();
}

void Dialog::on_pb_foogame_clicked()
{
    emit SIG_ENTER_EXTERNAL_ZONE(External_Malody);
}

