#include <avr/pgmspace.h>
#include <OLED_I2C.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <EEPROM.h>

// структура элемента записи меню
struct menuItem {
  int     index;      // индекс пункта меню
  int     prevIndex;  // индекс родительского пункта меню
  String  caption;    // текст на экране
  byte    typeItem;   // тип записи (0-меню, 1-субменю, 2-параметр, 9-выход)
  int     *param;
};

// структура элемента записи меню
struct dataEgg {
  String  typeEgg;    // вид яиц
  float   tMin;       // минимаотная температура
  float   tMax;       // максимальная температура
  byte    days;       // количество дней высиживания
};

// пропишем пины
int pinOneWire = 10;
int pinBtnUp = 7;
int pinBtnOk = 6;
int pinBtnDown = 5;

// переменные
int curPointX = 0;
menuItem *curMenu = NULL;   // текущее меню
menuItem *dispMenu[5];      // меню на экране
int curMenuItem = 0;        // текущая строка меню
boolean openMenu = false;
boolean openParam = false;
int countItems;             // количество строк в меню

// глабальные параметры
int brightLCD;              // яркость экрана 1..255
int timeCicle;              // частота переворота лотка

dataEgg eggs[3];            // список параметров высиживания

static const PROGMEM String title = "Hi!";

// ***************** MENU *******************
static const PROGMEM menuItem menu[] = {
{0, 0, "Menu", 0, NULL},
 {1, 0, "Type of egg", 0, NULL},
  {11, 1, "Chick", 1, NULL},
    {111, 11, "tempereture min", 2, &chick[0]},
    {112, 11, "tempereture max", 2, &chick[1]},
    {113, 11, "Count of day",    2, &chick[2]},
  {12, 1, "Duck", 1, NULL},
    {121, 12, "tempereture min", 2, &duck[0]},
    {122, 12, "tempereture max", 2, &duck[1]},
    {123, 12, "Count of day",    2, &duck[2]},
  {13, 1, "Goose", 1, NULL},
    {131, 13, "tempereture min", 2, &goose[0]},
    {132, 13, "tempereture max", 2, &goose[1]},
    {133, 13, "Count of day",    2, &goose[2]},
 {2, 0, "Setting", 0, NULL},
  {21, 2, "Bright LCD", 2, &brightLCD},
  {22, 2, "Time of circle", 2, &timeCicle},
 {9, 9, "Exit", 9, NULL}
 };

// количество элементов в массиве меню 
const int sizeMenu = sizeof(menu) / sizeof(menuItem);


// создание обьект дисплея
OLED oled(SDA, SCL, 8);
extern uint8_t SmallFont[];
extern uint8_t MediumNumbers[];
extern uint8_t BigNumbers[];

// ds18b20
OneWire oneWire(pinOneWire);
DallasTemperature ds18b20(&oneWire);
DeviceAddress devID;

void setup() {
  loadParam();
  // инициализация экрана
  oled.begin();
  oled.setBrightness(brightLCD); 
  setTitle("Termometer");
  
  // инициализаця кнопок
  pinMode(pinBtnUp, INPUT);
  pinMode(pinBtnOk, INPUT);
  pinMode(pinBtnDown, INPUT);

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
      setTitle(title);
      showTerm();
      //showGrafic();
    }
  clickBtn();
  oled.update();
}

void setTitle(String Text){
  oled.clrScr();
  oled.setFont(SmallFont);
  oled.print(Text, CENTER, 2);
}

void showTerm(){
  ds18b20.requestTemperatures();
  float term = ds18b20.getTempC(devID);
  oled.setFont(BigNumbers);
  oled.printNumF(term, 1, CENTER, 32);
}

void clickBtn() {
  //  ----- button up --------------
  if (openMenu && isClick(pinBtnUp)) {
    if (openParam) {
      (*(curMenu->param))++;
      showParam(curMenu);
    }
    if (curMenuItem > 0) showMenu(--curMenuItem);
  }
  //  ----- button ok --------------
  else if (isClick(pinBtnOk)) {
        if (!openMenu) curMenu = &menu[0];                                      // то поставить текущим первое меню
          else curMenu = dispMenu[curMenuItem];
          if (openParam){
              curMenu = getItemOfIndex(curMenu->prevIndex);
              openParam = false;
            }
        switch (curMenu->typeItem){                                             // получить тип записи меню
          case 0:                                                               
          case 1:                                                               // 0..1 - меню
            openMenu = true;
            selectSubMenu(curMenu);
            showMenu(0);
            break;
          case 2:                                                               // 2 - параметер
            showParam(curMenu);
            break;
          case 9:
            saveParam();
            openMenu = false;
            break;    
          }
        curMenuItem = 0;                                  // установить граф. указатель на первую запись
  
  //  ----- button down ------------    
  } else if (openMenu && isClick(pinBtnDown)){
            if (openParam){
              (*(curMenu->param))--;
              showParam(curMenu);
            }
            if (curMenuItem < countItems-1) showMenu(++curMenuItem);
      }
      // ждем отжатие кнопки
      while(isClick(pinBtnUp)) {
        // NOP
        };
      while(isClick(pinBtnOk)) {
        // NOP
        };
      while(isClick(pinBtnDown)) {
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
        dispMenu[countItems++] = getItemOfIndex(9); 
  } 

void showMenu(int posCursor) {
  setTitle(curMenu->caption);
  oled.setFont(SmallFont);
  for (int i = 0; i < countItems; i++) {
    oled.print(dispMenu[i]->caption, 9, 9*i+18);
    if (i == posCursor) oled.print(">", 1, 9*i+18);
  }}  

void showParam(menuItem *item){
  countItems = 0;
  oled.clrScr();
  setTitle(item->caption);
  openParam = true;
  oled.setFont(BigNumbers);
  oled.printNumI(*(item->param), CENTER, 32);
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

menuItem * getItemOfIndex(int ind) {
    for (int i = 0; i < sizeMenu; i++){
        if (menu[i].index == ind){
          return &menu[i];
        }
      }
    return NULL;
  }

void loadParam() {
  brightLCD = EEPROM.read(0); // яркость экрана 1..255
  timeCicle = EEPROM.read(1); // частота переворота лотка
// куры
  chick[0] = EEPROM.read(10); 
  chick[1] = EEPROM.read(11); 
  chick[2] = EEPROM.read(12); 
// утки
  duck[0] = EEPROM.read(20); 
  duck[1] = EEPROM.read(21); 
  duck[2] = EEPROM.read(22); 
// гуси
  goose[0] = EEPROM.read(30); 
  goose[1] = EEPROM.read(31); 
  goose[2] = EEPROM.read(32); 
  }

void saveParam() {
  EEPROM.write(0, brightLCD); // яркость экрана 1..255
  EEPROM.write(1, timeCicle); // частота переворота лотка
// куры
  EEPROM.write(10, chick[0]-200); 
  EEPROM.write(11, chick[1]-200); 
  EEPROM.write(12, chick[2]-200); 
// утки
  EEPROM.write(20, duck[0]-200); 
  EEPROM.write(21, duck[1]-200); 
  EEPROM.write(22, duck[2]-200); 
// гуси
  EEPROM.write(30, goose[0]-200);
  EEPROM.write(31, goose[0]-200);
  EEPROM.write(32, goose[0]-200);
  }       
