#ifndef DEF_H
#define DEF_H
//枚举棋盘上的位置的几种状态
enum ENUM_BLACK_OR_WHITE{Black=0,White,None};
// 判断输赢时需要的方向
enum enum_direction{ d_z , d_y , d_s , d_x , d_zs , d_yx , d_zx , d_ys ,d_count};
static const int dx[ d_count ] = {-1, 1, 0, 0, -1, 1, -1, 1};
static const int dy[ d_count ] = {0, 0, -1, 1, -1, 1, 1, -1};

enum Kind { PlaceMove, TopN };
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
#define MAX_DEPTH (4)
#endif // DEF_H
