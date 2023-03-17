#include <stdio.h>
#include <stdlib.h>
#include "diag/trace.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include "semphr.h"
#include "time.h"

#define CCM_RAM __attribute__((section(".ccmram")))



// initializing global variables
int LowerBoundarr [] = {50, 80, 110, 140, 170, 200};
int HigherBoundarr []= {150, 200, 250, 300, 350, 400};
int i=-1; 				//index for the two above arrays to determine when the two arrays end
int sender1_period; 	//store period of sender1
int sender2_period; 	//store period of sender2
int receiver_period;	//store period of the receiver
int sent1_msg =0;		//counter to calculate the number of sent messages from sender1
int sent2_msg =0;		//counter to calculate the number of sent messages from sender2
int received_msg =0;	//counter to calculate the number of received messages at the receiver
int blocked1_msg =0;	//counter to calculate the number of blocked messages from sender1
int blocked2_msg =0;	//counter to calculate the number of blocked messages from sender2

//Global variables to calculate average time period
int countersend1 = 0;	//counter to calculate the number of time periods for sender1
int countersend2 = 0;	//counter to calculate the number of time periods for sender2
int timercount1= 0;		//variable to store the sum of periods for sender1
int timercount2=0;		//variable to store the sum of periods for sender2



//Global Tasks, Queue, Timers and Semaphores handle
TaskHandle_t senderhand1 =0;
TaskHandle_t senderhand2 =0;
TaskHandle_t receiverhand =0;

QueueHandle_t BuffQueue =0;

TimerHandle_t TimerFunction1 =0;
TimerHandle_t TimerFunction2 =0;
TimerHandle_t TimerFunction3 =0;

SemaphoreHandle_t semasend1;
SemaphoreHandle_t semasend2;
SemaphoreHandle_t semarec3;



//Sender1 Task function
void Sender1task()
{
	char myTx [30];
	BaseType_t txstatus1;

	while(1)
	{
		xSemaphoreTake(semasend1, 0xFFFFFFFF);
		int timer= xTaskGetTickCount();
		//adding the int timer to the string in one variable myTx
		snprintf(myTx, 30, "Time is %d", timer);
		txstatus1 = xQueueSend( BuffQueue, &myTx, 0 );
		if ( txstatus1 != pdPASS )
		{
			blocked1_msg++;
			//printf( "Could not send to the queue.\r\n");
		}
		else sent1_msg++; //sent successfully

	}
}

//Sender2 Task function
void Sender2task()
{
	char myTx [30];
	BaseType_t txstatus2;


	while(1)
	{
		xSemaphoreTake(semasend2, 0xFFFFFFFF);
		int timer= xTaskGetTickCount();
		//adding the int timer to the string in one variable myTx
		snprintf(myTx, 30, "Time is %d", timer);
		txstatus2 = xQueueSend( BuffQueue, &myTx, 0 );
		if ( txstatus2 != pdPASS )
		{
			blocked2_msg++;
			//printf( "Could not send to the queue.\r\n");
		}
		else sent2_msg++; //sent successfully
	}

}

//Receiver Task function
void ReceiverTask()
{
	char myRx[30];
	BaseType_t rxstatus;

	while(1)
	{
		xSemaphoreTake(semarec3, 0xFFFFFFFF);
		rxstatus = xQueueReceive( BuffQueue, &myRx, 0 );

		if( rxstatus == pdPASS )
		{
			received_msg++;//sent successfully
			printf( "Received = %s \r\n" , myRx );
		}
		//else printf( "Could not receive from the queue.\r\n" );

	}

}

//Callback Function of the first timer that starts sender1 task
void Tsender1TimerCallback( TimerHandle_t TimerFunction1 )
{
	xSemaphoreGive(semasend1);
	sender1_period = UniformlyDistributedTimer (LowerBoundarr[i] , HigherBoundarr[i]);
	xTimerChangePeriod(TimerFunction1, pdMS_TO_TICKS(sender1_period), 0);

	timercount1 += sender1_period;
	countersend1++;

}

