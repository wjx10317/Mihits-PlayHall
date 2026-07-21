#include"ai_worker.h"
#include <QMetaType>
#include <climits>
#include <unordered_set>

AiWorker::AiWorker(QObject* parent)
    : QObject(parent)
{
    qRegisterMetaType<AiWorker::Task>("AiWorker::Task");
    qRegisterMetaType<AiWorker::Result>("AiWorker::Result");
}

AiWorker::~AiWorker()
{
}

void AiWorker::slot_doTask(AiWorker::Task task)
{

    Result r;
    r.id = task.id;
    r.kind = task.kind;
    r.player = task.player;
    r.bestX = r.bestY = -1;
    r.b1x = r.b1y = r.b2x = r.b2y = -1;
    r.b1s = r.b2s = 0;
    if(task.kind==TopN)
    {
        // 获取AI推荐的前2个最佳位置
        std::vector<std::pair<int,int>> topMoves;
        std::vector<int> scores;
        getTopNMoves(topMoves,scores,task);
        if(topMoves.size() >= 1 && topMoves[0].first!=-1 && topMoves[0].second!=-1)
        {
            r.b1x = topMoves[0].first;
            r.b1y = topMoves[0].second;
            r.b1s = scores[0];

        }
        if(topMoves.size() >= 2 && topMoves[1].first!=-1 && topMoves[1].second!=-1)
        {
            r.b2x = topMoves[1].first;
            r.b2y = topMoves[1].second;
            r.b2s = scores[1];
        }

    }
    else if(task.kind==PlaceMove)
    {
        findBestMove(r.bestX,r.bestY,task);
    }
    emit SIG_RESULT(r);
    return;
}
bool AiWorker::isWin(int x, int y, std::vector<std::vector<int> > &board)
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
bool AiWorker::isCrossLine(int x , int y)
{
    if(x>=15||x<0||y<0||y>=15)
    {
        return true;
    }
    return false;
}
int AiWorker::minmax(std::vector<std::vector<int>> &copyBoard, std::vector<std::pair<int, int> > &copyEveryStep
                       , int depth, int alpha, int beta, bool isMaximizing, int player)
{
    if( depth == 0 ) {
        int score = evaluateBoard(copyBoard);          // white - black
        return (player % 2 ==0)? -score : score;
    }
    int bestValue = isMaximizing ? INT_MIN : INT_MAX;
    int opponent = ( player%2 == Black )? White:Black;

    std::vector<std::pair<int,int>> candidates;
    getNeedHandlePos( copyEveryStep , candidates , copyBoard );
    if( candidates.empty() ) return 0;

    for(auto& pos : candidates){
        int x = pos.first;
        int y = pos.second;

        copyBoard[x][y] = isMaximizing ? player%2 : opponent;
        copyEveryStep.push_back( {x,y} );

        bool win = isWin( x, y , copyBoard );
        if(win){
            copyBoard[x][y] = None;
            copyEveryStep.pop_back();
            return isMaximizing?1000000:-1000000;
        }

        int value = minmax( copyBoard , copyEveryStep , depth-1 , alpha , beta , !isMaximizing , player);
        copyBoard[x][y] = None;
        copyEveryStep.pop_back();

        if( isMaximizing ){
            if( value > bestValue ){
                bestValue = value;
            }
            if( bestValue > alpha){
                alpha = bestValue;
                if( alpha >= beta ) break;
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
                    break;
            }

        }

    }
    return bestValue;
}
int AiWorker::evaluateBoard(int color, std::vector<std::vector<int>>& board)
{
    int value =0;
    for(int x =0;x<FIL_COLS;x++)
    {
        for(int y  =0;y<FIL_ROWS;y++)
        {
            if(board[x][y]!=color)
                continue;
            int j=d_z;
                        while(j<d_count)
                        {
                            int count =1;
                            std::vector<int>record;
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
                                       record.push_back(board[curx][cury]);
                                       break;
                                   }
                                }
                            }

                if( count >= 5 ) {
                    value += 200000;
                }else if( count == 4 ){
                    if( record[0] == 2 && record[1] == 2){
                        value += 10000;
                    }else if( (record[0] == 2) ||
                              (record[1] == 2)){
                        value += 1000;
                    }
                }else if( count == 3 ){
                    if( record[0] == 2 && record[1] == 2){
                        value += 900;
                    }else if( (record[0] == 2 ) ||
                              (record[1] == 2 )){
                        value += 100;
                    }
                }else if( count == 2){
                    if( record[0] == 2 && record[1] == 2){
                        value += 90;
                    }else if( (record[0] == 2) ||
                              (record[1] == 2)){
                        value += 10;
                    }
                }
                j+=2;
            }
        }
    }
    return value;
}
int AiWorker::evaluateBoard(std::vector<std::vector<int>>& board)
{
   int white =evaluateBoard(White,board);
   int black = evaluateBoard(Black,board);
   return white-black;
}
void AiWorker::getNeedHandlePos(std::vector<std::pair<int, int>> &copyEveryStep,
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
void AiWorker::findBestMove(int &bestX, int &bestY,Task task)
{
    int bestValue = INT_MIN;
    int player = task.player;
    int depth = task.depth;
    bestX = -1;
    bestY = -1;
    if(task.everyStep.empty())
    {
        bestX = 7;
        bestY = 8;

        return;
    }

    std::vector<std::pair<int, int>> candidates;
    std::vector<std::pair<int, int>> copyEveryStep = task.everyStep;
    std::vector<std::vector<int>> copyBoard = task.board;

    getNeedHandlePos( copyEveryStep , candidates , copyBoard );
    int min = INT_MIN;
    int max = INT_MAX;
    for( auto& pos : candidates){
        int x = pos.first;
        int y = pos.second;

        copyBoard[x][y] = player%2;
        if( isWin( x , y , copyBoard )){
            bestX = x;
            bestY = y;
            return;
        }

        copyBoard[x][y] = (player%2 == Black)? White:Black;
        if( isWin( x , y , copyBoard )){
            bestX = x;
            bestY = y;
            return;
        }
        copyBoard[x][y] = player%2;
        copyEveryStep.push_back({x, y});

        int moveValue = minmax(copyBoard,copyEveryStep,depth-1,min,max,false,player);
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

void AiWorker::getTopNMoves(std::vector<std::pair<int, int>>& topMoves,
                      std::vector<int>& scores,Task task)
{
    int player = task.player;
    int n= task.topN;
    int depth = task.depth;
    std::vector<std::pair<int, int>>everyStep = task.everyStep;
    std::vector<std::vector<int>> Board = task.board;
    topMoves.clear();
    scores.clear();

    // 获取候选位置
    std::vector<std::pair<int, int>> candidates;
    std::vector<std::pair<int, int>> copyEveryStep = everyStep;
    std::vector<std::vector<int>> copyBoard = Board;
    getNeedHandlePos( copyEveryStep , candidates , copyBoard );

    if(candidates.empty()) return;

    // 评估每个候选位置
    std::vector<std::pair<int, std::pair<int, int>>> scoredMoves;

    for(auto& pos : candidates){
        int x = pos.first;
        int y = pos.second;

        // 1. 检查玩家在此落子是否获胜
        copyBoard[x][y] = player%2;
        if(isWin( x, y , copyBoard )){
            scoredMoves.push_back({1000001, {x, y}});
            copyBoard[x][y] = None;
            continue;
        }

        // 2. 检查对手在此落子是否获胜（防守必堵点）
        int opponent = (player%2 == Black) ? White : Black;
        copyBoard[x][y] = opponent;
        if(isWin( x, y , copyBoard ))
        {
            scoredMoves.push_back({1000001, {x, y}});
            copyBoard[x][y] = None;
            continue;
        }
        // 恢复为玩家落子，后续minimax基于玩家已落子的棋盘
        copyBoard[x][y] = player%2;

        // 3. minimax评估（基于玩家已落子的正确棋盘）
        copyEveryStep.push_back({x, y});
        int score = minmax(copyBoard, copyEveryStep, depth - 1, INT_MIN, INT_MAX, false, player);
        copyBoard[x][y] = None;
        copyEveryStep.pop_back();
        scoredMoves.push_back({score, {x, y}});
    }

    // 按分数排序（降序）
    std::sort(scoredMoves.begin(), scoredMoves.end(),
              [](const std::pair<int, std::pair<int, int>>& a,
                 const std::pair<int, std::pair<int, int>>& b){
                  return a.first > b.first;
              });

    // 获取前N个最佳位置
    int count = std::min(n, (int)scoredMoves.size());
    for(int i = 0; i < count; i++){
        scores.push_back(scoredMoves[i].first);
        topMoves.push_back(scoredMoves[i].second);
    }
}

