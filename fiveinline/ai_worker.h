#ifndef AI_WORKER_H
#define AI_WORKER_H
#include <QObject>
#include <QMetaType>
#include <vector>
#include <utility>
#include "def.h"

class AiWorker : public QObject {
    Q_OBJECT
public:
    struct Task {
        quint64 id = 0;
        Kind kind = TopN;
        int player = 0;
        int depth = 0;
        int topN = 0;
        std::vector<std::vector<int>> board;
        std::vector<std::pair<int,int>> everyStep;
    };

    struct Result {
        quint64 id = 0;
        Kind kind = TopN;
        int player = 0;
        int bestX = -1, bestY = -1;
        int b1x = -1, b1y = -1, b1s = 0;
        int b2x = -1, b2y = -1, b2s = 0;
    };

    explicit AiWorker(QObject* parent = nullptr);
    ~AiWorker() override;

    // 普通成员：不要放进 slots，否则 moc 要解析 std::vector 等类型会挂
    bool isWin(int x, int y, std::vector<std::vector<int>>& board);
    bool isCrossLine(int x, int y);
    void getNeedHandlePos(std::vector<std::pair<int, int>>& copyEveryStep,
                          std::vector<std::pair<int, int>>& candidates,
                          std::vector<std::vector<int>>& board);
    int evaluateBoard(int color, std::vector<std::vector<int>>& board);
    int evaluateBoard(std::vector<std::vector<int>>& board);
    void findBestMove(int& bestX, int& bestY, Task task);
    int minmax(std::vector<std::vector<int>>& copyBoard,
               std::vector<std::pair<int, int>>& copyEveryStep,
               int depth, int alpha, int beta, bool isMaximizing, int player);
    void getTopNMoves(std::vector<std::pair<int, int>>& topMoves,
                      std::vector<int>& scores, Task task);

public slots:
    void slot_doTask(AiWorker::Task task);

signals:
    void SIG_RESULT(AiWorker::Result r);
};

Q_DECLARE_METATYPE(AiWorker::Task)
Q_DECLARE_METATYPE(AiWorker::Result)

#endif // AI_WORKER_H
