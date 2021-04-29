#ifndef MAIN_H
#define MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "camera/dcmi_camera.h"
#include "msgbus/messagebus.h"
#include "parameter/parameter.h"


//constants for the differents parts of the project
#define IMAGE_BUFFER_SIZE		640

#define DIST_CAPTURE_STATE		0			//mode de capture de distance: vérifier dans quel mode on se trouve
#define COLOR_CAPTURE_STATE		1			//mode de capture de couleur: appel des fonctions de capture d'image
#define TURN_STATE				2			//mode de virage: on appelle les fonctions de virage

/** Robot wide IPC bus. */
extern messagebus_t bus;

extern parameter_namespace_t parameter_root;

void SendUint8ToComputer(uint8_t* data, uint16_t size);

/**
 * @brief 	Renvoie l'état du système
 */
uint8_t get_state(void);

/**
 * @brief 	Permet de changer l'état du système
 */
void set_state(uint8_t sys_state);



#ifdef __cplusplus
}
#endif

#endif
