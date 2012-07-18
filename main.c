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
*	      3.3 <--|VCC           GND|--> GND
*             LED <--|P1.0	    xin|
*             TXD <--|P1.1         xout|
*	      RXD <--|P1.2         test|
*   Dir control A <--|P1.3	   -RST|
*   Dir control A <--|P1.4         P1.7|--> IR sensor FL
*    TA0.0 pwn-en <--|P1.5         P1.6|--> IR sensor FR
*   Dir control B <--|P2.0         P2.5|--> IR sensor R
*   Dir control B <--|P2.1 	   P2.4|
*                 <--|P2.2	   P2.3|
*                     -----------------
*
******************************************************************************/

#include  <msp430g2553.h>
#include  "stdint.h"
#include  "main.h"
#include  "rand.h"

unsigned int speed;				// Speed of motors aka duty cycle
char recv;					// Character from serial port

//************* MAIN *************//

void main(void){
	//Initialize
    msp_init();					// Setup msp430 options
    motor_init();				// Motor control setup
    sensor_init();				// IR sensors setup
    timerA_init();				// Timer setup
    sci_init();					// SCI setup

    __enable_interrupt(); 			// Enable all interrupts
    speed = 900;				// Default speed
    while(42){
    	autonomous();
    }

    //_BIS_SR(LPM0_bits + GIE);     		// Enter LPM0 and interrupt enabled.
}

//************* INITS *************//

void msp_init(){
	WDTCTL = WDTPW + WDTHOLD;       	// Stop WDT
	BCSCTL1 = CALBC1_1MHZ; 			// Set DCO
	DCOCTL = CALDCO_1MHZ;			// Clock at 1MHz
}

void motor_init(){
	P1DIR |= BIT3 + BIT4 + BIT5;    	// P1.3 and P1.4 are dir control (MOTOR 1) and P1.5 pwm en
	P2DIR |= BIT0 + BIT1;			// P2.0 and P2.1 are dir control (MOTOR 2) shared pwn en
	P1SEL |= BIT5;                  	// P1.5 TA0.0 options

	P1OUT |= BIT3;				// Dir bits must be opposite for motion
	P1OUT &= ~BIT4;				// If same, motor turns off
	P2OUT |= BIT1;
	P2OUT &= ~BIT0;
}

void sensor_init(){
	P1DIR &= ~BIT6;				// P1.6 set as input - IR FR
	P1DIR &= ~BIT7;				// P1.7 set as input - IR FL
	P2DIR &= ~BIT5;				// P2.5 set as input - IR R
	P1IE |= BIT6 + BIT7;			// P1.6 and P1.7 interrupt enabled
	P2IE |= BIT5;				// P2.5 interrupt enabled
	P1IES |= BIT6 + BIT7;			// Interrupts occur on High->Low edge
	P2IES |= BIT5;
}

void timerA_init(){
	CCTL0 = CCIE;                   	// CCR0 TA0 interrupt enabled
    CCR0 = 1000-1;                  		// PWM Period
    CCTL0 = OUTMOD_3;               		// CCR0 reset/set
    CCR1 = 950;                     		// CCR1 PWM duty cycle
    TACTL = TASSEL_2 + MC_1 + ID_3; 		// SMCLK/8, upmode
}

void sci_init(){
	P1SEL |= BIT1 + BIT2 ; 			// P1.1 = RXD, P1.2=TXD
	P1SEL2 |= BIT1 + BIT2 ; 		// P1.1 = RXD, P1.2=TXD
	UCA0CTL1 |= UCSSEL_2; 			// SMCLK
	//UCA0BR0 = 104;			// 1MHz 9600 Baud
	//UCA0BR1 = 0; 				// 1MHz 9600 Baud
	//UCA0MCTL = UCBRS0; 			// Modulation UCBRSx = 1
	UCA0BR0 = 8;                   		// 1MHz 115200 Baud
	UCA0BR1 = 0;                   		// 1MHz 115200 Baud
	UCA0MCTL = UCBRS2 + UCBRS0;    		// Modulation UCBRSx = 5b
	UCA0CTL1 &= ~UCSWRST; 			// **Initialize USCI state machine**
	IE2 |= UCA0RXIE; 			// Enable USCI_A0 RX interrupt
}

