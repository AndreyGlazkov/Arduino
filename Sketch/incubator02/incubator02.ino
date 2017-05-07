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

// структура элемента записи меню
struct dataEgg {
  String  caption;    // вид яиц
  float   t;          // температура
  byte    h;          // влажность
  byte    days;       // количество дней высиживания
  byte    rotate;     // время между переворотами лотков
};

// структура элемента записи меню
struct menuItem {
  byte     index;      // индекс пункта меню
  byte     prevIndex;  // индекс родительского пункта меню
  String  caption;    // текст на экране
  byte    typeItem;   // тип записи (0-меню, 1-субменю, 2-параметр, 9-выход)
  byte    typeParam;  // тип parametra (0-NULL, 1-byte, 2-int, 3-float)
  union {
      byte *b;
      int *i;
      float *f;
    };
};

// переменные
int curPointX = 0;
menuItem *curMenu = NULL;   // текущее меню
menuItem *dispMenu[5];      // меню на экране
byte curMenuItemNum = 0;    // текущая строка меню
boolean openMenu = false;
boolean openParam = false;
int countItems;             // количество строк в меню

// глабальные параметры
byte brightLCD;              // яркость экрана 1..255
float term;                  // температура
byte currTypeEgg = 0;        // текущий тип яиц

// ========= параметры высиживания ==========
dataEgg eggs[COUNT_TYPE_EGG] = {     
//   назван  темр  вл  дн  перв
    {"Rehs", 37.8, 50, 21, 180},        // куры
    {"Enrb", 37.8, 60, 28, 180},        // утки
    {"Uecb", 37.8, 70, 30, 180}         // гуси
  };

// ***************** MENU *******************
//static const PROGMEM 
menuItem menu[13];

// количество элементов в массиве меню 
const int sizeMenu = sizeof(menu) / sizeof(menuItem);

const unsigned char PROGMEM cooler [] = {
0xE0, 0xC0, 0x00, 0x00, 0x80, 0xDE, 0x7F, 0x3F, 0x33, 0x63, 0xC1, 0xE0, 0xE0, 0xE0, 0xE0, 0xC0,
0x03, 0x07, 0x07, 0x07, 0x07, 0x83, 0xC6, 0xCC, 0xFC, 0xFE, 0x7B, 0x01, 0x00, 0x00, 0x03, 0x07
};

const unsigned char PROGMEM heater [] = {
0x00, 0x00, 0x00, 0x7C, 0xFF, 0x81, 0x3C, 0xFF, 0x81, 0x3C, 0xFF, 0xC3, 0x00, 0x00, 0x00, 0x00,
0xC0, 0xC0, 0xC0, 0xD8, 0xDF, 0xC7, 0xD0, 0xDF, 0xC7, 0xD0, 0xDF, 0xC7, 0xC0, 0xC0, 0xC0, 0xC0
};

const unsigned char PROGMEM water [] = {
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
      oled.print("No find device.", CENTER, 32);         
    } else {
      ds18b20.setResolution(devID, 16);
    }  
}

void loop() {
  // put your main code here, to run repeatedly:
  if (!openMenu){
      setTitle(eggs[currTypeEgg].caption);
      showDate();
      showTerm();
      head();
    }
  clickBtn();
  oled.update();
}

void head() {
    if (term < eggs[currTypeEgg].t - 0.1) {
      digitalWrite(PIN_HEAT, HIGH);
      oled.drawBitmap(110, 0, heater, 16, 16);
      oled.drawBitmap(90, 0, cooler, 16, 16);
      oled.drawBitmap(70, 0, water, 16, 16);
    }
    else if (term > eggs[currTypeEgg].t) digitalWrite(PIN_HEAT, LOW);
  }

void setTitle(String Text){
  oled.clrScr();
  oled.setFont(RusFont);
  oled.print(Text, 10, 5);
}

