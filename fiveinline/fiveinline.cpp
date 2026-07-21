#include "fiveinline.h"
#include "ui_fiveinline.h"

#include<QMessageBox>
#include<algorithm>
#include<QThread>
FiveInLine::FiveInLine(QWidget *parent)
    : QWidget(parent),
      ui(new Ui::FiveInLine),m_board(FIL_ROWS,std::vector<int>(FIL_COLS,2/*2表示无*/)),
      m_movePoint(0,0),m_isMove_fl(false),m_turns(0),m_curturns(0),isOver(false),m_status(Black),iswaitexist(false),waittoplacestep(QPair<int,int>(-1,-1)),m_cpucolor(None),m_isSpectating(false),
      m_lastPos(0,0),m_aiThread(new QThread(this)),m_aiWorker(new AiWorker),m_aiJobId(0),m_aiBusy(false)
{
    ui->setupUi(this);
    m_pieceColor[Black] = QBrush(QColor(0,0,0));
    m_pieceColor[White] = QBrush(QColor(255,255,255));
    m_pieceColor[None] = QBrush(QColor(0,0,0,0));
    //connect(this,SIGNAL(SIG_PIECEDOWN(int,int,int)),this,SLOT(slot_piecedown(int,int,int)));
    connect(&m_timer,SIGNAL(timeout()),this,SLOT(repaint()));
    m_timer.start(1000/25);
    // AI评估防抖定时器：300ms内的多次请求合并为一次
    connect(&m_aiDebounceTimer, SIGNAL(timeout()), this, SLOT(slot_notifyAIBestMoves()));
    clear();
    //slot_startgame();
    initaivector();

    m_aiWorker->moveToThread(m_aiThread);
    connect(this,SIGNAL(SIG_SEND_TASK(AiWorker::Task)),m_aiWorker,SLOT(slot_doTask(AiWorker::Task)));
    connect(m_aiWorker, SIGNAL(SIG_RESULT(AiWorker::Result)),
            this, SLOT(slot_onAiResult(AiWorker::Result)));
    m_aiThread->start();   // 启动那一个工作线程
}

FiveInLine::~FiveInLine()
{
    if (m_aiThread)
    {
       m_aiThread->quit();
       m_aiThread->wait();
    }
    delete m_aiWorker;
    delete ui;
}

