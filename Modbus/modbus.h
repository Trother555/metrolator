#ifndef MODBUS_H
#define MODBUS_H

/*Вычисляет CRC16 по массиву указанного размера
(алгоритм взят из протокола связи вычислителя ВКТ-5 с системой верхнего уровня)*/
unsigned short CRC16(unsigned char* buffer, unsigned int size);



//Проверяет корректность кадра, принятого slave-устройством
char IsValidBufferSizeFromMaster(unsigned char* buffer, unsigned int size);


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
    unsigned char (*read)(unsigned char*, unsigned char*, unsigned char), //функция чтения данных из памяти slave-устройства
    unsigned char (*write)(unsigned char*, unsigned char*, unsigned char),//функция записи данных в память slave-устройства
    unsigned char* firstRegister,               //указатель на начало регистровой памяти в slave-устройстве
    unsigned short totalRegistersSize = 0xFFFFU,//общее количество регистров (все регистры полагаются двухбайтовыми,
                                                //по умолчанию размер памяти принимается максимально возможным)
    char isHighLowOrder = 0                     //порядок следования байтов в области записываемых значений (по умолчанию LowHigh)
);






//Далее следуют объявления функций формирования кадра на master-устройстве


/*Создание кадра для команды
"Чтение значений из нескольких регистров хранения"

(Выделяет память, которую нужно потом освободить!)*/
unsigned char* CreateBufferReadHoldingRegisters
(
    unsigned char slaveAddress,         //адрес slave-устройства, которому будет отправлен кадр
    unsigned short firstParamAddress,   //адрес регистра - начала памяти, которую нужно прочитать
    unsigned short countRegisters,      //количество регистров, которые нужно прочитать
    unsigned int& bufferSize            //размер сформированного кадра (выходной параметр)
);


/*Создание кадра для команды
"Чтение значений из нескольких регистров ввода"

(Выделяет память, которую нужно потом освободить!)*/
unsigned char* CreateBufferReadInputRegisters
(
    unsigned char slaveAddress,         //адрес slave-устройства, которому будет отправлен кадр
    unsigned short firstParamAddress,   //адрес регистра - начала памяти, которыю нужно прочитать
    unsigned short countRegisters,      //количество регистров, которые нужно прочитать
    unsigned int& bufferSize            //размер сформированного кадра (выходной параметр)
);


/*Создание кадра для команды
"Запись значения в один регистр хранения"

(Выделяет память, которую нужно потом освободить!)*/
unsigned char* CreateBufferWriteSingleHoldingRegister
(
    unsigned char slaveAddress,     //адрес slave-устройства, которому будет отправлен кадр
    unsigned short paramAddress,    //адрес регистра slave-устройства, в который нужно записать данные
    unsigned short paramValue,      //значение для записи
    unsigned int& bufferSize,       //размер сформированного кадра (выходной параметр)
    char isHighLowOrder = 0         //порядок следования байтов в записываемом значении (по умолчанию LowHigh)
);


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
    char isHighLowOrder = 0             //порядок следования байтов в записываемых значениях (по умолчанию LowHigh)
);




//Далее следуют функции обработки принятого от slave-устройства кадра


//Функция проверки корректности размера принятого от slave-устройства кадра
char IsValidBufferSizeFromSlave(unsigned char* buffer, unsigned int size);


/*Формирует строку в стиле C, содержащую информацию о принятом кадре в читаемом виде

(Выделяет память, которую нужно потом освободить!)*/
char* RecvBufferToString(unsigned char* buffer, unsigned int size, char isHighLowOrder);


/*Формирует строку в стиле С, содержащую информацию о принятом
кадре в виде списка байтов в шестнадцатеричной системе счисления

(Выделяет память, которую нужно потом освободить!)*/
char* StringOfBufferBytes(unsigned char* buffer, unsigned int size);

#endif // MODBUS_H
