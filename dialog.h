#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include<QCloseEvent>
QT_BEGIN_NAMESPACE
namespace Ui { class Dialog; }
QT_END_NAMESPACE

class Dialog : public QDialog
{
    Q_OBJECT
signals:
    void SIG_CLOSE();
    void SIG_JOINZONE(int zoneid);
    void SIG_ENTER_EXTERNAL_ZONE(int zoneid);
public:
    Dialog(QWidget *parent = nullptr);
    ~Dialog();
    void closeEvent(QCloseEvent* e);

private slots:
    void on_pb_fiveinline_clicked();
    void on_pb_foogame_clicked();

private:
    Ui::Dialog *ui;
};
#endif // DIALOG_H
