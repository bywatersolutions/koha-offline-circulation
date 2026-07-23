/*
* Copyright 2026 Kyle M Hall <kyle@bywatersolutions.com>
*
* This file is part of Koha Offline Circulation.
*
* Koha Offline Circulation is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* Koha Offline Circulation is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with Koha Offline Circulation.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef MOCKHTTPSERVER_H
#define MOCKHTTPSERVER_H

#include <QMap>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUrl>

#include <functional>

/* A minimal in-process HTTP server for exercising network clients
 * against real sockets in the unit tests. */
class MockHttpServer
{
    public:
        std::function<void( const QUrl & url, int * status, QByteArray * body )> handler;
        QList<QUrl> requests;
        QList<QMap<QByteArray, QByteArray> > requestHeaders;

        bool listen()
        {
            QObject::connect( &mServer, &QTcpServer::newConnection,
                              &mServer, [this]() { onNewConnection(); } );
            return mServer.listen( QHostAddress::LocalHost, 0 );
        }

        QString baseUrl() const
        {
            return QString( "http://127.0.0.1:%1" ).arg( mServer.serverPort() );
        }

    private:
        void onNewConnection()
        {
            QTcpSocket *socket = mServer.nextPendingConnection();
            QObject::connect( socket, &QTcpSocket::readyRead,
                              socket, [this, socket]() { onReadyRead( socket ); } );
            QObject::connect( socket, &QTcpSocket::disconnected,
                              socket, &QTcpSocket::deleteLater );
        }

        void onReadyRead( QTcpSocket *socket )
        {
            mBuffers[socket] += socket->readAll();
            if ( ! mBuffers[socket].contains( "\r\n\r\n" ) ) return;

            QByteArray request = mBuffers.take( socket );
            QList<QByteArray> lines = request.split( '\n' );

            QList<QByteArray> requestLine = lines.first().trimmed().split( ' ' );
            QUrl url( "http://127.0.0.1" + QString::fromUtf8( requestLine.value( 1 ) ) );

            QMap<QByteArray, QByteArray> headers;
            for ( int i = 1; i < lines.count(); i++ ) {
                QByteArray line = lines.at( i ).trimmed();
                if ( line.isEmpty() ) break;
                int colon = line.indexOf( ':' );
                headers.insert( line.left( colon ).toLower(), line.mid( colon + 1 ).trimmed() );
            }

            requests.append( url );
            requestHeaders.append( headers );

            int status = 200;
            QByteArray body = "[]";
            if ( handler ) handler( url, &status, &body );

            QByteArray response = "HTTP/1.1 " + QByteArray::number( status ) + " Status\r\n"
                                  "Content-Type: application/json\r\n"
                                  "Content-Length: " + QByteArray::number( body.size() ) + "\r\n"
                                  "Connection: close\r\n"
                                  "\r\n" + body;
            socket->write( response );
            socket->disconnectFromHost();
        }

        QTcpServer mServer;
        QMap<QTcpSocket *, QByteArray> mBuffers;
};

#endif // MOCKHTTPSERVER_H