void FiveInLine::paintEvent(QPaintEvent *event)
{
    QPainter painer(this);//用于绘图使用有参构造传入当前对象得到当前对象的画刷
    painer.setBrush(QBrush(QColor(255,160,0)));
    painer.drawRect(FIL_MARGIN_WIDTH-FIL_DISTANCE,
                    FIL_MARGIN_HEIGHT-FIL_DISTANCE,
                    (FIL_COLS+1)*FIL_SPACE+2*FIL_DISTANCE,(FIL_ROWS+1)*FIL_SPACE+2*FIL_DISTANCE);
    //绘制网格线
    // |
    for( int i = 1 ; i <= FIL_COLS ; ++i ){
        painer.setBrush( QBrush( QColor( 0 , 0 , 0 ) ) ); //通过画刷设置颜色
        QPoint p1( FIL_MARGIN_WIDTH + FIL_SPACE*i , FIL_MARGIN_HEIGHT + FIL_SPACE );
        QPoint p2( FIL_MARGIN_WIDTH + FIL_SPACE*i , FIL_MARGIN_HEIGHT + (FIL_ROWS)*FIL_SPACE );
        painer.drawLine( p1 , p2);
    }
    //绘制网格线
    // -
    for( int i = 1 ; i <= FIL_ROWS ; ++i ){
        painer.setBrush( QBrush( QColor( 0 , 0 , 0 ) ) ); //通过画刷设置颜色
        QPoint p1( FIL_MARGIN_WIDTH+FIL_SPACE, FIL_MARGIN_HEIGHT + FIL_SPACE*i );
        QPoint p2( FIL_MARGIN_WIDTH + (FIL_COLS)*FIL_SPACE , FIL_MARGIN_HEIGHT + FIL_SPACE*i );
        painer.drawLine( p1 , p2);
    }
    painer.setBrush( QBrush( QColor( 0 , 0 , 0 ) ));

    painer.drawEllipse(QPoint(290,290),6/2,6/2);

    //绘制棋子

   for(int x =0;x<FIL_ROWS;x++)
   {
       for(int y=0;y<FIL_COLS;y++)
       {
           painer.setBrush(m_pieceColor[m_board[x][y]]);
           if(m_board[x][y]!=None)
           {
               painer.drawEllipse(QPoint(FIL_MARGIN_WIDTH+FIL_SPACE*(x+1),FIL_MARGIN_HEIGHT+FIL_SPACE*(y+1)),
                                  FIL_PIECE_SIZE/2,FIL_PIECE_SIZE/2);
           }
       }
   }
   //绘制上一次落子的位置
   if( !m_lastPos.isNull()){
       painer.setBrush( QBrush( QColor( 0 , 250 , 0 ) ) );
       int x1 = FIL_MARGIN_WIDTH + FIL_SPACE + FIL_SPACE*m_lastPos.x() - FIL_PIECE_SIZE/2;
       int y1 = FIL_MARGIN_HEIGHT+FIL_SPACE + FIL_SPACE*m_lastPos.y() - FIL_PIECE_SIZE/2;

       int x2 = x1 + FIL_PIECE_SIZE;
       int y2 = y1;

       int x3 = x1;
       int y3 = y1 + FIL_PIECE_SIZE;

       int x4 = x2;
       int y4 = y3;

       painer.drawLine( QPoint(x1 , y1) , QPoint(x2 , y2));
       painer.drawLine( QPoint(x1 , y1) , QPoint(x3 , y3));
       painer.drawLine( QPoint(x2 , y2) , QPoint(x4 , y4));
       painer.drawLine( QPoint(x3 , y3) , QPoint(x4 , y4));
   }

   //绘制AI推荐位置（非填充圆）
   QPen aiPen;
   aiPen.setWidth(3);
   if(m_aiBest1Pos.x()!=-1&&m_aiBest1Pos.y()!=-1)
   {
       aiPen.setColor(QColor(255, 50, 50)); // 红色 — AI首选
       painer.setPen(aiPen);
       painer.setBrush(Qt::NoBrush);
       int cx = FIL_MARGIN_WIDTH + FIL_SPACE * (m_aiBest1Pos.x() + 1);
       int cy = FIL_MARGIN_HEIGHT + FIL_SPACE * (m_aiBest1Pos.y() + 1);
       painer.drawEllipse(QPoint(cx, cy), FIL_PIECE_SIZE/2 + 2, FIL_PIECE_SIZE/2 + 2);
   }
   if(m_aiBest2Pos.x()!=-1&&m_aiBest2Pos.y()!=-1)
   {
       aiPen.setColor(QColor(50, 150, 255)); // 蓝色 — AI次选
       painer.setPen(aiPen);
       painer.setBrush(Qt::NoBrush);
       int cx = FIL_MARGIN_WIDTH + FIL_SPACE * (m_aiBest2Pos.x() + 1);
       int cy = FIL_MARGIN_HEIGHT + FIL_SPACE * (m_aiBest2Pos.y() + 1);
       painer.drawEllipse(QPoint(cx, cy), FIL_PIECE_SIZE/2 + 2, FIL_PIECE_SIZE/2 + 2);
   }
   painer.setPen(Qt::NoPen); // 恢复默认

   //绘制移动
   if(m_isMove_fl)
   {
       painer.setBrush(m_pieceColor[getTurns()%2]);
       painer.drawEllipse((m_movePoint),
                          FIL_PIECE_SIZE/2,FIL_PIECE_SIZE/2);
   }

   event->accept();
}

void FiveInLine::mousePressEvent(QMouseEvent *event)
{
    if(!NotPlacedStep.empty()||iswaitexist)
        goto quit;
    if(isOver)
        goto quit;
    if(m_isSpectating)
        goto quit;
    if(m_status!=m_turns%2)
        goto quit;
    m_isMove_fl = true;
    m_movePoint=event->pos();
quit:
    event->accept();
}

