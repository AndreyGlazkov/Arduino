// Подключаем библиотеки:
#include <Wire.h>                                          // подключаем библиотеку для работы с шиной I2C
#include <iarduino_AM2320.h>                               // подключаем библиотеку для работы с датчиком температуры и влажности

// Объявляем переменные и константы:
iarduino_AM2320 sensor;                                    // объявляем объект sensor для работы с библиотекой iarduino_AM2320

void setup(){
  sensor.begin();                                           // инициируем сенсор
  Serial.begin(9600);
}

void loop(){
  switch(sensor.read()){    // читаем показания датчика
    case AM2320_OK:         Serial.println((String) "CEHCOP AM2320:  T=" + sensor.tem + "*C, PH=" + sensor.hum + "%");  break;
    case AM2320_ERROR_LEN:  Serial.println("OTIIPABKA HEBO3M.");     break; // объем передаваемых данных превышает буфер I2C
    case AM2320_ERROR_ADDR: Serial.println("HET CEHCOPA");           break; // получен NACK при передаче адреса датчика
    case AM2320_ERROR_DATA: Serial.println("OTIIPABKA HEBO3M.");     break; // получен NACK при передаче данных датчику
    case AM2320_ERROR_SEND: Serial.println("OTIIPABKA HEBO3M.");     break; // ошибка при передаче данных
    case AM2320_ERROR_READ: Serial.println("HET OTBETA OT CEHCOPA"); break; // получен пустой ответ датчика
    case AM2320_ERROR_ANS:  Serial.println("OTBET HEKOPPEKTEH");     break; // ответ датчика не соответствует запросу
    case AM2320_ERROR_LINE: Serial.println("HEPABEHCTBO CRC");       break; // помехи в линии связи (не совпадает CRC)
  }
  delay(1000);
}
