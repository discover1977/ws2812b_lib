/*
 * ws2812.h
 *
 *  Created on: 3 но€б. 2016 г.
 *      Author: gavrilov.iv
 */

#ifndef WS2812B_WS2812_H_
#define WS2812B_WS2812_H_


// These lines specify what pin the LED strip is on.
// You will either need to attach the LED strip's data line to PC0 or change these
// lines to specify a different pin.
#define LED_STRIP_PORT PORTC
#define LED_STRIP_DDR  DDRC
#define LED_STRIP_PIN  0


#endif /* WS2812B_WS2812_H_ */
