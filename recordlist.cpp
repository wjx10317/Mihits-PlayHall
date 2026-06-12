#include "recordlist.h"
#include "ui_recordlist.h"

recordlist::recordlist(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::recordlist)
{
    ui->setupUi(this);
    this->setWindowTitle("对局记录");
    m_layout =new QGridLayout;
    ui->wdg_recordGrid->setLayout(m_layout);
}

void recordlist::closeEvent(QCloseEvent *e)
{
    clearlist();
    clearGridLayout(m_layout);
}

recordlist::~recordlist()
{
    delete ui;
}

void recordlist::add_record(QString time, QString idA, QString idB)
{
    record* tmp = new record;
    tmp->setbaseinfo(time,idA,idB);
    m_layout->addWidget(tmp);
    m_recordlist.push_back(tmp);
    connect(tmp,SIGNAL(SIG_SELECT(QString)),this,SIGNAL(SIG_REQUEST(QString)));

}

void recordlist::clearlist()
{
    m_recordlist.clear();

}
void recordlist::clearGridLayout(QGridLayout* layout)
{
    if (!layout) return;

    QLayoutItem* item = nullptr;
    // 循环取出第0个元素，直到布局为空
    while ((item = layout->takeAt(0)) != nullptr)
    {
        // 1. 如果 item 内嵌子布局，递归清空并删除子布局
        if (item->layout())
        {
            clearGridLayout(qobject_cast<QGridLayout*>(item->layout()));
            delete item->layout();
        }

        // 2. 删除布局里的控件
        if (item->widget())
        {
            delete item->widget();
        }
        // 3. 删除布局项本身
        delete item;
    }
}
