/* This is AVR code for driving the RGB LED strips from Pololu.
   It allows complete control over the color of an arbitrary number of LEDs.
   This implementation disables interrupts while it does bit-banging with inline assembly.
 */

/* This line specifies the frequency your AVR is running at.
   This code supports 20 MHz, 16 MHz and 8MHz */
//#define F_CPU 20000000

// These lines specify what pin the LED strip is on.
// You will either need to attach the LED strip's data line to PC0 or change these
// lines to specify a different pin.
#define LED_STRIP_PORT PORTC
#define LED_STRIP_DDR  DDRC
#define LED_STRIP_PIN  0

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>
#include "bits_macros.h"

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

#define LED_COUNT 30
rgb_color colors[LED_COUNT];
#define RAND_MAX	LED_COUNT

WS2812Clear()
{
	for (int i = 0; i < LED_COUNT; i++) {
		colors[i] = (rgb_color){0, 0, 0};
	}
}

void WS2812SetRGB(uint8_t led, uint8_t r, uint8_t g, uint8_t b )
{
	colors[led] = (rgb_color){ r, g, b };
}

void WS2812SetHSV(uint16_t led, uint16_t hue, uint16_t saturation, uint16_t value)
{
	if(hue < 1536 && saturation < 256 && value < 256)
	{
		uint8_t red, green, blue;
		uint8_t min, max, inc, dec, hquot, hrem;

		if(saturation == 0)
		{
			colors[led] = (rgb_color){value, value, value};
			return;
		}

		hquot = hue / 256;
		hrem = hue % 256;

		max = value;
		min = (value * (255 - saturation)) / 255;
		inc = (value * ((saturation * hrem) / 255)) / 255;
		dec = (value * ((saturation * (255-hrem)) / 255)) / 255;


		switch (hquot)
		{
		case 0:
			red = max;
			green = inc;
			blue = min;
			break;
		case 1:
			red = dec;
			green = max;
			blue = min;
			break;
		case 2:
			red = min;
			green = max;
			blue = inc;
			break;
		case 3:
			red = min;
			green = dec;
			blue = max;
			break;
		case 4:
			red = inc;
			green = min;
			blue = max;
			break;
		case 5:
			red = max;
			green = min;
			blue = dec;
			break;
		}
		colors[led] = (rgb_color){red, green, blue};
	}
}

/** led_strip_write sends a series of colors to the LED strip, updating the LEDs.
 The colors parameter should point to an array of rgb_color structs that hold the colors to send.
 The count parameter is the number of colors to send.

 This function takes about 1.1 ms to update 30 LEDs.
 Interrupts must be disabled during that time, so any interrupt-based library
 can be negatively affected by this function.
 
 Timing details at 20 MHz (the numbers slightly different at 16 MHz and 8MHz):
  0 pulse  = 400 ns
  1 pulse  = 850 ns
  "period" = 1300 ns
 */
void __attribute__((noinline)) led_strip_write(rgb_color * colors, unsigned int count) 
{
  // Set the pin to be an output driving low.
  LED_STRIP_PORT &= ~(1<<LED_STRIP_PIN);
  LED_STRIP_DDR |= (1<<LED_STRIP_PIN);

  cli();   // Disable interrupts temporarily because we don't want our pulse timing to be messed up.
  while(count--)
  {
    // Send a color to the LED strip.
    // The assembly below also increments the 'colors' pointer,
    // it will be pointing to the next color at the end of this loop.
    asm volatile(
        "ld __tmp_reg__, %a0+\n"
        "ld __tmp_reg__, %a0\n"
        "rcall send_led_strip_byte%=\n"  // Send red component.
        "ld __tmp_reg__, -%a0\n"
        "rcall send_led_strip_byte%=\n"  // Send green component.
        "ld __tmp_reg__, %a0+\n"
        "ld __tmp_reg__, %a0+\n"
        "ld __tmp_reg__, %a0+\n"
        "rcall send_led_strip_byte%=\n"  // Send blue component.
        "rjmp led_strip_asm_end%=\n"     // Jump past the assembly subroutines.

        // send_led_strip_byte subroutine:  Sends a byte to the LED strip.
        "send_led_strip_byte%=:\n"
        "rcall send_led_strip_bit%=\n"  // Send most-significant bit (bit 7).
        "rcall send_led_strip_bit%=\n"
        "rcall send_led_strip_bit%=\n"
        "rcall send_led_strip_bit%=\n"
        "rcall send_led_strip_bit%=\n"
        "rcall send_led_strip_bit%=\n"
        "rcall send_led_strip_bit%=\n"
        "rcall send_led_strip_bit%=\n"  // Send least-significant bit (bit 0).
        "ret\n"

        // send_led_strip_bit subroutine:  Sends single bit to the LED strip by driving the data line
        // high for some time.  The amount of time the line is high depends on whether the bit is 0 or 1,
        // but this function always takes the same time (2 us).
        "send_led_strip_bit%=:\n"
#if F_CPU == 8000000
        "rol __tmp_reg__\n"                      // Rotate left through carry.
#endif
        "sbi %2, %3\n"                           // Drive the line high.

#if F_CPU != 8000000
        "rol __tmp_reg__\n"                      // Rotate left through carry.
#endif

#if F_CPU == 16000000
        "nop\n" "nop\n"
#elif F_CPU == 20000000
        "nop\n" "nop\n" "nop\n" "nop\n"
#elif F_CPU != 8000000
#error "Unsupported F_CPU"
#endif

        "brcs .+2\n" "cbi %2, %3\n"              // If the bit to send is 0, drive the line low now.

#if F_CPU == 8000000
        "nop\n" "nop\n"
#elif F_CPU == 16000000
        "nop\n" "nop\n" "nop\n" "nop\n" "nop\n"
#elif F_CPU == 20000000
        "nop\n" "nop\n" "nop\n" "nop\n" "nop\n"
        "nop\n" "nop\n"
#endif

        "brcc .+2\n" "cbi %2, %3\n"              // If the bit to send is 1, drive the line low now.

        "ret\n"
        "led_strip_asm_end%=: "
        : "=b" (colors)
        : "0" (colors),         // %a0 points to the next color to display
          "I" (_SFR_IO_ADDR(LED_STRIP_PORT)),   // %2 is the port register (e.g. PORTC)
          "I" (LED_STRIP_PIN)     // %3 is the pin number (0-8)
    );

    // Uncomment the line below to temporarily enable interrupts between each color.
    //sei(); asm volatile("nop\n"); cli();
  }
  sei();          // Re-enable interrupts now that we are done.
  _delay_us(50);  // Hold the line low for 15 microseconds to send the reset signal.
}

