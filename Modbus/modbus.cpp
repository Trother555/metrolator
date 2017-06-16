#include "modbus.h"
#include "string.h"
#include <stdio.h>
#include <stdlib.h>

#define SLAVE_ADDRESS buffer[0]
#define COMMAND buffer[1]


/*Вычисляет CRC16 по массиву указанного размера
(алгоритм взят из протокола связи вычислителя ВКТ-5 с системой верхнего уровня)*/
unsigned short CRC16(unsigned char* buffer, unsigned int size)
{
    union
    {
        unsigned char arr[2];
        unsigned short value;
    } sum;
    unsigned char shiftCount, *ptr;
    ptr = buffer;
    sum.value=0xFFFFU;
    for(; size>0; size--)
    {
        sum.value=(unsigned short)((sum.value/256U)*256U+((sum.value%256U)^(*ptr++)));
        for(shiftCount=0; shiftCount<8; shiftCount++)
        {
            if((sum.value&0x1)==1)
                sum.value=(unsigned short)((sum.value>>1)^0xA001U);
            else
                sum.value>>=1;
        }
    }
    return sum.value;
}


//Проверяет корректность кадра, принятого slave-устройством
char IsValidBufferSizeFromMaster(unsigned char* buffer, unsigned int size)
{
    if (size<2)
        return 0;
    switch(COMMAND)
    {
    case 0x03:
    case 0x04:
    case 0x06:
        if (size!=8)
            return 0;

        break;

    case 0x10:
        if (size<9)
            return 0;

        if (size != 9+buffer[6])
            return 0;

        break;

    default:
        return 0;
    }

    //Проверка корректности CRC
    if (CRC16(buffer,size-2) != (unsigned short)(buffer[size-1] << 8 | buffer[size - 2]))
        return 0;

    return 1;
}




//Изменение порядка следования байтов в массиве указанного размера
void ChangeByteOrder(unsigned char* buffer, unsigned int size)
{
    for (unsigned int i=0; i < size-1; i+=2)
    {
        //Меняются местами значения buffer[i] и buffer[i+1]
        unsigned char tmp = buffer[i];
        buffer[i] = buffer[i+1];
        buffer[i+1] = tmp;
    }
}


//Создание кадра ошибки
unsigned char* CreateErrorBuffer(unsigned char address, unsigned char command, unsigned char errorCode)
{
    unsigned char* result = (unsigned char*)malloc(5);
    result[0] = address;
    result[1] = command | (1U<<7);
    result[2] = errorCode;

    unsigned short crc = CRC16(result, 3);

    result[3] = crc & 0xFF;
    result[4] = crc >> 8;

    return result;
}


