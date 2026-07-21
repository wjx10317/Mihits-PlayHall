#include "recordlist.h"
#include "ui_recordlist.h"

recordlist::recordlist(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::recordlist)
{
    ui->setupUi(this);
    this->setWindowTitle("对局记录");
    m_layout = new QGridLayout;
    ui->wdg_recordGrid->setLayout(m_layout);
}

void recordlist::closeEvent(QCloseEvent *e)
{
    clearlist();
    e->accept();
}

recordlist::~recordlist()
{
    clearlist();
    delete ui;
}

void recordlist::add_record(QString time, QString idA, QString idB)
{
    record* tmp = new record;
    tmp->setbaseinfo(time, idA, idB);
    m_layout->addWidget(tmp);
    m_recordlist.push_back(tmp);
    connect(tmp, SIGNAL(SIG_SELECT(QString)), this, SIGNAL(SIG_REQUEST(QString)));
}

void recordlist::clearlist()
{
    // 以 m_recordlist 为准删控件，避免只 takeAt 清不干净导致再次打开叠加
    for (record* r : m_recordlist)
    {
        if (!r)
            continue;
        m_layout->removeWidget(r);
        r->hide();
        r->setParent(nullptr);
        delete r;
    }
    m_recordlist.clear();
    clearGridLayout(m_layout);
}

void recordlist::clearGridLayout(QGridLayout* layout)
{
    if (!layout)
        return;

    QLayoutItem* item = nullptr;
    while ((item = layout->takeAt(0)) != nullptr)
    {
        if (QLayout* child = item->layout())
        {
            if (auto* grid = qobject_cast<QGridLayout*>(child))
                clearGridLayout(grid);
            delete child;
        }
        if (QWidget* w = item->widget())
        {
            w->hide();
            w->setParent(nullptr);
            delete w;
        }
        delete item;
    }
}
