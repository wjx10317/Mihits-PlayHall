#ifndef ROOMITEM_H
#define ROOMITEM_H

#include <QDialog>

namespace Ui {
class roomitem;
}

class roomitem : public QDialog
{
    Q_OBJECT
signals:
    void SIG_JOINROOM(int );

public:
    explicit roomitem(QWidget *parent = nullptr);
     void setinfo(int );
     void setroomitem(int);
    ~roomitem();

private slots:
    void on_pb_join_clicked();

private:
    Ui::roomitem *ui;
    int m_roomid;
};

#endif // ROOMITEM_H
