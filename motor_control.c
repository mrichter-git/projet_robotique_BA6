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

#define	NUMBER_STEP_FULL_ROTATION	1000		//nombre de pas des moteurs pour une rotation compl�te
#define PI							3.1416
#define PERIMETRE_ROUE				130			//perimetre de la roue en mm
#define	DIAMETRE_EPUCK				54			//distance entre les deux roues du e-puck en mm
#define SPEED_SATURATION_MOTOR		MOTOR_SPEED_LIMIT*3/4	//r�duit la vitesse maximale des moteurs
#define ERROR_SUM_MAX				500
#define ERROR_SUM_PROX_MAX			100
#define ERROR_MARGIN_DIST_MM		5				//mm
#define ERROR_MARGIN_PROXIMITY		15
#define TURN_SPEED					350			//vitesse � laquelle on fait touner le e-puck(en pas/s)
#define NBR_STEP_90_DEGREE			PI*NUMBER_STEP_FULL_ROTATION*DIAMETRE_EPUCK/(4*PERIMETRE_ROUE)
#define PROXIMITY_LEFT				5
#define PROXIMITY_RIGHT				2
#define PROXIMITY_AVANT_DROITE		1
#define PROXIMITY_AVANT_GAUCHE		6
#define PROXIMITY_BACK_DROITE		3
#define PROXIMITY_BACK_GAUCHE		4



static bool motor_stop = false;
static uint8_t last_color = NO_COLOR; 			//derni�re couleur vue par la camera (selon le #define de process_image.h)
static uint8_t state = DIST_CAPTURE_STATE;
static int16_t error_sum = 0;
static int16_t error_sum_proximity_1 = 0;	//Terme int�grale du PID utilisant les capteurs de proximit�
static int16_t error_sum_proximity_2 = 0;
static int16_t error_sum_proximity_3 = 0;
static int16_t previous_error_1 = 0;				//derni�re erreur pour le terme diff�rentiel du PID
static int16_t previous_error_2 = 0;
static int16_t previous_error_3 = 0;



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


/* fonction: recentrage du robot au milieu de sa piste au moyen d'un regulateur PI
 * arguments: aucun
 * return: 	commande en vitesse pour les moteurs.
 */
int16_t proximity_regulator(void);



