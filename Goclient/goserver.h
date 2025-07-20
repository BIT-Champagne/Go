#ifndef GOSERVER_H
#define GOSERVER_H

#include <QTcpServer>
#include <QTcpSocket>
#include <QJsonObject>
#include <QJsonDocument>
#include <QList>

class GoServer : public QTcpServer
{
    Q_OBJECT
public:
    GoServer(QObject *parent = nullptr);

protected:
    void incomingConnection(qintptr socketDescriptor) override;

private slots:
    void readClient();
    void clientDisconnected();

private:
    QList<QTcpSocket*> clients;
    void sendMessage(QTcpSocket *socket, const QJsonObject &obj);
};

#endif // GOSERVER_H