/*Обработка принятого кадра slave-устройством и формирование кадра-ответа
(Выделяет память, которую нужно потом освободить!)

Функция read должна иметь вид:
unsigned char read(unsigned char* dest, unsigned char* src, unsigned char countRegisters);

Функция write должна иметь вид:
unsigned char write(unsigned char* dest, unsigned char* src, unsigned char countRegisters);*/
unsigned char* SlaveProcess
(
    unsigned char* buffer,                      //принятый кадр
    unsigned int bufferSize,                    //размер принятого кадра
    unsigned int& resultBufferSize,             //размер кадра-ответа (выходной параметр)
    unsigned char slaveAddress,                 //адрес slave-устройства
    unsigned char (*read)(unsigned char*, unsigned char*, unsigned char countRegisters), //функция чтения данных из памяти slave-устройства
    unsigned char (*write)(unsigned char*, unsigned char*, unsigned char countRegisters),//функция записи данных в память slave-устройства
    unsigned char* firstRegister,               //указатель на начало регистровой памяти в slave-устройстве
    unsigned short totalRegistersSize,          //общее количество регистров (все регистры полагаются двухбайтовыми,
                                                //по умолчанию размер памяти принимается максимально возможным)
    char isHighLowOrder                         //порядок следования байтов в области записываемых значений (по умолчанию LowHigh)
)
{
    //проверка длины и CRC
    if (!IsValidBufferSizeFromMaster(buffer, bufferSize))
    {
        resultBufferSize = 0;
        return NULL;
    }


    //Проверка правильности адреса устройства
    if (SLAVE_ADDRESS != slaveAddress && SLAVE_ADDRESS != 0)
    {
        resultBufferSize = 0;
        return NULL;
    }

    unsigned char* result;

    //Проверка кода функции
    //если неверный, то код ошибки 0x01
    if (COMMAND != 0x03 && COMMAND != 0x04 && COMMAND != 0x06 && COMMAND != 0x10)
    {
        resultBufferSize = 5;
        result = CreateErrorBuffer(SLAVE_ADDRESS, COMMAND, 0x01);

        return result;
    }

    //Проверка адреса регистра
    //Если неверный, то код ошибки 0x02
    if ((buffer[2]<<8 | buffer[3] > totalRegistersSize)
            || (COMMAND == 0x10 && (buffer[2]<<8|buffer[3]) + buffer[6] > totalRegistersSize)
            || ((COMMAND == 0x03 || COMMAND == 0x04) && (buffer[2]<<8|buffer[3]) + (buffer[4]<<1|buffer[5])*2 > totalRegistersSize))
    {
        resultBufferSize = 5;
        result = CreateErrorBuffer(SLAVE_ADDRESS, COMMAND, 0x02);

        return result;
    }

    //Все ошибки, что мог, обработал
    unsigned char errorCode;
    switch(COMMAND)
    {
    case 0x03:
    case 0x04:
        //Если slave-адрес в пришедшем кадре широковещательный, то отвечать не нужно
        if (SLAVE_ADDRESS == 0)
        {
            resultBufferSize = 0;
            return NULL;
        }

        //В противном случае формируется кадр-ответ
        resultBufferSize = (buffer[4]<<8 | buffer[5]) * 2 + 5;

        result = (unsigned char*)malloc(resultBufferSize);
        errorCode = read(result+3, firstRegister+(buffer[2]<<8 | buffer[3]), buffer[4]<<8 | buffer[5]);

        //Если функция чтения значений регистров вернула ошибку, то формируется кадр ошибки
        if (errorCode)
        {
            free(result);

            resultBufferSize = 5;

            return CreateErrorBuffer(SLAVE_ADDRESS, COMMAND, errorCode);
        }

        //Если указан порядок следования байтов HighLow, то необходимо
        //поменять местами байты в прочитанных значениях регистров
        if(isHighLowOrder)
        {
            ChangeByteOrder(result+3, (buffer[4]<<8 | buffer[5])*2);
        }
        result[0] = SLAVE_ADDRESS;
        result[1] = COMMAND;
        result[2] = (buffer[4]<<8 | buffer[5]) * 2;

        break;

    case 0x06:
        /*Если указан порядок следования байтов HighLow, то необходимо
        поменять местами байты в значении, которое записывается в указанный регистр*/
        if (isHighLowOrder)
        {
            unsigned short tmp = buffer[4] << 8 | buffer[5];
            errorCode = write(firstRegister + (buffer[2]<<8 | buffer[3]), (unsigned char*)&tmp, 1);
        }
        //Иначе просто записывается пришедшее значение
        else
        {
            errorCode = write(firstRegister + (buffer[2]<<8 | buffer[3]), buffer+4, 1);
        }

        //Если slave-адрес в пришедшем кадре широковещательный, то отвечать не нужно
        if (SLAVE_ADDRESS == 0)
        {
            resultBufferSize = 0;
            return NULL;
        }

        //Если функция записи значения в регистр вернула ошибку, то формируется кадр ошибки
        if (errorCode)
        {
            resultBufferSize = 5;

            return CreateErrorBuffer(SLAVE_ADDRESS, COMMAND, errorCode);
        }

        //Для этой команды ответ совпадает с пришедшим кадром
        resultBufferSize = bufferSize;
        result = (unsigned char*)malloc(resultBufferSize);
        memcpy(result, buffer, 6);

        break;

    case 0x10:
        /*Если указан порядок следования байтов HighLow, то необходимо
        поменять местами байты в значения, которое записываются в регистры*/
        if (isHighLowOrder)
        {
            ChangeByteOrder(buffer+7, buffer[6]);
        }

        unsigned char errorCode = write(firstRegister + (buffer[2]<<8 | buffer[3]), buffer+7, buffer[6]/2);

        //Если slave-адрес в пришедшем кадре широковещательный, то отвечать не нужно
        if (SLAVE_ADDRESS == 0)
        {
            resultBufferSize = 0;
            return NULL;
        }

        //Если функция записи значения в регистр вернула ошибку, то формируется кадр ошибки
        if (errorCode)
        {
            resultBufferSize = 5;

            return CreateErrorBuffer(SLAVE_ADDRESS, COMMAND, errorCode);
        }

        resultBufferSize = 8;
        result = (unsigned char*)malloc(resultBufferSize);

        memcpy(result, buffer, 6);

        break;
    }

    //В конец кадра добавляется CRC
    *(unsigned short*)(result + resultBufferSize - 2) = CRC16(result, resultBufferSize - 2);

    return result;
}







