#include <Time.h>
#include <TimeLib.h>
#include <DS1302.h>
#include <AM2320.h>
#include "U8glib.h"
#include <avr/pgmspace.h>
#include <EEPROM.h>
#include "rus6x10.h"
#include "MyDigitsHelveticaBold24.h"
#include <OneWire.h>
#include <DallasTemperature.h>

// пропишем пины
#define PIN_ROTATE    13
#define PIN_RTC_RST   12   // ce_pin
#define PIN_RTC_DAT   11   // io_pin
#define PIN_RTC_SCK   10   // sck_pin
#define PIN_ONE_WIRE  8
#define PIN_BTN_UP    7
#define PIN_BTN_OK    6
#define PIN_BTN_DOWN  5
#define PIN_HEAT      4
#define PIN_WATER     3

#define COUNT_TYPE_EGG 3

// структура 
struct dataEgg {
  byte    type;       // тип яиц
  byte    period;     // период высиживание (обычно 4 периода)
  float   t;          // температура
  byte    h;          // влажность
  byte    days;       // количество дней высиживания
  byte    rotate;     // время между переворотами лотков
};

  // структура элемента записи меню
  struct menuItem {
    byte id;           // id меню
    byte prevItem;     // родительский пункт меню
    char* caption;     // указатель для текста для экрана
    byte typeItem;     // тип записи (0-меню, 1-субменю, 2-параметр, 9-выход)
    byte typeParam;    // тип parametra (0-NULL, 1-byte, 2-int, 3-float)
    union {            // указатель на параметр
        byte *b;
        int *i;
        float *f;
      };            
  };

// переменные
int curPointX = 0;
menuItem * curMenu = NULL;  // текущее меню
byte curMenuItemNum = 0;    // текущая строка меню
boolean openMenu = false;
boolean openParam = false;
int countItems;             // количество строк в меню
char * bufTitle;            // переменная хранит зоголовок
float term = 0.0;           // температура
float term2 = 0.0;          // температура с второго датчика
float humid = 0.0;          // влажность
dataEgg * curDataEgg;       // указатель на текущеи параметры яиц
boolean error = false;      // есть ошибка
char * msg_err;             // текст ошибки   
boolean bl_cooler = false;  // включен вентелятор
boolean bl_rotate = false;  // переворот лотка
boolean err_ds18b20 = false;// ошибак подключения ds18b20
uint8_t curDay;
time_t lastRotate;

// глабальные параметры
byte brightLCD;              // яркость экрана 1..255
byte currTypeEgg = 0;        // текущий тип яиц
uint8_t startYear  = 17;       // год закладки яиц
uint8_t startMon    = 1;        // месяц закладки яиц
uint8_t startDay    = 1;        // число закладки яиц
uint16_t popryear = 2000;


// типы яиц
  const char typeEgg[3][6] PROGMEM = {
    "chick",
    "duck",
    "goose"
};

// ========= параметры высиживания ==========
dataEgg eggs[] = {     
//  тип перд темр  вл  дн  перв
//------------ куры -------------
    {0,  1,  37.9, 66, 11, 240},
    {0,  2,  37.3, 53, 17, 240},
    {0,  3,  37.3, 47, 19, 240},
    {0,  4,  37.0, 66, 21,   0},
//------------ утки -------------
    {1,  1,  38.0, 70,  8,   0},
    {1,  2,  37.5, 60, 13, 240},
    {1,  3,  37.2, 56, 24, 240},
    {1,  4,  37.0, 70, 28,   0},
//------------ гуси -------------
    {2,  1,  37.8, 63,  3,   0},
    {2,  2,  37.8, 54, 13, 240},
    {2,  3,  37.5, 56, 24, 240},
    {2,  4,  37.2, 57, 28,   0},
  };

