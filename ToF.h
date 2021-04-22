/*
 * ToF.h
 *
 *  Created on: 20 avr. 2021
 *      Author: franc
 */

#ifndef TOF_H_
#define TOF_H_

//initialisation du capteur time of flight et lancement du thread
void init_ToF(void);

//renvoie la dernière distance mesurée par le ToF en [mm]
uint16_t get_distance_mm(void);


#endif /* TOF_H_ */
