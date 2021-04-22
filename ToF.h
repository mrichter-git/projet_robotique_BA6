/*
 * ToF.h
 *
 *  Created on: 20 avr. 2021
 *      Author: franc
 */

#ifndef TOF_H_
#define TOF_H_

/**
 * @brief 	Débute le thread du Tof pour faire des mesures en continu
 */
void start_ToF(void);

/**
 * @brief 	Arrête les mesures du capteur et bloque son thread
 */
void ToF_stop(void);

/*
 * @brief 	Détecte si la distance mesurée est plus petite qu'une valeur target
 *
 * @return 	bool: plus petit ou plus grand
 */
bool ToF_target_dist(void);

/*
 * @brief 	Renvoie la dernière distance mesurée par le ToF en [mm]
 */
uint16_t get_distance_mm(void);


#endif /* TOF_H_ */
