//EXPT. 6: Interfacing Serial (I2C based) EEPROM to ARM7 Microcontroller
/*		Hardware Setup:-
    Connect a DB9 cable between PC and UART0 
		COMPORT Settings
		Baudrate:-9600
		Databits:-8
		Parity:-None
		Stopbits:1
		Setup terminal software to receive data in string format

		Clock Settings:
		FOSC	>>	12MHz (onboard)
		PLL		>>	M=5, P=2
		CCLK	>>  60MHz
		PCLK	>>  15MHz  

•	For baudrate 9600
o	U0DL (U0DLM+ U0DLL) = 15x106/16x9600 = (97)10 = (61) 16
•	Therefore U0DLM = 0x00 and U0DLL = 0x61
		
Interface Serial EEPROM by Jumper Setings J9 
	SDA0 and SCL0 is used
	
I2C Clock calculation
I2C Clock = PLCK/ SCLH+SCLL
FOR 100KHZ
SCLH+SCLL = 15MHZ/100 KHZ
SCLH+SCLL = 150
FOR 50% DUTY CYCLE
SCLH = 75
SCLL = 75
*/

#include<lpc214X.h>
#include "UART.h"

#define T_WR 100
#define ENABLE 0x040
#define START  0x020
#define STOP   0x10
#define SI     0x08
#define AA     0x04

#define DISABLE   0x040
#define START_CLR 0x020
#define STOP_CLR  0x10
#define SI_CLR    0x08
#define AA_CLR    0x04

int i,j,length,x;
unsigned int data[200];
const unsigned char data_to_write[15]={" PICT "};

void delay(unsigned int time)
{
	for(i=0;i<time;i++)
		for(j=0;j<1000;j++);
}


void i2c_init()
{
	PINSEL0 = 0X00000050;	// SELECT THE PIN for  I2C
	I2C0CONCLR  = DISABLE | START_CLR | STOP_CLR | SI_CLR | AA_CLR;
	I2C0SCLH = 75;		//set the bit frequency to 100 khz
	I2C0SCLL = 75;	
}

void blink_led()
{	
	for(i=0;i<=1;i++)
	{
		IOPIN1 = 0xFF <<16;	//port p0.16-po.23
		delay(300);
		IOPIN1 = 0x00 <<16;
		delay(300);
	}
}

void wait_for_ack(unsigned int status)
{
	while(1)				
	{
		if(I2C0CONSET & SI) 
		{
			if(I2C0STAT == status)
			{
				break;
			}
			else
			{
				blink_led();
				I2C0CONSET = STOP;
				I2C0CONCLR = 0xFF;
			}
		}		
	}
}

void i2c_eeprom_write(unsigned int dev_addr,unsigned int page_no,unsigned int page_offset,unsigned int no_bytes)
{
	IOPIN1 = 0x00 <<16;	//START OF WRITE
	delay(1000);
	I2C0CONSET = ENABLE;
	delay(1000);
/*---------------------transmit a START and address with write---------------------*/
	I2C0CONSET = START;
	wait_for_ack(0x08);
	IOPIN1 = 0x01 <<16;	//SUCCESSFUL TRANSMISSION OF START
	delay(1000);
	
	I2C0DAT = ( (dev_addr <<1)| 0XA0);	// transmit slave address with write bit
	I2C0CONCLR = SI_CLR;
	I2C0CONCLR = START_CLR;	// clear the START bit to avoid retransmit of START	

	wait_for_ack(0x18);
	IOPIN1 = 0x02 <<16;	//SUCCESSFUL TRANSMISSION OF DEVICE ADDRESS
	delay(1000);
/*-----------------------transmit page no-----------------------------*/
	I2C0DAT = page_no;			// transmit page address
	I2C0CONCLR =  SI_CLR;
	wait_for_ack(0x28);
	
	I2C0DAT = page_offset;			// transmit offset within the page
	I2C0CONCLR =  SI_CLR;
	wait_for_ack(0x28);
	IOPIN1 = 0x03 <<16;	//SUCCESSFUL TRANSMISSION OF PAGE ADDRESS AND EXACT LOCATION 
	delay(1000);
	
/*-----------------------------transmit data------------------------------*/

	while(no_bytes > 0)	
	{
		I2C0DAT = data_to_write[no_bytes];		//DATA TO WRITE IS "A" ON EVERY LOCATION OF RESPECTIVE PAGE  
		I2C0CONCLR = SI_CLR;
		no_bytes--;
		wait_for_ack(0x28);
		IOPIN1 = 0x04 <<16;	//SUCCESSFUL TRANSMISSION OF RESPECTIVE ADDRESS DATA
		delay(100);
	}
	IOPIN1 = 0x05 <<16;	//SUCCESSFUL TRANSMISSION OF ALL DATA
	delay(1000);
	
/*-------------------transmit a STOP and do acknowledge polling ---------------------------------*/
	
	I2C0CONSET = STOP ;
	delay(T_WR);
	I2C0CONCLR = DISABLE | START_CLR | STOP_CLR | SI_CLR | AA_CLR;
	IOPIN1 = 0x0F <<16;	//SUCCESSFUL WRITE
	delay(1000);
}

