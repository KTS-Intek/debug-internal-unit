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

#include "dbgextserver.h"
#include <QtCore>

///[!] matilda-bbb-settings
#include "src/matilda/settloader4matilda.h" //settloader4matilda.h"
#include "src/matilda/settloader4matildadefaults.h"

///[!] type-converter
#include "src/base/convertatype.h"
#include "src/shared/networkconverthelper.h"

#include "dbgaboutsourcetype.h"
#include "peredavatordefs.h"

DbgExtServer::DbgExtServer(const bool &verboseMode, const quint16 &port, QObject *parent) : QTcpServer(parent), verboseMode(verboseMode)
{
    serverIp = SettLoader4matildaDefaults::defServerName();
    serverP = port;// SettLoader4matildaDefaults::defDbgFireflyPort();
    stopAll = true;
    useThisPort = 0;

}

//----------------------------------------------------------------

void DbgExtServer::reStartServer()
{
    if(stopAll)
        return;

    if(isListening())
        close();

    emit stopAllSignal();
    close();

    if(true){
        SettLoader4matilda settLoader;

        whiteIpList = settLoader.loadOneSett(SETT_WHITE_IP_LIST).toStringList();
        blockThisIp = settLoader.loadOneSett(SETT_BLACK_IP_LIST).toStringList();

//        serverP = SettLoader4matildaDefaults::defDbgUargPort();//settLoader.loadOneSett(SETT_DBG_PORT).toInt();
//        serverIp = settLoader.loadOneSett(SETT_TCP_HOST_ADDR).toString();

        qDebug() << settLoader.loadOneSett(SETT_DBG_PORT) << SettLoader4matildaDefaults::defDbgUargPort() << serverIp << useThisPort << serverIp;
    }

    if(useThisPort > 0)
        serverP = useThisPort;

    if(serverP < 1 || serverP > 65534){
       qApp->exit(EXT_CODE_PORT_NOT_VALID);
       return;
    }

    if(serverIp.isEmpty())
        serverIp = SettLoader4matildaDefaults::defServerName();


    if(listen(QHostAddress(serverIp), serverP)){
        qDebug() << "dbg server done" << serverIp << serverP;
        emit onStateChanged(isListening());
        emit startUpdateTmr();
        return;
    }


    emit onStateChanged(isListening());

        qDebug() << "dbp server error " << errorString() << serverP << serverPort();

#ifndef VERSION_4_PC
    QTimer::singleShot(10000, this, SLOT(reStartServer()) );
#endif
}

//----------------------------------------------------------------

void DbgExtServer::startNow()
{
    stopAll = false;
    reStartServer();
}

//----------------------------------------------------------------

void DbgExtServer::stopAllNow()
{
    useThisPort = 0;
    stopAll = true;
    emit stopAllSignal();
    close();
    emit onStateChanged(isListening());
}

//----------------------------------------------------------------

void DbgExtServer::startStopF(bool start)
{
    if(start)
        startNow();
    else
        stopAllNow();
}

//----------------------------------------------------------------

void DbgExtServer::startStopF(bool start, quint16 port)
{
    useThisPort = port;
    if(start)
        startNow();
    else
        stopAllNow();
}

//----------------------------------------------------------------

void DbgExtServer::refreshBlockAndWhiteIpList()
{
    if(stopAll)
        return;

    SettLoader4matilda settLoader;
    whiteIpList = settLoader.loadOneSett(SETT_WHITE_IP_LIST).toStringList();
    blockThisIp = settLoader.loadOneSett(SETT_BLACK_IP_LIST).toStringList();

//    bool restatLater = (serverP != SettLoader4matildaDefaults::defDbgUargPort() ||
//            serverIp != settLoader.loadOneSett(SETT_TCP_HOST_ADDR).toString());


//    serverP = SettLoader4matildaDefaults::defDbgUargPort();//settLoader.loadOneSett(SETT_DBG_PORT).toInt();
//    serverIp = settLoader.loadOneSett(SETT_TCP_HOST_ADDR).toString();

//    if(restatLater)
//        QTimer::singleShot(500, this, SLOT(reStartServer()) );
//    else
//        QTimer::singleShot(59999, this, SLOT(refreshBlockAndWhiteIpList()) );
}

//----------------------------------------------------------------

void DbgExtServer::init4matilda()
{
#ifndef VERSION_4_PC //перевіряє налаштування блокування по IP
    QTimer *tmrUpdateSett = new QTimer(this);
    tmrUpdateSett->setInterval(59999);
    tmrUpdateSett->setSingleShot(true);

    connect(this, SIGNAL(startUpdateTmr()), tmrUpdateSett, SLOT(start()) );
    connect(this, SIGNAL(stopAllSignal()), tmrUpdateSett, SLOT(stop()) );
    connect(tmrUpdateSett, SIGNAL(timeout()), this, SLOT(refreshBlockAndWhiteIpList()) );
#endif

    if(verboseMode)
        connect(this, SIGNAL(appendDbgExtData(quint32,QString)), SLOT(appendDbgExtDataSlot(quint32,QString)));
    useThisPort = 0;
    stopAll = false;
    reStartServer();
}

