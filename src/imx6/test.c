/* SPDX-License-Identifier: MIT */
/*
 * Author: Andreas Werner <kernel@andy89.org>
 * Date: 2016
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <FreeRTOS.h>
#include <task.h>
#include <gpio.h>
#include <devs.h>
#include <iomux.h>
#include <uart.h>
#include <newlib_stub.h>
#include <nlibc_stub.h>
#include <irq.h>
#include <timer.h>
#include <pwm.h>
#include <mailbox_test.h>
#include <platform.h>
#include <rproctest.h>
#include <phytest.h>
#include <enettest.h>
#if defined(CONFIG_NEWLIB) || defined(CONFIG_NLIBC_PRINTF)
# define PRINTF(...) printf(__VA_ARGS__)
#else
# define PRINTF(...) 
#endif

#ifdef CONFIG_GPIO
static struct gpio *gpio = NULL;

static struct gpio_pin *ledPin = NULL;

int32_t initGPIO() {
	struct gpio *gpio2;
	gpio = gpio_init(GPIO_ID);
	if (gpio == NULL) {
		return -1;
	}
	/* Test if GET Dev work */
	gpio2  = gpio_init(1);
	if (gpio2 != NULL) {
		return -1;
	}
	userButton = gpioPin_init(gpio, PTC13, GPIO_INPUT, GPIO_OPEN);
	if (userButton == NULL) {
		return -1;
	}
	{
		int32_t ret;
		ret = gpioPin_setCallback(userButton, nucleo_userButtonISR, NULL, GPIO_EITHER);
		if (ret < 0) {
			return -1;
		}
		ret = gpioPin_enableInterrupt(userButton);
		if (ret < 0) {
			return -1;
		}
	}
	ledPin = gpioPin_init(gpio, PTA5, GPIO_OUTPUT, GPIO_PULL_UP);
	if (ledPin == NULL) {
		return -1;
	}
	
	return 0;
}
#endif

void vApplicationMallocFailedHook( void ) {
	CONFIG_ASSERT(0);
}

void vApplicationTickHook() {

}

void vApplicationStackOverflowHook( TaskHandle_t xTask, char *pcTaskName ) {
	CONFIG_ASSERT(0);
}

void vApplicationIdleHook( void ) {
}

#if (defined(CONFIG_GPIO) || defined(CONFIG_PWM)) && defined(CONFIG_INCLUDE_vTaskDelayUntil)
void ledTask(void *data) {
	bool up = true;
	uint64_t n = 10000;
# ifdef CONFIG_PWM
	struct pwm *pwm = data;
	int32_t ret;
# endif
	TickType_t waittime = 20;
	TickType_t lastWakeUpTime = xTaskGetTickCount();
	for(;;) {
		if (up) {
			if (n >= (20000 - 400)) {
				up = false;
			}
			n+=400;
		} else {
			if (n <= 400) {
				up = true;
			}
			n-=400;
		}
# ifdef CONFIG_PWM
		ret = pwm_setDutyCycle(pwm, n);
		CONFIG_ASSERT(ret == 0);
# endif
		if (n == 0 || n == 20000) {
			waittime = 1000;
# ifdef CONFIG_GPIO
			gpioPin_togglePin(ledPin);
# endif
		} else {
			waittime = 20;
		}
		vTaskDelayUntil(&lastWakeUpTime, waittime / portTICK_PERIOD_MS);
	}
}
#endif

#ifdef CONFIG_USE_STATS_FORMATTING_FUNCTIONS
void taskManTask(void *data) {
	TickType_t lastWakeUpTime = xTaskGetTickCount();
	static char taskBuff[5 * 1024];
	for(;;) {
		vTaskList(taskBuff);
		PRINTF("name\t\tState\tPrio\tStack\tTaskNr.\n");
		PRINTF("%s", taskBuff);
		PRINTF("blocked ('B'), ready ('R'), deleted ('D') or suspended ('S')\n");
#ifdef CONFIG_GENERATE_RUN_TIME_STATS
		PRINTF("name\t\tTime\t\t%%\n");
		vTaskGetRunTimeStats(taskBuff);
		PRINTF("%s", taskBuff);
#endif
		PRINTF("\n");
		vTaskDelayUntil(&lastWakeUpTime, 1000 / portTICK_PERIOD_MS);
	}
}
#endif
OS_DEFINE_TASK(taskLED, 128);
OS_DEFINE_TASK(taskMan, 512);
int main() {
	int32_t ret;
	ret = irq_init();
	CONFIG_ASSERT(ret == 0);
#if (defined(CONFIG_GPIO) || defined(CONFIG_PWM)) && defined(CONFIG_INCLUDE_vTaskDelayUntil)
	struct pwm *pwm = NULL;
#endif
#ifdef CONFIG_UART
	struct uart *uart = uart_init(UART2_ID, 115200);
#endif
#ifdef CONFIG_NEWLIB
	ret = newlib_init(uart, uart);
	CONFIG_ASSERT(ret == 0);
#endif
#ifdef CONFIG_NLIBC_PRINTF
	ret = nlibc_init(uart, uart);
	CONFIG_ASSERT(ret == 0);
#endif
	PRINTF("Init Devices\n");
#ifdef CONFIG_INSTANCE_NAME
	hal_printNames();
#endif
#ifdef CONFIG_GPIO
	ret = initGPIO();
	CONFIG_ASSERT(ret == 0);
#endif
#if (defined(CONFIG_GPIO) || defined(CONFIG_PWM)) && defined(CONFIG_INCLUDE_vTaskDelayUntil)
	ret = OS_CREATE_TASK(ledTask, "LED Task", 128, pwm, 1, taskLED);
	CONFIG_ASSERT(ret == pdPASS);
#endif
#ifdef CONFIG_USE_STATS_FORMATTING_FUNCTIONS
	ret = OS_CREATE_TASK(taskManTask, "Task Manager Task", 512, NULL, 1, taskMan);
	CONFIG_ASSERT(ret == pdPASS);
#endif
#ifdef CONFIG_MAILBOX_TEST
	mailbox_test();
#endif
#ifdef CONFIG_OPEN_AMP
	platform_init();
#endif
#ifdef CONFIG_RPROC_TEST
	rprocTest_init();
#endif
#ifdef CONFIG_PHY_TEST
	phytest_init();
#endif
#ifdef CONFIG_ENET_TEST
	enettest_init();
#endif
	PRINTF("Start Scheduler\n");
	vTaskStartScheduler ();
	for(;;);
	return 0;
}