int main()
{
	unsigned int time = 0;
	int effect = 4;

	SetBit(DDRC, 1);
	ClearBit(PORTC, 1);

	SetBit(PORTD, 2);

	while(1)
	{
		if (BitIsClear(PIND, 2)) {
			while(BitIsClear(PIND, 2));
			if ( ++effect == 8 ) effect = 0;
		}

		switch(effect)
				{
				case 0:;
					for(char j = 0; j < LED_COUNT; j++)
					{
						for(char i = 0; i < LED_COUNT; i++)
						{
							if (i==j) WS2812SetHSV(i, i*(1535/LED_COUNT+1), 255, 255);
							else {
								WS2812SetRGB(i, 0, 0, 0);
							}
						}
						led_strip_write(colors, LED_COUNT);
						_delay_ms(25);
					}
					break;
				case 1:;
					for(char l = 0; l < 2; l++)
					{
						for(char k = 0; k < 3; k++)
						{
							for(char j = 0; j < 8; j++)
							{
								for(char i = 0; i < LED_COUNT; i++)
								{
									if (j == 1 && i < (( l + 1 ) * LED_COUNT / 2) && i >= (l * LED_COUNT / 2) ) {
										WS2812SetRGB(i, (l == 0)?(255):(0), 0, (l == 1)?(255):(0));
									}
									else WS2812SetRGB(i, 0, 0, 0);
								}
								led_strip_write(colors, LED_COUNT);
								_delay_ms(5);
							}
						}
						_delay_ms(100);
					}
					break;
				case 2:;
					uint8_t level = 0;
					for(char l = 0; l < 2; l++)
					{
						for(char j = 0; j < LED_COUNT; j++)
						{
							for(char i = 0; i < LED_COUNT; i++)
							{
								if (i==j) level = 255;
								else if ((i + 1 == j) || (i - 1 == j)) level = 127;
								else if ((i + 2 == j) || (i - 2 == j)) level = 63;
								else if ((i + 3 == j) || (i - 3 == j)) level = 31;
								else level = 0;

								if (l) WS2812SetRGB(i, level, 0, 0);
								else WS2812SetRGB(LED_COUNT-i, level, 0, 0);
							}
							led_strip_write(colors, LED_COUNT);
							_delay_ms(40);
						}
					}
					break;
				case 3:;
					uint16_t df = (RAND_MAX / LED_COUNT + 1);
					for(uint8_t j = 0; j < 10; j++)
					{
						WS2812Clear();
						for(uint8_t i = 0; i < LED_COUNT / 5; i++)
						{
							WS2812SetRGB(rand() / df, rand() * 5, rand() * 5, rand() * 5);
						}
						led_strip_write(colors, LED_COUNT);
						_delay_ms(25);
					}
					break;
				case 4:;
					uint16_t steps = (1535/LED_COUNT+1);
					for(char j = 0; j<LED_COUNT; j++)
					{
						for(char i = 0; i<LED_COUNT; i++)
						{
							WS2812SetHSV(i, (steps*i+steps*j)%1535, 255, 50);
						}
						led_strip_write(colors, LED_COUNT);
						_delay_ms(40);
					}
					break;
				case 5:;
					for(char j = 0; j<2; j++)
					{
						for(char i = 0; i<LED_COUNT; i++)
						{
							WS2812SetRGB(i, 0, 255, 0);
						}
						led_strip_write(colors, LED_COUNT);
						_delay_ms(5);
						WS2812Clear();
						led_strip_write(colors, LED_COUNT);
						_delay_ms(75);
					}
					_delay_ms(250);
					break;
				case 6:;
					for(char j = 0; j < 4; j++)
					{
						WS2812Clear();
						for(char i = 0; i<LED_COUNT/2; i++)
						{
							if (i%4 == j)
							{
								WS2812SetRGB(i, 255, 127, 0);
								WS2812SetRGB(LED_COUNT - i -1, 255, 127, 0);
							}
						}
						led_strip_write(colors, LED_COUNT);
						_delay_ms(50);
					}
					break;
				case 7:;
					for(char j = 0; j < 2; j++)
					{
						for(char i = 0; i < LED_COUNT; i++)
						{
							WS2812SetRGB(i, 255, 255, 255);
						}
						led_strip_write(colors, LED_COUNT);
						_delay_ms(3);
						WS2812Clear();
						led_strip_write(colors, LED_COUNT);
						_delay_ms(100);
					}
					break;
				default:;
					effect = 0;
				}
	}
}