void FiveInLine::mouseMoveEvent(QMouseEvent *event)
{
    if(!NotPlacedStep.empty()||iswaitexist)
        goto quit;
    if(isOver)
        goto quit;
    if(m_isSpectating)
        goto quit;
    if(m_status!=m_turns%2)
        goto quit;
    if(m_isMove_fl)
    {
        m_movePoint=event->pos();
    }
quit:
    event->accept();
}

void FiveInLine::mouseReleaseEvent(QMouseEvent *event)
{
    m_isMove_fl =false;
    int x;
    int y;
    float col = (float)event->pos().x();
    float row = (float)event->pos().y();
    if(!NotPlacedStep.empty()||iswaitexist)
        goto quit;
    if(isOver)
        goto quit;
    if(m_isSpectating)
        goto quit;
    if(m_status!=m_turns%2)
        goto quit;
    col = (col-FIL_MARGIN_WIDTH-FIL_SPACE)/FIL_SPACE;
    row = (row-FIL_MARGIN_HEIGHT-FIL_SPACE)/FIL_SPACE;
    x = col-(int)col>0.5?col+1:col;
    y = row - (int)row>0.5?row+1:row;
    if(isCrossLine(x,y))
    {
        return;
    }

    emit SIG_PIECEDOWN(getTurns(),x,y);
quit:
    event->accept();
}

int FiveInLine::getTurns()
{
   return m_turns;

}
void FiveInLine::changeTurns()
{
    if(++m_turns%2==1)
    {
        ui->lb_whoseturn->setText("白子回合");
    }
    else
    {
        ui->lb_whoseturn->setText("黑子回合");
    }
    m_curturns++;


}


bool FiveInLine::isCrossLine(int x , int y)
{
    if(x>=15||x<0||y<0||y>=15)
    {
        return true;
    }
    return false;
}

void FiveInLine::slot_piecedown(int turns, int x, int y)
{
    if (turns != m_turns)
    {
        QMessageBox::warning(this, QString::fromUtf8("提示"),
                             QString::fromUtf8("落子数据异常 turns=%1 m_turns=%2")
                                 .arg(turns).arg(m_turns));
        return;
    }
    if(m_board[x][y]==None)
    {
        if(!NotPlacedStep.empty())
        {
            waittoplacestep = QPair<int,int>(x,y);
            iswaitexist = true;
        }
        else
        { PlacedStep.push(QPair<int,int>(x,y));}
           m_board[x][y] = turns%2;
           m_lastPos = QPoint(x,y);
           m_everyStepPos.push_back(std::pair<int,int>(x,y));
           ++m_aiJobId; // 局面变了，作废在途 AI 结果

        if( isWin( x , y )){
            changeTurns();
            QString str = (turns)%2 == Black?"黑方胜":"白方胜";
            ui->lb_whowin->setText(str);
            if(iswaitexist)
            {
                m_board[waittoplacestep.first][waittoplacestep.second] = None;
                iswaitexist=false;
            }
            emit SIG_PLAYERWIN((turns)%2);

        }else
        {
            if( m_cpucolor != getTurns()%2 ){
                //更新玩家的每种赢法 玩家的棋子个数
                for( int i=0 ; i < static_cast<int>(m_vecwin.size()); i++ )
                {

                    if( m_vecwin[i].board[x][y] == 1 )
                    {
                        m_vecwin[i].playerCount += 1;
                        m_vecwin[i].cpuCount = 100; //无效的值
                    }
                }
            }else{
                //更新电脑的每种赢法 电脑的棋子个数
                for(int k = 0 ; k <static_cast<int>(m_vecwin.size()); ++k ){
                    if( m_vecwin[k].board[x][y] == 1 )
                    {
                        m_vecwin[k].cpuCount += 1;
                        m_vecwin[k].playerCount = 100;
                    }
                }
            }
             changeTurns();
             if(m_cpucolor==getTurns()%2)
             {
                 piecedownbycpu();
             }
             if(iswaitexist)
             {
                 m_board[waittoplacestep.first][waittoplacestep.second] = None;
                 m_curturns--;
             }
        }
        // 自动触发AI评估（局势已变化）
       notifyAIBestMoves();
    }

}

