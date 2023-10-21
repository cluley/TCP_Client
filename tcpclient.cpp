#include "tcpclient.h"



/* ServiceHeader
 * Для работы с потоками наши данные необходимо сериализовать.
 * Поскольку типы данных не стандартные перегрузим оператор << Для работы с ServiceHeader
*/
QDataStream &operator <<(QDataStream &in, ServiceHeader &data){

    in << data.id;
    in << data.idData;
    in << data.status;
    in << data.len;

    return in;

};

QDataStream &operator >>(QDataStream &out, ServiceHeader &data){

    out >> data.id;
    out >> data.idData;
    out >> data.status;
    //out.skipRawData(3); если использовать memcpy (строка 96) и выравнивание по 4м байтам.
    out >> data.len;

    return out;
};


//struct ServiceHeader{

//    uint16_t id = ID;     //Идентификатор начала пакета
//    uint16_t idData = 0;  //Идентификатор типа данных
//    uint8_t  status = 0;  //Тип сообщения (запрос/ответ)
//    uint32_t len = 0;     //Длина пакета далее, байт

//};


/*
 * Поскольку мы являемся клиентом, инициализацию сокета
 * проведем в конструкторе. Также необходимо соединить
 * сокет со всеми необходимыми нам сигналами.
*/
TCPclient::TCPclient(QObject *parent) : QObject(parent)
{

    socket = new QTcpSocket(this);

    //Соединяемся с методом обработки входящих данных
    connect(socket, &QTcpSocket::readyRead, this, &TCPclient::ReadyReed);

    //Прокидываем сигналы статусов подключения
    connect(socket, &QTcpSocket::connected, this, [&]{
        emit sig_connectStatus(STATUS_SUCCES);
    });
    connect(socket, &QTcpSocket::errorOccurred, this, [&]{

        emit sig_connectStatus(ERR_CONNECT_TO_HOST);

    });

    //Прокидываем сигнал отключения
    connect(socket, &QTcpSocket::disconnected, this, &TCPclient::sig_Disconnected);

}

/* write
 * Метод отправляет запрос на сервер. Сериализировать будем
 * при помощи QDataStream
*/
void TCPclient::SendRequest(ServiceHeader head)
{
    //Сериализуем данные в массив байт
    QByteArray sendHdr;
    QDataStream outStr(&sendHdr, QIODevice::WriteOnly);
    outStr << head;

    socket->write(sendHdr);


}

/* write
 * Такой же метод только передаем еще данные.
*/
void TCPclient::SendData(ServiceHeader head, QString str)
{

    QByteArray sendData;

    //memcpy(sendData.data(), &head, sizeof(head));

    QDataStream outStr(&sendData, QIODevice::WriteOnly);

    outStr << head;
    outStr << str;

    socket->write(sendData);

}

/*
 * \brief Метод подключения к серверу
 */
void TCPclient::ConnectToHost(QHostAddress host, uint16_t port)
{
    socket->connectToHost(host, port);
}
/*
 * \brief Метод отключения от сервера
 */
void TCPclient::DisconnectFromHost()
{
    socket->disconnectFromHost();
}

/*
 * Метод обрабатывает сигнал readyReed. Поскольку данные могут придти
 * несколькими сегментами, нам необходимо собрать их все, а потом начать обрабатывать.
 * Для этого у нас в служебном заголовке есть длина данных. Т.е. нам нужно сначала прочитать
 * заголовок, после этого прочитать данные.
 */
void TCPclient::ReadyReed()
{

    QDataStream incStream(socket);

    if(incStream.status() != QDataStream::Ok){

        QMessageBox msg;
        msg.setText("Поток не открылся");
        msg.show();

    }

    while (incStream.atEnd() == false){


        if(servHeader.idData == 0){

            if(socket->bytesAvailable() < sizeof(ServiceHeader)){
                return;
            }
            else{

                incStream >> servHeader;

                if(servHeader.id != ID){
                    //Если более жесткий подход, то в случае ошибочного чтения данных можно разорвать соединение socket->disconnectFromHost();

                    //Более мягкий - поиск следующего заголовка
                    uint16_t hdr = 0;
                    while(incStream.atEnd() == false){
                        incStream >> hdr;
                        if(hdr == ID){
                            servHeader.id = hdr;
                            incStream >> servHeader.idData;
                            incStream >> servHeader.status;
                            incStream >> servHeader.len;
                            break;
                        }
                    }

                }
            }
        }

        if(socket->bytesAvailable() < servHeader.len){
            return;
        }
        else{
            ProcessingData(servHeader, incStream);
            servHeader.idData = 0;
            servHeader.len = 0;
            servHeader.status = 0;
        }

    }

}


/*
 * Остался метод обработки полученных данных. Согласно протоколу
 * мы должны прочитать данные из сообщения и вывести их в ПИ.
 * Поскольку все типы сообщений нам известны реализуем выбор через
 * switch. Реализуем получение времени.
*/
void TCPclient::ProcessingData(ServiceHeader header, QDataStream &stream)
{

    switch (header.idData){

        case GET_TIME:{

            QDateTime time;
            stream >> time;
            emit sig_sendTime(time);
            break;

        }
        case GET_SIZE:
        case GET_STAT:
        case SET_DATA:
        case CLEAR_DATA:
        default:
            return;
        }

}
