#include "ch.h"
#include "hal.h"
#include <math.h>
#include <usbcfg.h>
#include <chprintf.h>

#include "sensors/proximity.h"
#include "motor_control.h"
#include "main.h"
#include "motors.h"
#include "process_image.h"
#include "Tof.h"
#include "audio/play_sound_file.h"

#define	NUMBER_STEP_FULL_ROTATION	1000		//nombre de pas des moteurs pour une rotation complète
#define PI							3.1416
#define PERIMETRE_ROUE				130			//perimetre de la roue en mm
#define	DIAMETRE_EPUCK				54			//distance entre les deux roues du e-puck en mm
#define SPEED_SATURATION_MOTOR		MOTOR_SPEED_LIMIT	//réduit la vitesse maximale des moteurs
#define ERROR_SUM_MAX				500
#define ERROR_SUM_PROX_MAX			200
#define ERROR_MARGIN_DIST_MM		5				//mm
#define TURN_SPEED					350			//vitesse à laquelle on fait touner le e-puck(en pas/s)
#define NBR_STEP_90_DEGREE			PI*NUMBER_STEP_FULL_ROTATION*DIAMETRE_EPUCK/(4*PERIMETRE_ROUE)
#define PROXIMITY_LEFT			5
#define PROXIMITY_RIGHT			2


static bool motor_stop = false;
static uint8_t last_color = NO_COLOR; 			//dernière couleur vue par la camera (selon le #define de process_image.h)
static uint8_t state = DIST_CAPTURE_STATE;
static int16_t error_sum = 0;
static int16_t error_sum_proximity = 0;			//Terme intégrale du PI utilisant les capteurs de proximité



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


/* fonction: recentrage du robot au milieu de sa piste au moyen d'un regulateur PI
 * arguments: aucun
 * return: 	commande en vitesse pour les moteurs.
 */
int16_t proximity_regulator(void);



static THD_WORKING_AREA(waMotorController, 512);
static THD_FUNCTION(MotorController, arg) {

    chRegSetThreadName("Motor_Thd");
    (void)arg;

    //adresse du fichier audio pour animation sonore
    char sound[9];
    sound[0] = 'r';
    sound[1] = '2';
    sound[3] = 'd';
    sound[4] = '2';
    sound[5] = '.';
    sound[6] = 'w';
    sound[7] = 'a';
    sound[8] = 'v';

    //calibration of the proximity sensors
    //calibrate_ir();


    systime_t time;

    int16_t speed = 0;
    //int16_t turn_speed = 0; //composante de la vitesse dédiée au recentrage du robot

    while(1){

        time = chVTGetSystemTime();
        state = get_state();

       switch (state){
        case COLOR_CAPTURE_STATE:
        	last_color = get_couleur();
        	chprintf((BaseSequentialStream *)&SD3, "couleur = %d \n ", last_color);
        	set_state(DIST_CAPTURE_STATE);
        	break;
        case TURN_STATE:
        	turn_90_degree();
        	set_state(DIST_CAPTURE_STATE);
        	playSoundFile(sound,SF_SIMPLE_PLAY);
        	break;
        }

        //vitesse des moteurs
        if(motor_stop)	speed = 0;
        //marge d'erreur acceptable
        else if ((get_distance_mm()<(TURN_TARGET_DIST_MM+ERROR_MARGIN_DIST_MM))
                		&& (get_distance_mm()>(TURN_TARGET_DIST_MM-ERROR_MARGIN_DIST_MM))) speed=0;
        else speed = regulator(get_distance_mm(), TURN_TARGET_DIST_MM);

        //if(motor_stop) turn_speed = 0;
        //else	       turn_speed = proximity_regulator();

        left_motor_set_speed(speed);//-turn_speed);
        right_motor_set_speed(speed);//+turn_speed);

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
//*Implémentation des fonctions internes
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

	last_color=NO_COLOR;


	//virage à gauche
	if(last_color == ROUGE) {
		right_motor_set_pos(0);
		left_motor_set_pos(NBR_STEP_90_DEGREE);
		right_motor_set_speed(TURN_SPEED);
		left_motor_set_speed(-TURN_SPEED);

		while(right_motor_get_pos()<=NBR_STEP_90_DEGREE && left_motor_get_pos() >=0) {
			chprintf((BaseSequentialStream *)&SD3, "left motor = %d \n ", left_motor_get_pos());
			chprintf((BaseSequentialStream *)&SD3, "right motor = %d \n ", right_motor_get_pos());
		}
		right_motor_set_speed(0);		//éteint les moteurs après avoir effectué le virage
		left_motor_set_speed(0);
	}

	//virage à droite
	if(last_color == VERT)	{
		right_motor_set_pos(NBR_STEP_90_DEGREE);
		left_motor_set_pos(0);
		right_motor_set_speed(-TURN_SPEED);
		left_motor_set_speed(TURN_SPEED);
		while(right_motor_get_pos()>=0 && left_motor_get_pos()<= NBR_STEP_90_DEGREE) {
			__asm__ volatile("nop");
		}
		right_motor_set_speed(0);
		left_motor_set_speed(0);
	}

	//arrêter les moteurs
	if(last_color == BLEU) {
		motor_stop = true;
	}
}


int16_t proximity_regulator(void) {

	int16_t turn_speed;
	//implementing a PI regulator

	int16_t Kp = 50;
	float 	Ki = 0.1;

	//difference d'intensité entre les capteurs de droite et de gauche pour le centrage du robot en enlevant l'effet de la lunmière ambiante

	int16_t prox_value_difference = (get_prox(PROXIMITY_LEFT)-get_ambient_light(PROXIMITY_LEFT)) - (get_prox(PROXIMITY_RIGHT)-get_ambient_light(PROXIMITY_RIGHT));

	error_sum_proximity+= prox_value_difference;

	//antireset windup implementation
	if(error_sum_proximity > ERROR_SUM_PROX_MAX)	error_sum_proximity = ERROR_SUM_PROX_MAX;
	if(error_sum_proximity < -ERROR_SUM_PROX_MAX)	error_sum_proximity = -ERROR_SUM_PROX_MAX;

	turn_speed = Kp*prox_value_difference + Ki*error_sum_proximity;

	return turn_speed;
}


