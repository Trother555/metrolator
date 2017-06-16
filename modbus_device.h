#ifndef MODBUS_DEVICE_H
#define MODBUS_DEVICE_H

//Библиотека функций протокола Modbus, зависимых от устройства

#define ALL_MEMORY_SIZE (112)                   //Память Modbus устройства в байтах

//Таблица памяти Modbus
struct primary_table_s
{
  unsigned char all_memory[ALL_MEMORY_SIZE];	//Вся память Modbus
  unsigned char* end_memory;                    //Указатель на конец памяти
  primary_table_s():
    end_memory(all_memory + ALL_MEMORY_SIZE){}
};


//

//Основная функция протокола Modbus.
//Считывает пришедший кадр, если есть, отправляет ответ, если надо.
//Нужно выполнять, пока устройство активно в сети Modbus.
void MBS_operation();

//Начальная инициализация
//подключение к сети и т.д.
void MBS_init();

//Запись в память Modbus в обход протокола
//по смещению offset от src регистров в количестве reg_cnt
void MBS_write_registers(int offset, unsigned char* src, unsigned char reg_cnt);


//Определения смещений регистров в байтах относительно начала памяти

//Имя регистра      //Смещение      //Содержимое регистра

#define RG_SN =     (0x0);          //Серийный номер
#define RG_VP =     (0x4);          //Версия ПО счётчика
#define RG_CS =     (0x6);          //Контрольная сумма метрологического модуля ПО
#define RG_PP =     (0x8);          //Время и дата первичной проверки
#define RG_K1 =     (0xE);          //Калибровочный коэффициент k1
#define RG_K2 =     (0x12);         //Калибровочный коэффициент k2
#define RG_ADR =    (0x16);         //Сетевой адрес Modbus
#define RG_TV =     (0x30);         //Текущие показания счётчика
#define RG_PW =     (0x34);         //Напряжение батареи
#define RG_SA =     (0x38);         //Индекс суточного архива
#define RG_MA =     (0x3A);         //Индекс месячного архива
#define RG_TM =     (0x3C);         //Текущее время и дата
#define RG_FL =     (0x42);         //Флаги
#define RG_TP =     (0x50);         //Время и дата вскрытия
#define RG_MG =     (0x56);         //Время и дата воздействия сильного магнита
#define RG_HC =     (0x5C);         //Индекс журнала нештатных событий
#define RG_CC =     (0x5E);         //Индекс журнала системных событий

//Номера битов флагов флагового регистра относительно начала регистра

//Имя флага         //Номер бита    //Смысл флага

#define F_TP        (15)            //Вскрытие корпуса
#define F_MG        (14)            //Действие сильного магнита
#define F_R         (13)            //Противоток воды более 30с
#define F_CL        (1)             //Очистка памяти счётчика
#define F_BL        (0)             //Перевод в метрологический режим

#endif // MODBUS_DEVICE_H
