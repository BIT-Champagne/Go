#include "goserver.h"
#include "gameroom.h"
#include <QDebug>

GoServer::GoServer(QObject *parent) : QTcpServer(parent)
{
    if (!listen(QHostAddress::Any, 1234)) {
        qDebug() << "Server could not start!";
    } else {
        qDebug() << "Server started on port 1234";
    }
}

GoServer::~GoServer()
{
    close();
}

// 查找未满房间，若无则创建新房间
int GoServer::findOrCreateRoom()
{
    // 优先加入已有未满房间
    for (auto it = rooms.begin(); it != rooms.end(); ++it) {
        if (!it.value()->isFull()) {
            return it.key();
        }
    }
    // 无未满房间，创建新房间
    int newRoomId = nextRoomId++;
    rooms[newRoomId] =  QSharedPointer<GameRoom>(new GameRoom(newRoomId));
    qDebug() << "Created new room (ID:" << newRoomId << ")";
    return newRoomId;
}

// 获取客户端所在房间ID（通过socket属性存储）
int GoServer::getRoomId(QTcpSocket *socket)
{
    return socket->property("roomId").toInt();
}

// 处理新客户端连接
void GoServer::incomingConnection(qintptr socketDescriptor)
{
    QTcpSocket* clientSocket = new QTcpSocket(this);
    clientSocket->setSocketDescriptor(socketDescriptor);
    qDebug() << "New client connected (socket:" << clientSocket->socketDescriptor() << ")";

    // 分配房间
    int roomId = findOrCreateRoom();
    QSharedPointer<GameRoom> room = rooms[roomId];

    // 将客户端加入房间
    room->players.append(clientSocket);
    // 记录客户端所在房间（通过socket属性）
    clientSocket->setProperty("roomId", roomId);

    // 连接信号槽（处理消息和断开）
    connect(clientSocket, &QTcpSocket::readyRead, this, &GoServer::readClient);
    connect(clientSocket, &QTcpSocket::disconnected, this, &GoServer::clientDisconnected);

    // 若房间已满（2人），分配颜色并通知开始
    if (room->isFull()) {
        // 分配颜色
        room->playerColor[room->players[0]] = "black";
        room->playerColor[room->players[1]] = "white";
        // 通知两个客户端颜色信息
        sendMessage(room->players[0], QJsonObject{{"color", "black"}});
        sendMessage(room->players[1], QJsonObject{{"color", "white"}});
        qDebug() << "Room" << roomId << "is full (2 players), game started";
    }
}

// 处理客户端消息（转发给同房间对手）
void GoServer::readClient()
{
    QTcpSocket* senderSocket = qobject_cast<QTcpSocket*>(sender());
    if (!senderSocket) return;

    // 获取发送者所在房间
    int roomId = getRoomId(senderSocket);
    if (!rooms.contains(roomId)) {
        qDebug() << "Client not in any room";
        return;
    }
    QSharedPointer<GameRoom> room = rooms[roomId];

    // 读取消息
    QByteArray data = senderSocket->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        qDebug() << "Invalid message from client (socket:" << senderSocket->socketDescriptor() << ")";
        return;
    }

    // 转发给同房间的对手
    QTcpSocket* opponent = room->getOpponent(senderSocket);
    if (opponent && opponent->state() == QTcpSocket::ConnectedState) {
        opponent->write(data);  // 直接转发原始数据（确保格式一致）
        qDebug() << "Message forwarded in room" << roomId;
    }
}

// 处理客户端断开连接
void GoServer::clientDisconnected()
{
    QTcpSocket* clientSocket = qobject_cast<QTcpSocket*>(sender());
    if (!clientSocket) return;

    int roomId = getRoomId(clientSocket);
    if (!rooms.contains(roomId)) return;

    QSharedPointer<GameRoom> room = rooms[roomId];
    qDebug() << "Client disconnected from room" << roomId;

    // 从房间移除客户端
    room->players.removeOne(clientSocket);
    room->playerColor.remove(clientSocket);

    // 若房间为空，删除房间
    if (room->isEmpty()) {
        rooms.remove(roomId);
        qDebug() << "Room" << roomId << "is empty, deleted";
    } else {
        // 若房间还剩1人，通知其对手已离开
        if (!room->players.isEmpty()) {
            sendMessage(room->players[0], QJsonObject{{"info", "opponent_disconnected"}});
        }
    }

    clientSocket->deleteLater();
}

// 发送消息给客户端
void GoServer::sendMessage(QTcpSocket *socket, const QJsonObject &obj)
{
    QJsonDocument doc(obj);
    socket->write(doc.toJson());
}
