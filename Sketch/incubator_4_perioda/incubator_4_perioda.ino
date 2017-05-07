#include <avr/pgmspace.h>
#include <OLED_I2C.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <EEPROM.h>

// пропишем пины
#define PIN_ONE_WIRE  10
#define PIN_BTN_UP    7
#define PIN_BTN_OK    6
#define PIN_BTN_DOWN  5
#define PIN_HEAT      4

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
float term;                 // температура
byte curPeriod = 0;

// глабальные параметры
byte brightLCD;              // яркость экрана 1..255
byte currTypeEgg = 0;        // текущий тип яиц

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
  PROGMEM const char st_menu[]        = "VTY>";
  PROGMEM const char st_type_egg[]    = "nbg zbw";
  PROGMEM const char st_cur_egg[]     = "pfkj;ty zqwf";
    
  PROGMEM const char st_chick[]       = "rehbyst";
  PROGMEM const char st_duck[]        = "enbyst";
  PROGMEM const char st_goose[]       = "uecbyst";
  
  PROGMEM const char st_setting[]     = "ecnfyjdrb";
  PROGMEM const char st_briht[]       = "zhrjcnm";
  
  PROGMEM const char st_period_parm[] = "period parameters";
  PROGMEM const char st_period_1[]    = "gthbjl 1";  
  PROGMEM const char st_period_2[]    = "gthbjl 2";  
  PROGMEM const char st_period_3[]    = "gthbjl 3";  
  PROGMEM const char st_period_4[]    = "gthbjl 4";  

  PROGMEM const char st_temperature[] = "ntvgthfnehf";  
  PROGMEM const char st_humidity[]    = "dkf;yjcnm";  
  PROGMEM const char st_last_day[]    = "rjytw gthbjlf";  
  PROGMEM const char st_rotation[]    = "gthtdjhjn";  

  PROGMEM const char st_exit[]    = "ds[jl"; 

  PGM_P arrStr[] = {
      st_chick, st_duck, st_goose
    }; 

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
      { 0, 3, st_briht, 2, 1, &brightLCD},                                        //        |- яркость
      {19, 3, st_cur_egg, 0, 0, NULL},                                            //            |- заложеный тип яиц 
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
  0xE0, 0xC0, 0x00, 0x00, 0x80, 0xDE, 0x7F, 0x3F, 0x33, 0x63, 0xC1, 0xE0, 0xE0, 0xE0, 0xE0, 0xC0,
  0x03, 0x07, 0x07, 0x07, 0x07, 0x83, 0xC6, 0xCC, 0xFC, 0xFE, 0x7B, 0x01, 0x00, 0x00, 0x03, 0x07
  };
  
  PROGMEM const unsigned char heater [] = {
  0x00, 0x00, 0x00, 0x7C, 0xFF, 0x81, 0x3C, 0xFF, 0x81, 0x3C, 0xFF, 0xC3, 0x00, 0x00, 0x00, 0x00,
  0xC0, 0xC0, 0xC0, 0xD8, 0xDF, 0xC7, 0xD0, 0xDF, 0xC7, 0xD0, 0xDF, 0xC7, 0xC0, 0xC0, 0xC0, 0xC0
  };
  
  PROGMEM const unsigned char water [] = {
  0x00, 0x00, 0x00, 0x80, 0xE0, 0xF0, 0xFC, 0xFE, 0xFF, 0xFE, 0xFC, 0xF0, 0xE0, 0x80, 0x00, 0x00,
  0x00, 0x00, 0x1E, 0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 0x1E, 0x00
  };
  
  // создание обьект дисплея
  OLED oled(SDA, SCL, 8);
  extern uint8_t SmallFont[];
  extern uint8_t MediumNumbers[];
  extern uint8_t BigNumbers[];
  extern uint8_t RusFont[];

  // ds18b20
  OneWire oneWire(PIN_ONE_WIRE);
  DallasTemperature ds18b20(&oneWire);
  DeviceAddress devID;

  // создание датчка влажности
  //iarduino_AM2320 sensorh;

void setup() {
  // загрузить параметры из памяти
  loadParam();
  // инициализация экрана 
  oled.begin();
  oled.setBrightness(brightLCD); 
  // инициализаця кнопок
  pinMode(PIN_BTN_UP, INPUT);
  pinMode(PIN_BTN_OK, INPUT);
  pinMode(PIN_BTN_DOWN, INPUT);

  // нагреватель
  pinMode(PIN_HEAT, OUTPUT);

  ds18b20.begin();
  if (!ds18b20.getAddress(devID, 0)) {
      oled.setFont(SmallFont);
      oled.print(F("Error sessor term."), CENTER, 32);         
    } else {
      ds18b20.setResolution(devID, 16);
    }  
}

