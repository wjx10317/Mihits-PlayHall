#include "widget.h"
#include "ui_widget.h"
#include<QMessageBox>
FiveInLine::FiveInLine(QWidget *parent)
    : QWidget(parent),
      ui(new Ui::FiveInLine),m_board(FIL_ROWS,std::vector<int>(FIL_COLS,2/*2表示无*/)),
      m_movePoint(0,0),m_isMove_fl(false),m_turns(0),isOver(false),m_status(Black),m_cpucolor(White)
{
    ui->setupUi(this);
    m_pieceColor[Black] = QBrush(QColor(0,0,0));
    m_pieceColor[White] = QBrush(QColor(255,255,255));
    m_pieceColor[None] = QBrush(QColor(0,0,0,0));
    connect(this,SIGNAL(SIG_PIECEDOWN(int,int,int)),this,SLOT(slot_piecedown(int,int,int)));
    connect(&m_timer,SIGNAL(timeout()),this,SLOT(repaint()));
    m_timer.start(1000/25);
    clear();
    slot_startgame();
    initaivector();
}

FiveInLine::~FiveInLine()
{
    delete ui;
}

void FiveInLine::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);//用于绘图使用有参构造传入当前对象得到当前对象的画刷
    painter.setBrush(QBrush(QColor(255,160,0)));
    painter.drawRect(FIL_MARGIN_WIDTH-FIL_DISTANCE,
                    FIL_MARGIN_HEIGHT-FIL_DISTANCE,
                    (FIL_COLS+1)*FIL_SPACE+2*FIL_DISTANCE,(FIL_ROWS+1)*FIL_SPACE+2*FIL_DISTANCE);
    //绘制网格线
    // |
    for( int i = 1 ; i <= FIL_COLS ; ++i ){
        painter.setBrush( QBrush( QColor( 0 , 0 , 0 ) ) ); //通过画刷设置颜色
        QPoint p1( FIL_MARGIN_WIDTH + FIL_SPACE*i , FIL_MARGIN_HEIGHT + FIL_SPACE );
        QPoint p2( FIL_MARGIN_WIDTH + FIL_SPACE*i , FIL_MARGIN_HEIGHT + (FIL_ROWS)*FIL_SPACE );
        painter.drawLine( p1 , p2);
    }
    //绘制网格线
    // -
    for( int i = 1 ; i <= FIL_ROWS ; ++i ){
        painter.setBrush( QBrush( QColor( 0 , 0 , 0 ) ) ); //通过画刷设置颜色
        QPoint p1( FIL_MARGIN_WIDTH+FIL_SPACE, FIL_MARGIN_HEIGHT + FIL_SPACE*i );
        QPoint p2( FIL_MARGIN_WIDTH + (FIL_COLS)*FIL_SPACE , FIL_MARGIN_HEIGHT + FIL_SPACE*i );
        painter.drawLine( p1 , p2);
    }
    painter.setBrush( QBrush( QColor( 0 , 0 , 0 ) ));

    painter.drawEllipse(QPoint(290,290),6/2,6/2);

    //绘制棋子

   for(int x =0;x<FIL_ROWS;x++)
   {
       for(int y=0;y<FIL_COLS;y++)
       {
           painter.setBrush(m_pieceColor[m_board[x][y]]);
           if(m_board[x][y]!=None)
           {
               painter.drawEllipse(QPoint(FIL_MARGIN_WIDTH+FIL_SPACE*(x+1),FIL_MARGIN_HEIGHT+FIL_SPACE*(y+1)),
                                  FIL_PIECE_SIZE/2,FIL_PIECE_SIZE/2);
           }
       }
   }
   //绘制移动
   if(m_isMove_fl)
   {
       painter.setBrush(m_pieceColor[getTurns()%2]);
       painter.drawEllipse((m_movePoint),
                          FIL_PIECE_SIZE/2,FIL_PIECE_SIZE/2);
   }
   //绘制上一次落子的位置
   if( !m_lastPos.isNull()){
       painter.setBrush( QBrush( QColor( 0 , 250 , 0 ) ) ); //通过画刷设置颜色
       int x1 = FIL_MARGIN_WIDTH + FIL_SPACE + FIL_SPACE*m_lastPos.x() - FIL_PIECE_SIZE/2; //左上
       int y1 = FIL_MARGIN_HEIGHT+FIL_SPACE + FIL_SPACE*m_lastPos.y() - FIL_PIECE_SIZE/2;

       int x2 = x1 + FIL_PIECE_SIZE;//右上
       int y2 = y1;

       int x3 = x1; // 左下
       int y3 = y1 + FIL_PIECE_SIZE;

       int x4 = x2; //右下
       int y4 = y3;

       painter.drawLine( QPoint(x1 , y1) , QPoint(x2 , y2));
       painter.drawLine( QPoint(x1 , y1) , QPoint(x3 , y3));
       painter.drawLine( QPoint(x2 , y2) , QPoint(x4 , y4));
       painter.drawLine( QPoint(x3 , y3) , QPoint(x4 , y4));
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
        m_board[x][y] = turns%2;
        m_lastPos = QPoint(x,y);
        m_everyStepPos.push_back(std::pair<int,int>(x,y));
        if( isWin( x , y ) ){
            QString str = turns%2 == Black?"黑方胜":"白方胜";
            ui->lb_whowin->setText(str);
            Q_EMIT SIG_PLAYERWIN(turns%2);
        }else
        {
            if( m_cpucolor != getTurns()%2 ) {
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
                 //piecedownbycpu();
                 piecedownBybetterCpu();
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

bool FiveInLine::isWin(int x, int y, std::vector<std::vector<int> > &board)
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
            if( board[ix][iy] == board[x][y] ) {
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
            if( board[ix][iy] == board[x][y] ) {
                count++;
            }else break;
        }
        if( count >= 5 ) break; //不在看其他的直线
        else count = 1;//不够要看其他直线
        }
    if(count>=5)
    {
        return true;
    }
    else
        return false;
}

int FiveInLine::minmax(std::vector<std::vector<int> > &copyBoard, std::vector<std::pair<int, int> > &copyEveryStep
                       , int depth, int alpha, int beta, bool isMaximizing, int player)
{
    //其本质上是 dfs 只是 max层 取子节点的max值 , min层取 子节点的min值
    //分别对应 ai 最大化自己的得分 以及 玩家最小化 ai得分
    //一会实现剪枝
    //递归终点 评估棋盘
    if( depth == 0 ) {
       return evaluateBoard(copyBoard);
    }
    int bestValue = isMaximizing ? INT_MIN : INT_MAX; // max层取最小 便于更新
    int opponent = ( player%2 == Black )? White:Black;

    std::vector<pair<int,int>> candidates;
    getNeedHandlePos( copyEveryStep , candidates , copyBoard );
    if( candidates.empty() ) return 0;

    //遍历所有可能位置
    for(auto& pos : candidates){
        int x = pos.first;
        int y = pos.second;

        // 逻辑和 findBestMove类似
        // 尝试落子
        copyBoard[x][y] = isMaximizing ? player%2 : opponent;
        copyEveryStep.push_back( {x,y} );

        //检查是否获胜
        bool win = isWin( x, y , copyBoard );
        if(win){
            //返回不需要继续搜索
            copyBoard[x][y] = None;
            copyEveryStep.pop_back();
            return isMaximizing?1000000:-1000000; // 其实就是 正无穷和负无穷
        }

        //dfs 并撤销
        int value = minmax( copyBoard , copyEveryStep , depth-1 , alpha , beta , !isMaximizing , player);
        //撤销
        copyBoard[x][y] = None;
        copyEveryStep.pop_back();

        //更新最佳 //实现 alpha-beta剪枝
        if( isMaximizing ){
            if( value > bestValue ){
                bestValue = value;

            }
            if( bestValue > alpha){
                alpha = bestValue;
                if( alpha >= beta ) break; //alpha 剪枝
            }

        }else{
            if( value < bestValue )
            {
                bestValue = value;

            }
            if( bestValue < beta )
            {
                beta = bestValue;
                if(beta<=alpha)
                    break;//beta剪枝
            }

        }

    }
    return bestValue;
}

int FiveInLine::evaluateBoard(int color, vector<vector<int> > &board)
{
    int value =0;
    for(int x = 0;x<FIL_COLS;x++)
    {
        for(int y  = 0;y<FIL_ROWS;y++)
        {
            if(board[x][y]!=color)
                continue;
            // 处理8个方向
            int j=d_z;
                        while(j<d_count)
                        {
                            int count =1;
                            vector<int>record;
                            for(int a = 0;a<2;a++)
                            {
                                int curx =x;
                                int cury =y;
                                for(int i=0;i<4;i++)
                                {

                                   if(isCrossLine(curx+dx[j+a],cury+dy[j+a]))
                                   {
                                      record.push_back(-1);
                                       break;
                                   }
                                   curx+=dx[j+a];
                                   cury+=dy[j+a];
                                   if(board[curx][cury]==color)
                                   {
                                       count++;
                                   }
                                   else
                                   {
                                       // 空位：该端开放（活）
                                       record.push_back(board[curx][cury]);
                                       break;
                                   }
                                }
                            }

                //看完一组的个数 可以看是看棋子得分
                // 得分表如下：
                // 五连 200000  活四 10000 必赢
                // 死四 1000  活三 900 一定要拦截 补一个子就赢
                // 死三 100
                // 活二 90
                // 死二 10
                if( count >= 5 ) {
                    value += 200000;
                }else if( count == 4 ){
                    if( record[0] == 2 && record[1] == 2){
                        //huosi
                        value += 10000;
                    }else if( (record[0] == 2) ||
                              (record[1] == 2)){
                        //sisi
                        value += 1000;
                    }
                }else if( count == 3 ){
                    if( record[0] == 2 && record[1] == 2){
                        //huosan
                        value += 900;
                    }else if( (record[0] == 2 ) ||
                              (record[1] == 2 )){
                        //sisan
                        value += 100;
                    }
                }else if( count ==2){
                    if( record[0] == 2 && record[1] == 2){
                        //huoer
                        value += 90;
                    }else if( (record[0] == 2) ||
                              (record[1] == 2)){
                        //sier
                        value += 10;
                    }
                }
                j+=2;
            }
        }
    }
    return value;
}

int FiveInLine::evaluateBoard(vector<vector<int> > &board)
{
   int white =evaluateBoard(White,board);
   int black = evaluateBoard(Black,board);
   return white-black;
}

void FiveInLine::piecedownBybetterCpu()
{
    if( isOver ) return;

    int bestX = -1; int bestY = -1;

    findBestMove( bestX , bestY , getTurns() , MAX_DEPTH );

    if( bestX != -1 && bestY != -1 ){
        //找到最佳点 落子
        emit SIG_PIECEDOWN( getTurns() , bestX , bestY );
    }
}

void FiveInLine::findBestMove(int &bestX, int &bestY, int player, int depth)
{
    int bestValue = INT_MIN;
    bestX = -1;
    bestY = -1;

    vector<pair<int, int>> candidates;
    vector<pair<int, int>> copyEveryStep = m_everyStepPos; // 避免出现问题 我们使用拷贝的
    vector<vector<int>> copyBoard = m_board;//同理棋盘也是

    //获取要看的点
    getNeedHandlePos( copyEveryStep , candidates , copyBoard);
    int min = INT_MIN;
    int max = INT_MAX;
    //遍历所有可能位置
    for( auto& pos : candidates){
        int x = pos.first;
        int y = pos.second;

        //尝试在该位置下子 如果直接能获得胜利 那么就返回
        copyBoard[x][y] = player%2;
        if( isWin(x , y , copyBoard )){
            bestX = x;
            bestY = y;
            return;
        }
        //尝试在该位置 敌人下子 如果能获得胜利 那么也返回

        copyBoard[x][y] = (player%2 == Black)? White:Black;
        if( isWin(x , y , copyBoard )){
            bestX = x;
            bestY = y;
            return;
        }
        //切换回来 开始 搜索
        copyBoard[x][y] = player%2;
        copyEveryStep.push_back({x, y});

        //使用 极大值极小值搜索 配合 α-β剪枝
        int moveValue = minmax(copyBoard,copyEveryStep,depth-1,min,max,false,player);
        // dfs 撤销该点落子
        copyBoard[x][y] = None;
        copyEveryStep.pop_back();

        if( moveValue > bestValue ){
            bestValue = moveValue;
            bestX = x;
            bestY = y;
        }
        if(bestValue>min)
        {
            min = bestValue;
        }
    }
}
#include<unordered_set>
void FiveInLine::getNeedHandlePos(std::vector<std::pair<int, int>> &copyEveryStep,
                                  std::vector<std::pair<int, int>> &candidates,
                                  std::vector<std::vector<int>> &board)
{
    std::unordered_set<int> unique;
    //使用反向迭代器 反向遍历
    for( auto ite = copyEveryStep.rbegin() ; ite != copyEveryStep.rend() ; ++ite){
        auto & pos = *ite;
        int x = pos.first;
        int y = pos.second;
        for( int i = -1 ; i <= 1 ; ++i){
            for( int j = -1 ; j <= 1 ; ++j){
                if( i == 0 && j == 0 ) continue;
                int nx = x + i;
                int ny = y + j;

                //判断出界
                if( isCrossLine(nx,ny) ) continue;
                if( board [ nx ][ny] != None ) continue; // 只看空白点

                //去重
                int key = nx * 100 + ny ; // nx ny 不会超过15
                if( unique.count( key ) == 0 ) //没有
                {
                    unique.insert(key);
                    candidates.push_back( { nx , ny } );
                }
            }
        }
    }
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
    int u,v;
    int x,y,k;
    int Myscore[FIL_ROWS][FIL_COLS] ={};
    int Cpuscore[FIL_ROWS][FIL_COLS] = {};
    int max =0;
    //估分 找到没有子的点，看每种赢法(有该点)，对应的棋子个数，进行这个点的分数计算
    for( y = 0 ; y < FIL_ROWS ; ++y ){
        for(x = 0 ; x < FIL_COLS ; ++x ){
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
                            case 3: Cpuscore[x][y] += 4100;
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
