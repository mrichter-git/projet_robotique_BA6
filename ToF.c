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

static uint16_t dist_mm = 0;
static bool ToF_configured = false;
static thread_t *distThd;
static uint8_t state = DIST_CAPT_ST;


//Thread de mesure de la distance: différence avec celui de la librairie: mode de mesure et détéction de distance
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
    		//on copie la valeur mesurée dans une variable facile d'accès
    		VL53L0X_getLastMeasure(&device);
   			dist_mm = device.Data.LastRangeMeasure.RangeMilliMeter;

   			//détection d'état du système
   			if (ToF_color_target_hit()){	//si la distance voulue est atteinte, on change de mode
   	   			state = COLOR_CAPTURE_STATE;
   	   		}
   	   		else if (ToF_turn_target_hit()){
   	   			state = TURN_STATE;
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
    chThdWait(distThd); 		//Attend la fin de l'exécution du thread
    distThd = NULL;
    ToF_configured = false;
}

uint16_t get_distance_mm(void) {
	return distance_mm;
}

uint8_t get_state(void){
	return state;
}

//--------------------------------------------------------------------------------------------------------------
//*Déclaration des fonctions internes
//--------------------------------------------------------------------------------------------------------------

/*
 * @brief 	Détecte si la distance mesurée est plus petite qu'une valeur target pour la mesure de la couleur
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
 * @brief 	Détecte si la distance mesurée est plus petite qu'une valeur target pour tourner
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
