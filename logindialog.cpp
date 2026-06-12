#include "logindialog.h"
#include "ui_logindialog.h"
#include<QRegExp>
#include<QMessageBox>
#include<QDebug>
LoginDialog::LoginDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LoginDialog)
{
    ui->setupUi(this);
    SetUI();
}

LoginDialog::~LoginDialog()
{
    delete ui;
}
void LoginDialog::SetUI()
{
    this->setWindowTitle("登录&注册");
    QPixmap pixmap(":/images/background.jpg");
    QPalette pal(this->palette());
    pal.setBrush(QPalette::Background,pixmap);
    this->setPalette(pal);
    //默认登录界面
    ui->tabWidget->setCurrentIndex(0);
}

void LoginDialog::on_pb_clear_clicked()
{
    //控制组件清空
     ui->le_tel->setText("");
     ui->le_password->setText("");
}


void LoginDialog::on_pb_login_clicked()
{
    //登录信息过滤
    QString tel = ui->le_tel->text();
    QString password = ui->le_password->text();
    QRegExp exp("^1[3-8][0-9]{9}$");
    if(!exp.exactMatch(tel))
    {
        QMessageBox::about(this,"提示","电话号不合法");
        return ;
    }
    if(password.length()>20)
    {
        QMessageBox::about(this,"提示","密码过长");
        return ;
    }
    emit SIG_LOGIN_COMMIT(tel,password);
}


void LoginDialog::on_pb_clear_register_clicked()
{
    //注册信息过滤
   ui->le_tel_register->setText("");
    ui->le_password_register->setText("");
    ui->le_comfirm_register->setText("");
    ui->le_name_register->setText("");
}


void LoginDialog::on_pb_register_clicked()
{
    QString tel = ui->le_tel_register->text();
    QString password = ui->le_password_register->text();
    QString confirm = ui->le_comfirm_register->text();
    QString nick = ui->le_name_register->text();
    QRegExp exp("^1[3-8][0-9]{9}$");
    if(!exp.exactMatch(tel))
    {
        QMessageBox::about(this,"提示","电话号不合法");
        return ;
    }
    if(password.length()>20)
    {
        QMessageBox::about(this,"提示","密码过长");
        return ;
    }
    if(password!=confirm)
    {
        QMessageBox::about(this,"提示","两次输入密码不一致");
        return ;
    }
    QString tmpname = nick;
    tmpname.remove(' ');
    if(nick.length()>20)
    {
        QMessageBox::about(this,"提示","名称过长");
        return ;
    }
    if(nick!=tmpname)
    {
        QMessageBox::about(this,"提示","名字中不能包括空格");
        return ;

    }
    emit SIG_REGISTER_COMMIT(tel,password,nick);
}
void LoginDialog::closeEvent(QCloseEvent* e)
{
    if(QMessageBox::question(this,"退出","是否退出")==QMessageBox::Yes)
    {
        emit SIG_CLOSE();
        e->accept();
    }
    else
    {
        e->ignore();
    }
}