//Callback Function of the second timer that starts sender2 task
void Tsender2TimerCallback( TimerHandle_t TimerFunction2 )
{
	xSemaphoreGive(semasend2);
	sender2_period = UniformlyDistributedTimer (LowerBoundarr[i] , HigherBoundarr[i]);
	xTimerChangePeriod(TimerFunction2, pdMS_TO_TICKS(sender2_period), 0);

	timercount2 += sender2_period;
	countersend2++;
}

//Callback Function of the third timer that starts receiver task
void TreceiverTimerCallback( TimerHandle_t TimerFunction3 )
{
	xSemaphoreGive(semarec3);
	if (received_msg == 500)
		reset();
}

//random number generation function between two specific values
int UniformlyDistributedTimer (int min , int max )
{
	srand(time(0));
	int myRand = (int)rand();
	int range = max - min + 1;
	int myRand_scaled = (myRand % range) + min;
	return myRand_scaled;
}

//function to clear queues, set sender1, sender2 and receiver periodic time and end the scheduler after 6 iterations
void intializingfunc ()
{

	xQueueReset( BuffQueue );
	i++;
	if (i == 6)
	{
		xTimerDelete(TimerFunction1,0);
		xTimerDelete(TimerFunction2,0);
		xTimerDelete(TimerFunction3,0);
		printf("Game Over. \r\n");
		vTaskEndScheduler ();
	}

	sender1_period = UniformlyDistributedTimer (LowerBoundarr[i] , HigherBoundarr[i]);
	sender2_period = UniformlyDistributedTimer (LowerBoundarr[i] , HigherBoundarr[i]);
	receiver_period = 100;

}

//reset function to print the final output of each iteration after 500 message received and clear any stored values to start a new one
void reset()
{
	int Totalsent=  sent1_msg+sent2_msg;
	int TotalBlocked= blocked1_msg+blocked2_msg;
	printf("Successfully sent messages = %d \r\n" , Totalsent );
	printf("Successfully blocked messages = %d \r\n" , TotalBlocked );
	sent1_msg = 0;
	sent2_msg = 0;
	received_msg = 0;
	blocked1_msg = 0;
	blocked2_msg = 0;

	int avgtime;
	avgtime = ((timercount2 + timercount1)/ (countersend1 + countersend2));
	printf( "Average time for period %d is %d \r\n" , i+1 , avgtime );
	avgtime =0;
	timercount1 = 0;
	timercount2 = 0;
	countersend1 =0;
	countersend2 =0;

	intializingfunc ();
}



// ----------------------------------------------------------------------------
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wmissing-declarations"
#pragma GCC diagnostic ignored "-Wreturn-type"
// ----------------------------------------------------------------------------



void main()
{
	//creating queue
	char mydata[30];
	BuffQueue = xQueueCreate( 2 , sizeof( mydata ) );

	//creating semaphores
	semasend1 = xSemaphoreCreateBinary();
	semasend2 = xSemaphoreCreateBinary();
	semarec3 = xSemaphoreCreateBinary();

	//initialize the program
	intializingfunc ();

	//create timers
	TimerFunction1 = xTimerCreate( "Tsender1", pdMS_TO_TICKS(sender1_period), pdTRUE, ( void * ) 0, Tsender1TimerCallback);
	TimerFunction2 = xTimerCreate( "Tsender2", pdMS_TO_TICKS(sender2_period), pdTRUE, ( void * ) 0, Tsender2TimerCallback);
	TimerFunction3 = xTimerCreate( "Treceiver", pdMS_TO_TICKS(receiver_period), pdTRUE, ( void * ) 0, TreceiverTimerCallback);

	if( BuffQueue != NULL )
	{
		//create tasks
		xTaskCreate( Sender1task, "Sender1", 1024,( void * ) 0, 1, &senderhand1 );
		xTaskCreate( Sender2task, "Sender2", 1024, ( void * ) 0, 1, &senderhand2 );
		xTaskCreate( ReceiverTask, "Receiver", 1024, NULL, 2, &receiverhand );

	}
	else printf("Queue could not be created. \r\n");

	BaseType_t timerStarted1;
	BaseType_t timerStarted2;
	BaseType_t timerStarted3;

	if( TimerFunction1 != NULL && TimerFunction2 != NULL && TimerFunction3 != NULL)
	{
		timerStarted1 = xTimerStart(TimerFunction1, 0 );
		timerStarted2 = xTimerStart(TimerFunction2, 0 );
		timerStarted3 = xTimerStart(TimerFunction3, 0 );
	}

	if( timerStarted1 == pdPASS && timerStarted2 == pdPASS && timerStarted3 == pdPASS)
	{
		//start the scheduler
		printf("Start Scheduler. \r\n");
		vTaskStartScheduler();
	}

}



