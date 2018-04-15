/*
 * Copyright (c) 2016 Andreas Werner <kernel@andy89.org>
 * 
 * Permission is hereby granted, free of charge, to any person 
 * obtaining a copy of this software and associated documentation 
 * files (the "Software"), to deal in the Software without restriction, 
 * including without limitation the rights to use, copy, modify, merge, 
 * publish, distribute, sublicense, and/or sell copies of the Software, 
 * and to permit persons to whom the Software is furnished to do so, 
 * subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included 
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS 
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS 
 * IN THE SOFTWARE.
 */
#include <FreeRTOS.h>
#include <task.h>
#include <task.h>
#include <timer.h>
#include <gpio.h>
#include <devs.h>
#include <pwm.h>

struct timer *timer;
#ifndef CONFIG_TIMER_TEST_PWM
struct gpio_pin **rgb;
#else
struct pwm *pwm[3];
#endif
uint32_t rgbPins = BIT(0);
uint32_t state = 0x0;
uint64_t n = 10000;
bool up = true;
bool on = true;

static bool irqhandle(struct timer *timer, void *data) {
	(void) data;
#ifndef CONFIG_TIMER_TEST_PWM
	if (on) {
		if (rgbPins & BIT(0)) {
			gpioPin_setPin(rgb[0]);
		}
		if (rgbPins & BIT(1)) {
			gpioPin_setPin(rgb[1]);
		}
		if (rgbPins & BIT(2)) {
			gpioPin_setPin(rgb[2]);
		}
	} else {
		if (rgbPins & BIT(0)) {
			gpioPin_clearPin(rgb[0]);
		}
		if (rgbPins & BIT(1)) {
			gpioPin_clearPin(rgb[1]);
		}
		if (rgbPins & BIT(2)) {
			gpioPin_clearPin(rgb[2]);
		}
	}
#endif
	if (on) {
		if (up) {
			n+=100;
		} else {
			n-=100;
		}
		if (n >= 19800) {
			printf("down\n");
			up = false;
			if (state == 7) {
				state = 0;
			}
			switch (state++) {
				case 0:
					rgbPins = BIT(0);
					break;
				case 1:
					rgbPins = BIT(1);
					break;
				case 2:
					rgbPins = BIT(2);
					break;
				case 3:
					rgbPins = BIT(0) | BIT(1);
					break;
				case 4:
					rgbPins = BIT(1) | BIT(2);
					break;
				case 5:
					rgbPins = BIT(2) | BIT(0);
					break;
				case 6:
					rgbPins = BIT(0) | BIT(1) | BIT(2);
					break;
			}
		} else if (n <= 16000) {
		/*} else if (n <= 10000) {
		} else if (n <= 100) {*/
			printf("up\n");
			up = true;
		}
	}
#ifndef CONFIG_TIMER_TEST_PWM
# ifdef CONFIG_TIMER_TEST_ONESHOT
	if (on) {
		CONFIG_ASSERT(timer_oneshot(timer, n) == 0);
	} else {
		CONFIG_ASSERT(timer_oneshot(timer, 20000 - n) == 0);
	}
# else
	if (on) {
		CONFIG_ASSERT(timer_periodic(timer, n) == 0);
	} else {
		CONFIG_ASSERT(timer_periodic(timer, 20000 - n) == 0);
	}
# endif
	on = !on;
#else
	if (rgbPins & BIT(0)) {
		pwm_setDutyCycle(pwm[0], n);
	} else {
		pwm_setDutyCycle(pwm[0], 20000);
	}
	if (rgbPins & BIT(1)) {
		pwm_setDutyCycle(pwm[1], n);
	} else {
		pwm_setDutyCycle(pwm[1], 20000);
	}
	if (rgbPins & BIT(2)) {
		pwm_setDutyCycle(pwm[2], n);
	} else {
		pwm_setDutyCycle(pwm[2], 20000);
	}
#endif
	return false;
}
int32_t timertest_init(struct gpio_pin **rgbPins) {
	int32_t ret;
#ifndef CONFIG_TIMER_TEST_PWM
	rgb = rgbPins;
#else
	(void) rgbPins;
#endif
	timer = timer_init(FLEXTIMER0_ID, 128, 20000, 700);
	CONFIG_ASSERT(timer != NULL);
	ret = timer_setOverflowCallback(timer, &irqhandle, NULL);
	CONFIG_ASSERT(ret == 0);
#ifndef CONFIG_TIMER_TEST_PWM
	gpioPin_setPin(rgb[0]);
	gpioPin_setPin(rgb[1]);
	gpioPin_setPin(rgb[2]);
#else
	pwm[0] = pwm_init(FLEXTIMER0_PWM0_PTD15_ID);
	CONFIG_ASSERT(pwm[0]);
	pwm[1] = pwm_init(FLEXTIMER0_PWM1_PTD16_ID);
	CONFIG_ASSERT(pwm[1]);
	pwm[2] = pwm_init(FLEXTIMER0_PWM2_PTD0_ID);
	CONFIG_ASSERT(pwm[2]);
#endif
#ifndef CONFIG_TIMER_TEST_PWM
# ifdef CONFIG_TIMER_TEST_ONESHOT
	CONFIG_ASSERT(timer_oneshot(timer, n) == 0);
# else
	CONFIG_ASSERT(timer_periodic(timer, n) == 0);
# endif
#else
	CONFIG_ASSERT(pwm_setPeriod(pwm[0], 20000) == 0);
#endif
	return 0;
}