/*Создание кадра для команды
"Чтение значений из нескольких регистров хранения"

(Выделяет память, которую нужно потом освободить!)*/
unsigned char* CreateBufferReadHoldingRegisters
(
    unsigned char slaveAddress,         //адрес slave-устройства, которому будет отправлен кадр
    unsigned short firstParamAddress,   //адрес регистра - начала памяти, которыю нужно прочитать
    unsigned short countRegisters,      //количество регистров, которые нужно прочитать
    unsigned int& bufferSize            //размер сформированного кадра (выходной параметр)
)
{
    bufferSize = 8;

    unsigned char* buffer = (unsigned char*)malloc(bufferSize);
    buffer[0] = slaveAddress;
    buffer[1] = 0x03;
    buffer[2] = firstParamAddress >> 8;
    buffer[3] = firstParamAddress & 0xFFU;
    buffer[4] = countRegisters >> 8;
    buffer[5] = countRegisters & 0xFFU;

    unsigned short crc = CRC16(buffer, 6);

    buffer[6] = crc & 0xFFU;
    buffer[7] = crc >> 8;

    return buffer;
}


/*Создание кадра для команды
"Чтение значений из нескольких регистров ввода"

(Выделяет память, которую нужно потом освободить!)*/
unsigned char* CreateBufferReadInputRegisters
(
    unsigned char slaveAddress,         //адрес slave-устройства, которому будет отправлен кадр
    unsigned short firstParamAddress,   //адрес регистра - начала памяти, которыю нужно прочитать
    unsigned short countRegisters,      //количество регистров, которые нужно прочитать
    unsigned int& bufferSize            //размер сформированного кадра (выходной параметр)
)
{
    bufferSize = 8;

    unsigned char* buffer = (unsigned char*)malloc(bufferSize);
    buffer[0] = slaveAddress;
    buffer[1] = 0x04;
    buffer[2] = firstParamAddress >> 8;
    buffer[3] = firstParamAddress & 0xFFU;
    buffer[4] = countRegisters >> 8;
    buffer[5] = countRegisters & 0xFFU;

    unsigned short crc = CRC16(buffer, 6);

    buffer[6] = crc & 0xFFU;
    buffer[7] = crc >> 8;

    return buffer;
}


/*Создание кадра для команды
"Запись значения в один регистр хранения"

(Выделяет память, которую нужно потом освободить!)*/
unsigned char* CreateBufferWriteSingleHoldingRegister
(
    unsigned char slaveAddress,     //адрес slave-устройства, которому будет отправлен кадр
    unsigned short paramAddress,    //адрес регистра slave-устройства, в который нужно записать данные
    unsigned short paramValue,      //значение для записи
    unsigned int& bufferSize,       //размер сформированного кадра (выходной параметр)
    char isHighLowOrder             //порядок следования байтов в записываемом значении (по умолчанию LowHigh)
)
{
    bufferSize = 8;

    unsigned char* buffer = (unsigned char*)malloc(bufferSize);
    buffer[0] = slaveAddress;
    buffer[1] = 0x10;
    buffer[2] = paramAddress >> 8;
    buffer[3] = paramAddress & 0xFFU;

    //Обработка порядка следования байтов в записываемом значении
    if (isHighLowOrder)
    {
        buffer[4] = paramValue >> 8;
        buffer[5] = paramValue & 0xFFU;
    }
    else
    {
        buffer[4] = paramValue & 0xFFU;
        buffer[5] = paramValue >> 8;
    }

    unsigned short crc = CRC16(buffer, 6);

    buffer[6] = crc & 0xFFU;
    buffer[7] = crc >> 8;

    return buffer;
}


