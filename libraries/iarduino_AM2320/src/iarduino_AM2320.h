//	Библиотека для работы с датчиком температуры и влажности AM2320: http://iarduino.ru/shop/Sensory-Datchiki/cifrovoy-datchik-temperatury-i-vlazhnosti-am2320.html
//  Версия: 1.0.0
//  Последнюю версию библиотеки Вы можете скачать по ссылке: http://iarduino.ru/file/265.html
//  Подробное описание функции бибилиотеки доступно по ссылке: http://wiki.iarduino.ru/page/cifrovoy-datchik-temperatury-i-vlazhnosti-i2c-trema-modul/
//  Библиотека является собственностью интернет магазина iarduino.ru и может свободно использоваться и распространяться!
//  При публикации устройств или скетчей с использованием данной библиотеки, как целиком, так и её частей,
//  в том числе и в некоммерческих целях, просим Вас опубликовать ссылку: http://iarduino.ru
//  Автор библиотеки: Панькин Павел sajaem@narod.ru
//  Если у Вас возникли технические вопросы, напишите нам: shop@iarduino.ru

#ifndef iarduino_AM2320_h
#define iarduino_AM2320_h

#if defined(ARDUINO) && (ARDUINO >= 100)
#include <Arduino.h>
#else
#include <WProgram.h>
#endif
#include <Wire.h>

#define	AM2320_OK			0	//	успешное чтение данных
#define	AM2320_ERROR_LEN	1	//	объем передаваемых данных превышает буфер I2C
#define	AM2320_ERROR_ADDR	2	//	получен NACK при передаче адреса
#define	AM2320_ERROR_DATA	3	//	получен NACK при передаче данных
#define	AM2320_ERROR_SEND	4	//	ошибка при передаче данных
#define	AM2320_ERROR_READ	5	//	получен пустой ответ датчика
#define	AM2320_ERROR_ANS	6	//	ответ датчика не соответствует запросу
#define	AM2320_ERROR_LINE	7	//	помехи в линии связи (не совпадает CRC)

class iarduino_AM2320{
	public:		
	/**	функции доступные пользователю **/
		void		begin();						//	инициализация датчика
		uint8_t		read();							//	возвращает № ошибки, см. константы выше
	/**	переменные доступные пользователю **/
		float 		hum;							//	значение влажности   в %
		float		tem;							//	значение температуры в °C
	private:
		uint16_t	createCRC16(uint8_t*,uint8_t);	//	создание CRC16 (массив данных, длина массива)
	/**	внутренние переменные **/
		int8_t		VAR_address = 0x5C;				//	адрес датчика на шине I2C
		uint32_t	TIM_request = 0xffffffff;		//	время последнего запроса к датчику
};

#endif