void loop() {
  // put your main code here, to run repeatedly:
  if (!openMenu){
      setTitle(getStrFromPROGMEM(arrStr[currTypeEgg]));
      head();
      showDate();
      showTermHum();
    }
  clickBtn();
  oled.update();
}

  // возвращает строку из флеш-памяти
  char* getStrFromPROGMEM(char* str) {
        char buf[24];
        strcpy_P(buf, str);
        return buf; 
  }

  // контроль температуры
  void head() {
    if (term < eggs[currTypeEgg*4+curPeriod].t - 0.1) {
      digitalWrite(PIN_HEAT, HIGH);
      oled.drawBitmap(110, 0, heater, 16, 16);
      oled.drawBitmap(90, 0, cooler, 16, 16);
      oled.drawBitmap(70, 0, water, 16, 16);
    }
    else if (term > eggs[currTypeEgg*4+curPeriod].t) digitalWrite(PIN_HEAT, LOW);
  }

  void setTitle(char* text){
    oled.clrScr();
    oled.setFont(RusFont);
    oled.print(text, 10, 5);
  }
  
  void showTermHum(){
    ds18b20.requestTemperatures();
    term = ds18b20.getTempC(devID);
    oled.setFont(BigNumbers);
    oled.printNumF(term, 1, CENTER, 30);
  }

  void showDate() {
    static byte hour;
    static byte minut;
    static byte second;
    hour = millis() / 1000 / 60 / 60;
    minut = millis() / 1000 / 60 % 60;
    second = millis() / 1000 % 60;
    oled.setFont(SmallFont);
    oled.print((hour < 10 ? "0" : "") + String(hour) + (minut < 10 ? ":0" : ":") + String(minut) + (second < 10 ? ":0" : ":") + String(second), 4, 17);
    }

  void clickBtn() {
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
        showParam(curMenu);
      } else if (curMenuItemNum > 0) showMenu(--curMenuItemNum);
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
          showMenu(0);
          break;
        case 2:                                                               // 2 - параметер редактируемый
            switch (curMenu->typeParam) {
              case 0:
                currTypeEgg = curMenuItemNum;                                 // привенить выбранный тип яйца
                saveParam();                                                  // сохранить изминения
                openMenu = false;                                             // выход из режима меню
                break;
              default:
                showParam(curMenu);                                           // показать редактироване параметра
                break;  
              }
          break;
        case 3:
              currTypeEgg = curMenuItemNum;
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
                showParam(curMenu);
              }
              if (curMenuItemNum < countItems-1) showMenu(++curMenuItemNum);
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
  
  void showMenu(int posCursor) {
    setTitle(getStrFromPROGMEM(bufTitle));
    for (int i = 0; i < countItems; i++) {
      oled.setFont(RusFont);
      oled.print(getStrFromPROGMEM(dispMenu[i].caption), 9, 9*i+18);
      // отобразить значение параметра
        oled.setFont(SmallFont);
        switch(dispMenu[i].typeParam) {
          case 1: oled.print(String(*dispMenu[i].b), 90, 9*i+18); break;
          case 2: oled.print(String(*dispMenu[i].i), 90, 9*i+18); break;
          case 3: oled.print(String(*dispMenu[i].f), 90, 9*i+18); break;
        }  
      if (i == posCursor) oled.print("-", 1, 9*i+18);
    }}  
  
  // редактор параметров
  void showParam(menuItem *item){
    countItems = 0;
    oled.clrScr();
    setTitle(getStrFromPROGMEM(bufTitle));
    openParam = true;
    oled.setFont(BigNumbers);
    switch (item->typeParam) {
      case 0:
      case 1:
        oled.printNumI(*(item->b), CENTER, 32);
        break;
      case 3:
        oled.printNumF(*(item->f), 1, CENTER, 32);
        break;
      default:
        break;        
      }
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
        EEPROM.write(addr+i, raw[i]);
      }
    }
  
  void loadParam() {
    brightLCD = EEPROM.read(1); // яркость экрана 1..255
    currTypeEgg = EEPROM.read(2); // текущий тип яиц
    // параметры высиживания яиц
    for (byte i = 1; i <= COUNT_TYPE_EGG*4; i++) {
      eggs[i].t = eepromFloatRead(i*10);                     // температура
      eggs[i].h = EEPROM.read(i*10+5);                       // влажность
      eggs[i].days = EEPROM.read(i*10+6);                    // количество дней высиживания
      eggs[i].rotate = EEPROM.read(i*10+7);                  // время между переворотами лотков
      }
    }
  
  void saveParam() {
    EEPROM.write(1, brightLCD);     // яркость экрана 1..255
    EEPROM.write(2, currTypeEgg);   // текущий тип яиц
    // параметры высиживания яиц
    for (byte i = 1; i <= COUNT_TYPE_EGG*4; i++) {
      eepromFloatWrite(i*10, eggs[i].t);               // температура
      EEPROM.write(i*10+5, eggs[i].h);                 // влажность
      EEPROM.write(i*10+6, eggs[i].days);              // количество дней высиживания
      EEPROM.write(i*10+7, eggs[i].rotate);            // время между переворотами лотков
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
