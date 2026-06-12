#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include<QCloseEvent>
namespace Ui {
class LoginDialog;
}

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(QWidget *parent = nullptr);
    void SetUI();
    ~LoginDialog();
    void closeEvent(QCloseEvent* e);
signals:
    void SIG_LOGIN_COMMIT(QString,QString);
    void SIG_REGISTER_COMMIT(QString,QString,QString);
    void SIG_CLOSE();

private slots:
    void on_pb_clear_clicked();

    void on_pb_login_clicked();

    void on_pb_clear_register_clicked();

    void on_pb_register_clicked();

private:
    Ui::LoginDialog *ui;
};

#endif // LOGINDIALOG_H
