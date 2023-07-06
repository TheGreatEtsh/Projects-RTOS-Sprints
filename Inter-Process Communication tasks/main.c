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
#include "semphr.h"
#include "event_groups.h"

/* Peripheral includes. */
#include "serial.h"
#include "GPIO.h"


/*-----------------------------------------------------------*/

/* Constants to setup I/O and processor. */
#define mainBUS_CLK_FULL	( ( unsigned char ) 0x01 )

/* Constants for the ComTest demo application tasks. */
#define mainCOM_TEST_BAUD_RATE	( ( unsigned long ) 115200 )


#define BIT_0	( 1 << 0 )
#define BIT_1	( 1 << 1 )
#define BIT_2 ( 1 << 2 )
#define BIT_3 ( 1 << 3 )
#define BIT_4	( 1 << 4 )
#define TOGGLE_OFF  	0
#define TOGGLE_FAST		1
#define TOGGLE_SLOW		2
#define FIRST_TASK		0
#define	SECOND_TASK		1
#define THIRD_TASK		2
#define TAKEN					0
#define RELEASED			1
#define TASK					THIRD_TASK


#if		TASK 		==		FIRST_TASK
/*Initializing tasks and semaphores handles used*/
TaskHandle_t led_task_handler = NULL;
TaskHandle_t button_task_handler = NULL;
SemaphoreHandle_t	user_input;
#elif	TASK		==		SECOND_TASK

TaskHandle_t uart_0_task_handler = NULL;
TaskHandle_t uart_1_task_handler = NULL;

SemaphoreHandle_t uart_mutex;

#elif	TASK		==		THIRD_TASK
TaskHandle_t button_0_task_handler = NULL;
TaskHandle_t button_1_task_handler = NULL;
TaskHandle_t uart_0_task_handler = NULL;
TaskHandle_t uart_1_task_handler = NULL;

SemaphoreHandle_t uart_mutex;

EventGroupHandle_t print_state_handle;

pinState_t gl_new_button_state_0 = PIN_IS_LOW;
pinState_t gl_new_button_state_1 = PIN_IS_LOW;
pinState_t gl_past_button_state_0 = PIN_IS_LOW; 
pinState_t gl_past_button_state_1 = PIN_IS_LOW;
#endif

/*
 * Configure the processor for use with the Keil demo board.  This is very
 * minimal as most of the setup is managed by the settings in the project
 * file.
 */
static void prvSetupHardware( void );
/*-----------------------------------------------------------*/
#if		TASK 		==		FIRST_TASK
void led_task (void * pvParameters)
{
	for( ; ; )
	{
		vTaskDelay(1);
		/*This task will only toggle the led if the user_input semaphore is released*/
		if( xSemaphoreTake( user_input, ( TickType_t ) 0) == pdTRUE )
		{
			GPIO_toggle(PORT_0, PIN1);
			xSemaphoreGive( user_input );
			vTaskDelay(20);
		}
	}
}

void button_task (void * pvParameters)
{
	/*Counting variable used to know if the button is still pressed or not*/
	static int counting_variable = 0;
	/*variable checks if the semaphore is taken or not*/
	static char semaphore_taken	= RELEASED;
	/*Variable used to save pin's level*/
	static pinState_t button_state = PIN_IS_LOW;
	for( ; ; )
	{
		/*
		 *	if semaphore is not taken, it takes the semaphore for only once, 
		 *	as to not aqquires the semaphore more than once while it's already taken
		 */
		if (semaphore_taken == RELEASED)
		{
			if(xSemaphoreTake( user_input , (TickType_t) 0) == pdTRUE)
			{
				semaphore_taken = TAKEN;
			}
		}
		
		/*Read input pin state*/
		button_state = GPIO_read(PORT_0, PIN0);
		
		/*checks on pin's state*/
		if(button_state == PIN_IS_HIGH)
		{
			counting_variable++;
		}
		else
		{
			/*
			 *	if it's low check if the counting variable is not equal zero
			 *	TRUE : means the pin was high then turned low
			 * 	FALSE : means there this no change happened on the pin
			 */
			if (counting_variable != 0)
			{
				xSemaphoreGive ( user_input );
				counting_variable = 0;
				semaphore_taken = RELEASED;
			}
		}
		vTaskDelay (20);
	}
}
#elif	TASK		==		SECOND_TASK
void uart_0_task (void * pvParameters)
{
	/*counting variable used in for loop*/
	char counting_variable = 0;
	for( ; ; )
	{
		/*Delay 100 ms*/
		vTaskDelay(100);
		/*if mutex is released, print on uart 10 times then release the mutex*/
		if(xSemaphoreTake( uart_mutex , (TickType_t) 0) == pdTRUE)
		{
			for(counting_variable = 0; counting_variable < 10; counting_variable++)
			{
				while (vSerialPutString("\n FIRST TASK IS SENDING A STRING\0", 35) == pdFALSE);
			}
			xSemaphoreGive(uart_mutex);
		}
			
	}
}

