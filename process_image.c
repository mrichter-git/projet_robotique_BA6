#include "ch.h"
#include "hal.h"
#include <chprintf.h>
#include <usbcfg.h>

#include <camera/po8030.h>
#include "main.h"
#include "process_image.h"

//intensités maximales possibles pour chaque canal RGB
#define MAX_VALUE_RED	 31
#define MAX_VALUE_GREEN	 63
#define MAX_VALUE_BLUE	 31


//--------------------------------------------------------------------------------------------------------------
//*Déclaration des fonctions et variables globales internes
//--------------------------------------------------------------------------------------------------------------

static uint16_t couleur[4] = {0};	//memorise le nombre de fois que chaque couleur est dominante (rien=0, red = 1, green = 2
									//blue = 3

/* fonction:  detecte la couleur vue par la camera
 * arguments: intensité des 3 canaux RGB.
 * return:    aucun
 */
void detection_couleur(uint16_t red, uint16_t green, uint16_t blue);

/* fonction:  lancer une capture d'image
 * arguments: aucun
 * return:    aucun
 */
void capture_image(void);

/* fonction:  lit les canaux RGB de l'image pour faire la moyenne de chaque canal
 * arguments: l'adresse du premier element du tableau de taille 3 qui contiendra la moyenne de chaque canal
 * return:    aucun
 */
void lecture_image(uint16_t* moyennes_couleur);

//--------------------------------------------------------------------------------------------------------------
//*implémentations des fonctions
//--------------------------------------------------------------------------------------------------------------

void capture_image(void) {

	//Takes pixels 0 to IMAGE_BUFFER_SIZE of the line 10 + 11 (minimum 2 lines because reasons)
	po8030_advanced_config(FORMAT_RGB565, 0, 10, IMAGE_BUFFER_SIZE, 2, SUBSAMPLING_X1, SUBSAMPLING_X1);
	dcmi_enable_double_buffering();
	dcmi_set_capture_mode(CAPTURE_ONE_SHOT);
	dcmi_prepare();

    //demare a capture
	dcmi_capture_start();
	//attends que la capture soit terminée
	wait_image_ready();
}

//--------------------------------------------------------------------------------------------------------------

void lecture_image(uint16_t* moyennes_couleur){

	uint8_t *img_buff_ptr;

	//gets the pointer to the array filled with the last image in RGB565
	img_buff_ptr = dcmi_get_last_image_ptr();

	//variables contenant somme de chaque ligne de l'image pour les différents canaux de couleur RGB
	//pour en faire la moyenne après
	uint16_t somme_red = 0;
	uint16_t somme_green = 0;
	uint16_t somme_blue = 0;

	//calcul de la moyenne de chaque canal RGB pour extraire la couleur moyenne vue par la camera
	for(uint16_t i=0; i<(IMAGE_BUFFER_SIZE); i++) {

		//somme du canal Red
		somme_red += ((uint8_t)(*(img_buff_ptr+2*i) & (0b11111000))>>3);

		//somme du canal Green
		somme_green += (uint8_t)(((*(img_buff_ptr+2*i) & (0b00000111))<<3) | ((*(img_buff_ptr+2*i+1) & (0b11100000))>>5));

		//somme canal Blue
		somme_blue += (uint8_t)(*(img_buff_ptr+2*i+1) & (0b00011111));
	}

	//calcul des moyennes normalisées en fonction de leur max de chaque canal

	*(moyennes_couleur) = somme_red/MAX_VALUE_RED;// /(IMAGE_BUFFER_SIZE);
	//chprintf((BaseSequentialStream *)&SD3, "red = %d \n ", *(moyennes_couleur));

	*(moyennes_couleur + 1) = somme_green/MAX_VALUE_GREEN;// /(IMAGE_BUFFER_SIZE);
	//chprintf((BaseSequentialStream *)&SD3, "green = %d \n ", *(moyennes_couleur+1));

	*(moyennes_couleur + 2)= somme_blue/MAX_VALUE_BLUE;// /(IMAGE_BUFFER_SIZE);
	//chprintf((BaseSequentialStream *)&SD3, "blue = %d \n ", *(moyennes_couleur+2));


}

//-------------------------------------------------------------------------------------------------------------
/* Une couleur est considérée comme dominante si la valeur de son canal correspondant est plus grande que
 * la valeur moyenne de l'intensité capturée. Si plus d'un canal est dominant, la couleur detectée
 * est mise à 0 (aucune couleur n'est détectée)
 *
 */
void detection_couleur(uint16_t red, uint16_t green, uint16_t blue) {

	uint16_t threshold = 0;
	threshold = (red+green+blue)/(3);
	//chprintf((BaseSequentialStream *)&SD3, "\n");
	//chprintf((BaseSequentialStream *)&SD3, "thresh = %d \n ", threshold);


	bool red_dominant = (red >= threshold);
	//chprintf((BaseSequentialStream *)&SD3, "red = %d \n ", red);
	bool green_dominant = (green >= threshold);
	//chprintf((BaseSequentialStream *)&SD3, "green = %d \n ", green);
	bool blue_dominant = (blue >= threshold);
	//chprintf((BaseSequentialStream *)&SD3, "blue = %d \n ", blue);

	if(red_dominant & !green_dominant & !blue_dominant)				couleur[1] += 1;
	else if(!red_dominant & green_dominant & !blue_dominant)		couleur[2] += 1;
	else if(!red_dominant & !green_dominant & blue_dominant)		couleur[3] += 1;
	else 															couleur[0] += 1;
}

//-------------------------------------------------------------------------------------------------------

uint8_t get_couleur(void) {
	uint8_t dominant = 0;
	if ((couleur [1] > couleur [2]) && (couleur[1] > couleur[3])) 		dominant = 1;
	else if ((couleur [2] > couleur [1]) && (couleur[2] > couleur[3])) 	dominant = 2;
	else if ((couleur [3] > couleur [1]) && (couleur[3] > couleur[2])) 	dominant = 3;
	else																dominant = 0;
	return dominant;
}

//---------------------------------------------------------------------------------------------------------

void reset_couleur(void) {
	for (uint8_t i = 0; i <= 3; i++) couleur[i] = 0;
}

//---------------------------------------------------------------------------------------------------------
void capture_couleur(void) {
	//contiendra la moyenne de chaque canal RGB dans l'ordre Red, Green, Blue
	uint16_t moyennes_image[3] = {0};

	capture_image();
	lecture_image(moyennes_image);
	detection_couleur(moyennes_image[0], moyennes_image[1], moyennes_image[2]);
}

