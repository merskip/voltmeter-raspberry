#include <iostream>
#include <QTcpServer>
#include <QTcpSocket>

std::ostream& operator<<(std::ostream& str, const QString& string) {
    return str << string.toStdString();
}

int main(int argc, char *argv[]) {
    QTcpServer *server = new QTcpServer();
    
    if (!server->listen(QHostAddress::Any, 22444)) {
        std::cerr << "error: " << server->errorString() << std::endl;
        exit(1);
    }
    
    while (server->isListening()) {
        std::cout << "Oczekiwanie na połączenie..." << std::endl;
        
        if (!server->waitForNewConnection(-1))
            continue;
        
        QTcpSocket *client = server->nextPendingConnection();
        
        std::cout << " * Połączenie z "
                  << client->peerAddress().toString()
                  << ":" << client->peerPort() << std::endl;
        
        while (client->state() == QAbstractSocket::ConnectedState) {
            if (!client->waitForReadyRead(-1))
                continue;
            
            QByteArray recvMsg = client->readLine().trimmed();
            std::cout << " * Odebrano: " << QString(recvMsg) << std::endl;
        }
        
        std::cout << " * Połączenie zakończone" << std::endl;
        client->close();
    }

    return 0;
}
