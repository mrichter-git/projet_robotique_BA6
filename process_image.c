#include "ch.h"
#include "hal.h"
#include <chprintf.h>
#include <usbcfg.h>

#include <main.h>
#include <camera/po8030.h>

#include <process_image.h>

//intensités maximales possibles pour chaque canal RGB
#define MAX_VALUE_RED	 31
#define MAX_VALUE_GREEN	 63
#define MAX_VALUE_BLUE	 31


//déclaration des fonctions et variables globales internes

static uint8_t couleur = 0; 		//memorise la dernière couleure vue par la camera (pas de couleur = 0, red = 1, green = 2
									//blue = 3

/* fonction:  detecte la couleur vue par la camera
 * arguments: intensité des 3 canaux RGB.
 * return:    aucun
 */
void detection_couleur(uint8_t red, uint8_t green, uint8_t blue);


//semaphore
static BSEMAPHORE_DECL(image_ready_sem, TRUE);

static THD_WORKING_AREA(waCaptureImage, 256);
static THD_FUNCTION(CaptureImage, arg) {

    chRegSetThreadName(__FUNCTION__);
    (void)arg;

	//Takes pixels 0 to IMAGE_BUFFER_SIZE of the line 10 + 11 (minimum 2 lines because reasons)
	po8030_advanced_config(FORMAT_RGB565, 0, 10, IMAGE_BUFFER_SIZE, 2, SUBSAMPLING_X1, SUBSAMPLING_X1);
	dcmi_enable_double_buffering();
	dcmi_set_capture_mode(CAPTURE_ONE_SHOT);
	dcmi_prepare();

    while(1){
        //starts a capture
		dcmi_capture_start();
		//waits for the capture to be done
		wait_image_ready();
		//signals an image has been captured
		chBSemSignal(&image_ready_sem);
    }
}

//---------------------------------------------------------------------------------------------------------

static THD_WORKING_AREA(waProcessImage, 1024);
static THD_FUNCTION(ProcessImage, arg) {

    chRegSetThreadName(__FUNCTION__);
    (void)arg;

	uint8_t *img_buff_ptr;
	uint8_t image[IMAGE_BUFFER_SIZE] = {0};

	//systime_t time = chVTGetSystemTime();

    while(1){

    	//waits until an image has been captured
         chBSemWait(&image_ready_sem);

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
			somme_green +=((*(img_buff_ptr+2*i) & (0b00000111))<<3) | ((*(img_buff_ptr+2*i+1) & (0b11100000))>>5);

			//somme canal Blue
			somme_blue += (*(img_buff_ptr+2*i+1) & (0b00011111));
		}

		//calcul des moyennes de chaque canal
		uint8_t moyenne_red = somme_red/IMAGE_BUFFER_SIZE;
		uint8_t moyenne_green = somme_green/IMAGE_BUFFER_SIZE;
		uint8_t moyenne_blue = sommme_blue/IMAGE_BUFFER_SIZE;

		//SendUint8ToComputer(image, IMAGE_BUFFER_SIZE);

		//chprintf((BaseSequentialStream *)&SD3, "time = %d \n",(int) distance_cm);

    }
}



//--------------------------------------------------------------------------------------------------
void process_image_start(void){
	chThdCreateStatic(waProcessImage, sizeof(waProcessImage), NORMALPRIO, ProcessImage, NULL);
	chThdCreateStatic(waCaptureImage, sizeof(waCaptureImage), NORMALPRIO, CaptureImage, NULL);
}
//---------------------------------------------------------------------------------------------------
/* Une couleur est considérée comme dominante si la valeur de son canal correspondant est >= 0.5 valeur
 * max du canal. Si l'intensité de plus d'un canal est >= 0.5 valeur mac du canal, la couleur détéectée
 * est mise à 0 (aucune couleur n'est détectée)
 *
 */
void detection_couleur(uint8_t red, uint8_t green, uint8_t blue) {

	couleur = 0;

	bool red_dominant = (red >= MAX_VALUE_RED/2);
	bool green_dominant = (green>= MAX_VALUE_GREEN/2);
	bool blue_dominant = (blue >= MAX_VALUE_BLUE/2);

	if(red_dominant & !green_dominant & !blue_dominant)		couleur = 1;
	if(!red_dominant & green_dominant & !blue_dominant)		couleur = 2;
	if(!red_dominant & !green_dominant & blue_dominant)		couleur = 3;

}

//-------------------------------------------------------------------------------------------------------

uint8_t get_couleur(void) {
	return couleur;
}