void FiveInLine::slot_startgame()
{
    clear();

    ui->lb_whoseturn->setText("黑子回合");
    ui->lb_whowin->setText("");
    isOver =false;

}
bool FiveInLine::isWin(int x,int y)
{
        // 看四条直线 八个方向的同色棋子个数 只要有一个个数到5 就结束了
        int count = 1;
        //循环 看四条线
        for( int dr = d_z ; dr < d_count ; dr += 2){
            //先看一条线
            // <--
            for( int i = 1; i <= 4 ; ++i)
            {
                //获取偏移后的棋子坐标
                int ix = dx[ dr ]*i + x;
                int iy = dy[ dr ]*i + y;
                //判断是否出界
                if( isCrossLine(ix , iy) ) break;
                //看是否同色
                if( m_board[ix][iy] == m_board[x][y] ) {
                    count++;
                }else break;
            }
            // -->
            for( int i = 1; i <= 4 ; ++i)
            {
                //获取偏移后的棋子坐标
                int ix = dx[ dr + 1 ]*i + x;
                int iy = dy[ dr + 1 ]*i + y;
                //判断是否出界
                if( isCrossLine(ix , iy) ) break;
                //看是否同色
                if( m_board[ix][iy] == m_board[x][y] ) {
                    count++;
                }else break;
            }
            if( count >= 5 ) break; //不在看其他的直线
            else count = 1;//不够要看其他直线
            }
        if(count>=5)
        {
            isOver = true;
            return true;
        }
        else
            return false;
}
void FiveInLine::clear()
{
    //状态位
    isOver = true;
    m_turns = Black;
    m_curturns =0;
    m_isMove_fl = false;
    m_cpucolor =None;
    m_lastPos = QPoint(); // 重置最后一步位置
    m_everyStepPos.clear(); // 清空步骤列表
    m_isSpectating = false; // 重置观战标志
    m_aiDebounceTimer.stop(); // 停止防抖定时器
    ++m_aiJobId; // 作废在途 AI 任务
    m_aiBest1Pos = QPoint(-1, -1); m_aiBest1Valid = false;   // 清除AI推荐
    m_aiBest2Pos = QPoint(-1, -1); m_aiBest2Valid = false;
    for(int i=0;i<static_cast<int>(m_vecwin.size());i++)
    {
        m_vecwin[i].clear();
    }
    //界面
    for( int x = 0 ; x < FIL_COLS ;++x ){
        for( int y = 0 ; y < FIL_ROWS ;++y){
            m_board[x][y] = None;
        }
    }
    PlacedStep.clear();
    NotPlacedStep.clear();
}
void FiveInLine::setselfstatus(int _status)
{
    m_status = _status;
}

void FiveInLine::initaivector()
{
    // -
    for(int y=0;y<FIL_ROWS;y++)
    {
        for(int x =0;x<FIL_COLS-4;x++)
        {
            stru_win w;
            for(int k=0;k<5;k++)
            {
                w.board[x+k][y] =1;

            }
            m_vecwin.push_back(w);
        }
    }
    //|
    for(int x=0;x<FIL_COLS;x++)
    {
        for(int y =0;y<FIL_ROWS-4;y++)
        {
            stru_win w;
            for(int k=0;k<5;k++)
            {
                w.board[x][y+k] =1;

            }
            m_vecwin.push_back(w);
        }
    }
    //
    for(int y=0;y<FIL_ROWS-4;y++)
    {
        for(int x=0;x<FIL_COLS-4;x++)
        {
            stru_win w;
            for(int k=0;k<5;k++)
            {
                w.board[x+k][y+k] =1;

            }
            m_vecwin.push_back(w);
        }
    }
    //
    for(int y=0;y<FIL_ROWS-4;y++)
    {
        for(int x=FIL_COLS-1;x>=4;x--)
        {
            stru_win w;
            for(int k=0;k<5;k++)
            {
                w.board[x-k][y+k] =1;

            }
            m_vecwin.push_back(w);
        }
    }
}

