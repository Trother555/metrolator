#ifndef DEVICE_H
#define DEVICE_H
#include <QString>
#include "modbus_device.h"

/*
    Этот класс реализует логику счётчика
*/

enum DeviceState
{
    NORMAL = 0,
    ALARM
};

enum DeviceRegime
{
    TECHNOLOGICAL = 0,
    METROLOGICAL
};

enum AffectType
{
    CRACK = 0,
    REVERSE_STREAM,
    STRONG_MAGNET,
    MAGNET_BUTTON
};


class Device
{
    //Внутренняя память счётчика
    primary_table_s memory;

    //Индикатор состояния
    DeviceState state = NORMAL;
public:
    Device();
    //Device(DeviceConfiguration); //Нужен метод для восстановления предыдущего состоянеия

    //Внутри этой функции эмулируется работа счётчика:
    //Происходит обработка входящих сообщений
    //Отправка сообщений
    //Запись в регистры
    //другое...
    void Run();

    //Послать счётчику сообщение по протоколу Modbus
    QString SendMessage(QString msg);

    //Эта функция эмулирует внешнее воздействие на счётчик
    void Affect(AffectType);

};

#endif // DEVICE_H
