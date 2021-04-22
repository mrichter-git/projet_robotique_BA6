/*
 * ToF.c
 *
 *  Created on: 20 avr. 2021
 *      Author: franc
 */
#include "ToF.h"
#include "VL53L0X.h"

#define TARGET_DIST_MM			50

static uint16_t dist_mm = 0;
static bool ToF_configured = false;
static thread_t *distThd;

static BSEMAPHORE_DECL(target_hit_sem, FALSE);

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

    /* Loop de lecture du thread*/
    while (chThdShouldTerminateX() == false) {
    	if(ToF_configured){
    		VL53L0X_getLastMeasure(&device);
   			dist_mm = device.Data.LastRangeMeasure.RangeMilliMeter;
   			if (Tof_target_dist){
   				chBSemSignal(&target_hit_sem);
   			}
    	}
		chThdSleepMilliseconds(100);
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

bool ToF_target_dist(void) {

	if (dist_mm < TARGET_DIST_MM)
	{
		return true;
	}
	return false;
}

uint16_t get_distance_mm(void) {

	return distance_mm;
}
