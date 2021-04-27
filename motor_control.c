#include "motor_control.h"
#include "ch.h"
#include "hal.h"
#include <math.h>
#include <usbcfg.h>
#include <chprintf.h>


#include <main.h>
#include <motors.h>
#include <process_image.h>
#include <Tof.h>

static bool motor_stop = false;



int16_t pi_regulator(float distance, float command);
int16_t pi_regulator_angle(int16_t line_begin);

static int16_t error_sum = 0;
static int16_t error_sum_angle = 0;

static THD_WORKING_AREA(waMotorController, 256);
static THD_FUNCTION(PiMotorController, arg) {

    chRegSetThreadName("Motor Thd");
    (void)arg;

    motors_init();

    systime_t time;

    while(1){
        time = chVTGetSystemTime();


		 right_motor_set_speed(speed-speed_turn);
		 left_motor_set_speed(speed+speed_turn);

        //100Hz
        chThdSleepUntilWindowed(time, time + MS2ST(10)); //Prendre en compte le temps d'exec du controlleur
    }
}

void motor_controller_start(void){
	chThdCreateStatic(waMotorController,
					sizeof(waMotorController),
					NORMALPRIO,
					MotorController, NULL);

}

//--------------------------------------------------------------------------------------------------------------
//*Déclaration des fonctions internes
//--------------------------------------------------------------------------------------------------------------

int16_t pi_regulator(float distance, float command) {

	int16_t speed = 0;
	int16_t Kp = -150;
	float Ki = -1.75;

	speed = Kp*(command-distance) + Ki*error_sum;

	//implementing an antireset windup
	error_sum += command-distance;
	if(error_sum > 1000) error_sum = 1000;
	if(error_sum < -1000) error_sum = -1000;

	return speed;

}


