#include "motor_control.h"

#include "ch.h"
#include "hal.h"
#include <math.h>
#include <usbcfg.h>
#include <chprintf.h>


#include <main.h>
#include <motors.h>
#include <process_image.h>


int16_t pi_regulator(float distance, float command);
int16_t pi_regulator_angle(int16_t line_begin);

static int16_t error_sum = 0;
static int16_t error_sum_angle = 0;

static THD_WORKING_AREA(waPiRegulator, 256);
static THD_FUNCTION(PiRegulator, arg) {

    chRegSetThreadName(__FUNCTION__);
    (void)arg;

    systime_t time;

    int16_t speed = 0;
    int16_t speed_turn = 0;

    while(1){
        time = chVTGetSystemTime();

        /*
		*	To complete
		*/
        
        //applies the speed from the PI regulator
         speed =  pi_regulator(get_distance_cm(), 10);
         speed_turn = pi_regulator_angle(get_line_begin());

		 right_motor_set_speed(speed-speed_turn);
		 left_motor_set_speed(speed+speed_turn);

        //100Hz
        chThdSleepUntilWindowed(time, time + MS2ST(10));
    }
}

void pi_regulator_start(void){
	chThdCreateStatic(waPiRegulator, sizeof(waPiRegulator), NORMALPRIO, PiRegulator, NULL);
}

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

int16_t pi_regulator_angle(int16_t line_begin) {

	int16_t goal = get_center_line_begin();
	int16_t speed = 0;
	float Kp = -1;
	float Ki = -0.1;

	speed = Kp*(goal-line_begin) + Ki*error_sum_angle;

	error_sum_angle += goal-line_begin;
	if(error_sum_angle > 5)	error_sum_angle = 5;
	if(error_sum_angle < -5)	error_sum_angle = -5;

	return speed;
}