static THD_WORKING_AREA(waMotorController, 2048);
static THD_FUNCTION(MotorController, arg) {

    chRegSetThreadName("Motor_Thd");
    (void)arg;

    //adresse du fichier audio pour animation sonore
    //char sound[4] = "D:\"";

    //setSoundFileVolume(VOLUME_MAX);

    systime_t time;

    int16_t speed = 0;
    int16_t turn_speed = 0; //composante de la vitesse d�di�e au recentrage du robot

    bool captured = 0;

    while(1){

        time = chVTGetSystemTime();
        state = get_state();

       switch (state){
        case COLOR_CAPTURE_STATE:
        	capture_couleur();
        	set_state(DIST_CAPTURE_STATE);
        	captured = 1;
        	break;
        case COLOR_GOT_STATE:
        	if (captured){
        		last_color = get_couleur();
        		chprintf((BaseSequentialStream *)&SD3, "couleur = %d \n ", last_color);
        		reset_couleur();
        		captured = 0;
        	}
        	//turn_90_degree();
            //playSoundFile(sound,SF_SIMPLE_PLAY);
            set_state(TURNING_STATE);
        	break;
        case TURNING_STATE:
        	turn_90_degree();
        	set_state(DIST_CAPTURE_STATE);
        	break;
        }

        //vitesse des moteurs
        if(motor_stop)	speed = 0;
        //marge d'erreur acceptable
        else if ((get_distance_mm()<(TURN_TARGET_DIST_MM+ERROR_MARGIN_DIST_MM))
                		&& (get_distance_mm()>(TURN_TARGET_DIST_MM-ERROR_MARGIN_DIST_MM))) speed=0;
        else speed = regulator(get_distance_mm(), TURN_TARGET_DIST_MM);

        if(motor_stop) turn_speed = 0;
        else	       turn_speed = proximity_regulator();

        //speed = 0;
        //turn_speed = 0;

        left_motor_set_speed(speed+turn_speed);
        right_motor_set_speed(speed-turn_speed);

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

	//virage � gauche
	if(last_color == ROUGE) {
		right_motor_set_pos(0);
		left_motor_set_pos(NBR_STEP_90_DEGREE);
		right_motor_set_speed(TURN_SPEED);
		left_motor_set_speed(-TURN_SPEED);

		while(right_motor_get_pos()<=NBR_STEP_90_DEGREE && left_motor_get_pos() >=0) {
			//chprintf((BaseSequentialStream *)&SD3, "left motor = %d \n ", left_motor_get_pos());
			//chprintf((BaseSequentialStream *)&SD3, "right motor = %d \n ", right_motor_get_pos());
		}
		right_motor_set_speed(0);		//�teint les moteurs apr�s avoir effectu� le virage
		left_motor_set_speed(0);
	}

	//virage � droite
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

	//arr�ter les moteurs
	if(last_color == BLEU) {
		motor_stop = true;
	}
}


int16_t proximity_regulator(void) {

	int16_t turn_speed_1 = 0; 	//l'indice 1 se r�f�re aux termes du r�gulateur associ�s aux capteurs droite et gauche
	int16_t turn_speed_2 = 0;	//l'indice 2 se r�f�re aux termes du r�gulateur associ�s aux capteurs avant-droite et avant-gauche
	int16_t turn_speed_3 = 0;	//l'indice 3 se r�f�re aux termes du r�gulateur associ�s aux capteurs arri�re-droite et arri�re-gauche
	//implementing a PI regulator


	float Kp_1 = 0.1;
	float Ki_1 = 0.05;
	float Kd_1 = 0.07;

	float Kp_2 = 0.7;
	float Ki_2 = 0.03;
	float Kd_2 = 0.04;

	float Kp_3 = 0.6;
	float Ki_3 = 0.07;
	float Kd_3 = 0.03;

	//difference d'intensit� entre les capteurs de droite et de gauche pour le centrage du robot en enlevant l'effet de la lunmi�re ambiante

	int16_t prox_value_difference_1 = (get_prox(PROXIMITY_LEFT))-(get_prox(PROXIMITY_RIGHT));
	int16_t prox_value_difference_2 = (get_prox(PROXIMITY_AVANT_GAUCHE))-(get_prox(PROXIMITY_AVANT_DROITE));
	int16_t prox_value_difference_3 = (get_prox(PROXIMITY_BACK_GAUCHE))-(get_prox(PROXIMITY_BACK_DROITE));

	error_sum_proximity_1+= prox_value_difference_1; //termes int�graux du r�gulateur
	error_sum_proximity_2+= prox_value_difference_2;
	error_sum_proximity_3+= prox_value_difference_3;

	//+get_ambient_light(PROXIMITY_RIGHT)
	//get_ambient_light(PROXIMITY_LEFT)
	chprintf((BaseSequentialStream *)&SD3, "ir_left = %d \n ", get_prox(PROXIMITY_LEFT));
	chprintf((BaseSequentialStream *)&SD3, "ir_right = %d \n ", get_prox(PROXIMITY_RIGHT));


	//antireset windup implementation
	if(error_sum_proximity_1 > ERROR_SUM_PROX_MAX)	error_sum_proximity_1 = ERROR_SUM_PROX_MAX;
	if(error_sum_proximity_1 < -ERROR_SUM_PROX_MAX)	error_sum_proximity_1 = -ERROR_SUM_PROX_MAX;
	if(error_sum_proximity_2 > ERROR_SUM_PROX_MAX)	error_sum_proximity_2 = ERROR_SUM_PROX_MAX;
	if(error_sum_proximity_2 < -ERROR_SUM_PROX_MAX)	error_sum_proximity_2 = -ERROR_SUM_PROX_MAX;
	if(error_sum_proximity_3 > ERROR_SUM_PROX_MAX)	error_sum_proximity_3 = ERROR_SUM_PROX_MAX;
	if(error_sum_proximity_3 < -ERROR_SUM_PROX_MAX)	error_sum_proximity_3 = -ERROR_SUM_PROX_MAX;

	//ne tient pas compte des variations d�es au bruit sur la mesure
	if(prox_value_difference_1 <ERROR_MARGIN_PROXIMITY && prox_value_difference_1 > -ERROR_MARGIN_PROXIMITY	)  prox_value_difference_1 = 0;
	if(prox_value_difference_2 <ERROR_MARGIN_PROXIMITY && prox_value_difference_2 > -ERROR_MARGIN_PROXIMITY	)  prox_value_difference_2 = 0;
	if(prox_value_difference_3 <ERROR_MARGIN_PROXIMITY && prox_value_difference_3 > -ERROR_MARGIN_PROXIMITY	)  prox_value_difference_3 = 0;



	turn_speed_1 = Kp_1*prox_value_difference_1 + Ki_1*error_sum_proximity_1 +Kd_1*(prox_value_difference_1-previous_error_1);
	turn_speed_2 = Kp_2*prox_value_difference_2 + Ki_2*error_sum_proximity_2+Kd_2*(prox_value_difference_2-previous_error_2);
	turn_speed_3 = Kp_3*prox_value_difference_3 + Ki_3*error_sum_proximity_3+Kd_3*(prox_value_difference_2-previous_error_2);


	previous_error_1 = prox_value_difference_1;
	previous_error_2 = prox_value_difference_2;
	previous_error_3 = prox_value_difference_3;

	return turn_speed_1+turn_speed_2+turn_speed_3;
}


