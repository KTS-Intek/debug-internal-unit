/****************************************************************************
**
**   Copyright © 2017 The KTS-INTEK Ltd.
**   Contact: http://www.kts-intek.com.ua
**
**  This file is part of firefly-bbb.
**
**  firefly-bbb is free software: you can redistribute it and/or modify
**  it under the terms of the GNU General Public License as published by
**  the Free Software Foundation, either version 3 of the License, or
**  (at your option) any later version.
**
**  firefly-bbb is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
**
**  You should have received a copy of the GNU General Public License
**  along with firefly-bbb.  If not, see <http://www.gnu.org/licenses/>.
**
****************************************************************************/

#ifndef DBGEXTSERVER_H
#define DBGEXTSERVER_H

#include <QTcpServer>
#include <QTcpSocket>
#include <QObject>


class DbgExtServer : public QTcpServer
{
    Q_OBJECT
public:
    explicit DbgExtServer(const quint16 &port, QObject *parent = nullptr);

signals:
    void appendDbgExtData(quint32 sourceType, QString data);
    void stopAllSignal();
    void onStateChanged(bool);
    void onStateChanged(QString);

    void startUpdateTmr();

public slots:
    void reStartServer();
    void startNow();
    void stopAllNow();
    void startStopF(bool start);
    void startStopF(bool start, quint16 port);

    void refreshBlockAndWhiteIpList();

    void init4matilda();

private:
    void onConnectedStts(bool isConn);


protected:
    void incomingConnection(qintptr socketDescr);

private:
    QString serverIp;
    quint16 serverP;

    QStringList blockThisIp;
    QStringList whiteIpList;
    bool stopAll;
    quint16 useThisPort;

};


class DbgExtSocket : public QTcpSocket
{
    Q_OBJECT
public:
    explicit DbgExtSocket(QObject *parent = 0);

public slots:
    void sendAboutSourceType();

    void mWrite2Local(quint32 sourceType, QString writeData);



private slots:
    void mReadyRead();

    void onDisconn();

};

#endif // DBGEXTSERVER_H
