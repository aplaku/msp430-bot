/******************************************************************************
* Mark Hageman, Mark Labbato, Aldo Plaku
* MSP430-Bot
* 2012 Co-op Design Challenge (Texas Instruments)
*
* main.c
*
******************************************************************************/

/******************************************************************************
* Schematic
*              		     MSP430G2553
*                     -----------------
*		             |VCC           GND|
*             LED <--|P1.0		    xin|
*             TXD <--|P1.1         xout|
*		      RXD <--|P1.2         test|
*   Dir control A <--|P1.3		   -RST|
*   Dir control A <--|P1.4         P1.7|--> IR sensor FL
*	 TA0.0 pwn-en <--|P1.5         P1.6|--> IR sensor FR
*   Dir control B <--|P2.0         P2.5|--> IR sensor R
*   Dir control B <--|P2.1 		   P2.4|
*                 <--|P2.2		   P2.3|
*                     -----------------
*
******************************************************************************/

#include  <msp430g2553.h>
#include  "main.h"
#include  "rand.h"

unsigned int speed;					// Speed of motors aka duty cycle

void main(void){
	//Initialize
    msp_init();						// Setup msp430 options
    motor_init();					// Motor control setup
    sensor_init();					// IR sensors setup
    timerA_init();					// Interrupt setup

    //GIE;							// Global interrupt enable
    speed = 900;					// Default speed
    while(42){
    	autonomous();
    }

    //_BIS_SR(LPM0_bits + GIE);     // Enter LPM0 and interrupt enabled.
}

//************* INITS *************//
void msp_init(){
	WDTCTL = WDTPW + WDTHOLD;       // Stop WDT
}

void motor_init(){
	P1DIR |= BIT3 + BIT4 + BIT2;    // P1.3 and P1.4 are dir control (MOTOR 1) and P1.5 pwm en
	P2DIR |= BIT0 + BIT1;			// P2.0 and P2.1 are dir control (MOTOR 2) shared pwn en
	P1SEL |= BIT2;                  // P1.5 TA0.0 options

	P1OUT |= BIT3;					// Dir bits must be opposite for motion
	P1OUT &= ~BIT4;					// If same, motor turns off
	P2OUT |= BIT1;
	P2OUT &= ~BIT0;
}

void sensor_init(){
	P1DIR &= ~BIT6;					// P1.6 set as input - IR FR
	P1DIR &= ~BIT7;					// P1.7 set as input - IR FL
	P2DIR &= ~BIT5;					// P2.5 set as input - IR R
}

void timerA_init(){
	CCTL0 = CCIE;                   // CCR0 TA0 interrupt enabled
    CCR0 = 1000-1;                  // PWM Period
    CCTL1 = OUTMOD_3;               // CCR1 reset/set
    CCR1 = 950;                     // CCR1 PWM duty cycle
    TACTL = TASSEL_2 + MC_1 + ID_3; // SMCLK/8, upmode
}

//************* MOTION CONTROL *************//
//****** motor level ******//
void motorA_frw(){
	P1OUT |= BIT3;					// Dir bits set to move forward
	P1OUT &= ~BIT4;
}

void motorA_rvs(){
	P1OUT |= BIT4;					// Dir bits set to reverse motor
	P1OUT &= ~BIT3;
}

void motorB_frw(){
	P2OUT |= BIT1;					// Dir bits set to move forward
	P2OUT &= ~BIT0;
}

void motorB_rvs(){
	P2OUT |= BIT0;					// Dir bits set to reverse motor
	P2OUT &= ~BIT1;
}

//****** robot level ******//
void stop(){
	CCR1 = 0;						// Turn off pwn en to stop motors
//	__delay_cycles(200);
}

void forward(int speed) {
	stop();
	motorA_frw();
	motorB_frw();
	CCR1 = speed;					// Re-enable pwm at given speed
}

void reverse(int speed){
	stop(); 						// Disable pwn en
	motorA_rvs();
	motorB_rvs();
	CCR1 = speed;					// Re-enable pwm at given speed
}

void left(int speed){
	stop();							// Disable pwn en
	motorA_frw();					// Dir bits set to move forward with right wheel
	motorB_rvs();					// Dir bits set to move backwards with left wheel
	CCR1 = speed;					// Re-enable pwm at given speed
}

void right(int speed){
	CCR1 = 0;						// Disable pwn en
	motorB_frw();					// Dir bits set to move forward with left wheel
	motorA_rvs();					// Dir bits set to move backwards with right wheel
	CCR1 = speed;					// Re-enable pwm at given speed
}

//****** system level ******//
void autonomous(){
	// If IR-FL(P1.7) or IR-FR(P1.6) is low --> reverse
	if(((BIT6 & P1IN)==0x00) | ((BIT7 & P1IN)==0x00)){
		if ((BIT5 & P2IN) == 0){	// If IR-R(P2.5) is low when reversing --> turn
			right(speed);
	    }
	    else reverse(speed);		// If clear behind --> reverse
	}
	else forward(speed);			// Otherwise --> forward
}

void rnd_turn(){
	if(Random()){
		right(speed);
	}
	else left(speed);
}

//************* INTERRUPT VECTORS *************//
/*
#pragma vector=TIMER0_A0_VECTOR
__interrupt void Timer_A (void){

}
*/
