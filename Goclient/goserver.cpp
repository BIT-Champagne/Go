#include "goserver.h"

GoServer::GoServer(QObject *parent) : QTcpServer(parent) {}

void GoServer::incomingConnection(qintptr socketDescriptor)
{
    QTcpSocket *socket = new QTcpSocket(this);
    socket->setSocketDescriptor(socketDescriptor);

    if (clients.size() < 2) {
        clients.append(socket);
        connect(socket, &QTcpSocket::readyRead, this, &GoServer::readClient);
        connect(socket, &QTcpSocket::disconnected, this, &GoServer::clientDisconnected);

        if (clients.size() == 2) {
            // 给两个客户端分配颜色
            sendMessage(clients[0], {{"color", "black"}});
            sendMessage(clients[1], {{"color", "white"}});
        }
    } else {
        // 拒绝多余的连接
        socket->disconnectFromHost();
    }
}

void GoServer::readClient()
{
    QTcpSocket *sender = qobject_cast<QTcpSocket*>(sender());
    QByteArray data = sender->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject obj = doc.object();

    // 转发消息给另一个客户端
    for (QTcpSocket *client : clients) {
        if (client != sender) {
            sendMessage(client, obj);
        }
    }
}

void GoServer::clientDisconnected()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    clients.removeOne(socket);
    socket->deleteLater();
}

void GoServer::sendMessage(QTcpSocket *socket, const QJsonObject &obj)
{
    QJsonDocument doc(obj);
    QByteArray data = doc.toJson();
    socket->write(data);
}
