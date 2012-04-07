//******************************************************************************
// F2011 demo code for RC charge/discharge demo
//
// Version 0-00: 10-21-2006
//
// Single key detection 
// This version determines if one of 4 keys has been pressed. Base capacitance
// is tracked for changes and slider max endpoint is handled for no backoff
// or back in position detections.
//
// Testing using 3.5mm insulated keys. At 16MHz, ~20counts are achieved.
// Icc is ~10uA. Sensors are sampled ~20 times per second when a key is pressed;
// and ~6 times per second when no key is pressed after ~2 seconds. Current
// Consumption in this case is ~ 5uA.
//
// For demonstration, LEDs show position of finger.
// Last "Key" maintains LED status
//
// Zack Albus
//******************************************************************************
#include <io430x21x2.h>
#include <intrinsics.h>
#include <in430.h>
#include "PORT.h"
#include "F2132Driver.h"
#include "RC_Charge.h"

static unsigned int base_cnt;
static unsigned int meas_cnt;
static int delta_cnt;
static unsigned char key_press;
static unsigned int timer_count;
//static int cycles;

/*
  WDTCTL = WDTPW + WDTHOLD;             // Stop watchdog timer
  BCSCTL1 = CALBC1_16MHZ;               // Set DCO to 1, 8, 12 or 16MHz
  DCOCTL = CALDCO_16MHZ;
  BCSCTL1 |= DIVA_0;                    // ACLK/(0:1,1:2,2:4,3:8)
  //BCSCTL3 |= LFXT1S_2;                // LFXT1 = VLO
  IE1 |= WDTIE;                         // enable WDT interrupt
  
  _EINT();                              // Enable interrupts
*/

void Baseline_Capacitance_Initial(void)
{
	unsigned char i;
	
  measure_count();                      // Establish an initial baseline capacitance

  base_cnt = meas_cnt;

  for(i=15; i>0; i--)                   // Repeat and average base measurement
  {		
    base_cnt= (meas_cnt+base_cnt)/2;
  }
}

unsigned char Read_Key_Press(void)
{
	measure_count();                    // Measure all sensors

  delta_cnt =  meas_cnt - base_cnt;  	// Calculate delta: c_change

  // Handle baseline measurment for a base C decrease
  if (delta_cnt < 0)             			// If negative: result decreased
  {                              			// below baseline, i.e. cap decreased
   	base_cnt  = (base_cnt+meas_cnt) >> 1; // Re-average baseline down quickly
   	delta_cnt = 0;             				// Zero out delta for position determination
  }

  if (delta_cnt > KEY_lvl)       			// Determine if each key is pressed per a preset threshold
  {
     key_press = 1;              	 	// Specific key pressed
  } 
  else
	{
     key_press = 0;
	}
	return key_press;
}
		/*
    // Delay to next sample, sample more slowly if no keys are pressed
    if (key_press)
    {
      BCSCTL1 = (BCSCTL1 & 0x0CF) + DIVA_0; // ACLK/(0:1,1:2,2:4,3:8)
      cycles = 20;
    }
    else
    {
      cycles--;
      if (cycles > 0)
        BCSCTL1 = (BCSCTL1 & 0x0CF) + DIVA_0; // ACLK/(0:1,1:2,2:4,3:8)
      else
      {
        BCSCTL1 = (BCSCTL1 & 0x0CF) + DIVA_3; // ACLK/(0:1,1:2,2:4,3:8)
        cycles = 0;
      }
    }
		*/
		//pulse_LED();
		
    //WDTCTL = WDT_delay_setting;         // WDT, ACLK,interval timer

void Adjust_Baseline_Up(void)	// accomodate for genuine changes in sensor C
{
	// Handle baseline measurment for a base C increase
  if (!key_press)                   	// Only adjust baseline up if no keys are touched
  {
    base_cnt = base_cnt + 1;  				// Adjust baseline up, should be slow to
  }  

}
// Measure count result (capacitance) of each sensor
// Routine setup for four sensors, not dependent on Num_Sen value!
void measure_count(void)
{ 
	unsigned char active_key;
	
	active_key=CAP;
	
  TACTL = TASSEL_2+MC_2;                // SMCLK, cont mode
//****************************************************************************
// Negative cycle
//****************************************************************************
	
	P1DIR |= active_key;
	P1OUT &=~active_key;     // everything is low
  /* Take the active key high to charge the pad */
  P1OUT |= active_key;
  /* Allow a short time for the hard pull high to really charge the pad */
  _NOP();
  _NOP();
  _NOP();
  // Enable interrupts (edge set to low going trigger)
  // set the active key to input (was output high), and start the
  // timed discharge of the pad.
  P1IES|= active_key;                //-ve edge trigger
  P1IE |= active_key;
  P1DIR &= ~active_key;
  /* Take a snaphot of the timer... */
  timer_count = TAR;
  LPM0;
  /* Return the key to the driven low state, to contribute to the "ground" area
     around the next key to be scanned. */
  P1IE &= ~active_key;                // disable active key interrupt
  P1OUT &= ~active_key;               // switch active key to low to discharge the key
  P1DIR |= active_key;                // switch active key to output low to save power
	
	meas_cnt =timer_count;
}

/*
#pragma vector=PORT1_VECTOR
__interrupt void port_1_interrupt(void)
{
  P1IFG = 0;                            // clear flag
  timer_count = TAR - timer_count;      // find the charge/discharge time
  LPM0_EXIT;                            // Exit LPM3 on reti
}
*/

/*
void pulse_LED(void)
{ 
  unsigned char CALBC1;
  CALBC1 = CALBC1_1MHZ;
  BCSCTL1 =(BCSCTL1&0xF0)| CALBC1; // Set DCO to 1, 8, 12 or 16MHz;
	//BCSCTL1 = CALBC1_1MHZ;
  DCOCTL = CALDCO_1MHZ;
	
	Timer0_A0_Initiate(8192);

  //吃timera1初始化載入ACLK
  
  if (key_press)
	{
	  LED_HIGH;
	}
	else
	{
		LED_LOW;
	}
	
	TACTL = MC_0;
	
	TACTL = TACLR + TASSEL_2 + MC_1; // SMCLK, up mode, int enabled
	
  LPM0;             							 // Enter LPM0 w/ interrupt
	
	TACTL = TACLR;
	
  CALBC1  = CALBC1_16MHZ;
  BCSCTL1 = (BCSCTL1 &0xF0)| CALBC1; // Set DCO to 1, 8, 12 or 16MHz
	//BCSCTL1 = CALBC1_16MHZ;
  DCOCTL  = CALDCO_16MHZ;
}
*/

/*
// Watchdog Timer interrupt service routine
#pragma vector=WDT_VECTOR
__interrupt void watchdog_timer(void)
{
  //TACCTL1 ^= CCIS_0;                   // Create SW capture of CCR1
  LPM3_EXIT;                            // Exit LPM3 on reti
}


// Timer A1 interrupt service routine
#pragma vector=TIMER0_A0_VECTOR
__interrupt void Timer_A0 (void)
{
    TACCTL1 &= ~CCIE;                 // interrupt disbled
    LPM0_EXIT;
}
*/