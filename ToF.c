/*
 * ToF.c
 *
 *  Created on: 20 avr. 2021
 *      Author: franc
 */
#include "ToF.h"
#include "VL53L0X.h"

static uint16_t distance_mm = 0;

void init_ToF(void) {

}

uint16_t get_distance_mm(void) {

	return distance_mm;
}