void showTerm(){
  ds18b20.requestTemperatures();
  term = ds18b20.getTempC(devID);
  oled.setFont(BigNumbers);
  oled.printNumF(term, 1, CENTER, 34);
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
      initMenu();
      curMenu = &menu[0];                                      // то поставить текущим первое меню
    }
      else curMenu = dispMenu[curMenuItemNum];
      
      // если открыто редактирование параметра закрыть его и перейти в меню
      if (openParam){
          curMenu = getItemByIndex(curMenu->prevIndex);
          openParam = false;
        }

    // определить тип записи меню и выбрать дейстие (показать меню, редактирование параметра, выход)
    switch (curMenu->typeItem){                                             // получить тип записи меню
      case 0:                                                               
      case 1:                                                               // 0..1 - меню
        openMenu = true;
        selectSubMenu(curMenu);
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

void selectSubMenu(menuItem *item) {
    countItems = 0;
    int ind = item->index;
    // заполнить массив записями меню для дисплея
        for (int i = 0; i < sizeMenu; i++) {
            item++;
            if (item->prevIndex == ind) dispMenu[countItems++] = item;
         }
        dispMenu[countItems++] = getItemByIndex(9); // добавить Exit
  } 

void showMenu(int posCursor) {
  setTitle(curMenu->caption);
  oled.setFont(RusFont);
  for (int i = 0; i < countItems; i++) {
    oled.print(dispMenu[i]->caption, 9, 9*i+18);
    if (i == posCursor) oled.print("-", 1, 9*i+18);
  }}  

// редактор параметров
void showParam(menuItem *item){
  countItems = 0;
  oled.clrScr();
  setTitle(item->caption);
  oled.printNumI(item->index, 2, 18);
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

menuItem * getPrevItem(menuItem *item) {
    int ind = item->prevIndex;    
    for (int i = 0; i < sizeMenu; i++){
        if (menu[i].index == ind){
          return &menu[i];
        }
      }
    return NULL;
  } 

// получить пункт меню по его индексу
menuItem * getItemByIndex(int ind) {
    for (int i = 0; i < sizeMenu; i++){
        if (menu[i].index == ind){
          return &menu[i];
        }
      }
    return NULL;
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
  brightLCD = EEPROM.read(0); // яркость экрана 1..255
  currTypeEgg = EEPROM.read(1); // текущий тип яиц
  // параметры высиживания яиц
  for (byte i = 0; i < COUNT_TYPE_EGG; i++) {
    eggs[i].t = eepromFloatRead(i*100+10);                   // температура
    eggs[i].h = EEPROM.read(i*100+20);                       // влажность
    eggs[i].days = EEPROM.read(i*100+30);                    // количество дней высиживания
    eggs[i].rotate = EEPROM.read(i*100+40);                  // время между переворотами лотков
    }
  }

void saveParam() {
  EEPROM.write(0, brightLCD);     // яркость экрана 1..255
  EEPROM.write(1, currTypeEgg);   // текущий тип яиц
  // параметры высиживания яиц
  for (byte i = 0; i < COUNT_TYPE_EGG; i++) {
    eepromFloatWrite(i*100+10, eggs[i].t);             // температура
    EEPROM.write(i*100+20, eggs[i].h);                 // влажность
    EEPROM.write(i*100+30, eggs[i].days);              // количество дней высиживания
    EEPROM.write(i*100+40, eggs[i].rotate);            // время между переворотами лотков
    }
  }

void initMenu() {
menu[0] = {0, 0, "vty.", 0, 0, NULL};                                               // меню
menu[1] =  {1, 0, "nbg zbw", 0, 0, NULL};                                           // |-тип яиц
menu[2] =   {11, 1, eggs[0].caption, 2, 0, NULL};                                   //    |-куринные
menu[3] =   {12, 1, eggs[1].caption, 2, 0, NULL};                                   //    |-утиные
menu[4] =   {13, 1, eggs[2].caption, 2, 0, NULL};                                   //    |-гусиные
menu[5] =  {2, 0, "yfcnhjqrb", 0, 0, NULL};                                         // |-настройки
menu[6] =   {21, 2, "zhrjcnm",     2, 1, &brightLCD};                               //    |-яркость
menu[7] =   {22, 2, "gfhfvtnhs", 0, 0, NULL};                                       //    |-параметры яиц
menu[8] =    {221, 22, "ntvgthfnehf", 2, 3, (byte *)&(eggs[currTypeEgg].t)};        //        |-температура
menu[9] =    {222, 22, "dkf;yjcnm", 2, 1, &(eggs[currTypeEgg].h)};                  //        |-влажность        
menu[10]=    {223, 22, "chjr dscb;", 2, 1, &(eggs[currTypeEgg].days)};              //        |-кол дней
menu[11]=    {224, 22, "gthtdjhjn", 2, 1, &(eggs[currTypeEgg].rotate)};             //        |-переворот
menu[12]=    {9, 9, "Ds[jl", 9, 0, NULL};                                            // |-Выход
  }         