void i2c_eeprom_read(unsigned int dev_addr,unsigned int page_no,unsigned int page_offset,unsigned int no_bytes)
{
	IOPIN0 = 0x00 <<16;	//START OF READ
	delay(1000);
	length = no_bytes;
	
	I2C0CONSET = ENABLE ;
	delay(1000);

/*---------------------transmit a START and address---------------------*/

	I2C0CONSET = START;
	wait_for_ack(0x08);
	IOPIN1 = 0x10 <<16;	//SUCCESSFUL TRANSMISSION OF START
	delay(1000);

	I2C0CONCLR = START_CLR;	// clear the START bit to avoid retransmit of START	
	I2C0DAT = ( (dev_addr <<1)| 0XA0);
	I2C0CONCLR = SI_CLR;

	wait_for_ack(0x18);	
	IOPIN1 = 0x20 <<16;	//SUCCESSFUL TRANSMISSION OF DEVICE ADDRESS
	delay(1000);
/*-----------------------transmit page no-----------------------------*/

	I2C0DAT = page_no;
	I2C0CONCLR =  SI_CLR;
	wait_for_ack(0x28);
	
	I2C0DAT = page_offset;
	I2C0CONCLR =  SI_CLR;
	wait_for_ack(0x28);
	IOPIN1 = 0x30 <<16;	//SUCCESSFUL TRANSMISSION OF PAGE ADDRESS AND EXACT LOCATION 
	delay(1000);
	I2C0CONCLR = SI_CLR;

	I2C0CONSET =  START ;	//RESTART FOR READ
	wait_for_ack(0x10);
	I2C0CONCLR = START_CLR;	//put the address and the read bit	

	I2C0CONSET = AA;
	I2C0DAT = ( (dev_addr <<1)| 0XA1);
	I2C0CONCLR = SI_CLR;
	wait_for_ack(0x40);  
	I2C0CONCLR = SI_CLR;
	IOPIN1 = 0x40 <<16;	//SUCCESSFUL RESTART
	delay(1000);
	
/*-----------------------------receive data------------------------------*/	

	while(no_bytes>0)
	{
		data[no_bytes] = I2C0DAT;
		no_bytes--;	
		wait_for_ack(0x50);
	  I2C0CONCLR = SI_CLR;
		IOPIN1 = 0x40 <<16;	//SUCCESSFUL RECEPTION OF RESPECTIVE ADDRESS DATA
		delay(100);
	}
	IOPIN1 = 0x50 <<16;	//SUCCESSFUL RECEPTION OF ALL DATA
	delay(100);
	wait_for_ack(0x50);
	I2C0CONCLR = AA_CLR;   
	I2C0CONCLR = SI_CLR;
	wait_for_ack(0x58);
	
/*-------------------transmit a STOP---------------------------------*/
	
	I2C0CONSET =STOP ;
	I2C0CONCLR = DISABLE | START_CLR | STOP_CLR | SI_CLR | AA_CLR;
	IOPIN1 = 0xF0 <<16;	//SUCCESSFUL READ
	delay(1000);
  
	IOPIN1 = 0xFF <<16;	//SUCCESSFUL READ AND WRITE
	delay(1000);
	
}


int main(void)
{	int k=0;
	IODIR1=0XFFFFFFFF;											//making port A OUTPUT FOR SHOWING LED
	PINSEL1 = 0X00000000;										//MAKING PORT 0 AS GPIO
	i2c_init();															//INITIALIZE I2C
	i2c_eeprom_write(0x01,0x00,0x00,5);		//WRITE EEPROM
	i2c_eeprom_read(0x01,0x00,0x00,5);			//READ EEPROM
					
		while(k<200)
	{	
		show_on_UART0(data[k]);
		k++;
	}
return 0;
}

