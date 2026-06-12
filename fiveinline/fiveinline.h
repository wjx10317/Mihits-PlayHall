#ifndef FIVEINLINE_H
#define FIVEINLINE_H

#include <QWidget>
#include<QPainter>
#include<QPaintEvent>
#include<QMouseEvent>
#include<QTimer>
#include<QStack>
#include<QPair>
QT_BEGIN_NAMESPACE
namespace Ui { class FiveInLine; }
QT_END_NAMESPACE

// 界面是 580*580 固定

// 外边距
#define FIL_MARGIN_HEIGHT     (50)
#define FIL_MARGIN_WIDTH      (50)
// 行列数 五子棋规范为 15*15
#define FIL_COLS              (15)
#define FIL_ROWS              (15)
// 边和边之间的边距
#define FIL_SPACE   (30)
// 棋子的大小
#define FIL_PIECE_SIZE  (28)
// 棋盘边缘缩进的距离
#define FIL_DISTANCE      (10)

// 1. 首先绘制棋盘 网格线 棋子
#include<vector>
struct stru_win
{
    stru_win(): board( FIL_COLS , std::vector<int>( FIL_ROWS , 0 ) )
    , playerCount(0), cpuCount(0)
    {

    }
    void clear()
    {
        playerCount=0;
        cpuCount =0;
    }
    std::vector<std::vector<int>> board; //胜利的棋子布局
    int playerCount; //该赢法玩家的棋子个数
    int cpuCount; //该赢法电脑的棋子个数
};
class FiveInLine : public QWidget
{
    Q_OBJECT
signals:
    void SIG_PIECEDOWN(int ,int,int);
    void SIG_PLAYERWIN(int);
public:
    FiveInLine(QWidget *parent = nullptr);
    ~FiveInLine();
    void paintEvent(QPaintEvent*);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    int getTurns();
    void changeTurns();

    bool isCrossLine(int ,int);
    bool isWin(int x,int y);
    void clear();
    void setselfstatus(int _status);
    /*ai*/
    void initaivector();
    void piecedownbycpu();
    void setcpucolor(int);


    void releasepiece(QPair<int,int>);
    void reloadpiece(QPair<int,int> tmp);

public slots:
    void slot_piecedown(int ,int,int);
    void slot_startgame();


private:
    Ui::FiveInLine *ui;

    std::vector<std::vector<int>>m_board;
    QPoint m_movePoint;
    bool m_isMove_fl;
    QBrush m_pieceColor[3];
    int m_turns;
    int m_curturns;
    QTimer m_timer;
    bool isOver;
    int m_status;
    bool iswaitexist;
    // 判断输赢时需要的方向
    enum enum_direction{ d_z , d_y , d_s , d_x , d_zs , d_yx , d_zx , d_ys ,d_count};
    //根据方向对坐标的偏移 每次是一个单位
    int dx[ d_count ] = {-1, 1, 0, 0, -1, 1, -1, 1};
    int dy[ d_count ] = {0, 0, -1, 1, -1, 1, 1, -1};
    QStack<QPair<int,int>>PlacedStep;
    QStack<QPair<int,int>>NotPlacedStep;
    QPair<int,int>waittoplacestep;



    //初级ai
    std::vector<stru_win>m_vecwin;
    int m_cpucolor;
    //高级ai

public:
    enum ENUM_BLACK_OR_WHITE{Black=0,White,None};

private slots:
    void on_left_clicked();
    void on_max_left_clicked();
    void on_right_clicked();
    void on_max_right_clicked();
};
#endif // FIVEINLINE_H
