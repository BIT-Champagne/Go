#ifndef GOSERVER_H
#define GOSERVER_H

#include "gameroom.h"

class GoServer : public QTcpServer
{
    Q_OBJECT
public:
    explicit GoServer(QObject *parent = nullptr);
    ~GoServer();

protected:
    void incomingConnection(qintptr socketDescriptor) override;

private slots:
    void readClient();       // 处理客户端消息
    void clientDisconnected();  // 处理客户端断开

private:
    QMap<int, QSharedPointer<GameRoom>> rooms;  // 管理所有房间（房间ID -> 房间对象）
    int nextRoomId = 1;         // 下一个可用房间ID

    // 查找或创建可用房间
    int findOrCreateRoom();
    // 发送消息给客户端
    void sendMessage(QTcpSocket* socket, const QJsonObject& obj);
    // 获取客户端所在房间ID
    int getRoomId(QTcpSocket* socket);
};

#endif // GOSERVER_H
