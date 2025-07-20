#include "gameroom.h"

GameRoom::GameRoom(int roomId, QObject *parent) : QObject(parent), m_roomId(roomId)
{
    // 初始化棋盘（全部为空）
    m_board.resize(BOARD_SIZE, std::vector<Stone>(BOARD_SIZE, EMPTY));
    m_currentTurn = BLACK;  // 黑方先行
    qDebug() << "Room" << m_roomId << "created (board initialized)";
}

// 验证落子合法性（服务器端权威校验）
bool GameRoom::isValidMove(int x, int y, Stone player)
{
    // 检查坐标是否在棋盘内
    if (x < 0 || x >= BOARD_SIZE || y < 0 || y >= BOARD_SIZE)
        return false;
    // 检查是否为空位
    if (m_board[x][y] != EMPTY)
        return false;
    // 检查是否当前回合
    if (player != m_currentTurn)
        return false;
    return true;
}

// 判赢逻辑（简化为“横/竖/斜向连成5子”）
bool GameRoom::checkWin(int x, int y, Stone player)
{
    return false;
}
