/*
 * ToF.h
 *
 *  Created on: 20 avr. 2021
 *      Author: franc
 */

#ifndef TOF_H_
#define TOF_H_

#define DIST_CAPTURE_STATE		0			//mode de capture de distance: vérifier dans quel mode on se trouve
#define COLOR_CAPTURE_STATE		1			//mode de capture de couleur: appel des fonctions de capture d'image
#define TURN_STATE				2			//mode de virage: on appelle les fonctions de virage


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

uint8_t get_state(void);


#endif /* TOF_H_ */