/*Создание кадра для команды
"Запись значений в несколько регистров хранения"

(Выделяет память, которую нужно потом освободить!)*/
unsigned char* CreateBufferWriteMultipleHoldingRegisters
(
    unsigned char slaveAddress,         //адрес slave-устройства, которому будет отправлен кадр
    unsigned short firstParamAddress,   //адрес регистра - начала памяти для записи на slave-устройство
    unsigned short countRegisters,      //количество регистров для записи
    unsigned char countBytes,           //общий размер записываемой памяти в байтах
    unsigned short* values,             //массив значений, которые нужно записать по указанному адресу на slave-устройство
    unsigned int& bufferSize,           //размер сформированного кадра (выходной параметр)
    char isHighLowOrder                 //порядок следования байтов в записываемых значениях (по умолчанию LowHigh)
)
{
    bufferSize = 9 + countBytes;

    unsigned char* buffer = (unsigned char*)malloc(bufferSize);
    buffer[0] = slaveAddress;
    buffer[1] = 0x10;
    buffer[2] = firstParamAddress >> 8;
    buffer[3] = firstParamAddress & 0xFFU;
    buffer[4] = countRegisters & 0xFFU;
    buffer[5] = countRegisters >> 8;
    buffer[6] = countBytes;

    for (unsigned char i=0; i < countBytes/2; ++i)
    {
        //Обработка порядка следования байтов в записываемых значениях
        if (isHighLowOrder)
        {
            buffer[7+2*i] = (unsigned char)(values[i] >> 8);
            buffer[8+2*i] = (unsigned char)(values[i] & 0xFF);
        }
        else
        {
            buffer[7+2*i] = (unsigned char)(values[i] & 0xFF);
            buffer[8+2*i] = (unsigned char)(values[i] >> 8);
        }
    }

    unsigned short crc = CRC16(buffer, bufferSize - 2);

    //Добавление CRC в конец кадра
    buffer[bufferSize - 2] = crc & 0xFFU;
    buffer[bufferSize - 1] = crc >> 8;

    return buffer;
}


//Функция проверки корректности размера принятого от slave-устройства кадра и правильности CRC
char IsValidBufferSizeFromSlave(unsigned char* buffer, unsigned int size)
{
    //Явно некорректный кадр
    if(size<2)
        return 0;

    switch(COMMAND)
    {
    case 0x03:
    case 0x04:
        if (size<3)
            return 0;

        if (size != 5+buffer[2])
            return 0;

        break;

    case 0x06:
    case 0x10:
        if (size!=8)
            return 0;

        break;
    //Сообщения об ошибке:
    case 0x83:
    case 0x84:
    case 0x86:
    case 0x90:
        if (size != 5)
            return 0;

        break;

    default:
        return 0;
    }

    return 1;
}