void uart_1_task (void * pvParameters)
{
	/*counting variable used in for loop*/
	char counting_variable = 0;
	/*counting variable used in for load loop*/
	int load = 0;
	for( ; ; )
	{
		/*Delay 500 ms*/
		vTaskDelay(500);
		
		/*
		 * if mutex is released, print on uart 10 times 
		 * and loop 100000 with every print 
		 * then release the mutex
		 */
		if(xSemaphoreTake( uart_mutex , (TickType_t) 0) == pdTRUE)
		{
			for(counting_variable = 0; counting_variable < 10; counting_variable++)
			{
				/*load 100000 loop*/
				for(load = 0; load < 100000; load++)
				{
					if (load == 99999)
					{
						while(vSerialPutString("\n SECOND TASK IS SENDING A STRING\0", 35) == pdFALSE);
					}
				}
			}
			
			xSemaphoreGive(uart_mutex);
		}
	}
}	

#elif	TASK		==		THIRD_TASK

/**
 * @brief This task is used to check on event flag 
 *				to print a specific msg about button's status
 */
void uart_0_task (void * pvParameters)
{
	/*counting variable used in for loop*/
	EventBits_t print_state;
	for( ; ; )
	{
		/*Delay 100 ms*/
		vTaskDelay(100);
		
		/*Saving print_state event group status in a local Event Bits variable*/
		print_state = xEventGroupWaitBits(
			print_state_handle,
			BIT_0 | BIT_1 | BIT_2 | BIT_3,
			pdTRUE,
			pdFALSE,
			0 );
		
		/*Take the mutex if it's released*/
		if(xSemaphoreTake( uart_mutex , (TickType_t) 0) == pdTRUE)
		{
			/*Print a msg on UART crossponding to the flag  number*/
			if(print_state == BIT_0)
			{
				print_state = 0;
				while(vSerialPutString("\n\n Button 1 is Released\n Button 2 is Released\n", 50) == pdFALSE);
			}
			else if(print_state == BIT_1)
			{
				print_state = 0;
				while(vSerialPutString("\n\n Button 1 is Released\n Button 2 is Pressed\n", 50) == pdFALSE);
			}
			else if(print_state == BIT_2)
			{
				print_state = 0;
				while(vSerialPutString("\n\n Button 1 is Pressed\n Button 2 is Released\n", 50) == pdFALSE);
			}
			else if(print_state == BIT_3)
			{
				print_state = 0;
				while(vSerialPutString("\n\n Button 1 is Pressed\n Button 2 is Pressed\n" , 45) == pdFALSE);
			}
			/*Release the mutex*/
			xSemaphoreGive(uart_mutex);
		}
	}
}

void uart_1_task (void * pvParameters)
{
	for( ; ; )
	{
		/*Delay 100 ms*/
		vTaskDelay(100);
		/*if mutex is released, take the mutex, print on uart then release the mutex*/
		if(xSemaphoreTake( uart_mutex , (TickType_t) 0) == pdTRUE)
		{
			while (vSerialPutString("\n A TASK IS SENDING A STRING", 30) == pdFALSE);
			xSemaphoreGive(uart_mutex);
		}
			
	}
}

