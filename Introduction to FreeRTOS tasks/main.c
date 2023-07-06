/*
 * FreeRTOS Kernel V10.2.0
 * Copyright (C) 2019 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

/* 
	NOTE : Tasks run in system mode and the scheduler runs in Supervisor mode.
	The processor MUST be in supervisor mode when vTaskStartScheduler is 
	called.  The demo applications included in the FreeRTOS.org download switch
	to supervisor mode prior to main being called.  If you are not using one of
	these demo application projects then ensure Supervisor mode is used.
*/


/*
 * Creates all the demo application tasks, then starts the scheduler.  The WEB
 * documentation provides more details of the demo application tasks.
 * 
 * Main.c also creates a task called "Check".  This only executes every three 
 * seconds but has the highest priority so is guaranteed to get processor time.  
 * Its main function is to check that all the other tasks are still operational.
 * Each task (other than the "flash" tasks) maintains a unique count that is 
 * incremented each time the task successfully completes its function.  Should 
 * any error occur within such a task the count is permanently halted.  The 
 * check task inspects the count of each task to ensure it has changed since
 * the last time the check task executed.  If all the count variables have 
 * changed all the tasks are still executing error free, and the check task
 * toggles the onboard LED.  Should any task contain an error at any time 
 * the LED toggle rate will change from 3 seconds to 500ms.
 *
 */

/* Standard includes. */
#include <stdlib.h>
#include <stdio.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "lpc21xx.h"

/* Peripheral includes. */
#include "serial.h"
#include "GPIO.h"


/*-----------------------------------------------------------*/

/* Constants to setup I/O and processor. */
#define mainBUS_CLK_FULL	( ( unsigned char ) 0x01 )

/* Constants for the ComTest demo application tasks. */
#define mainCOM_TEST_BAUD_RATE	( ( unsigned long ) 115200 )

#define TOGGLE_OFF  	0
#define TOGGLE_FAST		1
#define TOGGLE_SLOW		2
#define FIRST_TASK		0
#define	SECOND_TASK		1
#define THIRD_TASK		2
#define TASK					THIRD_TASK


#if		TASK 		==		FIRST_TASK
TaskHandle_t LedTask_Handler = NULL;
#elif	TASK		==		SECOND_TASK
TaskHandle_t Led0Task_Handler = NULL;
TaskHandle_t Led1Task_Handler = NULL;
TaskHandle_t Led2Task_Handler = NULL;
#elif	TASK		==		THIRD_TASK
TaskHandle_t LedTask_Handler = NULL;
TaskHandle_t ButtonTask_Handler = NULL;
char gl_toggling_state = 0;
#endif

/*
 * Configure the processor for use with the Keil demo board.  This is very
 * minimal as most of the setup is managed by the settings in the project
 * file.
 */
static void prvSetupHardware( void );
/*-----------------------------------------------------------*/
#if		TASK 		==		FIRST_TASK
void Led_Task (void * pvParameters)
{
	for( ; ; )
	{
		/*Toggle LED every 1000ms - 1 sec -*/
		GPIO_write(PORT_0, PIN1, PIN_IS_HIGH);
		
		vTaskDelay(1000);
		
		GPIO_write(PORT_0, PIN1, PIN_IS_LOW);
		
		vTaskDelay(1000);
		
	}
}
#elif	TASK		==		SECOND_TASK
void Led0_Task (void * pvParameters)
{
	for( ; ; )
	{
		/*Toggle LED every 100 ms*/
		vTaskDelay(100);
		GPIO_toggle(PORT_0, PIN1);
	}
}

void Led1_Task (void * pvParameters)
{
	for( ; ; )
	{
		/*Toggle LED every 500 ms*/
		vTaskDelay(500);
		GPIO_toggle(PORT_0, PIN2);
	}
}	

void Led2_Task (void * pvParameters)
{
	for( ; ; )
	{
		/*Toggle LED every 1 sec*/
		vTaskDelay(1000);
		GPIO_toggle(PORT_0, PIN3);
	}
}
#elif	TASK		==		THIRD_TASK
void Led_Task (void * pvParameters)
{
	for( ; ; )
	{
		/**
		 *	Check on Toggling State
		 *	TOGGLE_FAST :	delay = 100ms + Toggle pin
		 *	TOGGLE_OFF 	:	delay = 100ms + Write Low on the pin
		 *	TOGGLE_SLOW :	delay = 400ms + Toggle pin
		 */
		if (gl_toggling_state == TOGGLE_FAST)
		{
			GPIO_toggle(PORT_0, PIN1);
			vTaskDelay(100);
		}
		else if (gl_toggling_state == TOGGLE_OFF)
		{
			GPIO_write(PORT_0, PIN1, PIN_IS_LOW);
			vTaskDelay(100);
		}
		else if (gl_toggling_state == TOGGLE_SLOW)
		{
			GPIO_toggle(PORT_0, PIN1);
			vTaskDelay(400);
		}
	}
}