/*Формирует строку в стиле C, содержащую информацию о принятом кадре в читаемом виде

(Выделяет память, которую нужно потом освободить!)*/
char* RecvBufferToString(unsigned char* buffer, unsigned int size, char isHighLowOrder)
{
    char *resultString = (char*)malloc(1024);   //cтрока-результат
    char errorMessage[100];                     //сообщение об ошибке
    int pos = 0;                                //позиция для следующей записи в формируемой строке-результате

    if (!IsValidBufferSizeFromSlave(buffer, size))
    {
        sprintf(resultString, "Пришло сообщение некорректной длины %u", size);
        return resultString;
    }

    unsigned short crc = CRC16(buffer, size-2);

    if (crc != (unsigned short)(buffer[size-1] << 8 | buffer[size-2]))
    {
        sprintf(resultString, "Несовпадение CRC\nПринято: %04hX, вычислено: %04hX",
                (unsigned short)(buffer[size-1] << 8 | buffer[size-2]), crc);
        return resultString;
    }

    switch (COMMAND)
    {
    case 0x03:
    case 0x04:
        pos = sprintf(resultString, "Адрес устройства: 0x%02hhX\nКоманда: 0x%02hhX\nКоличество байт: %hhu",
                      SLAVE_ADDRESS, COMMAND, buffer[2]);

        //Вывод в результирующую строку полученных значений
        for(unsigned char i=0; i < buffer[2]/2; ++i)
        {
            if (isHighLowOrder)
            {
                pos += sprintf(resultString+pos, "\nValue[%hhu] = 0x%04hX", i, buffer[3+2*i]<<8|buffer[4+2*i]);
            }
            else
            {
                pos += sprintf(resultString+pos, "\nValue[%hhu] = 0x%04hX", i, buffer[4+2*i]<<8|buffer[3+2*i]);
            }
        }

        break;

    case 0x06:
        pos = sprintf(resultString, "Адрес устройства: 0x%02hhX\nКоманда: 0x%02hhX\nАдрес параметра: 0x%04hX\n",
                      SLAVE_ADDRESS, COMMAND, buffer[2]<<8|buffer[3]);

        //Вывод в результирующую строку записанного значения
        if (isHighLowOrder)
        {
            sprintf(resultString+pos, "Значение записанного параметра: 0x%04hX", buffer[4]<<8|buffer[5]);
        }
        else
        {
            sprintf(resultString+pos, "Значение записанного параметра: 0x%04hX", buffer[5]<<8|buffer[4]);
        }

        break;

    case 0x10:
        sprintf(resultString, "Адрес устройства: 0x%02hhX\nКоманда: 0x%02hhX\nАдрес первого параметра: 0x%04hX\nКоличество параметров: %hu",
                SLAVE_ADDRESS, COMMAND, buffer[2]<<8|buffer[3], buffer[4]<<8|buffer[5]);

        break;


    default:
        //Если пришёл кадр-ошибка
        if (COMMAND & 0x80)
        {
            //Формирование сообщения об ошибке
            switch(buffer[2])
            {
            case 0x00:
                sprintf(errorMessage, "нет ошибки");
                break;
            case 0x01:
                sprintf(errorMessage, "неизвестная функция");
                break;
            case 0x02:
                sprintf(errorMessage, "неверный адрес регистра");
                break;
            case 0x03:
                sprintf(errorMessage, "неверный формат данных");
                break;
            case 0x04:
                sprintf(errorMessage, "неисправность оборудования");
                break;
            case 0x05:
                sprintf(errorMessage, "устройство приняло запрос и занято его обработкой");
                break;
            case 0x06:
                sprintf(errorMessage, "устройство занято обработкой предыдущей команды");
                break;
            case 0x08:
                sprintf(errorMessage, "ошибка при работе с памятью");
                break;
            default:
                sprintf(errorMessage, "неизвестная ошибка");
                break;
            }

            sprintf(resultString, "Команда: 0x%02hhX, Ошибка: %s (код: 0x%02hhX)", buffer[1], errorMessage, buffer[2]);
        }
        else
        {
            sprintf(resultString, "Неизвестная команда с кодом 0x%02hhX", buffer[1]);
        }
    }

    return resultString;
}


/*Формирует строку в стиле С, содержащую информацию о принятом
кадре в виде списка байтов в шестнадцатеричной системе счисления

(Выделяет память, которую нужно потом освободить!)*/
char* StringOfBufferBytes(unsigned char* buffer, unsigned int size)
{
    char *str = new char[size*3+1];
    int pos = 0;

    for (unsigned short i = 0; i < size; ++i)
    {
        pos+=sprintf(str+pos, "%02hhX ", buffer[i]);
    }
    return str;
}
