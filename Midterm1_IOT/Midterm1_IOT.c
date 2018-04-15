/*
 * Midterm1_IOT.c
 *
 */ 

#define F_CPU 16000000 //16MHz
#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <util/delay.h>
#define	FOSC 16000000UL
//#define BAUD 38400
//#define BAUD 115200
#define BAUD 9600

#define SETUP_UBRR (FOSC/16)/BAUD-1

uint16_t ubrr =  SETUP_UBRR;
char AT[] = "AT\r\n";
char ATRST[]="AT+RST\r\n";
char ATCIPSEND[]="AT+CIPSEND=%d\r\n";
//char ATCWJAP[] ="AT+CWJAP=\"SSID\",\"PASSWORD\"\r\n";
char CIPSTART[] =  "AT+CIPSTART=\"TCP\",\"api.thingspeak.com\",80\r\n";
char SEND_DATA[] = "GET https://api.thingspeak.com/update?api_key=THINGSPEAKSENDKEY&field1=%0.2f\r\n\r\n\r\n\r\n";
char CLOSE[]="AT+CIPCLOSE\r\n";


void UART_init(void)
{
	UBRR0H = (uint8_t)(ubrr>>8);
	UBRR0L = (uint8_t)ubrr;
	/* Double speed off */
	UCSR0A =  0 ;
	/*  Enable receiver and transmitter */
	UCSR0B=(1<<RXEN0)|(1<<TXEN0);
	/* Frame: 8 bit/ 1 stop bit / no parity*/
	UCSR0C=(1<<UCSZ00)|(1<<UCSZ01);
}

void UARTSendByte(char u8Data)
{
	/* wait while previous byte is completed */
	while(!(UCSR0A&(1<<UDRE0))){};
	/*  Transmit data */
	UDR0=u8Data;
}

void USARTSendStr(char* _str)
{
	int thesize=strlen(_str);
	for (uint8_t i=0; i<thesize;i++)
	{
		UARTSendByte(_str[i]);
	}
}

void adc_init(void)
{
	ADMUX = 0; // use ADC0
	ADMUX |=  (1 << REFS0); // // use AVcc as the reference
	/*
		ADC operates within a frequency range between 50Kz and 200Kz
		With a prescaler of 128 at CPU clock frequency of 16Mz, we will be in range:
		F_ADC=F_CPU/128 = 125
		So:
	*/
	ADCSRA |= (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // 128 pre-scale for 16Mhz
	ADCSRB = 0; // 0 for free running mode
	ADCSRA |= (1 << ADEN); // Enable the ADC
}

void sendTemperature(float temp)
{
		char send_data[128];
		char atcipsend[32];
		int16_t len;

		/* Format data to send according to temperature. Returns length of string. */
		len = sprintf(send_data, SEND_DATA, temp);
		/* Format AT+CIPSEND=%d according to data length to be sent */
		sprintf(atcipsend, ATCIPSEND, len);
		
		/* Restart ESP8266 */
		USARTSendStr(ATRST);
		_delay_ms(2000);
		/* Connect to Access Point */
		USARTSendStr(ATCWJAP);
		_delay_ms(5000);
		/* Establish TCP connection */
		USARTSendStr(CIPSTART);
		_delay_ms(2000);
		/*Establish length of data will be sent */
		USARTSendStr(atcipsend);
		_delay_ms(2000);
		/* Send the data */
		USARTSendStr(send_data);
		_delay_ms(2000);
		/* Close connection */
		USARTSendStr(CLOSE); 
}
int main(void)
{
	int16_t adc_value;
	float temp;

	adc_init();
	UART_init();

    while (1)
	{
		/* Start the ADC conversion */
		ADCSRA |= (1 << ADSC); 
		/* Wait for completion. ADSC reads 1 while still in progress */
		while (ADCSRA	&	(1<<ADSC));
		/* save ADC value to a variable */
		adc_value=ADCL;
		adc_value = (ADCH<<8) + adc_value;
		/*
		 For LM34:
		 Vout = + 10.0 mV/ºF
		 So the temperature will be:
		 temp = adc_value * (5V/adcresolution) / 10mV
		 or:
		*/
		temp = ((float)adc_value * 5 / 1024) / 0.010;
		sendTemperature(temp);
		/*
		 Delay 60 seconds (thingspeak.com restricts to 15 seconds delay between queries on the free account,
		 but it appears that sometimes it is much longer than that).
		*/
		//_delay_ms(60000); 
		_delay_ms(15000);
	}
}

