#include "timers.h"


//*****************************************************************************
// Verifies that the base address is a valid GPIO base address
//*****************************************************************************
static bool verify_base_addr(uint32_t base_addr)
{
   switch( base_addr )
   {
     case TIMER0_BASE:
     case TIMER1_BASE:
     case TIMER2_BASE:
     case TIMER3_BASE:
     case TIMER4_BASE:
     case TIMER5_BASE:
     {
       return true;
     }
     default:
     {
       return false;
     }
   }
}

//*****************************************************************************
// Returns the RCGC and PR masks for a given TIMER base address
//*****************************************************************************
static bool get_clock_masks(uint32_t base_addr, uint32_t *timer_rcgc_mask, uint32_t *timer_pr_mask)
{
  // Set the timer_rcgc_mask and timer_pr_mask using the appropriate
  // #defines in ../include/sysctrl.h
  switch(base_addr)
  {
    case TIMER0_BASE:
    {
      *timer_rcgc_mask = SYSCTL_RCGCTIMER_R0;
      *timer_pr_mask = SYSCTL_PRTIMER_R0;
      break;
    }
    case TIMER1_BASE:
    {
      *timer_rcgc_mask = SYSCTL_RCGCTIMER_R1;
      *timer_pr_mask = SYSCTL_PRTIMER_R1;
      break;
    }
    case TIMER2_BASE:
    {
      *timer_rcgc_mask = SYSCTL_RCGCTIMER_R2;
      *timer_pr_mask = SYSCTL_PRTIMER_R2;
      break;
    }
    case TIMER3_BASE:
    {
      *timer_rcgc_mask = SYSCTL_RCGCTIMER_R3;
      *timer_pr_mask = SYSCTL_PRTIMER_R3;
      break;
    }
    case TIMER4_BASE:
    {
      *timer_rcgc_mask = SYSCTL_RCGCTIMER_R4;
      *timer_pr_mask = SYSCTL_PRTIMER_R4;
      break;
    }
    case TIMER5_BASE:
    {
      *timer_rcgc_mask = SYSCTL_RCGCTIMER_R5;
      *timer_pr_mask = SYSCTL_PRTIMER_R5;
      break;
    }
    default:
    {
      return false;
    }
  }
  return true;
}


//*****************************************************************************
// Sets timer to run a number of ticks before generating an interrupt.
//
//The function returns true if the base_addr is a valid general purpose timer
//*****************************************************************************
bool gp_timer_wait(uint32_t base_addr, uint32_t ticks)
{
  TIMER0_Type *gp_timer;
  
  // Verify the base address.
  if ( ! verify_base_addr(base_addr) )
  {
    return false;
  }

  // Type cast the base address to a TIMER0_Type struct
  gp_timer = (TIMER0_Type *)base_addr;

  // Stop the timers
	gp_timer->CTL &= ~TIMER_CTL_TAEN;
	gp_timer->CTL &= ~TIMER_CTL_TBEN;
  
  // Set the Interval Load Register
	gp_timer->TAILR = ticks;

  // Clear any timeout interrupts before we wait
	gp_timer->ICR |= TIMER_ICR_TATOCINT;
  
  // Enable the Timer
	gp_timer->CTL |= TIMER_CTL_TAEN;
    
  return true;
}


