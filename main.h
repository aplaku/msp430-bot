/******************************************************************************
 * Mark Hageman, Mark Labbato, Aldo Plaku
 * MSP430-Bot
 * 2012 Co-op Design Challenge (Texas Instruments)
 *
 * main.h
 *
 * Function declarations and defines.
 ******************************************************************************/

#ifndef MAIN_H_
#define MAIN_H_


//*****Function declarations******

//Inits
void msp_init();
void motor_init();
void sensor_init();
void timerA_init();

//Motion control
void motorA_frw();
void motorA_rvs();
void motorB_frw();
void motorB_rvs();
void forward(int speed);
void reverse(int speed);
void stop();
void left(int speed);
void right(int speed);
void autonomous();
void rnd_turn();


#endif /* MAIN_H_ */