//************* MOTION CONTROL *************//

//****** motor level ******//

void motorA_frw(){
	P1OUT |= BIT3;				// Dir bits set to move forward
	P1OUT &= ~BIT4;
}

void motorA_rvs(){
	P1OUT |= BIT4;				// Dir bits set to reverse motor
	P1OUT &= ~BIT3;
}

void motorB_frw(){
	P2OUT |= BIT1;				// Dir bits set to move forward
	P2OUT &= ~BIT0;
}

void motorB_rvs(){
	P2OUT |= BIT0;				// Dir bits set to reverse motor
	P2OUT &= ~BIT1;
}

//****** unit level ******//

void stop(){
	CCR1 = 0;				// Turn off pwn en to stop motors
	delay_ms(30);
}

void forward(int speed) {
	stop();
	motorA_frw();
	motorB_frw();
	CCR1 = speed;				// Re-enable pwm at given speed
}

void reverse(int speed){
	stop(); 				// Disable pwn en
	motorA_rvs();
	motorB_rvs();
	CCR1 = speed;				// Re-enable pwm at given speed
}

void left(int speed){
	stop();					// Disable pwn en
	motorA_frw();				// Dir bits set to move forward with right wheel
	motorB_rvs();				// Dir bits set to move backwards with left wheel
	CCR1 = speed;				// Re-enable pwm at given speed
}

void right(int speed){
	CCR1 = 0;				// Disable pwn en
	motorB_frw();				// Dir bits set to move forward with left wheel
	motorA_rvs();				// Dir bits set to move backwards with right wheel
	CCR1 = speed;				// Re-enable pwm at given speed
}

//****** system level ******//

void autonomous(){
	//pseudo-random motion?
	unsigned int rnd = timer_rand(100);
	if(rnd < 70) {				// 70% forward
		forward(speed);
		delay_ms(4000+timer_rand(1000));
	}
	else if(rnd < 85) {			// 15 % right
		 right(speed);
		 delay_ms(2000+timer_rand(1000));
	}
	else {					// 15% left
		left(speed);
		delay_ms(2000+timer_rand(10000));
	}
}

void rnd_turn(){
	if(Random()){
		right(speed);
	}
	else left(speed);
}

//************* INTERRUPT VECTORS *************//

// Port 1 interrupt service routine
// Occurs when sensor pins detect hi->lo
#pragma vector=PORT1_VECTOR
__interrupt void Port_1(void){
	// If IR-FL(P1.7) or IR-FR(P1.6) is low --> reverse
	if(((BIT6 & P1IN)==0x00) | ((BIT7 & P1IN)==0x00)){
		if ((BIT5 & P2IN) == 0){	// If IR-R(P2.5) is low when reversing --> turn
			rnd_turn();
		}
		else{
			reverse(speed);		// If clear behind --> reverse
			__delay_cycles(350);	// Reverse for a bit
			rnd_turn();		// Turn away
		}
	}
	P1IFG &= ~BIT6; 			// P1.6 IFG cleared
	P1IFG &= ~BIT7; 			// P1.7 IFG cleared
	P2IFG &= ~BIT5; 			// P2.5 IFG cleared
}

// USCI interrupt service routine
// Occurs when receive buffer is full
#pragma vector=USCIAB0RX_VECTOR
__interrupt void USCI0RX_ISR(void){ 		// RX interrupt
   recv = UCA0RXBUF; 				// Read RX
}

/*
#pragma vector=TIMER0_A0_VECTOR
__interrupt void Timer_A (void){

}
*/
