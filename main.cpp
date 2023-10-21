#include "mainwindow.h"
#include <QApplication>
#include <QDebug>


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;

//#pragma pack(push, 1)

//    struct str
//    {
//        uint32_t x = 10;
//        uint8_t  z = 25;
//        uint32_t y = 20;
//    };

//    str st;

//    qDebug() << sizeof(uint32_t);
//    qDebug() << sizeof(st);

//#pragma pack(pop)


    w.show();
    return a.exec();
}
