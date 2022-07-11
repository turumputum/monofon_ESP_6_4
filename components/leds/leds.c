/*
 * leds.c
 *
 *  Created on: Oct 20, 2020
 *      Author: turum
 */
#include <stdlib.h>
#include <string.h>
#include "stateConfig.h"
#include "leds.h"
#include "esp_log.h"
#include "driver/rmt.h"
#include "led_strip.h"
#include <math.h>

int effectTick;
float front[LED_COUNT];
int ledBright = 0;

int lightLeds = 0;
int ledStep = 0;

extern stateStruct monofon_state;
extern configuration monofon_config;

RgbColor RGB;
HsvColor HSV;

int currentBright;

led_strip_t *strip;

static const char *TAG = "leds";
#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG

RgbColor HsvToRgb(HsvColor hsv) {
	RgbColor rgb;
	unsigned char region, remainder, p, q, t;

	if (hsv.s == 0) {
		rgb.r = hsv.v;
		rgb.g = hsv.v;
		rgb.b = hsv.v;
		return rgb;
	}

	region = hsv.h / 43;
	remainder = (hsv.h - (region * 43)) * 6;

	p = (hsv.v * (255 - hsv.s)) >> 8;
	q = (hsv.v * (255 - ((hsv.s * remainder) >> 8))) >> 8;
	t = (hsv.v * (255 - ((hsv.s * (255 - remainder)) >> 8))) >> 8;

	switch (region) {
	case 0:
		rgb.r = hsv.v;
		rgb.g = t;
		rgb.b = p;
		break;
	case 1:
		rgb.r = q;
		rgb.g = hsv.v;
		rgb.b = p;
		break;
	case 2:
		rgb.r = p;
		rgb.g = hsv.v;
		rgb.b = t;
		break;
	case 3:
		rgb.r = p;
		rgb.g = q;
		rgb.b = hsv.v;
		break;
	case 4:
		rgb.r = t;
		rgb.g = p;
		rgb.b = hsv.v;
		break;
	default:
		rgb.r = hsv.v;
		rgb.g = p;
		rgb.b = q;
		break;
	}

	return rgb;
}

HsvColor RgbToHsv(RgbColor rgb) {
	HsvColor hsv;
	unsigned char rgbMin, rgbMax;

	rgbMin =
			rgb.r < rgb.g ?
					(rgb.r < rgb.b ? rgb.r : rgb.b) :
					(rgb.g < rgb.b ? rgb.g : rgb.b);
	rgbMax =
			rgb.r > rgb.g ?
					(rgb.r > rgb.b ? rgb.r : rgb.b) :
					(rgb.g > rgb.b ? rgb.g : rgb.b);

	hsv.v = rgbMax;
	if (hsv.v == 0) {
		hsv.h = 0;
		hsv.s = 0;
		return hsv;
	}

	hsv.s = 255 * (rgbMax - rgbMin) / hsv.v;
	if (hsv.s == 0) {
		hsv.h = 0;
		return hsv;
	}

	if (rgbMax == rgb.r)
		hsv.h = 0 + 43 * (rgb.g - rgb.b) / (rgbMax - rgbMin);
	else if (rgbMax == rgb.g)
		hsv.h = 85 + 43 * (rgb.b - rgb.r) / (rgbMax - rgbMin);
	else
		hsv.h = 171 + 43 * (rgb.r - rgb.g) / (rgbMax - rgbMin);

	return hsv;
}

void initLeds() {
	uint32_t startTick = xTaskGetTickCount();
		uint32_t heapBefore = xPortGetFreeHeapSize();

	RGB.r = monofon_config.RGB.r;
	RGB.g = monofon_config.RGB.g;
	RGB.b = monofon_config.RGB.b;

	HSV.h = 0;
	HSV.s = 255;
	HSV.v = monofon_config.brightMax;

	//rmt_config_t config = RMT_DEFAULT_CONFIG_TX(12, RMT_TX_CHANNEL);
	rmt_config_t config = RMT_DEFAULT_CONFIG_TX(8, RMT_TX_CHANNEL);
    // set counter clock to 40MHz
    config.clk_div = 2;
	config.mem_block_num = 8;
    ESP_ERROR_CHECK(rmt_config(&config));
    ESP_ERROR_CHECK(rmt_driver_install(config.channel, 0, 0));

    // install ws2812 driver
    led_strip_config_t strip_config = LED_STRIP_DEFAULT_CONFIG(LED_COUNT, (led_strip_dev_t)config.channel);
    strip = led_strip_new_rmt_ws2812(&strip_config);
    if (!strip) {
        ESP_LOGE(TAG, "install WS2812 driver failed");
    }
    // Clear LED strip (turn off all LEDs)
    ESP_ERROR_CHECK(strip->clear(strip, 24));

	for (int t = 0; t < LED_COUNT; t++) {
		float val = sin((float) (6.28 / LED_COUNT) * (float) t);
		if (val > 0) {
			front[t] = val;
		}
	}
	ESP_LOGD(TAG, "SD_card init complite. Duration: %d ms. Heap usage: %d free heap:%d", (xTaskGetTickCount() - startTick) * portTICK_RATE_MS, heapBefore - xPortGetFreeHeapSize(), xPortGetFreeHeapSize());

}