void FiveInLine::piecedownbycpu()
{
    // 调用高级AI（Minimax + Alpha-Beta剪枝）
    piecedownBybetterCpu();
}

// 低级AI实现（已保留但未使用）
void FiveInLine::piecedownByBasicCpu()
{
    if(isOver)return;
    if(m_cpucolor!=getTurns()%2) return;
    int u =0,v = 0;
    int x,y,k;
    int Myscore[FIL_ROWS][FIL_COLS] ={};
    int Cpuscore[FIL_ROWS][FIL_COLS] = {};
    int max =0;
    //估分 找到没有子的点，看每种赢法(有该点)，对应的棋子个数，进行这个点的分数计算
    for( x = 0 ; x < FIL_COLS ; ++x ){
        for( y = 0 ; y < FIL_ROWS ; ++y ){
            if( m_board[x][y] == None ){
                //遍历所有赢法
                for( k = 0 ; k < static_cast<int>(m_vecwin.size()); ++k ){
                    //评估玩家在 x, y 分数
                    if( m_vecwin[k].board[x][y] == 1 )
                    {
                        //根据该赢法，棋子个数
                        switch( m_vecwin[k].playerCount ){
                            case 1: Myscore[x][y] += 200;
                            break;
                            case 2: Myscore[x][y] += 400;
                            break;
                            case 3: Myscore[x][y] += 2000;
                            break;
                            case 4: Myscore[x][y] += 10000;
                            break;
                        }
                    }
                    //评估电脑在 x, y 分数
                    if( m_vecwin[k].board[x][y] == 1 )
                    {
                        //根据该赢法，棋子个数
                        switch( m_vecwin[k].cpuCount )
                        {
                            case 1: Cpuscore[x][y] += 220;
                            break;
                            case 2: Cpuscore[x][y] += 420;
                            break;
                            case 3: Cpuscore[x][y] += 2200;
                            break;
                            case 4: Cpuscore[x][y] += 200000;
                            break;
                        }
                    }

                }
            }
        }
    }
    //估分之后找最优点 -- 一定在无子的点
    for( x = 0 ; x < FIL_COLS ; ++x ){
        for( y = 0 ; y < FIL_ROWS ; ++y ){
            if( m_board[x][y] == None ){
                //先看玩家
                if( Myscore[x][y] > max ){
                    max = Myscore[x][y];
                    u = x;
                    v = y;
                }else if( Myscore[x][y] == max ){
                    if( Cpuscore[x][y] > Cpuscore[u][v] ){
                        u = x;
                        v = y;
                    }
                }
                //再看电脑
                if( Cpuscore[x][y] > max ){
                    max = Cpuscore[x][y];
                    u = x;
                    v = y;
                }else if( Cpuscore[x][y] == max ){
                    if( Myscore[x][y] > Myscore[u][v] ){
                        u = x;
                        v = y;
                    }
                }
            }
        }
    }

    emit SIG_PIECEDOWN(m_cpucolor,u,v);
}

void FiveInLine::setcpucolor(int color)
{
    m_cpucolor = color;
}



void FiveInLine::on_left_clicked()
{
    if(!PlacedStep.empty())
    {
        QPair<int,int>tmp = PlacedStep.top();
        NotPlacedStep.push(tmp);
        releasepiece(tmp);
        PlacedStep.pop();
    }
}


void FiveInLine::on_max_left_clicked()
{
    while(!PlacedStep.empty())
    {
        notifyAIBestMoves();

        QPair<int,int>tmp = PlacedStep.top();
        NotPlacedStep.push(tmp);
        releasepiece(tmp);
        PlacedStep.pop();
    }
}


