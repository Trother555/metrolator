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
    Run();
};

#endif // DEVICE_H
