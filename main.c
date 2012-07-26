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
*             3.3 <--|VCC           GND|--> GND
*             LED <--|P1.0          xin|
*             LED <--|P1.1         xout|
*             LED <--|P1.2         test|
*   Dir control A <--|P1.3         -RST|
*    TA0.0 pwn-en <--|P1.4         P1.7|--> IR sensor FL
*   Dir control A <--|P1.5         P1.6|--> IR sensor FR
*   Dir control B <--|P2.0         P2.5|--> IR sensor R
*   Dir control B <--|P2.1         P2.4|--> LED
*             LED <--|P2.2         P2.3|--> LED
*                     -----------------
*
******************************************************************************/

#include  <msp430g2553.h>
#include  "stdlib.h"
#include  "main.h"

//State of motion
unsigned short dir;
#define Mforward 0;
#define Mreverse 1;
#define Mleft    2;
#define Mright   3;
#define speed  900;
#define period 999;

int count = 0;
char led_dir = 'f';

//************* MAIN *************//

void main(void){

	msp_init();                     // Setup msp430 options
	led_init();
	timerB_init();                  // TimerA1 setup for LEDs
	__enable_interrupt();           // Enable all interrupts
    // Wait until R sensor is hit (Software on switch)
    P2DIR &= ~BIT5;                 // P2.5 set as input - IR R
    while((P2IN & BIT5) != 0x00){}
    //Initialize

    motor_init();                   // Motor control setup
    sensor_init();                  // IR sensors setup
    timerA_init();                  // TimerA0 setup
    //sci_init();                     // SCI setup
    
    stop();
    while(42){
        autonomous();
    }
}

//************* INITS *************//

void msp_init(){
    WDTCTL = WDTPW + WDTHOLD;       // Stop WDT
    BCSCTL1 = CALBC1_1MHZ;          // Set DCO
    DCOCTL = CALDCO_1MHZ;           // Clock at 1MHz
}

void motor_init(){
    P1DIR |= BIT3 + BIT4 + BIT5;    // P1.3 and P1.5 are dir control (MOTOR 1) and P1.4 pwm en
    P2DIR |= BIT0 + BIT1;           // P2.0 and P2.1 are dir control (MOTOR 2) shared pwn en
    P1SEL |= BIT4;                  // P1.4 TA0.0 options

    P1OUT |= BIT3;                  // Dir bits must be opposite for motion
    P1OUT &= ~BIT5;                 // If same, motor turns off
    P2OUT |= BIT1;
    P2OUT &= ~BIT0;
}

void sensor_init(){
    P1DIR &= ~BIT6;                 // P1.6 set as input - IR FR
    P1DIR &= ~BIT7;                 // P1.7 set as input - IR FL
    P2DIR &= ~BIT5;                 // P2.5 set as input - IR R
    P1IE  |= BIT6 + BIT7;           // P1.6 and P1.7 interrupt enabled
    P1IES |= BIT6 + BIT7;           // Interrupts occur on High->Low edge
}

void timerA_init(){
    //Note: interrupt not enabled
    TACCR0 = period;                // PWM Period
    TACCTL0 = OUTMOD_3;             // CCR0 reset/set
    TACCR1 = speed;                 // CCR1 PWM duty cycle
    TACTL = TASSEL_2 + MC_1 + ID_3; // SMCLK/8, upmode
}

void timerB_init(){
	TA1CCTL0 |= CCIE;               // Enable Timer 1 interrupt
	TA1CCR0 = 250;                  // Timer period
	TA1CTL = TASSEL_1 + MC_1 + ID1;// Aclock/8, count up
}

void led_init(){
    P1DIR |= BIT0 + BIT1 + BIT2;    // LED OUTPUT setup P1
    P2DIR |= BIT2 + BIT3 + BIT4;    // LED OUTPUT setup P2
}

void sci_init(){
    P1SEL |= BIT1 + BIT2 ;          // P1.1 = RXD, P1.2=TXD
    P1SEL2 |= BIT1 + BIT2 ;         // P1.1 = RXD, P1.2=TXD
    UCA0CTL1 |= UCSSEL_2;           // SMCLK
    //UCA0BR0 = 104;                // 1MHz 9600 Baud
    //UCA0BR1 = 0;                  // 1MHz 9600 Baud
    //UCA0MCTL = UCBRS0;            // Modulation UCBRSx = 1
    UCA0BR0 = 8;                    // 1MHz 115200 Baud
    UCA0BR1 = 0;                    // 1MHz 115200 Baud
    UCA0MCTL = UCBRS2 + UCBRS0;     // Modulation UCBRSx = 5b
    UCA0CTL1 &= ~UCSWRST;           // **Initialize USCI state machine**
    IE2 |= UCA0RXIE;                // Enable USCI_A0 RX interrupt
}

