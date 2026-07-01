// 自动触发AI评估并发送信号
void FiveInLine::notifyAIBestMoves()
{
    // 如果游戏已结束，不评估
    if(isOver) return;
    
    // 获取下一步的玩家
    int nextPlayer = (m_turns % 2 == Black) ? White : Black;
    
    // 获取AI推荐的前2个最佳位置
    std::vector<std::pair<int,int>> topMoves;
    std::vector<int> scores;
    getTopNMoves(2, nextPlayer, topMoves, scores);
    
    // 提取前2个最佳位置（如果没有，坐标为-1）
    int best1_x = -1, best1_y = -1, best1_score = 0;
    int best2_x = -1, best2_y = -1, best2_score = 0;
    
    if(topMoves.size() >= 1)
    {
        best1_x = topMoves[0].first;
        best1_y = topMoves[0].second;
        best1_score = scores[0];
    }
    
    if(topMoves.size() >= 2)
    {
        best2_x = topMoves[1].first;
        best2_y = topMoves[1].second;
        best2_score = scores[1];
    }
    
    // 发送信号给上层（使用简单类型）
    emit SIG_AI_BEST_MOVES(nextPlayer, best1_x, best1_y, best1_score, best2_x, best2_y, best2_score);
}