// ***************** СТРОКИ *******************
  PROGMEM const char st_menu[]        = "МЕНЮ";
  PROGMEM const char st_type_egg[]    = "ПАРАМЕТРЫ ЯИЦ";
  PROGMEM const char st_cur_egg[]     = "ТИП ЯЙЦА";
    
  PROGMEM const char st_chick[]       = "КУРИНЫЕ";
  PROGMEM const char st_duck[]        = "УТИНЫЕ";
  PROGMEM const char st_goose[]       = "ГУСИНЫЕ";
  
  PROGMEM const char st_setting[]     = "УСТАНОВКИ";
  PROGMEM const char st_briht[]       = "ЯРКОСТЬ";
  PROGMEM const char st_datetime[]    = "ЧАСЫ";
  PROGMEM const char st_startdate[]   = "ДАТА ЗАКЛАДКИ";
  PROGMEM const char st_date[]        = "ДАТА";
  PROGMEM const char st_time[]        = "ВРЕМЯ";
  
  PROGMEM const char st_period_parm[] = "ПАРАМ. ПЕРИОДОВ";
  PROGMEM const char st_period_1[]    = "ПЕРИОД 1";  
  PROGMEM const char st_period_2[]    = "ПЕРИОД 2";  
  PROGMEM const char st_period_3[]    = "ПЕРИОД 3";  
  PROGMEM const char st_period_4[]    = "ПЕРИОД 4";  

  PROGMEM const char st_temperature[] = "ТЕМПЕРАТУРА";  
  PROGMEM const char st_humidity[]    = "ВЛАЖНОСТЬ";  
  PROGMEM const char st_last_day[]    = "ПОСЛ. ДЕНЬ";  
  PROGMEM const char st_rotation[]    = "ПЕРЕВОРОТ";  

  PROGMEM const char st_exit[]        = "ВЫХОД";
  
  PROGMEM const char rts_not_run[]    = "ЧАСЫ НЕ ВЫСТАВЛЕНЫ";

  PROGMEM const char err_term[]       = "ОШИБКА ТЕМПЕРАТУРЫ";

  PGM_P arrStr[] = {
      st_chick, st_duck, st_goose
    }; 

