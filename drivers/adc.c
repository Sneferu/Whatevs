#include "adc.h"

/******************************************************************************
 * Initializes ADC to use Sample Sequencer #3, triggered by the processor,
 * no IRQs
 *****************************************************************************/
bool initializeADC(  uint32_t adc_base )
{
  ADC0_Type  *myADC;
  uint32_t rcgc_adc_mask;
  uint32_t pr_mask;
  

  // examine the adc_base.  Verify that it is either ADC0 or ADC1
  // Set the rcgc_adc_mask and pr_mask  
  switch (adc_base) 
  {
    case ADC0_BASE :
    {
      
      // set rcgc_adc_mask
      rcgc_adc_mask = SYSCTL_RCGCADC_R0;
      // Set pr_mask 
			pr_mask = SYSCTL_PRADC_R0;
      break;
    }
    case ADC1_BASE :
    {
       
      // set rcgc_adc_mask
      rcgc_adc_mask = SYSCTL_RCGCADC_R1;
      // Set pr_mask 
			pr_mask = SYSCTL_PRADC_R1;
      break;
    }
    
    default:
      return false;
  }
  
  // Turn on the ADC Clock
  SYSCTL->RCGCADC |= rcgc_adc_mask;
  
  // Wait for ADCx to become ready
  while( (pr_mask & SYSCTL->PRADC) != pr_mask){}
    
  // Type Cast adc_base and set it to myADC
  myADC = (ADC0_Type *)adc_base;
  
  myADC->ACTSS &= ~ADC_ACTSS_ASEN3;
  // disable sample sequencer #3 by writing a 0 to the 
  // corresponding ASENn bit in the ADCACTSS register 

  myADC->EMUX |= ADC_EMUX_EM3_PROCESSOR;
  // Set the event multiplexer to trigger conversion on a processor trigger
  // for sample sequencer #3.

  myADC->SSCTL3 = ADC_SSCTL3_IE0 | ADC_SSCTL3_END0;
  // Set IE0 and END0 in SSCTL3
  
  return true;
}

/******************************************************************************
 * Reads SSMUX3 for the given ADC.  Busy waits until completion
 *****************************************************************************/
uint32_t getADCValue( uint32_t adc_base, uint8_t channel)
{
  ADC0_Type  *myADC;
  uint32_t result;
  
  if( adc_base == 0)
  {
    return false;
  }
  
  myADC = (ADC0_Type *)adc_base;
  
  myADC->SSMUX3 = channel;          // Set the Channel
  
  myADC->ACTSS |= ADC_ACTSS_ASEN3;  // Enable SS3
  
  myADC->PSSI =   ADC_PSSI_SS3;     // Start SS3
  
  while( (myADC->RIS & ADC_RIS_INR3)  == 0)
  {
    // wait
  }
  
  result = myADC->SSFIFO3 & 0xFFF;    // Read 12-bit data
  
  myADC->ISC  = ADC_ISC_IN3;          // Ack the conversion
  
  return result;
}

