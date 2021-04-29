/*
 * ToF.h
 *
 *  Created on: 20 avr. 2021
 *      Author: franc
 */

#ifndef TOF_H_
#define TOF_H_

#define COLOR_TARGET_DIST_MM	50			//distance ou l'on mesure la couleur du mur du fond
#define TURN_TARGET_DIST_MM		20			//distance ou on commence le virage

/**
 * @brief 	Débute le thread du Tof pour faire des mesures en continu
 */
void start_ToF(void);

/**
 * @brief 	Arrête les mesures du capteur et bloque son thread
 */
void ToF_stop(void);

/**
 * @brief 	Renvoie la dernière distance mesurée par le ToF en [mm]
 */
uint16_t get_distance_mm(void);

#endif /* TOF_H_ */


