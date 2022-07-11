/*
 * leds.h
 *
 *  Created on: Oct 20, 2020
 *      Author: turum
 */



#ifndef INC_LEDS_H_
#define INC_LEDS_H_

#define RMT_TX_CHANNEL RMT_CHANNEL_0

#define LED_COUNT 24

typedef struct RgbColor {
	unsigned char r;
	unsigned char g;
	unsigned char b;
} RgbColor;

typedef struct HsvColor {
	unsigned char h;
	unsigned char s;
	unsigned char v;
} HsvColor;


void initLeds();
void refreshLeds();
void showState(int ledS);

#endif /* INC_LEDS_H_ */
