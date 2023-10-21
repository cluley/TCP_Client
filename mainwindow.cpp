#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    client = new TCPclient(this);
    //Доступность полей по умолчанию
    ui->le_data->setEnabled(false);
    ui->pb_request->setEnabled(false);
    ui->lb_connectStatus->setText("Отключено");
    ui->lb_connectStatus->setStyleSheet("color: red");


    //При отключении меняем надписи и доступность полей.
    connect(client, &TCPclient::sig_Disconnected, this, [&]{

        ui->lb_connectStatus->setText("Отключено");
        ui->lb_connectStatus->setStyleSheet("color: red");
        ui->pb_connect->setText("Подключиться");
        ui->le_data->setEnabled(false);
        ui->pb_request->setEnabled(false);
        ui->spB_port->setEnabled(true);
        ui->spB_ip1->setEnabled(true);
        ui->spB_ip2->setEnabled(true);
        ui->spB_ip3->setEnabled(true);
        ui->spB_ip4->setEnabled(true);

    });

    connect(client, &TCPclient::sig_connectStatus, this, &MainWindow::DisplayConnectStatus);

    connect(client, &TCPclient::sig_Error, this, &MainWindow::DisplayError);

 /*
  * Соединяем сигналы со слотами
 */

    connect(client, &TCPclient::sig_sendTime, this, &MainWindow::DisplayTime);

    connect(client, &TCPclient::sig_sendFreeSize, this, &MainWindow::DisplayFreeSpace);

    connect(client, &TCPclient::sig_sendStat, this, &MainWindow::DisplayStat);

    connect(client, &TCPclient::sig_SendReplyForSetData, this, &MainWindow::SetDataReply);

    connect(client, &TCPclient::sig_Success, this, &MainWindow::DisplaySuccess);
}

MainWindow::~MainWindow()
{
    delete ui;
}

/*!
 * \brief Группа методо отображения различных данных
 */
void MainWindow::DisplayTime(QDateTime time)
{
    ui->tb_result->append("Время на сервере: " + time.toString());
}
void MainWindow::DisplayFreeSpace(uint32_t freeSpace)
{
    ui->tb_result->append("Свободное место на сервере: " + QString::number(freeSpace));
}
void MainWindow::SetDataReply(QString replyString)
{
    ui->tb_result->append("Вернувшаяся строка: " + replyString);
}
void MainWindow::DisplayStat(StatServer stat)
{
    ui->tb_result->append("Статистика сервера:"
                          "\nКоличество принятых байт: "          + QString::number(stat.incBytes)  +
                          "\nКоличество переданных байт: "        + QString::number(stat.sendBytes) +
                          "\nКоличество принятых пакетов	: "   + QString::number(stat.revPck)    +
                          "\nКоличество переданных пакетов	: "   + QString::number(stat.sendPck)   +
                          "\nВремя работы сервера: "              + QString::number(stat.workTime)  + " сек" +
                          "\nКоличество подключённых клиентов: "  + QString::number(stat.clients));
}
void MainWindow::DisplayError(uint16_t error)
{
    switch (error) {
    case ERR_NO_FREE_SPACE:
        ui->tb_result->append("Недостаточно свободного места на сервере");
        break;
    case ERR_CONNECT_TO_HOST:
        ui->tb_result->append("Не указана длина сообщения в заголовке");
        break;
    case ERR_NO_FUNCT:
        ui->tb_result->append("Функционал не реализован");
        break;
    default:
        break;
    }
}
/*!
 * \brief Метод отображает квитанцию об успешно выполненном сообщениии
 * \param typeMess ИД успешно выполненного сообщения
 */
void MainWindow::DisplaySuccess(uint8_t typeMess)
{
    switch (typeMess) {
    case STATUS_SUCCES:
        ui->tb_result->append("Провозглашаю, что в первый день января 1863 года от Рождества Христова все байты, "
                              "удерживаемые как рабы на территории какого бы то ни было сервера или части сервера, "
                              "данные которых находится в состоянии мятежа против Соединенных Серверов, "
                              "с того времени и навечно объявляются свободными.");
        break;
    default:
        ui->tb_result->append("Север пал!");
        break;
    }

}

/*!
 * \brief Метод отображает статус подключения
 */
void MainWindow::DisplayConnectStatus(uint16_t status)
{

    if(status == ERR_CONNECT_TO_HOST){

        ui->tb_result->append("Ошибка подключения к порту: " + QString::number(ui->spB_port->value()));

    }
    else{
        ui->lb_connectStatus->setText("Подключено");
        ui->lb_connectStatus->setStyleSheet("color: green");
        ui->pb_connect->setText("Отключиться");
        ui->spB_port->setEnabled(false);
        ui->pb_request->setEnabled(true);
        ui->spB_ip1->setEnabled(false);
        ui->spB_ip2->setEnabled(false);
        ui->spB_ip3->setEnabled(false);
        ui->spB_ip4->setEnabled(false);
    }

}

/*!
 * \brief Обработчик кнопки подключения/отключения
 */
void MainWindow::on_pb_connect_clicked()
{
    if(ui->pb_connect->text() == "Подключиться"){

        uint16_t port = ui->spB_port->value();

        QString ip = ui->spB_ip4->text() + "." +
                     ui->spB_ip4->text() + "." +
                     ui->spB_ip4->text() + "." +
                     ui->spB_ip4->text();

        client->ConnectToHost(QHostAddress(ip), port);

    }
    else{

        client->DisconnectFromHost();
    }
}

/*
 * Для отправки сообщения согласно ПИВ необходимо
 * заполнить заголовок и передать его на сервер. В ответ
 * сервер вернет информацию в соответствии с типом сообщения
*/
void MainWindow::on_pb_request_clicked()
{

   ServiceHeader header;

   header.id = ID;
   header.status = STATUS_SUCCES;
   header.len = 0;

   switch (ui->cb_request->currentIndex()){

   case 0:
       header.idData = GET_TIME;
       break;
   case 1:
       header.idData = GET_SIZE;
       break;
   case 2:
       header.idData = GET_STAT;
       break;
   case 3:{
       header.idData = SET_DATA;

       QString txt2send = ui->le_data->text();

       header.len = txt2send.size();

       client->SendData(header, txt2send);
       return;
   }
   case 4:
       header.idData = CLEAR_DATA;
       break;
   default:
   ui->tb_result->append("Такой запрос не реализован в текущей версии");
   return;

   }

   client->SendRequest(header);

}

/*!
 * \brief Обработчик изменения индекса запроса
 */
void MainWindow::on_cb_request_currentIndexChanged(int index)
{
    //Разблокируем поле отправления данных только когда выбрано "Отправить данные"
    if(ui->cb_request->currentIndex() == 3){
        ui->le_data->setEnabled(true);
    }
    else{
        ui->le_data->setEnabled(false);
    }
}

