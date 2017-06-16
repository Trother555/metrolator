#include "device.h"
#include "Modbus/modbus_general.h"

Device::Device()
{

}

void Device::Run()
{

}

QString Device::SendMessage(QString msg)
{
    //TODO: pass msg to SlaveProcess
    /*unsigned char* SlaveProcess
    (
        unsigned char* buffer,                      //принятый кадр
        unsigned int bufferSize,                    //размер принятого кадра
        unsigned int& resultBufferSize,             //размер кадра-ответа (выходной параметр)
        unsigned char slaveAddress,                 //адрес slave-устройства
        unsigned char (*read)(unsigned char*, unsigned char*, unsigned char), //функция чтения данных из памяти slave-устройства
        unsigned char (*write)(unsigned char*, unsigned char*, unsigned char),//функция записи данных в память slave-устройства
        unsigned char* firstRegister,               //указатель на начало регистровой памяти в slave-устройстве
        unsigned short totalRegistersSize = 0xFFFFU,//общее количество регистров (все регистры полагаются двухбайтовыми,
                                                    //по умолчанию размер памяти принимается максимально возможным)
        char isHighLowOrder = 0                     //порядок следования байтов в области записываемых значений (по умолчанию LowHigh)
    );*/

}
