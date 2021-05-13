/*
 * ToF.c
 *
 *  Created on: 20 avr. 2021
 *      Authors: Michael et Francesco
 *      Code modifié sur la base de: VL530X.c et VL530X.h, author: Eliot Ferragni
 */
#include "ch.h"
#include "hal.h"
#include <chprintf.h>
#include <sensors/VL53L0X/VL53L0X.h>
#include <i2c_bus.h>


#include "ToF.h"
#include "main.h"
#include "process_image.h"



//--------------------------------------------------------------------------------------------------------------
//*Déclaration des variables globales internes et déclaration des fonctions privées
//--------------------------------------------------------------------------------------------------------------

static uint16_t dist_mm = 0;
static bool ToF_configured = false;
static thread_t *distThd;

/*
 * @brief 	Détecte si la distance mesurée est plus petite qu'une valeur target pour la mesure de la couleur
 *
 * @return 	bool
 */
bool ToF_color_target_hit(void);

/*
 * @brief 	Détecte si la distance mesurée est plus petite qu'une valeur target pour tourner
 *
 * @return 	bool
 */
bool ToF_turn_target_hit(void);


//--------------------------------------------------------------------------------------------------------------
//*Implémentation du thread ToF
//--------------------------------------------------------------------------------------------------------------

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

    /* Loop de lecture du thread*/
    while (chThdShouldTerminateX() == false) {
    	if(ToF_configured && get_state()==DIST_CAPTURE_STATE){
    		//on copie la valeur mesurée dans une variable facile d'accès
    		VL53L0X_getLastMeasure(&device);
   			dist_mm = device.Data.LastRangeMeasure.RangeMilliMeter;

   			//détection d'état du système
   			if (ToF_color_target_hit()){	//si la distance voulue est atteinte, on change de mode
   				set_state(COLOR_CAPTURE_STATE);
   	   		}
   	   		else if (ToF_turn_target_hit()){
   	   			set_state(COLOR_GOT_STATE);
   	   		}
    	}
    	//chprintf((BaseSequentialStream *)&SD3, "state = %d \n ", get_state());
    	//chprintf((BaseSequentialStream *)&SD3, "dist = %d \n ", dist_mm);
		chThdSleepMilliseconds(100);
    }
}


//--------------------------------------------------------------------------------------------------------------
//*Implémentation des fonctions publiques
//--------------------------------------------------------------------------------------------------------------

void ToF_start(void) {

	if(ToF_configured) {
		return;
	}

	i2c_start();

	distThd = chThdCreateStatic(waToFThd, //initialisation du thread: nom:distThd
                     sizeof(waToFThd),
                     NORMALPRIO,
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
	return dist_mm;
}

//--------------------------------------------------------------------------------------------------------------
//*Implémentation des Fonctions privées
//--------------------------------------------------------------------------------------------------------------

bool ToF_color_target_hit(void) {

	if (dist_mm <= COLOR_TARGET_DIST_MM && dist_mm > TURN_TARGET_DIST_MM)
	{
		return true;
	}
	return false;
}

bool ToF_turn_target_hit(void) {

	if (dist_mm <= TURN_TARGET_DIST_MM)
	{
		return true;
	}
	return false;
}
