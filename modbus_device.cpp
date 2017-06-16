#include "modbus_device.h"
#include "QtSerialPort/QSerialPort"
#include "QtSerialPort/QSerialPortInfo"

void MBS_init()
{
    QList<QSerialPortInfo> portInfo = QSerialPortInfo::availablePorts();
    QSerialPort port = QSerialPort();
}