// *****************  МЕНЮ  ************************
  PROGMEM const menuItem menu[] = {
  { 1, 0, st_menu, 0, 0, NULL},                                                   // меню
    { 2, 1, st_type_egg, 0, 0, NULL},                                             //   |- тип яиц
      { 4, 2, st_chick, 0, 0, NULL},                                              //       |- куриные 
        { 5, 4, st_period_1, 1, 0, NULL},                                         //           |- период 1
          { 0, 5, st_temperature, 2, 3, (byte *)&eggs[0].t},                      //               |- температура
          { 0, 5, st_humidity,    2, 1, &eggs[0].h},                              //               |- влажность
          { 0, 5, st_last_day,    2, 1, &eggs[0].days},                           //               |- номер дня конца 1-го периода
          { 0, 5, st_rotation,    2, 1, &eggs[0].rotate},                         //               |- интенсивность переворота лотков
        { 6, 4, st_period_2, 1, 0, NULL},                                         //           |- период 2
          { 0, 6, st_temperature, 2, 3, (byte *)&eggs[1].t},                      //               |- температура
          { 0, 6, st_humidity,    2, 1, &eggs[1].h},                              //               |- влажность
          { 0, 6, st_last_day,    2, 1, &eggs[1].days},                           //               |- номер дня конца 1-го периода
          { 0, 6, st_rotation,    2, 1, &eggs[1].rotate},                         //               |- интенсивность переворота лотков
        { 7, 4, st_period_3, 1, 0, NULL},                                         //           |- период 3
          { 0, 7, st_temperature, 2, 3, (byte *)&eggs[2].t},                      //               |- температура
          { 0, 7, st_humidity,    2, 1, &eggs[2].h},                              //               |- влажность
          { 0, 7, st_last_day,    2, 1, &eggs[2].days},                           //               |- номер дня конца 1-го периода
          { 0, 7, st_rotation,    2, 1, &eggs[2].rotate},                         //               |- интенсивность переворота лотков
        { 8, 4, st_period_4, 1, 0, NULL},                                         //           |- период 4
          { 0, 8, st_temperature, 2, 3, (byte *)&eggs[3].t},                      //               |- температура
          { 0, 8, st_humidity,    2, 1, &eggs[3].h},                              //               |- влажность
          { 0, 8, st_last_day,    2, 1, &eggs[3].days},                           //               |- номер дня конца 1-го периода
          { 0, 8, st_rotation,    2, 1, &eggs[3].rotate},                         //               |- интенсивность переворота лотков
      { 9, 2, st_duck, 0, 0, NULL},                                               //       |- утки 
        {10, 9, st_period_1, 1, 0, NULL},                                         //           |- период 1
          { 0, 10, st_temperature, 2, 3, (byte *)&eggs[4].t},                     //               |- температура
          { 0, 10, st_humidity,    2, 1, &eggs[4].h},                             //               |- влажность
          { 0, 10, st_last_day,    2, 1, &eggs[4].days},                          //               |- номер дня конца 1-го периода
          { 0, 10, st_rotation,    2, 1, &eggs[4].rotate},                        //               |- интенсивность переворота лотков
        {11, 9, st_period_2, 1, 0, NULL},                                         //           |- период 2
          { 0, 11, st_temperature, 2, 3, (byte *)&eggs[5].t},                     //               |- температура
          { 0, 11, st_humidity,    2, 1, &eggs[5].h},                             //               |- влажность
          { 0, 11, st_last_day,    2, 1, &eggs[5].days},                          //               |- номер дня конца 1-го периода
          { 0, 11, st_rotation,    2, 1, &eggs[5].rotate},                        //               |- интенсивность переворота лотков
        {12, 9, st_period_3, 1, 0, NULL},                                         //           |- период 3
          { 0, 12, st_temperature, 2, 3, (byte *)&eggs[6].t},                     //               |- температура
          { 0, 12, st_humidity,    2, 1, &eggs[6].h},                             //               |- влажность
          { 0, 12, st_last_day,    2, 1, &eggs[6].days},                          //               |- номер дня конца 1-го периода
          { 0, 12, st_rotation,    2, 1, &eggs[6].rotate},                        //               |- интенсивность переворота лотков
        {13, 9, st_period_4, 1, 0, NULL},                                         //           |- период 4
          { 0, 13, st_temperature, 2, 3, (byte *)&eggs[7].t},                     //               |- температура
          { 0, 13, st_humidity,    2, 1, &eggs[7].h},                             //               |- влажность
          { 0, 13, st_last_day,    2, 1, &eggs[7].days},                          //               |- номер дня конца 1-го периода
          { 0, 13, st_rotation,    2, 1, &eggs[7].rotate},                        //               |- интенсивность переворота лотков
      {14, 2, st_goose, 0, 0, NULL},                                              //       |- гуси 
        {15, 14, st_period_1, 1, 0, NULL},                                        //           |- период 1
          { 0, 15, st_temperature, 2, 3, (byte *)&eggs[8].t},                     //               |- температура
          { 0, 15, st_humidity,    2, 1, &eggs[8].h},                             //               |- влажность
          { 0, 15, st_last_day,    2, 1, &eggs[8].days},                          //               |- номер дня конца 1-го периода
          { 0, 15, st_rotation,    2, 1, &eggs[8].rotate},                        //               |- интенсивность переворота лотков
        {16, 14, st_period_2, 1, 0, NULL},                                        //           |- период 2
          { 0, 16, st_temperature, 2, 3, (byte *)&eggs[9].t},                     //               |- температура
          { 0, 16, st_humidity,    2, 1, &eggs[9].h},                             //               |- влажность
          { 0, 16, st_last_day,    2, 1, &eggs[9].days},                          //               |- номер дня конца 1-го периода
          { 0, 16, st_rotation,    2, 1, &eggs[9].rotate},                        //               |- интенсивность переворота лотков
        {17, 14, st_period_3, 1, 0, NULL},                                        //           |- период 3
          { 0, 17, st_temperature, 2, 3, (byte *)&eggs[10].t},                    //               |- температура
          { 0, 17, st_humidity,    2, 1, &eggs[10].h},                            //               |- влажность
          { 0, 17, st_last_day,    2, 1, &eggs[10].days},                         //               |- номер дня конца 1-го периода
          { 0, 17, st_rotation,    2, 1, &eggs[10].rotate},                       //               |- интенсивность переворота лотков
        {18, 14, st_period_4, 1, 0, NULL},                                        //           |- период 4
          { 0, 18, st_temperature, 2, 3, (byte *)&eggs[7].t},                     //               |- температура
          { 0, 18, st_humidity,    2, 1, &eggs[11].h},                            //               |- влажность
          { 0, 18, st_last_day,    2, 1, &eggs[11].days},                         //               |- номер дня конца 1-го периода
          { 0, 18, st_rotation,    2, 1, &eggs[11].rotate},                       //               |- интенсивность переворота лотков
    { 3, 1, st_setting, 0, 0, NULL},                                              //   |- установки                                              
      {20, 3, st_datetime, 0, 0, NULL},                                           //        |- часы (DS1302)
        { 0, 20, st_date,  4, 5, NULL},                                           //               |- дата 
        { 0, 20, st_time,  4, 6, NULL},                                           //               |- время 
      {21, 3, st_startdate, 4, 7, NULL},                                          //        |- дата закладки
      {19, 3, st_cur_egg, 0, 0, NULL},                                            //        |- заложеный тип яиц 
        { 0, 19, st_chick, 3, 0, NULL},                                           //               |- куриные 
        { 0, 19, st_duck,  3, 0, NULL},                                           //               |- утиные 
        { 0, 19, st_goose, 3, 0, NULL},                                           //               |- гусиные 
    
  {99, 0, st_exit, 9, 0, NULL}                                                    //     |- выход
  };

  // количество элементов в массиве меню 
  const byte sizeMenu = sizeof(menu) / sizeof(menuItem);

  // меню отображаемое на экране
  const byte sizeDispMenu = 5;
  menuItem dispMenu[sizeDispMenu];
  
  PROGMEM const unsigned char cooler [] = {
  0x18,     // 0 0 0 1 1 0 0 0
  0x20,     // 0 0 1 0 0 0 0 0
  0x94,     // 1 0 0 1 0 1 0 0
  0xAA,     // 1 0 1 0 1 0 1 0
  0x52,     // 0 1 0 1 0 0 1 0
  0x08,     // 0 0 0 0 1 0 0 0
  0x30      // 0 0 1 1 0 0 0 0
  };
  
  PROGMEM const unsigned char heater [] = {
  0xFF,     // 1 1 1 1 1 1 1 1
  0x01,     // 0 0 0 0 0 0 0 1
  0xFF,     // 1 1 1 1 1 1 1 1
  0x80,     // 1 0 0 0 0 0 0 0
  0xFF,     // 1 1 1 1 1 1 1 1
  0x01,     // 0 0 0 0 0 0 0 1
  0xFF      // 1 1 1 1 1 1 1 1
  };

  PROGMEM const unsigned char water [] = {
  0x42,     // 0 1 0 0 0 0 1 0
  0xA5,     // 1 0 1 0 0 1 0 1
  0x99,     // 1 0 0 1 1 0 0 1
  0xBD,     // 1 0 1 1 1 1 0 1
  0x99,     // 1 0 0 1 1 0 0 1
  0x99,     // 1 0 0 1 1 0 0 1
  0x99      // 1 0 0 1 1 0 0 1
  };

  PROGMEM const unsigned char rotate_l [] = {
  0x01,     // 0 0 0 0 0 0 0 1
  0x03,     // 0 0 0 0 0 0 1 1
  0x06,     // 0 0 0 0 0 1 1 0
  0x0C,     // 0 0 0 0 1 1 0 0
  0x18,     // 0 0 0 1 1 0 0 0
  0x30,     // 0 0 1 1 0 0 0 0
  0x60,     // 0 1 1 0 0 0 0 0
  0xC0      // 1 1 0 0 0 0 0 0
  };

  PROGMEM const unsigned char rotate_r [] = {
  0xC0,     // 0 0 0 0 0 0 0 1
  0x60,     // 0 0 0 0 0 0 1 1
  0x30,     // 0 0 0 0 0 1 1 0
  0x18,     // 0 0 0 0 1 1 0 0
  0x0C,     // 0 0 0 1 1 0 0 0
  0x06,     // 0 0 1 1 0 0 0 0
  0x03,     // 0 1 1 0 0 0 0 0
  0x01      // 1 1 0 0 0 0 0 0
  };


  // *********** СОЗДАНИЕ ОБЬЕКТОВ *****************
  // создание обьект дисплея
  U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NONE|U8G_I2C_OPT_DEV_0);

  // ds18b20
  OneWire oneWire(PIN_ONE_WIRE);
  DallasTemperature ds18b20(&oneWire);
  DeviceAddress devID;

  // создание датчка влажности
  AM2320 sensorh;

  // часы реального времени
  DS1302 rtc(PIN_RTC_RST, PIN_RTC_DAT, PIN_RTC_SCK); 

  void drawInfo(void) {
    // ОСНОВНАЯ ИНФОРМАЦИЯ
      u8g.setFont(rus6x10);
      u8g.drawStr(1, 12, getStrFromPROGMEM(arrStr[currTypeEgg]));                                   // Заголовак
      u8g.drawStr(0, 24, getDateTimeStr());                                                         // дата и время
      if (err_ds18b20) u8g.drawStr(50, 36, "err ds18b20");
      u8g.drawStr(0, 35, getInfoPeriod());                                                          // информация о периоде
      u8g.drawStr(65, 48, "0");                                                                     // температура и влажность
      u8g.drawStr(120, 48, "%");
      u8g.setFont(MyDigitsHelveticaBold24);
      u8g.setPrintPos(1, 64);
      u8g.println(term, 1);
      u8g.setPrintPos(80, 64);
      u8g.println(humid, 0);
      
      // изобразить нагрев
      if (digitalRead(PIN_HEAT) == HIGH) {
        u8g.drawBitmapP( 80, 5, 1, 7, heater); 
        }
        
      // изобразить увлажнителя  
      if (digitalRead(PIN_WATER) == HIGH) {
        u8g.drawBitmapP( 95, 5, 1, 7, water); 
        }
        
      // изобразить повората лотка
      if (digitalRead(PIN_ROTATE) == HIGH) {
        u8g.drawBitmapP( 110, 5, 1, 8, rotate_l); 
        } else {
          u8g.drawBitmapP( 110, 5, 1, 8, rotate_r); 
          }                                       
  }

  void drawMenu(int posCursor) {
    byte p = 28;
    u8g.setFont(rus6x10);
    // Заголовак
    u8g.drawStr(1, 12, getStrFromPROGMEM(bufTitle));
    for (int i = 0; i < countItems; i++) {
      u8g.drawStr(9, 9*i+p, getStrFromPROGMEM(dispMenu[i].caption));
      // отобразить значение параметра
        u8g.setPrintPos(90, 9*i+p);
        switch(dispMenu[i].typeParam) {
          case 1: u8g.println(*dispMenu[i].b); break;
          case 2: u8g.println(*dispMenu[i].i); break;
          case 3: u8g.println(*dispMenu[i].f, 1); break;
        }  
      if (i == posCursor) u8g.drawStr( 1, 9*i+p, "-");
      }
    }  

  // редактор параметров
  void drawParam(menuItem *item){
    countItems = 0;
    // Заголовак
    u8g.setFont(rus6x10);
    u8g.drawStr(1, 12, getStrFromPROGMEM(bufTitle));
    openParam = true;
    // значение параметра
    u8g.setFont(MyDigitsHelveticaBold24);
    u8g.setPrintPos(10, 54);
    switch (item->typeParam) {
      case 0:
      case 1:
          u8g.println(*(item->b));
        break;
      case 3:
        u8g.println(*(item->f), 1);
        break;
      default:
        break;        
      }
    }

  void drawErr() {
    u8g.setFont(rus6x10);
    u8g.drawStr(1, 36, getStrFromPROGMEM(msg_err));
    }

  // редактор даты
  void ReadDate(uint8_t &p_day, uint8_t &p_mon, uint8_t &p_year) {
    boolean b_update = true;                                                // флаг показвыввает необходимость пеперисовки экрана
    byte curreg = 0;
    boolean b_exit = false;
    char buf[50];
    while(!b_exit) {
      if (b_update) { 
        u8g.firstPage();  
        do {
          u8g.setFont(rus6x10);
          u8g.drawStr(1, 12, getStrFromPROGMEM(st_date));                                   // Заголовак
          u8g.drawStr(5, 60, getStrFromPROGMEM(st_exit)); 
          u8g.setFont(MyDigitsHelveticaBold24);
          snprintf(buf, sizeof(buf), "%02d.%02d.%02d",
                                    p_day, p_mon, p_year);
          u8g.drawStr(2, 45, buf);
          switch(curreg) {
            case 0:
            case 1:
            case 2:
                u8g.drawFrame(0+curreg*45, 18, 39, 30);
                break;
            case 3:    
                u8g.drawFrame(0, 50, 40, 13);
                break;
            }
        } while( u8g.nextPage() );
        delay(200);
        b_update = false;
      }  
        if (isClick(PIN_BTN_OK)) {
            b_update = true;
            if (curreg < 4) {
                curreg++;
              } else {
                b_exit = true;
              }
            while(isClick(PIN_BTN_OK)) {}; // ожидание отжатие кнопки   
         } else 
        if (isClick(PIN_BTN_UP)) {
            b_update = true;
            switch(curreg) {
              case 0: p_day == 31 ? 1 : p_day++; break;
              case 1: p_mon == 12 ? 1 : p_mon++; break;
              case 2: p_year == 99 ? 0 : p_year++; break;
            }
         } else 
        if (isClick(PIN_BTN_DOWN)) {
            b_update = true;
            switch(curreg) {
              case 0: p_day == 1 ? 31 : p_day--; break;
              case 1: p_mon == 1 ? 12 : p_mon--; break;
              case 2: p_year == 0 ? 99 : p_year--; break;
            }    
         }
      }
    }

  // редактор времени
  void ReadTime(uint8_t &p_hh, uint8_t &p_mm) {
    boolean b_update = true;                                                // флаг показвыввает необходимость пеперисовки экрана
    byte curreg = 0;
    boolean b_exit = false;
    char buf[50];
    while(!b_exit) {
      if (b_update) { 
        u8g.firstPage();  
        do {
          u8g.setFont(rus6x10);
          u8g.drawStr(1, 12, getStrFromPROGMEM(st_time));                                   // Заголовак
          u8g.drawStr(5, 60, getStrFromPROGMEM(st_exit)); 
          u8g.setFont(MyDigitsHelveticaBold24);
          snprintf(buf, sizeof(buf), "%02d : %02d",
                                    p_hh, p_mm);
          u8g.drawStr(10, 45, buf);
          switch(curreg) {
            case 0:
                u8g.drawFrame(5, 18, 47, 30);
                break;
            case 1:
                u8g.drawFrame(70, 18, 47, 30);
                break;
            case 2:    
                u8g.drawFrame(0, 50, 40, 13);
                break;
            }
        } while( u8g.nextPage() );
        delay(200);
        b_update = false;
      }  
        if (isClick(PIN_BTN_OK)) {
            b_update = true;
            if (curreg < 4) {
                curreg++;
              } else {
                b_exit = true;
              }
            while(isClick(PIN_BTN_OK)) {}; // ожидание отжатие кнопки   
         } else 
        if (isClick(PIN_BTN_UP)) {
            b_update = true;
            switch(curreg) {
              case 0: p_hh == 23 ? 0 : p_hh++; break;
              case 1: p_mm == 59 ? 0 : p_mm++; break;
            }    
         } else 
        if (isClick(PIN_BTN_DOWN)) {
            b_update = true;
            switch(curreg) {
              case 0: p_hh == 0 ? 23 : p_hh--; break;
              case 1: p_mm == 0 ? 59 : p_mm--; break;
            }    
         }
      }
    }

  uint8_t getDateDiff(uint8_t yr1, uint8_t mon1, uint8_t day1, uint8_t yr2, uint8_t mon2, uint8_t day2) {
    tmElements_t tmSet1;
    tmElements_t tmSet2;

    tmSet1.Year   = yr1 - 70;
    tmSet1.Month  = mon1;
    tmSet1.Day    = day1;

    tmSet2.Year   = yr2 - 70;
    tmSet2.Month  = mon2;
    tmSet2.Day    = day2;
    
    time_t t1 = makeTime(tmSet1);
    time_t t2 = makeTime(tmSet2);
    
    return (t1-t2)/60/60/24;
    };

  void calcNumCurDay() {
      Time t = rtc.time();
      curDay = getDateDiff(t.yr-popryear, t.mon, t.date, startYear, startMon, startDay);
    }

  void calcCurPeriod() {
    curDataEgg = &eggs[currTypeEgg*4];
    for (byte i = 0; i < COUNT_TYPE_EGG*4; i++) {
      if (eggs[i].type == currTypeEgg && eggs[i].days < curDay) { 
        curDataEgg = &eggs[i];
      }
    }
    }