void button_0_task (void * pvParameters)
{
	for( ; ; )
	{
		/*Read button's state*/
		gl_new_button_state_0 = GPIO_read(PORT_0, PIN0);
		
		/*Check if the new state is changed or not*/
		if (gl_new_button_state_0 != gl_past_button_state_0)
		{
			/*Check on button's state and raise a crossponding flag*/
			if (gl_new_button_state_0 == PIN_IS_LOW && gl_new_button_state_1 == PIN_IS_LOW)
			{
				xEventGroupSetBits(print_state_handle, BIT_0);
			}
			else if (gl_new_button_state_0 == PIN_IS_LOW && gl_new_button_state_1 == PIN_IS_HIGH)
			{
				xEventGroupSetBits(print_state_handle, BIT_1);
			}
			else if (gl_new_button_state_0 == PIN_IS_HIGH && gl_new_button_state_1 == PIN_IS_LOW)
			{
				xEventGroupSetBits(print_state_handle, BIT_2);
			}
			else if (gl_new_button_state_0 == PIN_IS_HIGH && gl_new_button_state_1 == PIN_IS_HIGH)
			{
				xEventGroupSetBits(print_state_handle, BIT_3);
			}
			/*update the past state for the button*/
			gl_past_button_state_0 = gl_new_button_state_0;
		}
		
		vTaskDelay (20);

	}
}


void button_1_task (void * pvParameters)
{
	for( ; ; )
	{
		/*Read button's state*/
		gl_new_button_state_1 = GPIO_read(PORT_0, PIN1);
		
		/*Check if the new state is changed or not*/
		if (gl_new_button_state_1 != gl_past_button_state_1)
		{
			/*Check on button's state and raise a crossponding flag*/
			if (gl_new_button_state_0 == PIN_IS_LOW && gl_new_button_state_1 == PIN_IS_LOW)
			{
				xEventGroupSetBits(print_state_handle, BIT_0);
			}
			else if (gl_new_button_state_0 == PIN_IS_LOW && gl_new_button_state_1 == PIN_IS_HIGH)
			{
				xEventGroupSetBits(print_state_handle, BIT_1);
			}
			else if (gl_new_button_state_0 == PIN_IS_HIGH && gl_new_button_state_1 == PIN_IS_LOW)
			{
				xEventGroupSetBits(print_state_handle, BIT_2);
			}
			else if (gl_new_button_state_0 == PIN_IS_HIGH && gl_new_button_state_1 == PIN_IS_HIGH)
			{
				xEventGroupSetBits(print_state_handle, BIT_3);
			}
			
			/*update the past state for the button*/
			gl_past_button_state_1 = gl_new_button_state_1;
		}
		
		vTaskDelay (20);
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
		/*Create Semaphores/Mutex here*/
		user_input = xSemaphoreCreateBinary();
	
    /* Create Tasks here */
		xTaskCreate(
									led_task,
									"Led_Task",
									configMINIMAL_STACK_SIZE,
									(void *) NULL,
									2,
									&led_task_handler);
	
		xTaskCreate(
									button_task,
									"Button Task",
									configMINIMAL_STACK_SIZE,
									(void *) NULL,
									1,
									&button_task_handler);
									
#elif	TASK		==		SECOND_TASK
		/*Create Semaphores/Mutex here*/
		uart_mutex = xSemaphoreCreateMutex();
    
		if(uart_mutex != NULL)
		{
			/* Create Tasks here */
			xTaskCreate(
										uart_0_task,
										"UART 0 Task",
										configMINIMAL_STACK_SIZE,
										(void *) NULL,
										2,
										&uart_0_task_handler);
										
			xTaskCreate(
										uart_1_task,
										"UART 1 Task",
										configMINIMAL_STACK_SIZE,
										(void *) NULL,
										1,
										&uart_1_task_handler);
		
		}
									
#elif	TASK		==		THIRD_TASK
		print_state_handle = xEventGroupCreate();
		uart_mutex = xSemaphoreCreateMutex();

    /* Was the event group created successfully? */
    if( print_state_handle != NULL && uart_mutex != NULL)
    {
        xTaskCreate(
											uart_0_task,
											"UART 0 Task",
											configMINIMAL_STACK_SIZE,
											(void *) NULL,
											2,
											&uart_0_task_handler);
										
				xTaskCreate(
											uart_1_task,
											"UART 1 Task",
											configMINIMAL_STACK_SIZE,
											(void *) NULL,
											1,
											&uart_1_task_handler);
				xTaskCreate(
											button_0_task,
											"Button 0 Task",
											configMINIMAL_STACK_SIZE,
											(void *) NULL,
											3,
											&button_0_task_handler);
				xTaskCreate(
											button_1_task,
											"Button 1 Task",
											configMINIMAL_STACK_SIZE,
											(void *) NULL,
											3,
											&button_1_task_handler);
		
    }

		
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


