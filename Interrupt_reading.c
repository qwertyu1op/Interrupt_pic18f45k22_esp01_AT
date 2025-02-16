/*=============================================================================
	File Name:	Interrupt_rx.c  
	Author:		Nihal
	Date:		15/02/2025
	Modified:	None
	© Nihal, 2025

	Description: waiting for rx buffer to fill up and then checking the state 
=============================================================================*/

/* Preprocessor ===============================================================
   Hardware Configuration Bits ==============================================*/
#pragma config FOSC		= INTIO67
#pragma config PLLCFG	= OFF
#pragma config PRICLKEN = ON
#pragma config FCMEN	= OFF
#pragma config IESO		= OFF
#pragma config PWRTEN	= OFF 
#pragma config BOREN	= ON
#pragma config BORV		= 285 
#pragma config WDTEN	= OFF
#pragma config PBADEN	= OFF
#pragma config LVP		= OFF
#pragma config MCLRE	= EXTMCLR

// Libraries ==================================================================
#include <p18f45k22.h>
#include <stdio.h>
#include <stdlib.h>

// Constants  =================================================================
#define TRUE	1	
#define FALSE	0

#define ESP_OKAY 1
#define ESP_CONNECT 2

#define RC2READ RCREG1
#define RC2FLAG PIR1bits.RC1IF
#define BUFSIZE 100	

// Global Variables  ==========================================================
//RESPONSES POSSIBLE
const char okay[]="OK";//AT,AT+RST,AT+CWMODE,AT+CWJAP,AT+CIPSTART,AT+CIPSEND,AT+CIPCLOSE
const char noap[]="No AP";//AT+CWJAP?,
const char error[]="ERROR";//ALL ERRORS

char buf[BUFSIZE];
char* ptr=buf;

void isr(void);

// Functions  =================================================================

#pragma code interruptVector=0x0008
void interruptVector()
{
    _asm //reduce to assembly language
        GOTO isr //go find ISR function and execute it
    _endasm //return to C code
}
#pragma code

/*>>> setOscTo16Mhz: ===========================================================
Author:      Nihal Brarath
Date:        24/05/2024
Modified:    None
Desc:        Sets the internal oscillator to 16 Mhz, wait till its stable
Input:       None 
Returns:     None 
 ============================================================================*/
void setOscTo16MHZ()
{
	OSCCON=0x72;
	while(!OSCCONbits.HFIOFS);

} // eo setOscTo16Mhz::

/*>>> configIOPorts: ===========================================================
Author:      Nihal Brarath
Date:        24/05/2024
Modified:    None
Desc:        Configs the port to Digital Input, which is the safest config for pin
Input:       None 
Returns:     None 
 ============================================================================*/
void configIOPorts(void)
{
    ANSELA = 0x07;//set a0,a1,a2 as analog;a4,a5,a6,a7 as digital
    LATA = SAFELOW; //LATCH A PORT TO GND
    TRISA = 0xFF;    //set port A all to inputs

    ANSELB = SAFELOW;//set port B as digitalp
    LATB = SAFELOW;    //set port B all to GND
    TRISB = 0xF0; //set lower nibble to O/P

    ANSELC = SAFELOW;//set LOWER NIBBLE TO
    LATC = SAFELOW;    //set port C all to GND
    TRISC = 0xF0; //set lower nibble to O/P

    ANSELD = SAFELOW;//set port D as digitalp
    LATD = SAFELOW;    //set port D all to GND
    TRISD = 0xFF; //set port D all to input
    
    
} // eo configIOPorts::


/*>>> configAsyncUART: ===========================================================
Author:      Nihal Brarath
Date:        24/05/2024
Modified:    None
Desc:        Configs the USART on pin 25,26
Input:       None 
Returns:     None 
 ============================================================================*/
void configAsyncUART()
{
	RCONbits.IPEN=TRUE;//Pain point probably cause we are setting this to false for intConfig
	INTCONbits.GIE=TRUE;
	PIE1bits.RC1IE=TRUE;
	
	//STEP1 SET BAUDRATE 114K
	SPBRG1H:SPBRGH1=34;
	TXSTA1bits.BRGH= TRUE;
	BAUDCON1bits.BRG16= TRUE;
	
	//STEP2 MAKE TRIS 1
	TRISCbits.RC6=TRUE;
	TRISCbits.RC7=TRUE;

	//STEP3 ENABLE ASYNC AND ENABLES PORT PINS
	TXSTA1bits.SYNC=FALSE;
	RCSTA1bits.SPEN=TRUE;

	//TX ENABLE
	TXSTA1bits.TXEN=TRUE;
	
	//RX ENABLE
	RCSTA1bits.CREN=TRUE;
	
	//NOW WHEN WE SEND A BYTE TO TXREG1 THIS WILL TRANSMIT set ansel maybe?
} // eo configAsyncUART::

/*>>> intConfig: ===========================================================
Author:      Nihal Brarath
Date:        02/08/2024
Modified:    None
Desc:        Initialize the interrupts
Input:       None 
Returns:     None 
 ============================================================================*/
void intConfig(void)
{
    INTCON=0xC0;
    RCONbits.IPEN= FALSE;
    PIE1bits.RC1IE=TRUE;
    
} // eo intConfig::

/*>>> isr: ===========================================================
Author:      Nihal Brarath
Date:        14/06/2024
Modified:    None
Desc:        Function for timer0 interrupt trigger event and receivers interupt trigger event
Input:       None 
Returns:     None 
 ============================================================================*/
void isr(void)
{
   	while(RC2FLAG)
	   {
	       *ptr=RC2READ;
	       if(*ptr=='\r')
	       {
	           *(ptr+1)=0;
	           printf("\n\r%s",buf);
	           ptr=buf;
	       }
	       else
	       {
	           	LATC=0x0F;
				ptr++;
	       }
	   }    
    
  INTCON|=0xC0;  
    
}

/*>>> sendUART: ===========================================================
Author:      Nihal Brarath
Date:        24/05/2024
Modified:    None
Desc:        SEND CHAR ONE BY ONE
Input:       None 
Returns:     None 
 ============================================================================*/
void sendUART(char* data)
{
	while(*data)
	{
		//while(!PIR1bits.TX1IF); may not be as accurate as we think
		while(!TXSTAbits.TRMT);
		TXREG1=*data++;
		
	}
//	while(!PIR1bits.RC1IF); to check if i am getting any response
//	PIR1bits.RC1IF=FALSE;
} // eo sendUART::
  
/*>>> sysConfig: ===========================================================
Author:      Nihal Brarath
Date:        02/08/2024
Modified:    None
Desc:        Initialize the interrupts
Input:       None 
Returns:     None 
 ============================================================================*/
void sysConfig(void)
{
    intConfig();
	configAsyncUART();
    setOscTo16MHZ();
	configIOPorts();
} // eo sysConfig::      


/*=== MAIN: FUNCTION ==========================================================
 ============================================================================*/
void main( void )
{
	sysConfig();
	sendUART("AT+GMR");
	//because of the isr i should get something in the buf
	
} // eo main::
