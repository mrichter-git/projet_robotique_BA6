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

#define	NUMBER_STEP_FULL_ROTATION	1000		//nombre de pas des moteurs pour une rotation complète
#define PI							3.1416
#define PERIMETRE_ROUE				130			//perimetre de la roue en mm
#define	DIAMETRE_EPUCK				54			//distance entre les deux roues du e-puck en mm
#define SPEED_SATURATION_MOTOR		MOTOR_SPEED_LIMIT	//réduit la vitesse maximale des moteurs

static bool motor_stop = false;
uint8_t last_color = NO_COLOR; 			//dernière couleur vue par la camera (selon le #define de process_image.h)
uint8_t state = DIST_CAPTURE_STATE;

/*	fonction: calcul de la vitesse des moteurs souhaitée à travers un régulateur P
 * 	argument: @distance :distance du epuck par rapport au mur du fond
 * 			  @command  :distance souhaitée entre le epuck et le mur du fond
 * 	return:	  vitesse des moteurs souhaitée
 */
int16_t regulator(uint16_t distance, uint16_t command);


/* fonction:  tourne de 90° dans la direction indiquée par la couleur.
 * 			  NO_COLOR = e-puck tourne pas mais continue d'avancer
 * 			  ROUGE	   = e-puck tourne à gauche
 * 			  VERT	   = e-puck tourne à droite
 * 			  BLEU	   = e-puck s'arrête
 * arguments: aucun.
 * return:    aucun
 */
void turn_90_degree(void);

static THD_WORKING_AREA(waMotorController, 256);
static THD_FUNCTION(MotorController, arg) {

    chRegSetThreadName("Motor Thd");
    (void)arg;

    motors_init();

    systime_t time;

    int16_t speed = 0;

    while(1){

        time = chVTGetSystemTime();
        state = get_state();

        switch (state){
        case COLOR_CAPTURE_STATE:
        	last_color = get_couleur();
        	//set_state(DIST_CAPTURE_STATE);
        	break;
        case TURN_STATE:
        	//turn_90_degree();
        	set_state(DIST_CAPTURE_STATE);
        	break;
        }

        //vitesse des moteurs
        if(motor_stop)	{speed = 0;}
        else {speed = regulator(get_distance_mm(), TURN_TARGET_DIST_MM);}


	    right_motor_set_speed(speed);
	    left_motor_set_speed(speed);

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

int16_t regulator(uint16_t distance, uint16_t command) {

	int16_t speed = 0;
	int16_t Kp = 50;
	//float Ki = -1.75;
   speed = Kp*(command-distance); //+ Ki*error_sum;

	/*//implementing an antireset windup
	error_sum += command-distance;
*/
	if(speed > SPEED_SATURATION_MOTOR)   speed = SPEED_SATURATION_MOTOR;
    if(speed < -SPEED_SATURATION_MOTOR)  speed = -SPEED_SATURATION_MOTOR;

	return speed;

}


void turn_90_degree(void) {

	int32_t nbr_step_a_faire = 0;
	//virage à gauche
	if(last_color == ROUGE) {
		nbr_step_a_faire = PI*PERIMETRE_ROUE*DIAMETRE_EPUCK/(4*NUMBER_STEP_FULL_ROTATION);
	}

	//virage à droite
	if(last_color == VERT)	{
		nbr_step_a_faire = -PI*PERIMETRE_ROUE*DIAMETRE_EPUCK/(4*NUMBER_STEP_FULL_ROTATION);
	}

	//arrêter les moteurs
	if(last_color == BLEU) {
		motor_stop = true;
	}

	left_motor_set_pos(left_motor_get_pos()-nbr_step_a_faire);
	right_motor_set_pos(right_motor_get_pos()+nbr_step_a_faire);
}