void refreshLeds() {

	uint32_t startTick = xTaskGetTickCount();
    uint32_t heapBefore = xPortGetFreeHeapSize();

	if (monofon_config.rainbow == 1) {
		if (HSV.h < 255) {
			HSV.h++;
		} else {
			HSV.h = 0;
		}
		RGB = HsvToRgb(HSV);

	} else if (monofon_config.rainbow == 0) {
		RGB.b = monofon_config.RGB.b;
		RGB.r = monofon_config.RGB.r;
		RGB.g = monofon_config.RGB.g;
	}

	if (monofon_state.phoneUp) {
		if (monofon_config.animate == 1) {
			for (int i = 0; i < LED_COUNT; i++) {
				strip->set_pixel(strip, i, (float)RGB.r * front[i],(float)RGB.g * front[i], (float)RGB.b * front[i]);

			}
			strip->refresh(strip, 1);


			//shift front
			float tmp = front[0];
			for (int t = 0; t < (LED_COUNT - 1); t++) {
				front[t] = front[t + 1];
			}
			front[LED_COUNT - 1] = tmp;
		} else {
			if(currentBright<monofon_config.brightMin){
				currentBright++;
			}else if (currentBright>monofon_config.brightMin){
				currentBright--;
			}
			float tmpBright = ((float)currentBright/255);
			for (int i = 0; i < LED_COUNT; i++) {

				strip->set_pixel(strip, i, RGB.r * tmpBright,RGB.g * tmpBright, RGB.b * tmpBright);
			}

			strip->refresh(strip, 1);
		}

	} else {
		if(currentBright<monofon_config.brightMax){
						currentBright++;
					}else if (currentBright>monofon_config.brightMax){
						currentBright--;
					}
		float tmpBright = ((float)currentBright/255);
		for (int i = 0; i < LED_COUNT; i++) {
			strip->set_pixel(strip, i, RGB.r * tmpBright,RGB.g * tmpBright, RGB.b * tmpBright);
			//ws2812_setPixel_gammaCorrection(RGB.r * tmpBright,RGB.g * tmpBright, RGB.b * tmpBright, i);
		}
		//ws2812_light();
		
		strip->refresh(strip, 1);
		

	}

	//ESP_LOGD(TAG, "ReFresh leds complite, Duration: %d ms. Heap usage: %d", (xTaskGetTickCount() - startTick) * portTICK_RATE_MS, heapBefore - xPortGetFreeHeapSize());

}

void showState(int ledS){
	HSV.s = 255;
	HSV.v = 255;
	if(ledS==LED_STATE_SD_ERROR){
		HSV.h = 0;//red
	}else if(ledS==LED_STATE_CONFIG_ERROR){
		HSV.h = 32;//orange
	}else if(ledS==LED_STATE_CONTENT_ERROR){
		HSV.h = 64;//yellow
	}else if(ledS==LED_STATE_SENSOR_ERROR){
		HSV.h = 192;//purple
	}else if(ledS==LED_STATE_WIFI_FAIL){
		HSV.h = 224;//pink
	}

	RGB = HsvToRgb(HSV);

	for(int i=0; i<LED_COUNT; i++){
		for(int y=0; y<LED_COUNT; y++){
			if(i>=y){
				strip->set_pixel(strip, y, RGB.r,RGB.g,RGB.b);
			}else{
				strip->set_pixel(strip, y, 0,0,0);
			}
		}
		strip->refresh(strip, 100);
		vTaskDelay(pdMS_TO_TICKS(30));
	}

	for(int i=0; i<LED_COUNT; i++){
		for(int y=0; y<LED_COUNT; y++){
			if(i<y){
				strip->set_pixel(strip, y, RGB.r,RGB.g,RGB.b);
			}else{
				strip->set_pixel(strip, y, 0,0,0);
			}
		}
		strip->refresh(strip, 100);
		vTaskDelay(pdMS_TO_TICKS(30));
	}

}
