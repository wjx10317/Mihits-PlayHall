#include "fiveinlinezone.h"
#include "ui_fiveinlinezone.h"
#include<QMessageBox>
fiveinlinezone::fiveinlinezone(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::fiveinlinezone),m_list(new recordlist)
{
    ui->setupUi(this);
    this->setWindowTitle("五子棋专区");
    m_layout =new QGridLayout;
    ui->wdg_roomGrid->setLayout(m_layout);
    m_vecroomitem.push_back(nullptr);
    for(int i=0;i<120;i++)
    {
      roomitem* item = new roomitem;
      item->setinfo(i+1);
      m_vecroomitem.push_back(item);
      m_layout->addWidget(item,i/2,i%2);
      connect(item,SIGNAL(SIG_JOINROOM(int)),this,SIGNAL(SIG_ZONE_JOINROOM(int)));

    }
    connect(m_list,SIGNAL(SIG_REQUEST(QString)),this,SIGNAL(SIG_SINGLERECORD(QString)));
}

void fiveinlinezone::closeEvent(QCloseEvent *e)//退出事件
{
    if(QMessageBox::question(this,"退出提示","是否退出专区")==QMessageBox::Yes)
    {
        emit SIG_CLOSE();
        e->accept();
    }
    else
    {
        e->ignore();
    }
}

void fiveinlinezone::addrecord(QString time, QString idA, QString idB)
{
    m_list->add_record(time,idA,idB);
}

void fiveinlinezone::hidelist()
{
    m_list->clearlist();
    m_list->hide();
}

std::vector<roomitem *> &fiveinlinezone::getvecroomitem()
{
    return m_vecroomitem;
}

fiveinlinezone::~fiveinlinezone()
{
    delete ui;
}

void fiveinlinezone::on_pb_recordlist_clicked()
{
    // 每次打开先清空，避免上次残留（尤其是看过详情后 hidelist 未删控件）
    m_list->clearlist();
    m_list->show();
    emit SIG_ALLRECORD();
}

