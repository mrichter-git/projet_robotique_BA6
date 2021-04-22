/*
 * ToF.c
 *
 *  Created on: 20 avr. 2021
 *      Author: Michael
 */
#include "ToF.h"
#include "VL53L0X.h"
#include "process_image.h"

#define COLOR_TARGET_DIST_MM	50 			//distance ou l'on mesure la couleur du mur du fond
#define TURN_TARGET_DIST_MM		20			//distance ou on commence le virage
#define DIST_CAPTURE_STATE		0			//mode de capture de distance: v�rifier dans quel mode on se trouve
#define COLOR_CAPTURE_STATE		1			//mode de capture de couleur: appel des fonctions de capture d'image
#define TURN_STATE				2			//mode de virage: on appelle les fonctions de virage

static uint16_t dist_mm = 0;
static bool ToF_configured = false;
static thread_t *distThd;
static uint8_t state = DIST_CAPT_ST;


//Thread de mesure de la distance: diff�rence avec celui de la librairie: mode de mesure et d�t�ction de distance
static THD_WORKING_AREA(waToFThd, 512);
static THD_FUNCTION(ToFThd, arg) {

	chRegSetThreadName("ToF Thd");
	VL53L0X_Error status = VL53L0X_ERROR_NONE;

	(void)arg;
	static VL53L0X_Dev_t device;

	device.I2cDevAddr = VL53L0X_ADDR;

	status = VL53L0X_init(&device);

	if(status == VL53L0X_ERROR_NONE){
		VL53L0X_configAccuracy(&device, VL53L0X_DEFAULT_MODE);
	}
	if(status == VL53L0X_ERROR_NONE){
		VL53L0X_startMeasure(&device, VL53L0X_DEVICEMODE_CONTINUOUS_RANGING);
	}
	if(status == VL53L0X_ERROR_NONE){
		ToF_configured = true;
	}

	uint8_t couleur = 0;

    /* Loop de lecture du thread*/
    while (chThdShouldTerminateX() == false) {
    	if(ToF_configured){
    		//on copie la valeur mesur�e dans une variable facile d'acc�s
    		VL53L0X_getLastMeasure(&device);
   			dist_mm = device.Data.LastRangeMeasure.RangeMilliMeter;

   			//d�tection d'�tat du syst�me
   			switch (state){
   				case DIST_CAPTURE_STATE: 	//�tat de d�tection de distance
   					if (ToF_color_target_hit()){	//si la distance voulue est atteinte, on change de mode
   						state = COLOR_CAPTURE_STATE;
   					}
   					else if (ToF_turn_target_hit()){
   						state = TURN_STATE;
   					}
   					break;
   				case COLOR_CAPTURE_STATE:
   					couleur = get_couleur();
   					state = DIST_CAPTURE_STATE;		//de cette mani�re, on peut bouger les murs et quand m�me continuer le fnctionnement
   					break;
   				case TURN_STATE:
   					//� d�finir: process_COULEUR(couleur);
   					break;
   			}
    	}
		chThdSleepMilliseconds(50);
    }
}

void ToF_start(void) {

	if(VL53L0X_configured) {
		return;
	}

	i2c_start();

	distThd = chThdCreateStatic(waToFThd, //initialisation du thread: nom:distThd
                     sizeof(waToFThd),
                     NORMALPRIO + 10,
                     ToFThd,
                     NULL);
}

void ToF_stop(void) {
    chThdTerminate(distThd);
    chThdWait(distThd); 		//Attend la fin de l'ex�cution du thread
    distThd = NULL;
    ToF_configured = false;
}

uint16_t get_distance_mm(void) {

	return distance_mm;
}

//--------------------------------------------------------------------------------------------------------------
//*D�claration des fonctions internes
//--------------------------------------------------------------------------------------------------------------

/*
 * @brief 	D�tecte si la distance mesur�e est plus petite qu'une valeur target pour la mesure de la couleur
 *
 * @return 	bool: plus petit ou plus grand
 */
bool ToF_color_target_hit(void) {

	if (dist_mm <= COLOR_TARGET_DIST_MM)
	{
		return true;
	}
	return false;
}

/*
 * @brief 	D�tecte si la distance mesur�e est plus petite qu'une valeur target pour tourner
 *
 * @return 	bool: plus petit ou plus grand
 */
bool ToF_turn_target_hit(void) {

	if (dist_mm <= TURN_TARGET_DIST_MM)
	{
		return true;
	}
	return false;
}
