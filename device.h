#ifndef DEVICE_H
#define DEVICE_H

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

struct DeviceConfiguration
{
    DeviceState state;
    DeviceRegime regime;
};

class Device
{
    DeviceState state = NORMAL;
    DeviceRegime regime = TECHNOLOGICAL;
public:
    Device();
    Device(DeviceConfiguration);
    //Внутри этой функции эмулируется работа счётчика:
    //Происходит обработка входящих сообщений
    //Отправка сообщений
    //Запись в регистры
    //другое...
    Run();
};

#endif // DEVICE_H
