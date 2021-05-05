#include "ch.h"
#include "hal.h"
#include <math.h>
#include <usbcfg.h>
#include <chprintf.h>


#include "motor_control.h"
#include "main.h"
#include "motors.h"
#include "process_image.h"
#include "Tof.h"

#define	NUMBER_STEP_FULL_ROTATION	1000		//nombre de pas des moteurs pour une rotation compl�te
#define PI							3.1416
#define PERIMETRE_ROUE				130			//perimetre de la roue en mm
#define	DIAMETRE_EPUCK				54			//distance entre les deux roues du e-puck en mm
#define SPEED_SATURATION_MOTOR		MOTOR_SPEED_LIMIT	//r�duit la vitesse maximale des moteurs
#define ERROR_SUM_MAX				500
#define ERROR_MARGIN_DIST_MM		5				//mm
#define TURN_SPEED					400			//vitesse � laquelle on fait touner le e-puck(en pas/s)
#define LEFT_MOTOR					0			//moteur de gauche
#define	RIGHT_MOTOR					1			//moteur de droite

static bool motor_stop = false;
uint8_t last_color = NO_COLOR; 			//derni�re couleur vue par la camera (selon le #define de process_image.h)
uint8_t state = DIST_CAPTURE_STATE;
static int16_t error_sum = 0;



/*	fonction: calcul de la vitesse des moteurs souhait�e � travers un r�gulateur P
 * 	argument: @distance :distance du epuck par rapport au mur du fond
 * 			  @command  :distance souhait�e entre le epuck et le mur du fond
 * 	return:	  vitesse des moteurs souhait�e
 */
int16_t regulator(uint16_t distance, uint16_t command);


/* fonction:  tourne de 90� dans la direction indiqu�e par la couleur.
 * 			  NO_COLOR = e-puck tourne pas mais continue d'avancer
 * 			  ROUGE	   = e-puck tourne � gauche
 * 			  VERT	   = e-puck tourne � droite
 * 			  BLEU	   = e-puck s'arr�te
 * arguments: aucun.
 * return:    aucun
 */
void turn_90_degree(void);

/* fonction:  fait bouger le @moteur (gauche ou droite) de @step pas � la vitesse @speed
 * arguments: @step: nombre de pas � effectuer, @speed: vitesse souhait�e;
 * return:	  aucun
 */
void motor_do_N_step(uint16_t step, int16_t speed, uint8_t moteur);

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

       /* switch (state){
        case COLOR_CAPTURE_STATE:
        	last_color = get_couleur();
        	chprintf((BaseSequentialStream *)&SD3, "couleur = %d \n ", last_color);
        	set_state(DIST_CAPTURE_STATE);
        	break;
        case TURN_STATE:
        	//turn_90_degree();
        	set_state(DIST_CAPTURE_STATE);
        	break;
        }

        //vitesse des moteurs
        if(motor_stop)	speed = 0;
        //marge d'erreur acceptable
        else if ((get_distance_mm()<(TURN_TARGET_DIST_MM+ERROR_MARGIN_DIST_MM))
                		&& (get_distance_mm()>(TURN_TARGET_DIST_MM-ERROR_MARGIN_DIST_MM))) speed=0;
        else speed = regulator(get_distance_mm(), TURN_TARGET_DIST_MM);
*/
        speed=0;
        right_motor_set_speed(speed);
        left_motor_set_speed(speed);
        turn_90_degree();

        //100Hz
        chThdSleepUntilWindowed(time, time + MS2ST(10)); //Prendre en compte le temps d'exec du controlleur
    }
}

void motor_controller_start(void){
	chThdCreateStatic(waMotorController,
					sizeof(waMotorController),
					NORMALPRIO+4,
					MotorController, NULL);

}

//--------------------------------------------------------------------------------------------------------------
//*Impl�mentation des fonctions internes
//--------------------------------------------------------------------------------------------------------------

int16_t regulator(uint16_t distance, uint16_t command) {

	int16_t speed = 0;
	int16_t Kp = 50;
	float Ki = 0.29;
	error_sum += distance-command;
	speed = Kp*(distance-command) + Ki*error_sum;

	//implementing an antireset wind-up
	if (error_sum > ERROR_SUM_MAX ) error_sum = ERROR_SUM_MAX;
	else if (error_sum < -ERROR_SUM_MAX) error_sum = -ERROR_SUM_MAX;

	if(speed > SPEED_SATURATION_MOTOR)   speed = SPEED_SATURATION_MOTOR;
    if(speed < -SPEED_SATURATION_MOTOR)  speed = -SPEED_SATURATION_MOTOR;

	return speed;

}


void turn_90_degree(void) {

	last_color=ROUGE;
	int32_t nbr_step_a_faire = nbr_step_a_faire = PI*NUMBER_STEP_FULL_ROTATION*DIAMETRE_EPUCK/(4*PERIMETRE_ROUE);
	//virage � gauche
	if(last_color == ROUGE) {
		motor_do_N_step(nbr_step_a_faire, TURN_SPEED, RIGHT_MOTOR);
		motor_do_N_step(nbr_step_a_faire, -TURN_SPEED, LEFT_MOTOR);
	}

	//virage � droite
	if(last_color == VERT)	{
		motor_do_N_step(nbr_step_a_faire, TURN_SPEED, LEFT_MOTOR);
		motor_do_N_step(nbr_step_a_faire, -TURN_SPEED, RIGHT_MOTOR);
	}

	//arr�ter les moteurs
	if(last_color == BLEU) {
		motor_stop = true;
	}
}


void motor_do_N_step(uint16_t step, int16_t speed, uint8_t moteur){
	left_motor_set_speed(0);
	right_motor_set_speed(0);

	if(speed > 0) {
		if(moteur == LEFT_MOTOR) {
			left_motor_set_pos(0);
			while(left_motor_get_pos() <= step){
				left_motor_set_speed(speed);
			}
			left_motor_set_speed(0);
		}

		if(moteur == RIGHT_MOTOR) {
			right_motor_set_pos(0);
			while(right_motor_get_pos() <= step) {
				right_motor_set_speed(speed);
			}
			right_motor_set_speed(0);
		}
	}

	if(speed <0) {
		if(moteur == LEFT_MOTOR) {
			left_motor_set_pos(step);
			while(left_motor_get_pos() >= 0) {
				left_motor_set_speed(speed);
			}
			left_motor_set_speed(0);
		}
		if(moteur == RIGHT_MOTOR) {
			right_motor_set_pos(step);
			while(right_motor_get_pos() >= 0){
				right_motor_set_speed(speed);
			}
			right_motor_set_speed(0);
		}
	}
}