void setup() {
  // инициализаця кнопок
  pinMode(PIN_BTN_UP, INPUT);
  pinMode(PIN_BTN_OK, INPUT);
  pinMode(PIN_BTN_DOWN, INPUT);

  // выходы
  pinMode(PIN_HEAT, OUTPUT);      // нагреватель
  pinMode(PIN_WATER, OUTPUT);     // увлвжнитель

  // часы
  rtc.writeProtect(false);
  rtc.halt(false);
  //Time t(2017, 4, 26, 12, 20, 00, Time::kWednesday);
  //rtc.time(t);

  // датчик температуры
  ds18b20.begin();
  if (!ds18b20.getAddress(devID, 0)) {
      err_ds18b20 = true;
    } else {
      ds18b20.setResolution(devID, 16);
    } 
  //при старте удерживать кнопку "МЕНЮ" для загрузки значений по умолчанию 
  if (!isClick(PIN_BTN_OK)) {
        //loadParam();
    }    
  // загрузить параметры из памяти
   //saveParam();
  
}

void loop() {
  if (!openMenu){
      getTerm();
      getHum();
      calcNumCurDay();
      calcCurPeriod();
      cntrHead();
      cntrWater();
      cntrRotar();
      delay(500);
    }

  clickBtn();
  
  // прорисовка экрана
  u8g.firstPage();  
  do {
    if (!openMenu) {
       if (error) {
        drawErr();
        } else {
          drawInfo();
          }
    } else {
      if (openParam) {
          drawParam(curMenu);
        } else {
          drawMenu(curMenuItemNum);
        }
    }
  } while( u8g.nextPage() );
}

  // возвращает строку из флеш-памяти
  char* getStrFromPROGMEM(char* str) {
        char buf[24];
        strcpy_P(buf, str);
        return buf; 
  }

  // контроль температуры
  void cntrHead() {
    if (term < term2-10 || term > term2+10) {
      error = true;
      msg_err = err_term;
      return;
      } else {
        error = false;
        }
    if (term < curDataEgg->t - 0.1) {
      digitalWrite(PIN_HEAT, HIGH);
      } else if (term > curDataEgg->t){
        digitalWrite(PIN_HEAT, LOW);
        }
  }

  // контроль влажности
  void cntrWater() {
    if (humid < curDataEgg->h - 5) {
       digitalWrite(PIN_WATER, HIGH);
      } else if (humid > curDataEgg->h) {
        digitalWrite(PIN_WATER, LOW);
      }
    }

  // контроль переворота лотков
  void cntrRotar() {
    // текущее время
    Time t = rtc.time();
    tmElements_t elem;
    
    elem.Second = t.sec;
    elem.Minute = t.min;
    elem.Hour   = t.hr;
    elem.Day    = t.date;
    elem.Month  = t.mon;
    elem.Year   = t.yr;
    
    // перевести текущее время в формат time_t
    time_t t_now = makeTime(elem);
    
    if (lastRotate > (t_now - curDataEgg->rotate*60*60) ) {
       lastRotate = t_now;
       bl_rotate = !bl_rotate;
       digitalWrite(PIN_ROTATE, bl_rotate);
      }
    }

  void getTerm(){
    ds18b20.requestTemperatures();
    term = ds18b20.getTempC(devID);
  }

  void getHum(){
    sensorh.Read();
    humid = sensorh.h;
    term2 = sensorh.t;
  }

  char* getDateTimeStr() {
    Time t = rtc.time();
    char buf[50];
    snprintf(buf, sizeof(buf), "%02d.%02d.%04d %02d.%02d.%04d",
           startDay, startMon, startYear+popryear, t.date, t.mon, t.yr);
    return buf;
    }

  char* getInfoPeriod() {
    char buf[50];
    snprintf(buf, sizeof(buf), "П%d Д%d  :  t%02d.%01d h%d",
      curDataEgg->period, curDay, (int)curDataEgg->t, (int)(curDataEgg->t*10)%10, curDataEgg->h);
    return buf;
  }
  
  void clickBtn() {
  Time t(2013, 9, 22, 1, 38, 50, Time::kSunday);
  uint8_t yr;
   //  ----- button up --------------
    if (openMenu && isClick(PIN_BTN_UP)) {
      if (openParam) {
        switch (curMenu->typeParam) {
          case 1:
           (*(curMenu->b))++; 
           break;
          case 2:
           (*(curMenu->i))++; 
           break;
          case 3:
           (*(curMenu->f)) += 0.1; 
           break;
          }
      } else if (curMenuItemNum > 0) --curMenuItemNum;
    }
    //  ----- button ok --------------
    if (isClick(PIN_BTN_OK)) {
      if (!openMenu) {
        openMenu = true;
        openParam = false;
        loadDispMenu(0);
        curMenu = &dispMenu[0];                                      // то поставить текущим первый пункт меню
      }
        else curMenu = &dispMenu[curMenuItemNum];
        
        // если открыто редактирование параметра закрыть его и перейти в меню
        if (openParam){
            loadDispMenu(0);
            curMenu = &dispMenu[0];                                      // то поставить текущим первое меню
            openParam = false;
          }
  
      // определить тип записи меню и выбрать дейстие (показать меню, редактирование параметра, выход)
      bufTitle = dispMenu[curMenuItemNum].caption;
      switch (curMenu->typeItem){                                             // получить тип записи меню
        case 0:                                                               
        case 1:                                                               // 0..1 - меню
          loadDispMenu(curMenu->id);
          curMenuItemNum = 0;
          break;
        case 2:                                                               // 2 - параметер редактируемый
            switch (curMenu->typeParam) {
              case 0:
                currTypeEgg = curMenuItemNum;                                 // привенить выбранный тип яйца
                saveParam();                                                  // сохранить изминения
                openMenu = false;                                             // выход из режима меню
                break;
              default:
                openParam = true;                                           // показать редактироване параметра
                break;  
              }
          break;
        case 3:
              currTypeEgg = curMenuItemNum;
              break;
        case 4:
          switch(curMenu->typeParam) {
            case 5:
              t = rtc.time();
              yr = t.yr - popryear;
              ReadDate(t.date, t.mon, yr);                           // редактировать дату часов
              t.yr = popryear + yr;
              rtc.time(t);
              break;
            case 6:
              t = rtc.time();
              ReadTime(t.hr, t.min);                                        // редактировать время часов
              rtc.time(t);
              break;
            case 7:
              ReadDate(startDay, startMon, startYear);                      // редактировать дату закладки
              break;
            default:
              openMenu = false;
              break;
            
            }      
        case 9:                                                               // 9 - выход
          saveParam();                                                        // сохранить изминения
          openMenu = false;                                                   // выход из режима меню
          break;    
        }
      curMenuItemNum = 0;                                                     // установить граф. указатель на первую запись
    }
    //  ----- button down ------------    
    if (openMenu && isClick(PIN_BTN_DOWN)){
              if (openParam){
                switch (curMenu->typeParam) {
                  case 1:
                   (*(curMenu->b))--; 
                   break;
                  case 2:
                   (*(curMenu->i))--; 
                   break;
                  case 3:
                   (*(curMenu->f)) -= 0.1; 
                   break;
                  }
              }
              if (curMenuItemNum < countItems-1) ++curMenuItemNum;
        }
        // ждем отжатие кнопки
        while(isClick(PIN_BTN_UP)) {
          // NOP
          };
        while(isClick(PIN_BTN_OK)) {
          // NOP
          };
        while(isClick(PIN_BTN_DOWN)) {
          // NOP
          };
    }
  
  boolean isClick(int btn) {
    boolean result;
    if (digitalRead(btn) == HIGH) {
      delay(10);
      if (digitalRead(btn) == HIGH) result = true;
        else result = false;
        } else result = false;
      return result;
    }
  
  // чтение числа типа float с памяти
  float eepromFloatRead(int addr) {
    byte raw[4];
    for (byte i = 0; i < 4; i++) {
        raw[i] = EEPROM.read(addr+i);
      }
      float &num = (float &)raw;
      return num;
    }
  
  // запись числа типа float в память
  void eepromFloatWrite(int addr, float num) {
    byte raw[4];
    (float &)raw = num;
    for (byte i = 0; i < 4; i++) {
        EEPROM.update(addr+i, raw[i]);
      }
    }
  
  void loadParam() {
    brightLCD   = EEPROM.read(1); // яркость экрана 1..255
    currTypeEgg = EEPROM.read(2); // текущий тип яиц
    startYear   = EEPROM.read(3); // год закладки яиц
    startMon    = EEPROM.read(4); // месяц закладки яиц
    startDay    = EEPROM.read(5); // число закладки яиц
    // параметры высиживания яиц
    for (byte i = 1; i <= COUNT_TYPE_EGG*4; i++) {
      eggs[i].t = eepromFloatRead(i*10);                     // температура
      eggs[i].h = EEPROM.read(i*10+5);                       // влажность
      eggs[i].days = EEPROM.read(i*10+6);                    // количество дней высиживания
      eggs[i].rotate = EEPROM.read(i*10+7);                  // время между переворотами лотков
      }
    }
  
  void saveParam() {
    EEPROM.update(1, brightLCD);     // яркость экрана 1..255
    EEPROM.update(2, currTypeEgg);   // текущий тип яиц
    EEPROM.update(3, startYear);     // год закладки яиц
    EEPROM.update(4, startMon);      // месяц закладки яиц
    EEPROM.update(5, startDay);      // число закладки яиц

    // параметры высиживания яиц
    for (byte i = 1; i <= COUNT_TYPE_EGG*4; i++) {
      eepromFloatWrite(i*10, eggs[i].t);               // температура
      EEPROM.update(i*10+5, eggs[i].h);                 // влажность
      EEPROM.update(i*10+6, eggs[i].days);              // количество дней высиживания
      EEPROM.update(i*10+7, eggs[i].rotate);            // время между переворотами лотков
      }
    }

  // загрузить записи меню в меню отображение на экране
  void loadDispMenu(byte id) {
     countItems = 0;
     menuItem bufferItem;
     for (byte i = 0; i < sizeMenu; i++) {
       memcpy_P(&bufferItem, &menu[i], sizeof(menuItem));
       if (bufferItem.prevItem == id || bufferItem.id == 99) {
        dispMenu[countItems] = bufferItem;
        countItems++;
       }
     }
  }         
