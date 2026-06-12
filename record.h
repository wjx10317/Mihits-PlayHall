#ifndef RECORD_H
#define RECORD_H

#include <QWidget>

namespace Ui {
class record;
}

class record : public QWidget
{
    Q_OBJECT

signals:
    void SIG_SELECT(QString);
public:
    explicit record(QWidget *parent = nullptr);
    ~record();
public:
    void setbaseinfo(QString time, QString idA, QString idB);
private slots:


    void on_pb_select_clicked();

private:
    Ui::record *ui;
    QString m_time;
    QString hostid;
    QString playerid;

};

#endif // RECORD_H