void project_adc_init(){
	//PS2_X_ADC_CHANNEL = 1
	//PS2_Y_ADC_CHANNEL = 0
	ADC0_Type  *myADC = (ADC0_Type*)ADC0_BASE;
	ADC0_Type  *myADC1 = (ADC0_Type*)ADC1_BASE;
	
	// Turn on the ADC0 Clock
  SYSCTL->RCGCADC |= SYSCTL_RCGCADC_R0;
	// Wait for ADC to become ready
	while( (SYSCTL_PRADC_R0 & SYSCTL->PRADC) != SYSCTL_PRADC_R0);
	
	// Turn on the ADC1 Clock
  SYSCTL->RCGCADC |= SYSCTL_RCGCADC_R1;
	// Wait for ADC to become ready
	while( (SYSCTL_PRADC_R1 & SYSCTL->PRADC) != SYSCTL_PRADC_R1);
	
	// disable sample sequencer #0 by writing a 0 to the 
	// corresponding ASENn bit in the ADCACTSS register
	myADC->ACTSS &= ~ADC_ACTSS_ASEN0;
	myADC1->ACTSS &= ~ADC_ACTSS_ASEN3;
	
	// Set the event multiplexer to constantly sample for SS0
	myADC->EMUX |= ADC_EMUX_EM0_ALWAYS;
	
	//Set the other to processor
	myADC1->EMUX |= ADC_EMUX_EM3_PROCESSOR;
	
	//Set the SSMUX3 channel to the AIN2
	myADC1->SSMUX3 = 2; 
	
	//Set the SSMUX0 channels to the yyxxx
	myADC->SSMUX0 |= (0<< ADC_SSMUX0_MUX0_S ) & ADC_SSMUX0_MUX0_M;
	myADC->SSMUX0 |= (0<< ADC_SSMUX0_MUX1_S ) & ADC_SSMUX0_MUX1_M;
	myADC->SSMUX0 |= (1<< ADC_SSMUX0_MUX2_S ) & ADC_SSMUX0_MUX2_M;
	myADC->SSMUX0 |= (1<< ADC_SSMUX0_MUX3_S ) & ADC_SSMUX0_MUX3_M;
	myADC->SSMUX0 |= (1<< ADC_SSMUX0_MUX4_S ) & ADC_SSMUX0_MUX4_M;
	
	//Sets S4 as the end sample and enables interrupts
	myADC->SSCTL0 |= ADC_SSCTL0_END4;
	//myADC->SSCTL0 |= (ADC_SSCTL0_IE0 | ADC_SSCTL0_IE1 | ADC_SSCTL0_IE2 | ADC_SSCTL0_IE3 | ADC_SSCTL0_IE4);
	myADC->IM |= ADC_IM_DCONSS0;
	myADC1->IM |= ADC_IM_DCONSS3;
	
	//Routes ADC output to the DC
	myADC->SSOP0 |= (ADC_SSOP0_S0DCOP | ADC_SSOP0_S1DCOP | ADC_SSOP0_S2DCOP | ADC_SSOP0_S3DCOP | ADC_SSOP0_S4DCOP);
	myADC->SSDC0 |= ADC_SSDC0_S0DCSEL_M & (0<<ADC_SSDC0_S0DCSEL_S);
	myADC->SSDC0 |= ADC_SSDC0_S1DCSEL_M & (1<<ADC_SSDC0_S1DCSEL_S);
	myADC->SSDC0 |= ADC_SSDC0_S2DCSEL_M & (2<<ADC_SSDC0_S2DCSEL_S);
	myADC->SSDC0 |= ADC_SSDC0_S3DCSEL_M & (3<<ADC_SSDC0_S3DCSEL_S);
	myADC->SSDC0 |= ADC_SSDC0_S4DCSEL_M & (4<<ADC_SSDC0_S4DCSEL_S);
	
	//Sets S0 to be low band, interrupts enabled, sample once.
	myADC->DCCMP0 |= ADC_DCCMP0_COMP0_M & (0x400 << ADC_DCCMP0_COMP0_S);
	myADC->DCCMP0 |= ADC_DCCMP0_COMP1_M & (0xC00 << ADC_DCCMP0_COMP1_S);
	myADC->DCCTL0 |= (ADC_DCCTL0_CIC_LOW | ADC_DCCTL0_CIE | ADC_DCCTL0_CIM_ONCE);
	
	//Sets S1 to be low band, interrupts enabled, sample once.
	myADC->DCCMP1 |= ADC_DCCMP1_COMP0_M & (0x400 << ADC_DCCMP1_COMP0_S);
	myADC->DCCMP1 |= ADC_DCCMP1_COMP1_M & (0xC00 << ADC_DCCMP1_COMP1_S);
	myADC->DCCTL1 |= (ADC_DCCTL1_CIC_LOW | ADC_DCCTL1_CIE | ADC_DCCTL1_CIM_ONCE);
	
	//Sets S2 to be high band, interrupts enabled, sample once.
	myADC->DCCMP2 |= ADC_DCCMP2_COMP0_M & (0x400 << ADC_DCCMP2_COMP0_S);
	myADC->DCCMP2 |= ADC_DCCMP2_COMP1_M & (0xC00 << ADC_DCCMP2_COMP1_S);
	myADC->DCCTL2 |= (ADC_DCCTL2_CIC_HIGH | ADC_DCCTL2_CIE | ADC_DCCTL2_CIM_ONCE);
	
	//Sets S3 to be low band, interrupts enabled, sample once.
	myADC->DCCMP3 |= ADC_DCCMP3_COMP0_M & (0x400 << ADC_DCCMP3_COMP0_S);
	myADC->DCCMP3 |= ADC_DCCMP3_COMP1_M & (0xC00 << ADC_DCCMP3_COMP1_S);
	myADC->DCCTL3 |= (ADC_DCCTL3_CIC_LOW | ADC_DCCTL3_CIE | ADC_DCCTL3_CIM_ONCE);
	
	//Sets S4 to be high band, interrupts enabled, sample once.
	myADC->DCCMP4 |= ADC_DCCMP4_COMP0_M & (0x400 << ADC_DCCMP4_COMP0_S);
	myADC->DCCMP4 |= ADC_DCCMP4_COMP1_M & (0xC00 << ADC_DCCMP4_COMP1_S);
	myADC->DCCTL4 |= (ADC_DCCTL4_CIC_HIGH | ADC_DCCTL4_CIE | ADC_DCCTL4_CIM_ONCE);
	
	
	
	//Activate NVIC ADC0SS0 interrupts
	NVIC_SetPriority(ADC0SS0_IRQn, 1);
	NVIC_SetPriority(ADC1SS3_IRQn, 1);
 
  // Enable the Interrupt in the NVIC
  NVIC_EnableIRQ(ADC0SS0_IRQn);
	NVIC_SetPriority(ADC0SS3_IRQn, 1);
	
	//Reenable SS0
	myADC->ACTSS |= ADC_ACTSS_ASEN0;
}