//*****************************************************************************
// Configure a general purpose timer to be a 32-bit timer.  
//
// Paramters
//  base_address          The base address of a general purpose timer
//
//  mode                  bit mask for Periodic, One-Shot, or Capture
//
//  count_up              When true, the timer counts up.  When false, it counts
//                        down
//
//  enable_interrupts     When set to true, the timer generates and interrupt
//                        when the timer expires.  When set to false, the timer
//                        does not generate interrupts.
//
//The function returns true if the base_addr is a valid general purpose timer
//*****************************************************************************
bool gp_timer_config_32(uint32_t base_addr, uint32_t mode, bool count_up, bool enable_interrupts)
{
  uint32_t timer_rcgc_mask;
  uint32_t timer_pr_mask;
  TIMER0_Type *gp_timer;
	IRQn_Type IRQtype;
  
  // Verify the base address.
  if ( ! verify_base_addr(base_addr) )
  {
    return false;
  }
  
  // get the correct RCGC and PR masks for the base address
  get_clock_masks(base_addr, &timer_rcgc_mask, &timer_pr_mask);
  
  // Turn on the clock for the timer
  SYSCTL->RCGCTIMER |= timer_rcgc_mask;

  // Wait for the timer to turn on
  while( (SYSCTL->PRTIMER & timer_pr_mask) == 0) {};

  // Type cast the base address to a TIMER0_Type struct
	gp_timer = (TIMER0_Type *)base_addr;
    
  // Stop the timers
	gp_timer->CTL &= ~TIMER_CTL_TAEN;
	gp_timer->CTL &= ~TIMER_CTL_TBEN;

  // Set the timer to be a 32-bit timer
	gp_timer->CFG |= TIMER_CFG_32_BIT_TIMER;
		
  // Clear the timer mode 
	gp_timer->TAMR &= ~TIMER_TAMR_TAMR_M;
  // Set the mode
	gp_timer->TAMR |= mode;

  // Set the timer direction.  count_up: 0 for down, 1 for up.
  gp_timer->TAMR &= ~TIMER_TAMR_TACDIR;
  if( count_up )
  {
    // Set the direction bit
		gp_timer->TAMR |= TIMER_TAMR_TACDIR;
  }
  
  // Deal with timer interrupts
	if(!enable_interrupts){
		gp_timer->IMR &= ~TIMER_IMR_TATOIM;
	}else{
		if((int)gp_timer == TIMER0_BASE) IRQtype = TIMER0A_IRQn;
		else if((int)gp_timer == TIMER1_BASE) IRQtype = TIMER1A_IRQn;
		else IRQtype = TIMER2A_IRQn;
		gp_timer->IMR |= TIMER_IMR_TATOIM;
		// Set the Priority
		NVIC_SetPriority(IRQtype, 2);
 
		// Enable the Interrupt in the NVIC
		NVIC_EnableIRQ(IRQtype);
	}
    
  return true;  
}

void project_timers_init(){
	//Initialize system timer
	SysTick_Config(250000);
	
	//Initialize other timers
	gp_timer_config_32(TIMER0_BASE, TIMER_TAMR_TAMR_PERIOD, false, true);
	gp_timer_config_32(TIMER1_BASE, TIMER_TAMR_TAMR_PERIOD, false, true);
	gp_timer_config_32(TIMER2_BASE, TIMER_TAMR_TAMR_PERIOD, false, true);
	((TIMER0_Type*)TIMER0_BASE)->TAILR = 25000; // Half millisecond
	((TIMER0_Type*)TIMER0_BASE)->CTL |= TIMER_CTL_TAEN;
	//TODO prescaler

	((TIMER0_Type*)TIMER1_BASE)->TAILR = 250000000; //Five seconds
	((TIMER0_Type*)TIMER1_BASE)->CTL |= TIMER_CTL_TAEN;
	
	((TIMER0_Type*)TIMER2_BASE)->TAILR = 16000000; //Roughly 1/3 second
	((TIMER0_Type*)TIMER2_BASE)->CTL |= TIMER_CTL_TAEN;
	
	//Watchdog
	SYSCTL->RCGCWD |= SYSCTL_RCGCWD_R0;

  // Wait for the timer to turn on
  while( (SYSCTL->PRWD & SYSCTL_PRWD_R0) == 0) {};
	WATCHDOG0->LOAD = 500000000;
	WATCHDOG0->CTL |= 2; //Sets up system reset
	WATCHDOG0->CTL |= 1; //Lock and enable
}


