#include <AM2320.h>
#include "U8glib.h"
#include <avr/pgmspace.h>
#include <EEPROM.h>
#include "rus6x10.h"
#include "MyDigitsHelveticaBold24.h"
#include <OneWire.h>
#include <DallasTemperature.h>

  // создание обьект дисплея
  U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NONE|U8G_I2C_OPT_DEV_0);

  // датчмк влажности
  AM2320 sensorh;
  
void setup() {
  // put your setup code here, to run once:
  u8g.setColorIndex(1);
}

void loop() {
  // put your main code here, to run repeatedly:
    u8g.firstPage();  
  do {
    draw();
  } while( u8g.nextPage() );
  delay(1000);
}

void draw(void) {
  // graphic commands to redraw the complete screen should be placed here  
  u8g.setFont(rus6x10);
  u8g.drawStr(5, 12, "ПРИВЕТ МИР");
  sensorh.Read();
  u8g.drawStr(65, 48, "0");
  u8g.drawStr(120, 48, "%");
  u8g.setFont(MyDigitsHelveticaBold24);
  u8g.setPrintPos(1, 64);
  u8g.println(sensorh.t, 1);
  u8g.setPrintPos(80, 64);
  u8g.println(sensorh.h, 0);
}