//************* MOTION CONTROL *************//

//****** motor level ******//

void motorA_frw(){
    P1OUT |= BIT3;                  // Dir bits set to move forward
    P1OUT &= ~BIT5;
}

void motorA_rvs(){
    P1OUT |= BIT5;                  // Dir bits set to reverse motor
    P1OUT &= ~BIT3;
}

void motorB_frw(){
    P2OUT |= BIT1;                  // Dir bits set to move forward
    P2OUT &= ~BIT0;
}

void motorB_rvs(){
    P2OUT |= BIT0;                  // Dir bits set to reverse motor
    P2OUT &= ~BIT1;
}

//****** unit level ******//

void stop(){
    CCR0 = 0;                       // PWM disabled
    delay_ms(30);
}

void forward() {
    stop();
    dir = Mforward;
    motorA_frw();
    motorB_frw();
    CCR0 = period;                  // Re-enable pwm at given speed
}

void reverse(){
    stop();                         // Disable pwn en
    dir = Mreverse;
    motorA_rvs();
    motorB_rvs();
    CCR0 = period;                  // Re-enable pwm at given speed
}

void left(){
    stop();                         // Disable pwn en
    dir = Mleft;
    motorA_frw();                   // Dir bits set to move forward with right wheel
    motorB_rvs();                   // Dir bits set to move backwards with left wheel
    CCR0 = period;                  // Re-enable pwm at given speed
}

void right(){
    stop();                         // Disable pwn en
    dir = Mright;
    motorB_frw();                   // Dir bits set to move forward with left wheel
    motorA_rvs();                   // Dir bits set to move backwards with right wheel
    CCR0 = period;                  // Re-enable pwm at given speed
}

//****** system level ******//

void autonomous(){
    //pseudo-random motion?
    unsigned int rnd = rand() % 100;
    srand(rnd);
    if(rnd < 60) {                  // 60% forward
        forward();
        delay_ms(2000+(rand() % 1000));
    }
    else if(rnd < 80) {             // 20% right
         right();
         delay_ms(1000+(rand() % 500));
    }
    else {                          // 20% left
        left();
        delay_ms(1000+(rand() % 500));
    }
}

void rnd_turn(){
    //50-50 change at left or right turn
    unsigned int trand = rand() % 100;
    srand(trand);
    if(trand > 50){
        right();
    }
    else left();
}

//************* INTERRUPT VECTORS *************//

// Port 1 interrupt service routine
// Occurs when sensor pins detect hi->lo
#pragma vector=PORT1_VECTOR
__interrupt void Port_1(void){
    // If IR-FL(P1.7) or IR-FR(P1.6) is low --> reverse
    if(((BIT6 & P1IN)==0x00) | ((BIT7 & P1IN)==0x00)){
            reverse();              // If clear behind --> reverse
            delay_ms(1000+rand() % 1000);	// Reverse for a bit
            rnd_turn();             // Turn away
    }
    P1IFG &= ~BIT6;                 // P1.6 IFG cleared
    P1IFG &= ~BIT7;                 // P1.7 IFG cleared
}

// USCI interrupt service routine
// Occurs when receive buffer is full
#pragma vector=USCIAB0RX_VECTOR
__interrupt void USCI0RX_ISR(void){ // RX interrupt
   recv = UCA0RXBUF; 				// Read RX
}


#pragma vector=TIMER1_A0_VECTOR
__interrupt void Timer_A1 (void){
	// Available pins: P1.0-P1.2 and P2.2-P2.4
	// Count determines LED pin to drive High
	// LED chaser aka cylon eye
    if(count==0) {
    	P1OUT |= BIT0;
    	P1OUT &= ~BIT1;
    }
    else if(count==1) {
    	P1OUT |= BIT1;
    	P1OUT &= ~BIT0;
    	P1OUT &= ~BIT2;
    }
    else if(count==2) {
    	P1OUT |= BIT2;
    	P1OUT &= ~BIT1;
    	P2OUT &= ~BIT2;
    }
    else if(count==3) {
    	P2OUT |= BIT2;
    	P1OUT &= ~BIT2;
    	P2OUT &= ~BIT3;
    }
    else if(count==4) {
    	P2OUT |= BIT3;
    	P2OUT &= ~BIT2;
    	P2OUT &= ~BIT4;
    }
    else {
    	P2OUT |= BIT4;
    	P2OUT &= ~BIT3;
    }

    if(count==5) led_dir='b';       // If at last led, count down
    if((count==0) & (led_dir=='b')) led_dir='f'; // If at first led, count back up

    if(led_dir=='f') count++;       // Inc or dec based on direction
    else count--;

}