void Button_Task (void * pvParameters)
{
	/**
	 * Counting variable : is used to indicate the eneteries 
	 * to task while the button was still pressed
	 * 
	 * pressing_time : is Used to indicate to the total time the button was pressed
	 */
	static int counting_variable = 0, pressing_time = 0;
	static pinState_t button_state = PIN_IS_LOW;
	
	for( ; ; )
	{
		vTaskDelay(50);
		
		/*Gets pin's state*/
		button_state = GPIO_read(PORT_0, PIN0);
		
		/*If it is high means it is still pressed*/
		if(button_state == PIN_IS_HIGH)
		{
			/*increase counting variable*/
			counting_variable++;
		}
		/*else means it is released*/
		else
		{
			/*check if it was pressed in the last entery*/
			if (counting_variable != 0)
			{
				/*calculate pressing time and change toggling state to the corresponding time*/
				pressing_time = counting_variable * 50;
				if ( pressing_time < 2000)
				{
					gl_toggling_state = TOGGLE_OFF;
				}
				else if (pressing_time >= 2000 && pressing_time < 4000)
				{
					gl_toggling_state = TOGGLE_SLOW;
				}
				else if (pressing_time >=	4000)
				{
					gl_toggling_state = TOGGLE_FAST;
				}
				counting_variable = 0;
			}
		}
			
	}
}

#endif
/*
 * Application entry point:
 * Starts all the other tasks, then starts the scheduler. 
 */
int main( void )
{
	/* Setup the hardware for use with the Keil demo board. */
	prvSetupHardware();
	
#if		TASK 		==		FIRST_TASK
	
    /* Create Tasks here */
		xTaskCreate(
									Led_Task,
									"Led_Task",
									configMINIMAL_STACK_SIZE,
									(void *) NULL,
									1,
									&LedTask_Handler);
#elif	TASK		==		SECOND_TASK
		xTaskCreate(
									Led0_Task,
									"Led 0 Task",
									configMINIMAL_STACK_SIZE,
									(void *) NULL,
									1,
									&Led0Task_Handler);
		xTaskCreate(
									Led1_Task,
									"Led 1 Task",
									configMINIMAL_STACK_SIZE,
									(void *) NULL,
									1,
									&Led1Task_Handler);
		xTaskCreate(
									Led2_Task,
									"Led 2 Task",
									configMINIMAL_STACK_SIZE,
									(void *) NULL,
									1,
									&Led2Task_Handler);
#elif	TASK		==		THIRD_TASK
		xTaskCreate(
									Led_Task,
									"Led Task",
									configMINIMAL_STACK_SIZE,
									(void *) NULL,
									1,
									&LedTask_Handler);
		xTaskCreate(
									Button_Task,
									"Button Task",
									configMINIMAL_STACK_SIZE,
									(void *) NULL,
									2,
									&ButtonTask_Handler);
#endif
	
	/* Now all the tasks have been started - start the scheduler.

	NOTE : Tasks run in system mode and the scheduler runs in Supervisor mode.
	The processor MUST be in supervisor mode when vTaskStartScheduler is 
	called.  The demo applications included in the FreeRTOS.org download switch
	to supervisor mode prior to main being called.  If you are not using one of
	these demo application projects then ensure Supervisor mode is used here. */
	vTaskStartScheduler();

	/* Should never reach here!  If you do then there was not enough heap
	available for the idle task to be created. */
	for( ;; );
}
/*-----------------------------------------------------------*/

/* Function to reset timer 1 */
void timer1Reset(void)
{
	T1TCR |= 0x2;
	T1TCR &= ~0x2;
}

/* Function to initialize and start timer 1 */
static void configTimer1(void)
{
	T1PR = 1000;
	T1TCR |= 0x1;
}

static void prvSetupHardware( void )
{
	/* Perform the hardware setup required.  This is minimal as most of the
	setup is managed by the settings in the project file. */

	/* Configure UART */
	xSerialPortInitMinimal(mainCOM_TEST_BAUD_RATE);

	/* Configure GPIO */
	GPIO_init();
	
	/* Config trace timer 1 and read T1TC to get current tick */
	configTimer1();

	/* Setup the peripheral bus to be the same as the PLL output. */
	VPBDIV = mainBUS_CLK_FULL;
}
/*-----------------------------------------------------------*/


