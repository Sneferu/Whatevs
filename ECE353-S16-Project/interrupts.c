#include "driver_defines.h"
#include "TM4C123.h"
#include "pc_buffer.h"
#include "uart_irqs.h"
#include "pc_buffer.h"
#include "validate.h"

volatile bool BTN_UP = false;
volatile bool BTN_DOWN = false;
volatile bool BTN_LEFT = false;
volatile bool BTN_RIGHT = false;

volatile bool PS2_UP = false;
volatile bool PS2_DOWN = false;
volatile bool PS2_RIGHT = false;
volatile bool PS2_LEFT = false;

volatile bool BTN_PS2	= false;

volatile bool ALERT_MOVE = false;
volatile bool ALERT_5_MS = false;
volatile bool ALERT_5_S = false;

volatile int PACKETS_R = 0;
volatile int PACKETS_S = 0;

//Systick Handler
void SysTick_Handler(){
	static int BTNR_time;
	static int BTNL_time;
	static int BTNU_time;
	static int BTND_time;
	static int BTNP_time;
	GPIOA_Type  *gpioPort = (GPIOA_Type*) GPIOF_BASE; //Button base
	if(~(gpioPort->DATA) & PIN_1){
		BTNU_time++;
		if(BTNU_time == 7) BTN_UP = true;
	}else{
		BTNU_time = 0;
	}
	if(~(gpioPort->DATA) & PIN_2){
		BTNR_time++;
		if(BTNR_time == 7) BTN_RIGHT = true;
	}else{
		BTNR_time = 0;
	}
	if(~(gpioPort->DATA) & PIN_3){
		BTNL_time++;
		if(BTNL_time == 7){ BTN_LEFT = true;}
	}else{
		BTNL_time = 0;
	}
	if(~(gpioPort->DATA) & PIN_4){
		BTND_time++;
		if(BTND_time == 7) BTN_DOWN = true;
	}else{
		BTND_time = 0;
	}
	gpioPort = (GPIOA_Type*) GPIOE_BASE;
	if(~(gpioPort->DATA) & PIN_0){
		BTNP_time++;
		if(BTNP_time == 7) BTN_PS2 = true;
	}else{
		BTNP_time = 0;
	}
	ALERT_5_MS = true;
}

//Timer0A handler
void TIMER0A_Handler(){
	//Clear interrupt
	((TIMER0_Type*)TIMER0_BASE)->ICR |= TIMER_ICR_TATOCINT;

	//Acknowledge previous conversions
	((ADC0_Type*)(ADC0_BASE))->ISC  = ADC_ISC_IN0;
	((ADC0_Type*)(ADC1_BASE))->ISC  = ADC_ISC_IN3;
	//Start ADCs
	((ADC0_Type*)(ADC0_BASE))->PSSI =  ADC_PSSI_SS0;
	((ADC0_Type*)(ADC1_BASE))->PSSI =  ADC_PSSI_SS3;
}

//Timer 1 handler
void TIMER1A_Handler(){
	ALERT_5_S = true;
	//Clear interrupt
	((TIMER0_Type*)TIMER1_BASE)->ICR |= TIMER_ICR_TATOCINT;
	//TODO
}

//Timer0A handler
void TIMER2A_Handler(){
	ALERT_MOVE = true;
	//Clear interrupt
	((TIMER0_Type*)TIMER2_BASE)->ICR |= TIMER_ICR_TATOCINT;
}

//SS0 handler
void ADC0SS0_Handler(){
	ADC0_Type* adc = (ADC0_Type*)ADC0_BASE;
	if(adc->DCISC & ADC_DCISC_DCINT1){
		PS2_DOWN = true;
		adc->DCISC |= ADC_DCISC_DCINT0 | ADC_DCISC_DCINT1 | ADC_DCISC_DCINT2 | ADC_DCISC_DCINT3 | ADC_DCISC_DCINT4 | ADC_DCISC_DCINT5;
	}
	else if(adc->DCISC & ADC_DCISC_DCINT2){
		PS2_UP = true;
		adc->DCISC |= ADC_DCISC_DCINT0 | ADC_DCISC_DCINT1 | ADC_DCISC_DCINT2 | ADC_DCISC_DCINT3 | ADC_DCISC_DCINT4 | ADC_DCISC_DCINT5;
	}
	else if(adc->DCISC & ADC_DCISC_DCINT3){
		PS2_RIGHT = true;
		ALERT_MOVE = true;
		adc->DCISC |= ADC_DCISC_DCINT0 | ADC_DCISC_DCINT1 | ADC_DCISC_DCINT2 | ADC_DCISC_DCINT3 | ADC_DCISC_DCINT4 | ADC_DCISC_DCINT5;
	}
	else if(adc->DCISC & ADC_DCISC_DCINT4){
		PS2_LEFT = true;
		adc->DCISC |= ADC_DCISC_DCINT0 | ADC_DCISC_DCINT1 | ADC_DCISC_DCINT2 | ADC_DCISC_DCINT3 | ADC_DCISC_DCINT4 | ADC_DCISC_DCINT5;
	}
}
	
//SS1 Handler
void ADC1SS3_Handler(){
	//TODO
	((ADC0_Type*)ADC1_BASE)->DCISC |= ADC_DCISC_DCINT0;

}


//*****************************************************************************
// Tx Portion of the UART ISR Handler
//*****************************************************************************
__INLINE static void UART0_Tx_Flow(PC_Buffer *tx_buffer)
{
 char c;
 
 // Check to see if we have any data in the circular queue
 if(!pc_buffer_empty(tx_buffer))
 {
 // Move data from the circular queue to the hardware FIFO
 // until the hardware FIFO is full or the circular buffer
 // is empty.
 /*ADD CODE*/
		while((!(UART0->FR & UART_FR_TXFF)) && (!pc_buffer_empty(tx_buffer))){
			pc_buffer_remove(tx_buffer, &c);
			UART0->DR = c;
		}
 }
 else
 {
     // Any data in the hardware FIFO will continue to be transmitted
     // but the TX empty interrupt will not be enabled since there
     // is no data in the circular buffer.
 
     // Disable the TX interrupts.
     UART0->IM &= ~UART_IM_TXIM;
  }

  // Clear the TX interrupt so it can trigger again when the hardware
  // FIFO is empty
	UART0->ICR |= UART_ICR_TXIC;
}

extern PC_Buffer UART0_Rx_Buffer;
extern PC_Buffer UART0_Tx_Buffer;


//*****************************************************************************
// Rx Portion of the UART ISR Handler
//*****************************************************************************
__INLINE static void UART0_Rx_Flow(PC_Buffer *rx_buffer)
{
  // Loop until all characters in the RX FIFO have been removed
	while(!(UART0->FR & UART_FR_RXFE)){

      // Inside Loop: Add the character to the circular buffer
		pc_buffer_add(rx_buffer, UART0->DR);
	}
  // Clear the RX interrupts so it can trigger again when the hardware
  // FIFO becomes full
	UART0->ICR |= UART_ICR_RXIC | UART_ICR_RTIC;

}

//*****************************************************************************
// UART0 Interrupt Service handler
//*****************************************************************************
void UART0_Handler(void)
{
    uint32_t  status;

    // Check to see if RXMIS or RTMIS is active
    status = UART0->MIS;

    if ( status & (UART_MIS_RXMIS | UART_MIS_RTMIS ) )
    {
       UART0_Rx_Flow(&UART0_Rx_Buffer);
    }
		
		if ( status & (UART_RIS_TXRIS ) )
    {
       UART0_Tx_Flow(&UART0_Tx_Buffer);
    }
		
    return;
}
