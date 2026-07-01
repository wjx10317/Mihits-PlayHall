#include "fiveinline.h"
#include "ui_fiveinline.h"
#include<QMessageBox>
FiveInLine::FiveInLine(QWidget *parent)
    : QWidget(parent),
      ui(new Ui::FiveInLine),m_board(FIL_ROWS,std::vector<int>(FIL_COLS,2/*2表示无*/)),
      m_movePoint(0,0),m_isMove_fl(false),m_turns(0),isOver(false),m_status(Black),iswaitexist(false),waittoplacestep(QPair<int,int>(-1,-1)),m_cpucolor(None)
{
    ui->setupUi(this);
    m_pieceColor[Black] = QBrush(QColor(0,0,0));
    m_pieceColor[White] = QBrush(QColor(255,255,255));
    m_pieceColor[None] = QBrush(QColor(0,0,0,0));
    //connect(this,SIGNAL(SIG_PIECEDOWN(int,int,int)),this,SLOT(slot_piecedown(int,int,int)));
    connect(&m_timer,SIGNAL(timeout()),this,SLOT(repaint()));
    m_timer.start(1000/25);
    clear();
    //slot_startgame();
    initaivector();
}

FiveInLine::~FiveInLine()
{
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
    if(isOver)
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
    if(isOver)
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
    if(isOver)
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

        if( isWin( x , y )){
            changeTurns();
            QString str = (turns)%2 == Black?"黑方胜":"白方胜";
            ui->lb_whowin->setText(str);
            if(iswaitexist)
            {
                m_board[waittoplacestep.first][waittoplacestep.second] = None;
            }
            Q_EMIT SIG_PLAYERWIN((turns)%2);
        }else
        {
            if( m_cpucolor != getTurns()%2 ){
                //更新玩家的每种赢法 玩家的棋子个数
                for( int i=0 ; i < m_vecwin.size() ; i++ )
                {

                    if( m_vecwin[i].board[x][y] == 1 )
                    {
                        m_vecwin[i].playerCount += 1;
                        m_vecwin[i].cpuCount = 100; //无效的值
                    }
                }
            }else{
                //更新电脑的每种赢法 电脑的棋子个数
                for(int k = 0 ; k < m_vecwin.size() ; ++k ){
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
    m_isMove_fl = false;
    m_cpucolor =None;
    for(int i=0;i<m_vecwin.size();i++)
    {
        m_vecwin[i].clear();
    }
    //界面
    for( int x = 0 ; x < FIL_COLS ;++x ){
        for( int y = 0 ; y < FIL_ROWS ;++y){
            m_board[x][y] = None;
        }
    }
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
                for( k = 0 ; k < m_vecwin.size() ; ++k ){
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

    Q_EMIT SIG_PIECEDOWN(m_cpucolor,u,v);
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
}
void FiveInLine::reloadpiece(QPair<int, int>tmp)
{
    m_board[tmp.first][tmp.second] = m_curturns%2;
    m_curturns++;
}

