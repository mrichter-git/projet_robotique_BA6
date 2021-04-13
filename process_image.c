#include "ch.h"
#include "hal.h"
#include <chprintf.h>
#include <usbcfg.h>

#include <main.h>
#include <camera/po8030.h>

#include <process_image.h>

#define NBR_AVERAGE 1

static float distance_cm = 0;
static int16_t line_begin = 320;
static int16_t center_line_begin = 320;

//internal function

uint16_t BlackLine_pixel_width(uint8_t* image, uint16_t size);

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


static THD_WORKING_AREA(waProcessImage, 1024);
static THD_FUNCTION(ProcessImage, arg) {

    chRegSetThreadName(__FUNCTION__);
    (void)arg;

	uint8_t *img_buff_ptr;
	uint8_t image[IMAGE_BUFFER_SIZE] = {0};

	//systime_t time = chVTGetSystemTime();

    while(1){

    	//waits until an image has been captured
       // chBSemWait(&image_ready_sem);
		//gets the pointer to the array filled with the last image in RGB565    
		//img_buff_ptr = dcmi_get_last_image_ptr();

		/*
		*	To complete
		*/


		/*systime_t time2 = time;
		time = chVTGetSystemTime();
		chprintf((BaseSequentialStream *)&SD3, "time = %d \n", time-time2);
		*/

		chBSemWait(&image_ready_sem);
		img_buff_ptr = dcmi_get_last_image_ptr();

		for(uint16_t i = 0; i < IMAGE_BUFFER_SIZE; ++i)
		{
			uint8_t pixel_collect_1 = 0;

			pixel_collect_1 = (*(img_buff_ptr+2*i) & 0b00000111) << 3;
			pixel_collect_1 |= (*(img_buff_ptr + 2*i+1) & (0b00000111<<5)) >> 5;

			image[i] = pixel_collect_1;
		}

		SendUint8ToComputer(image, IMAGE_BUFFER_SIZE);
		int16_t pixel_width = BlackLine_pixel_width(image, IMAGE_BUFFER_SIZE);

		distance_cm =  1355.1/pixel_width;
		//chprintf((BaseSequentialStream *)&SD3, "time = %d \n",(int) distance_cm);

    }
}



float get_distance_cm(void){
	return distance_cm;
}

void process_image_start(void){
	chThdCreateStatic(waProcessImage, sizeof(waProcessImage), NORMALPRIO, ProcessImage, NULL);
	chThdCreateStatic(waCaptureImage, sizeof(waCaptureImage), NORMALPRIO, CaptureImage, NULL);
}

uint16_t BlackLine_pixel_width(uint8_t* image, uint16_t size) {
	bool line_begun = false;
	uint16_t line_pixel_width = 0;

	/*float moyenne = 0;

	for(uint16_t i = 0; i < size; ++i)
	{
		moyenne += image[i]/size;

	}*/

	for(uint16_t i = 0; i < size; ++i)
	{
		if(image[i] < 5) ++line_pixel_width;
		if(image[i-1] < 5 && image[i] < 5 && image[i+1] < 5 && i>0 && i<size && !line_begun)
		{
			line_begin = i;
			line_begun = true;
		}
	}
	center_line_begin = size/2 - line_pixel_width/2;

	return line_pixel_width;
}

int16_t get_line_begin(void) {

	return line_begin;
}


int16_t get_center_line_begin(void) {

	return center_line_begin;

}


