#include "iarduino_AM2320.h"
#include "Wire.h"

/*	НАЗНАЧЕНИЕ РЕГИСТРОВ ДАТЧИКА AM2320
	0x00	High humidity
	0x01	Low humidity
	0x02	High temperature
	0x03	Low temperature
	0x04	Retention
	0x05	Retention
	0x06	Retention
	0x07	Retention
	0x08	Model High
	0x09	Model Low
	0x0A	The version number
	0x0B	Device ID (24-31) Bit
	0x0C	Device ID (24-31) Bit
	0x0D	Device ID (24-31) Bit
	0x0E	Device ID (24-31) Bit
	0x0F	Status Register
	0x10	Users register a high
	0x11	Users register a low
	0x12	Users register 2 high
	0x13	Users register 2 low
	0x14	Retention
	0x15	Retention
	0x16	Retention
	0x17	Retention
	0x18	Retention
	0x19	Retention
	0x1A	Retention
	0x1B	Retention
	0x1C	Retention
	0x1D	Retention
	0x1E	Retention
	0x1F	Retention
*/

void	iarduino_AM2320::begin(){
			Wire.begin();
}

uint8_t	iarduino_AM2320::read(){
			uint8_t data[8], result=10, len=0;
			uint16_t i;
//			Не обращаемся к датчику чаще чем 1 раз в 0,5 сек.
			if( TIM_request>millis()||(TIM_request+500)<millis()){	//	если зафиксировано переполнение millis() или прошло больше 0.5 сек после последнего обращения
				TIM_request=millis(); result=0;						//	разрешаем обратиться к датчику
			}
//			Будим датчик
			if(result==0){
				Wire.beginTransmission(VAR_address);				//	инициируем передачу данных по шине I2C к датчику
				result=Wire.endTransmission();						//	выполняем инициированную передачу без данных (бит RW=0 => запись)
				if(result==2){delay(2); result=0;}					//	если датчик вернул NACK, то ждём 2мс пока он проснётся
			}
//			Отправляем запрос к датчику
			if(result==0){
				Wire.beginTransmission(VAR_address);				//	инициируем передачу данных по шине I2C к датчику
				Wire.write(0x03);									//	указываем, код функции = 0x03 - чтение данных
				Wire.write(0x00);									//	указываем, адрес первого читаемого регистра = 0x00
				Wire.write(0x04);									//	указываем, количество читаемых регистров = 0x04
				result=Wire.endTransmission();						//	выполняем инициированную передачу данных (бит RW=0 => запись)
			}
//			Получаем ответ от датчика
			if(result==0){
				if(Wire.requestFrom(VAR_address,0x08)){				//	запрашиваем 8 байт данных от датчика, функция Wire.requestFrom() возвращает количество принятых байт
					while(Wire.available()){						//	если есть данные для чтения из буфера I2C
						if(len<8){									//	если прочитано менее 8 байт
							data[len]=Wire.read();					//	читаем очередной принятый байт
						} len++;
					}
				}else{result=5;}									//	принято 0 байт
			}
//			Проверяем корректность ответа
			if(result==0){
				if(len!=8){result=6;}								//	принят ответ не соответствующий запросу ( количество полученных байт отличается от запрошенных)
				if(data[0]!=0x03){result=6;}						//	принят ответ не соответствующий запросу ( код ответа не соответствует коду запроса )
				if(data[1]!=0x04){result=6;}						//	принят ответ не соответствующий запросу ( количество прочитанных регистров не соответствует количество запрошенных)
			}
//			Проверяем CRC
			if(result==0){
				i=data[7]<<8; i+=data[6];							//	преобразуем CRC датчика (он записан в последних 2 байтах ответа, при этом старший байт CRC является последним)
				if(i!=createCRC16(data,6)){result=7;}				//	сравниваем  CRC датчика с вычисленным по 6 байтам принятого ответа
			}
//			Преобразуем полученные данный в значения температуры и влажности
			if(result==0){
				i =data[2];      i<<=8; hum=i+data[3]; hum/=10;		//	влажность   = data[2]data[3]
				i =data[4]&0x7F; i<<=8; tem=i+data[5]; tem/=10;		//	температура = data[4]data[5], где старший бит указывает на знак (0-плюс, 1-минус)
				if(data[4]&0x80){tem*=-1;}							//	добавляем знак к температуре
			}
			if(result==10){result=0;}								//	запрет на обращение к датчику, не является ошибкой
			return result;
}

uint16_t iarduino_AM2320::createCRC16(uint8_t *i, uint8_t j){
			uint16_t c=0xFFFF;										//	предустанавливаем CRC в значение 0xFFFF
			for(uint8_t a=0; a<j; a++){								//	проходим по элементам массива
				c^=i[a];											//	выполняем операцию XOR между CRC и очередным байтом массива i
				for(uint8_t b=0; b<8; b++){							//	проходим по битам байта
					if(c & 0x01)	{c>>=1; c^=0xA001;	}			//	сдвигаем значение crc на 1 байт вправо, если младший (сдвинутый) байт был равен 1, то
					else			{c>>=1;				}			//	выполняем операцию XOR между CRC и полиномом 0xA001
				}
			}														//	по хорошему, в конце, надо выполнить операцию XOR между результатом CRC и значением 0xFFFF, но датчик этого не делает
			return c;
}