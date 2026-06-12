#ifndef FIVEINLINEZONE_H
#define FIVEINLINEZONE_H

#include <QDialog>
#include"roomitem.h"
#include "recordlist.h"
#include<QGridLayout>
#include<QCloseEvent>
namespace Ui {
class fiveinlinezone;
}

class fiveinlinezone : public QDialog
{
    Q_OBJECT
signals:
    void SIG_CLOSE();
    void SIG_ZONE_JOINROOM(int);
    void SIG_ALLRECORD();
    void SIG_SINGLERECORD(QString);
public:
    explicit fiveinlinezone(QWidget *parent = nullptr);
    void closeEvent(QCloseEvent* e);
    void setroomlist();
    void addrecord(QString time,QString idA,QString idB);
    void hidelist();

    std::vector<roomitem*>& getvecroomitem();
    ~fiveinlinezone();

private slots:


    void on_pb_recordlist_clicked();

private:
    Ui::fiveinlinezone *ui;
    QGridLayout* m_layout;
    std::vector<roomitem*> m_vecroomitem;
    recordlist*m_list;

};

#endif // FIVEINLINEZONE_H
