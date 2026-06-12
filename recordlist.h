#ifndef RECORDLIST_H
#define RECORDLIST_H

#include <QWidget>
#include<QGridLayout>
#include<QCloseEvent>
#include "record.h"
namespace Ui {
class recordlist;
}

class recordlist : public QWidget
{
    Q_OBJECT
signals:
    void SIG_REQUEST(QString);
public:
    explicit recordlist(QWidget *parent = nullptr);
    void closeEvent(QCloseEvent* e);
    ~recordlist();
public:
    void add_record(QString time,QString idA,QString idB);
    void clearlist();
    void clearGridLayout(QGridLayout* layout);
private:
    Ui::recordlist *ui;
    QGridLayout* m_layout;
    std::vector<record*>m_recordlist;
};

#endif // RECORDLIST_H
