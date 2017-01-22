/*
 * ws2812.h
 *
 *  Created on: 3 но€б. 2016 г.
 *      Author: gavrilov.iv
 */

#ifndef WS2812B_WS2812_H_
#define WS2812B_WS2812_H_

#define LED_COUNT 6
#define RAND_MAX	LED_COUNT

// These lines specify what pin the LED strip is on.
// You will either need to attach the LED strip's data line to PC0 or change these
// lines to specify a different pin.
#define LED_STRIP_PORT PORTB
#define LED_STRIP_DDR  DDRB
#define LED_STRIP_PIN  0

/** The rgb_color struct represents the color for an 8-bit RGB LED.
    Examples:
      Black:      (rgb_color){ 0, 0, 0 }
      Pure red:   (rgb_color){ 255, 0, 0 }
      Pure green: (rgb_color){ 0, 255, 0 }
      Pure blue:  (rgb_color){ 0, 0, 255 }
      White:      (rgb_color){ 255, 255, 255} */
typedef struct rgb_color
{
  unsigned char red, green, blue;
} rgb_color;

rgb_color colors[LED_COUNT];

/* Users functions */
void WS2812Init();
void WS2812Clear();
void WS2812SetRGB(uint8_t led, uint8_t r, uint8_t g, uint8_t b );
void WS2812SetHSV(uint16_t led, uint16_t hue, uint16_t saturation, uint16_t value);
void WS2812Write(rgb_color * colors, unsigned int count);

#endif /* WS2812B_WS2812_H_ */
