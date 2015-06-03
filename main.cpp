#include <iostream>
#include <iomanip>
#include <string.h>

// Qt4
#include <QFile>
#include <QTcpServer>
#include <QTcpSocket>
#include <QNetworkInterface>
#include <QDateTime>
#include <QElapsedTimer>

// I2C
#include <linux/i2c-dev.h>

const unsigned int I2C_BUS_NUMBER = 1;
const unsigned int I2C_SLAVE_ADDRESS = 0x48;

std::ostream& operator<<(std::ostream& str, const QString& string) {
    return str << string.toStdString();
}

QHostAddress getLocalAddress() {
    QHostAddress local(QHostAddress::LocalHost);
    QList<QHostAddress> list = QNetworkInterface::allAddresses();
    for (auto addr : list) {
        if (addr.protocol() == QAbstractSocket::IPv4Protocol) {
            if (addr != local)
                return addr;
        }
    }
    return local;
}

int main(int argc, char *argv[]) {
    QFile *i2c = new QFile(QString("/dev/i2c-%1").arg(I2C_BUS_NUMBER));
    QTcpServer *server = new QTcpServer();
    
    if (!i2c->open(QIODevice::ReadWrite | QIODevice::Unbuffered)) {
        std::cerr << "[error] " << i2c->fileName() << ": " << i2c->errorString() << std::endl;
        exit(1);
    }
    
    int i2c_fd = i2c->handle();
    if (ioctl(i2c_fd, I2C_SLAVE, I2C_SLAVE_ADDRESS) == -1) {
        std::cerr << "[error] ioctl: " << strerror(errno)<< std::endl;
        exit(1);
    }

    QHostAddress address = getLocalAddress();
    if (!server->listen(address, 22444)) {
        std::cerr << "[error] server: " << server->errorString() << std::endl;
        exit(1);
    }
    std::cout << "Nasłuchiwanie na "
              << server->serverAddress().toString()
              << ":" << server->serverPort() << std::endl;

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

            if (recvMsg == "get_one") {
                char data[] = {0x04, 0, 0, 0, 0};

                i2c->write(data, 1);
                i2c->read(nullptr, 1);

                i2c->read(&data[1], 4);

                QDateTime time = QDateTime::currentDateTime();
                QByteArray response;
                response.append(QString::number(time.toMSecsSinceEpoch()));
                response.append(" " + QString::number(data[1]));
                response.append(" " + QString::number(data[2]));
                response.append(" " + QString::number(data[3]));
                response.append(" " + QString::number(data[4]));
                response.append("\n");
                client->write(response);
            } else if (recvMsg.startsWith("frame")) {
                QList<QByteArray> list = recvMsg.split(' ');
                if (list.size() < 2) {
                    client->write("error: too few arguments\n");
                    continue;
                }

                bool ok;
                int duration = list.at(1).toUInt(&ok);

                if (!ok) {
                    client->write("error: invalid argument\n");
                    continue;
                }

                QElapsedTimer timer = QElapsedTimer();
                QList<char*> dataList;
                timer.start();

                char dataOut[] = {0x04};
                i2c->write(dataOut, 1);
                i2c->read(nullptr, 1);

                char *item;
                while (!timer.hasExpired(duration)) {
                    item = new char[4];
                    i2c->read(item, 4);
                    dataList.append(item);
                }

                int dataSize = dataList.size();
                QByteArray response;
                response.append(QString::number(dataSize) + "\n");
                client->write(response);

                for (char *data : dataList) {
                    response.clear();
                    response.append(QString::number(data[0]) + " ");
                    response.append(QString::number(data[1]) + " ");
                    response.append(QString::number(data[2]) + " ");
                    response.append(QString::number(data[3]) + "\n");
                    client->write(response);

                    delete[] data;
                }
                dataList.clear();
            } else if (recvMsg == "quit") {
                client->write("Bye!\n");
                client->flush();
                client->close();
                server->close();
                break;
            } else {
                std::cout << " * Nieznane polecenie: \""
                          << QString(recvMsg) << "\"" << std::endl;
                client->write("error: unknown command\n");
            }
        }
        
        std::cout << " * Połączenie zakończone" << std::endl;
        client->close();
    }
    server->close();
    std::cout << "Serwer zatrzymany" << std::endl;

    return 0;
}
