#ifndef GAMEROOM_H
#define GAMEROOM_H
#include <QTcpServer>
#include <QTcpSocket>
#include <QMap>
#include <QList>
#include <QJsonObject>
#include <QJsonDocument>

class GameRoom : public QObject
{
    Q_OBJECT
public:
    GameRoom(int roomId, QObject *parent = nullptr);

    // 棋盘尺寸（19×19围棋）
    static const int BOARD_SIZE = 19;
    // 棋子类型
    enum Stone { EMPTY, BLACK, WHITE };

    int m_roomId;

    QList<QTcpSocket*> players;
    QMap<QTcpSocket*, QString> playerColor;

    bool isFull() const { return players.size() == 2; }
    bool isEmpty() const { return players.isEmpty(); }
    QTcpSocket* getOpponent(QTcpSocket* player) const {
        if (players[0] == player) return players.size() > 1 ? players[1] : nullptr;
        if (players[1] == player) return players[0];
        return nullptr;
    }

private:
    // 棋盘
    std::vector<std::vector<Stone>> m_board;
    // 当前回合（黑方先行）
    Stone m_currentTurn;

    // 验证落子合法性
    bool isValidMove(int x, int y, Stone player);
    // 新判断赢棋
    bool checkWin(int x, int y, Stone player);
};
#endif // GAMEROOM_H