void DbgExtServer::appendDbgExtDataSlot(quint32 sourceType, QString data)
{
        qDebug() << QDateTime::currentDateTime().toString("yyyy/MM/dd hh:mm:ss.zzz") << "appendDbgExtData=" << sourceType << data;
}

//----------------------------------------------------------------

void DbgExtServer::onConnectedStts(bool isConn)
{
    emit onStateChanged(isConn);
    emit onStateChanged(isConn ? tr("Stop") : tr("Start"));
}

//----------------------------------------------------------------

void DbgExtServer::incomingConnection(qintptr socketDescr)
{
    DbgExtSocket *socket = new DbgExtSocket(this);
    if(!socket->setSocketDescriptor(socketDescr)){
        qDebug() << "dbg incomingConnection if(!socket->setSocketDescriptor(socketDescr)){";
        socket->deleteLater();
        return;
    }

    QString strIP = NetworkConvertHelper::showNormalIP(socket->peerAddress());
//     qDebug() << "remote IP:" << strIP;

     if(!whiteIpList.isEmpty() && !whiteIpList.contains(strIP)){
//         emit add2log(QString("ip: %1 in black list. WhiteList enabled!").arg(strIP));
         qDebug() << "ip in black list";
         socket->write("\r\n******** Доступ закрито! **********\r\nr\n******** Access denied! **********\r\n");
         socket->waitForReadyRead(500);
         socket->close();
         socket->deleteLater();
         return;
     }else{

         if(blockThisIp.contains(strIP)){
//             emit add2log(QString("ip: %1 in black list.").arg(strIP));
             qDebug() << "ip in black list";
             socket->write("\r\n******** Доступ закрито! **********\r\nr\n******** Access denied! **********\r\n");
             socket->waitForReadyRead(500);
             socket->close();
             socket->deleteLater();
             return;
         }
     }

     socket->sendAboutSourceType();
    connect(this, SIGNAL(appendDbgExtData(quint32,QString)), socket, SLOT(appendDbgExtData(quint32,QString)) );
    connect(this, SIGNAL(stopAllSignal()), socket, SLOT(onDisconn()) );


//    emit add2log(QString("Dbg onNewConnection %1, %2, %3.").arg(strIP).arg(socket->peerName()).arg(socket->peerPort()));
}

//----------------------------------------------------------------

//----------------------------------------------------------------

DbgExtSocket::DbgExtSocket(QObject *parent) : QTcpSocket(parent)
{
    QTimer::singleShot(12 * 60 * 60 * 1000, this, SLOT(onDisconn()) );
    connect(this, SIGNAL(disconnected()), SLOT(onDisconn()) );
    connect(this, SIGNAL(readyRead()), SLOT(mReadyRead()));
}

//----------------------------------------------------------------

void DbgExtSocket::sendAboutSourceType()
{
    mWrite2Local(1, ConvertAtype::varHash2str(DbgAboutSourceType::getAboutSourcType()));
}

//----------------------------------------------------------------

void DbgExtSocket::mWrite2Local(quint32 sourceType, QString writeData)
{
    /*
     * sourceType
     * 0 - compressed packed
     * 1 - QVH about sourceType to className
     * .
     * . reserved space
     * 101 - client4command
     * 102 - firefly
     * 103 - fireflydbgserver
     * 104 - fireflysocket
     * 105 - lighting socket
     * 106 - lighting scheduler
     * 107 - server4command
     * 108 - thelordofboos
*/
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_6); //Qt_4_0);
    out << (quint32)0;
    out << sourceType << QVariant(writeData).toByteArray();


    quint32 blSize = block.size();

    if(blSize < 500){
        out.device()->seek(0);
        out << (quint32)(blSize - sizeof(quint32));
    }else{
        block.clear();

        QByteArray blockUncompr;
        QDataStream outUncompr(&blockUncompr, QIODevice::WriteOnly);
        outUncompr.setVersion(QDataStream::Qt_5_6); //Qt_4_0);
        outUncompr << sourceType << QVariant(writeData).toByteArray();

        QDataStream out(&block, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_5_6); //Qt_4_0);
        out << (quint32)0;
        out << (int)0 << qCompress(blockUncompr,9);
        out.device()->seek(0);
        blSize = block.size();
        out << (quint32)(blSize - sizeof(quint32));
    }
    write(block);         //     qDebug() << block.toHex();
    waitForBytesWritten(50);

}

void DbgExtSocket::appendDbgExtData(quint32 sourceType, QString writeData)
{
    mWrite2Local( (sourceType < 100 ) ? 255 : sourceType, writeData);
}

//----------------------------------------------------------------

void DbgExtSocket::mReadyRead()
{
    readAll();
}

//----------------------------------------------------------------

void DbgExtSocket::onDisconn()
{
    if(state() == QAbstractSocket::ConnectedState || state() == QAbstractSocket::ConnectingState)
        close();
    deleteLater();
}

//----------------------------------------------------------------