// ----------------------------------------------------------------------------
#pragma GCC diagnostic pop
// ----------------------------------------------------------------------------

void vApplicationMallocFailedHook( void )
{
	/* Called if a call to pvPortMalloc() fails because there is insufficient
	free memory available in the FreeRTOS heap.  pvPortMalloc() is called
	internally by FreeRTOS API functions that create tasks, queues, software
	timers, and semaphores.  The size of the FreeRTOS heap is set by the
	configTOTAL_HEAP_SIZE configuration constant in FreeRTOSConfig.h. */
	for( ;; );
}
/*-----------------------------------------------------------*/

void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName )
{
	( void ) pcTaskName;
	( void ) pxTask;

	/* Run time stack overflow checking is performed if
	configconfigCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
	function is called if a stack overflow is detected. */
	for( ;; );
}
/*-----------------------------------------------------------*/

void vApplicationIdleHook( void )
{
	volatile size_t xFreeStackSpace;

	/* This function is called on each cycle of the idle task.  In this case it
	does nothing useful, other than report the amout of FreeRTOS heap that
	remains unallocated. */
	xFreeStackSpace = xPortGetFreeHeapSize();

	if( xFreeStackSpace > 100 )
	{
		/* By now, the kernel has allocated everything it is going to, so
		if there is a lot of heap remaining unallocated then
		the value of configTOTAL_HEAP_SIZE in FreeRTOSConfig.h can be
		reduced accordingly. */
	}
}

void vApplicationTickHook(void) {}

StaticTask_t xIdleTaskTCB CCM_RAM;
StackType_t uxIdleTaskStack[configMINIMAL_STACK_SIZE] CCM_RAM;

void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize)
{
	/* Pass out a pointer to the StaticTask_t structure in which the Idle task's
  state will be stored. */
	*ppxIdleTaskTCBBuffer = &xIdleTaskTCB;

	/* Pass out the array that will be used as the Idle task's stack. */
	*ppxIdleTaskStackBuffer = uxIdleTaskStack;

	/* Pass out the size of the array pointed to by *ppxIdleTaskStackBuffer.
  Note that, as the array is necessarily of type StackType_t,
  configMINIMAL_STACK_SIZE is specified in words, not bytes. */
	*pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

static StaticTask_t xTimerTaskTCB CCM_RAM;
static StackType_t uxTimerTaskStack[configTIMER_TASK_STACK_DEPTH] CCM_RAM;

/* configUSE_STATIC_ALLOCATION and configUSE_TIMERS are both set to 1, so the
application must provide an implementation of vApplicationGetTimerTaskMemory()
to provide the memory that is used by the Timer service task. */

void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize)
{
	*ppxTimerTaskTCBBuffer = &xTimerTaskTCB;
	*ppxTimerTaskStackBuffer = uxTimerTaskStack;
	*pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}
