#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "ch.h"
#include "hal.h"
#include "memory_protection.h"
#include <usbcfg.h>
#include <motors.h>
#include <camera/po8030.h>
#include <chprintf.h>
#include "audio/play_sound_file.h"
#include "audio/play_melody.h"
#include "audio/audio_thread.h"
#include "sensors/proximity.h"
#include "process_image.h"
#include "main.h"
#include "motor_control.h"
#include "ToF.h"
#include "sdio.h"

//declaration du messagebus utilis? par les capteurs de proximit?
messagebus_t bus;
MUTEX_DECL(bus_lock);
CONDVAR_DECL(bus_condvar);


static uint8_t state = DIST_CAPTURE_STATE;

void SendUint8ToComputer(uint8_t* data, uint16_t size) 
{
	chSequentialStreamWrite((BaseSequentialStream *)&SD3, (uint8_t*)"START", 5);
	chSequentialStreamWrite((BaseSequentialStream *)&SD3, (uint8_t*)&size, sizeof(uint16_t));
	chSequentialStreamWrite((BaseSequentialStream *)&SD3, (uint8_t*)data, size);
}

static void serial_start(void)
{
	static SerialConfig ser_cfg = {
	    115200,
	    0,
	    0,
	    0,
	};

	sdStart(&SD3, &ser_cfg); // UART3.
}

int main(void)
{

    halInit();
    chSysInit();
    mpu_init();

    //starts the serial communication
    serial_start();
    //start the USB communication
    usb_start();

    //initialise message bus
    messagebus_init(&bus, &bus_lock, &bus_condvar);

    //starts the camera
    dcmi_start();
	po8030_start();
	camera_init();

	//inits the motors
	motors_init();

	//proximity sensor intialisation and calibration
	proximity_start();
	calibrate_ir();



	//stars the threads for the ToF sensor and the control of the motors
	ToF_start();
	motor_controller_start();

    /* Infinite loop. */
    while (1) {
    	//waits 1 second
        chThdSleepMilliseconds(1000);
    }
}

#define STACK_CHK_GUARD 0xe2dee396
uintptr_t __stack_chk_guard = STACK_CHK_GUARD;

void __stack_chk_fail(void)
{
    chSysHalt("Stack smashing detected");
}

uint8_t get_state(void){
	return state;
}

void set_state(uint8_t sys_state){
	state = sys_state;
}