void FiveInLine::on_right_clicked()
{
    if(!NotPlacedStep.empty())
    {
        QPair<int,int>tmp = NotPlacedStep.top();
        PlacedStep.push(tmp);
        reloadpiece(tmp);
        NotPlacedStep.pop();
        if(NotPlacedStep.empty()&&iswaitexist)
        {
            NotPlacedStep.push(waittoplacestep);
            iswaitexist = false;
        }
    }

}
void FiveInLine::on_max_right_clicked()
{
    while(!NotPlacedStep.empty())
    {
        QPair<int,int>tmp = NotPlacedStep.top();
        PlacedStep.push(tmp);
        reloadpiece(tmp);
        NotPlacedStep.pop();
        if(NotPlacedStep.empty()&&iswaitexist)
        {
            NotPlacedStep.push(waittoplacestep);
            iswaitexist = false;
        }
    }

}
void FiveInLine::releasepiece(QPair<int, int>tmp)
{
    m_board[tmp.first][tmp.second] = None;
    m_curturns--;
    // 同步 m_everyStepPos 和 m_lastPos
    if(!m_everyStepPos.empty())
    {
        m_everyStepPos.pop_back();
    }
    if(!m_everyStepPos.empty())
    {
        m_lastPos = QPoint(m_everyStepPos.back().first, m_everyStepPos.back().second);
    }
    else
    {
        m_lastPos = QPoint();
    }
    notifyAIBestMoves();

}
void FiveInLine::reloadpiece(QPair<int, int>tmp)
{
    m_board[tmp.first][tmp.second] = m_curturns%2;
    m_curturns++;
    // 同步 m_everyStepPos 和 m_lastPos
    m_everyStepPos.push_back(std::pair<int,int>(tmp.first, tmp.second));
    m_lastPos = QPoint(tmp.first, tmp.second);

    notifyAIBestMoves();

}
void FiveInLine::setSpectating(bool spectating)
{
    m_isSpectating = spectating;
}







void FiveInLine::piecedownBybetterCpu()
{
    if( isOver ) return;

    AiWorker::Task task;
    task.id = ++m_aiJobId;
    task.kind = PlaceMove;
    task.player = getTurns();
    task.depth = MAX_DEPTH;
    task.board = m_board;
    task.everyStep = m_everyStepPos;
    emit SIG_SEND_TASK(task);

}




// 请求AI评估（防抖：多次快速调用合并为一次，300ms后执行）
// 非观战/回看模式不触发评估（玩家不需要看到AI推荐）
void FiveInLine::notifyAIBestMoves()
{
    if(!m_isSpectating) return;
    m_aiDebounceTimer.start(300);
}

// 防抖后真正执行AI评估并发送信号
void FiveInLine::slot_notifyAIBestMoves()
{
    m_aiDebounceTimer.stop();
    if(!m_isSpectating) return;

    // 获取下一步的玩家（m_curturns 已落子数，%2 即为下一步颜色）
    int nextPlayer = m_curturns % 2;
    AiWorker::Task task;
    task.id = ++m_aiJobId;
    task.kind = TopN;
    task.player = nextPlayer;
    task.topN =2;
    task.depth = MAX_DEPTH;
    task.board = m_board;
    task.everyStep = m_everyStepPos;
    emit SIG_SEND_TASK(task);
}
void FiveInLine::slot_onAiResult(AiWorker::Result r) {
    if (r.id != m_aiJobId) return;   // 过期：新落子/新请求已发出
    if (r.kind == PlaceMove) {
        // PlaceMove 填的是 bestX/bestY，不是 b1x/b1y
        if (isOver) return;
        if (m_cpucolor != getTurns() % 2) return;
        if (r.bestX < 0 || r.bestY < 0) return;
        emit SIG_PIECEDOWN(getTurns(), r.bestX, r.bestY);
    }
    else
    {
        if (!m_isSpectating) return;
        if(r.b1x!=-1 && r.b2y!=-1)
        {

            m_aiBest1Pos = QPoint(r.b1x, r.b1y);
        }
        else
        {
            m_aiBest1Pos = QPoint(-1, -1);

        }
        if(r.b2x!=-1 && r.b2y!=-1)
        {

            m_aiBest2Pos = QPoint(r.b2x, r.b2y);
        }
        else
        {
            m_aiBest2Pos = QPoint(-1, -1);

        }
        if(m_everyStepPos.empty())
        {
            m_aiBest1Pos = QPoint(8, 7);
            r.b1x =8;
            r.b1y =7;
        }
        emit SIG_AI_BEST_MOVES(r.player, r.b1x,r.b1y,r.b1s,r.b2x,r.b2y,r.b2s);
    }
}

