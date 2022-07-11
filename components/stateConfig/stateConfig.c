#include <stdio.h>
#include <string.h>
#include "stateConfig.h"
#include "ini.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "esp_spiffs.h"
#include "ff.h"
#include "diskio.h"
#include "sdcard_scan.h"
#include <errno.h>

#include "audio_error.h"
#include "audio_mem.h"

#include <dirent.h>

#define TAG "stateConfig"

#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG

#define SDCARD_SCAN_URL_MAX_LENGTH (255)

extern configuration monofon_config;
extern stateStruct monofon_state;

static int handler(void *user, const char *section, const char *name, const char *value) {
	configuration *pconfig = (configuration*) user;

#define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0
	if (MATCH("led", "RGB.r")) {
		pconfig->RGB.r = atoi(value);
	} else if (MATCH("led", "RGB.g")) {
		pconfig->RGB.g = atoi(value);
	} else if (MATCH("led", "RGB.b")) {
		pconfig->RGB.b = atoi(value);
	} else if (MATCH("led", "animate")) {
		pconfig->animate = atoi(value);
	} else if (MATCH("led", "rainbow")) {
		pconfig->rainbow = atoi(value);
	} else if (MATCH("led", "brightMax")) {
		pconfig->brightMax = atoi(value);
	} else if (MATCH("led", "brightMin")) {
		pconfig->brightMin = atoi(value);
	} else if (MATCH("sound", "volume")) {
		pconfig->volume = atoi(value);
	} else if (MATCH("sound", "playerMode")) {
		pconfig->playerMode = atoi(value);
	} else if (MATCH("sound", "defaultLang")) {
		pconfig->defaultLang = atoi(value);
	} else if (MATCH("sens", "sensInverted")) {
		pconfig->sensInverted = atoi(value);
	} else if (MATCH("sens", "sensDebug")) {
		pconfig->sensDebug = atoi(value);
	} else if (MATCH("sens", "sensMode")) {
		pconfig->sensMode = atoi(value);
	} else if (MATCH("WIFI", "WIFI_mode")) {
		pconfig->WIFI_mode = atoi(value);
	} else if (MATCH("WIFI", "WIFI_ssid")) {
		pconfig->WIFI_ssid = strdup(value);
	} else if (MATCH("WIFI", "WIFI_pass")) {
		pconfig->WIFI_pass = strdup(value);
	} else if (MATCH("WIFI", "WIFI_channel")) {
		pconfig->WIFI_channel = atoi(value);
	} else if (MATCH("WIFI", "ipAdress")) {
		pconfig->ipAdress = strdup(value);
	} else if (MATCH("WIFI", "netMask")) {
		pconfig->netMask = strdup(value);
	} else if (MATCH("WIFI", "gateWay")) {
		pconfig->gateWay = strdup(value);
	} else if (MATCH("WIFI", "DHCP")) {
		pconfig->DHCP = atoi(value);
	} else if (MATCH("system", "logLevel")) {
		pconfig->logLevel = atoi(value);
	} else if (MATCH("WIFI", "FTP_login")) {
		pconfig->FTP_login = strdup(value);
	} else if (MATCH("WIFI", "FTP_pass")) {
		pconfig->FTP_pass = strdup(value);
	} else if (MATCH("WIFI", "device_name")) {
		pconfig->device_name = strdup(value);
	} else if (MATCH("WIFI", "mqttBrokerAdress")) {
		pconfig->mqttBrokerAdress = strdup(value);
	} else {
		return 0; /* unknown section/name, error */
	}
	return 1;
}

void writeErrorTxt(const char *buff) {

	FILE *errFile;

	errFile = fopen("/sdcard/error.txt", "w");
	if (!errFile) {
		ESP_LOGE(TAG, "fopen() failed");
		return;
	}
	unsigned int bytesWritten;
	bytesWritten = fprintf(errFile, buff);
	if (bytesWritten == 0) {
		ESP_LOGE(TAG, "fwrite() failed");
		return;
	}

	fclose(errFile);

}

void load_Default_Config(void) {
	uint32_t startTick = xTaskGetTickCount();
	uint32_t heapBefore = xPortGetFreeHeapSize();

	monofon_config.device_name = strdup("");

	monofon_config.WIFI_mode = 1; // softAp

	monofon_config.WIFI_ssid = strdup("");
	monofon_config.WIFI_pass = strdup("monofonpass");
	monofon_config.WIFI_channel = 6;

	monofon_config.DHCP = 0;

	monofon_config.ipAdress = strdup("10.0.0.1");
	monofon_config.netMask = strdup("255.255.255.0");
	monofon_config.gateWay = strdup("10.0.0.1");

	monofon_config.FTP_login = strdup("user");
	monofon_config.FTP_pass = strdup("pass");

	monofon_config.mqttBrokerAdress = strdup("");

	monofon_config.logLevel = 3; // 0-none, 1-error, 2-warn, 3-info, 4-debug

	monofon_config.brightMax = 200;
	monofon_config.brightMin = 0;
	monofon_config.RGB.r = 0;
	monofon_config.RGB.g = 0;
	monofon_config.RGB.b = 200;
	monofon_config.animate = 1;
	monofon_config.rainbow = 0;

	monofon_config.playerMode = 1; // SD_card source
	monofon_config.defaultLang = 0;
	monofon_config.volume = 60;

	monofon_config.sensMode = 1; //hall sensor
	monofon_config.sensInverted = 0;
	monofon_config.sensDebug = 0;

	monofon_state.numOfLang = 0;

	ESP_LOGD(TAG, "Load default config complite. Duration: %d ms. Heap usage: %d", (xTaskGetTickCount() - startTick) * portTICK_RATE_MS, heapBefore - xPortGetFreeHeapSize());
}

uint8_t loadConfig(void) {
	uint32_t startTick = xTaskGetTickCount();
	uint32_t heapBefore = xPortGetFreeHeapSize();

	ESP_LOGD(TAG, "Init config");

	FRESULT res;
	FF_DIR dir;
	FILINFO fno;

	res = f_opendir(&dir, "/");
	res = scan_in_dir((const char* ) { "ini" }, &dir, &fno);
	if (res == FR_OK && (strcmp(fno.fname, "config.ini") == 0)) {
		ESP_LOGD(TAG, "found config file: %s", fno.fname);
		res = ini_parse("/sdcard/config.ini", handler, &monofon_config);
		if (res != 0) {
			ESP_LOGE(TAG, "Can't load 'config.ini' check line: %d, set default\n", res);
			return res;
		}
	} else {
		ESP_LOGD(TAG, "config file not found, create default config");
		saveConfig();
		return res;
	}

	ESP_LOGD(TAG, "Load config complite. Duration: %d ms. Heap usage: %d", (xTaskGetTickCount() - startTick) * portTICK_RATE_MS, heapBefore - xPortGetFreeHeapSize());
	return res;

	ESP_LOGD(TAG, "\nLoad config:");
	ESP_LOGD(TAG, "[LOG]");
	ESP_LOGD(TAG, "monofon_config.logLevel = %d", monofon_config.logLevel);
	ESP_LOGD(TAG, "[WIFI]");
	ESP_LOGD(TAG, "monofon_config.WIFI_mode = %d", monofon_config.WIFI_mode);
	ESP_LOGD(TAG, "monofon_config.WIFI_ssid = %s", monofon_config.WIFI_ssid);
	ESP_LOGD(TAG, "monofon_config.WIFI_pass = %s", monofon_config.WIFI_pass);
	ESP_LOGD(TAG, "monofon_config.WIFI_channel = %d", monofon_config.WIFI_channel);
	ESP_LOGD(TAG, "monofon_config.ipAdress = %s", monofon_config.ipAdress);
	ESP_LOGD(TAG, "monofon_config.netMask = %s", monofon_config.netMask);
	ESP_LOGD(TAG, "monofon_config.gateWay = %s", monofon_config.gateWay);
	ESP_LOGD(TAG, "[leds]\n");
	ESP_LOGD(TAG, "monofon_config.brightMax = %d", monofon_config.brightMax);
	ESP_LOGD(TAG, "monofon_config.brightMin = %d", monofon_config.brightMin);
	ESP_LOGD(TAG, "monofon_config.RGB.r = %d", monofon_config.RGB.r);
	ESP_LOGD(TAG, "monofon_config.RGB.g = %d", monofon_config.RGB.g);
	ESP_LOGD(TAG, "monofon_config.RGB.b = %d", monofon_config.RGB.b);
	ESP_LOGD(TAG, "monofon_config.animate = %d", monofon_config.animate);
	ESP_LOGD(TAG, "monofon_config.rainbow = %d", monofon_config.rainbow);
	ESP_LOGD(TAG, "[lang]");
	ESP_LOGD(TAG, "monofon_config.defaultLang = %d", monofon_config.defaultLang);
	ESP_LOGD(TAG, "[sens]");
	ESP_LOGD(TAG, "monofon_config.sensInverted = %d", monofon_config.sensInverted);
	ESP_LOGD(TAG, "monofon_config.sensDebug = %d", monofon_config.sensDebug);

}

int saveConfig(void) {

	ESP_LOGD(TAG, "saving file");

	FILE *configFile;
	char tmp[100];

	if (remove("/sdcard/config.ini")) {
		ESP_LOGD(TAG, "/sdcard/config.ini delete failed");
		//return ESP_FAIL;
	}

	configFile = fopen("/sdcard/config.ini", "w");
	if (!configFile) {
		ESP_LOGE(TAG, "fopen() failed");
		return ESP_FAIL;
	}

	sprintf(tmp, ";config file Monofon \r\n\r\n");
	fprintf(configFile, tmp);
	memset(tmp, 0, strlen(tmp));

	sprintf(tmp, "\r\n[WIFI] \r\n");
	fprintf(configFile, tmp);
	memset(tmp, 0, strlen(tmp));

	sprintf(tmp, "device_name = %s \r\n", monofon_config.device_name);
	fprintf(configFile, tmp);
	memset(tmp, 0, strlen(tmp));
	sprintf(tmp, "WIFI_mode = %d ;0-disable, 1-AP mode, 2-station mode \r\n", monofon_config.WIFI_mode);
	fprintf(configFile, tmp);
	memset(tmp, 0, strlen(tmp));
	sprintf(tmp, "WIFI_ssid = %s \r\n", monofon_config.WIFI_ssid);
	fprintf(configFile, tmp);
	memset(tmp, 0, strlen(tmp));
	sprintf(tmp, "WIFI_pass = %s \r\n", monofon_config.WIFI_pass);
	fprintf(configFile, tmp);
	memset(tmp, 0, strlen(tmp));
	sprintf(tmp, "WIFI_channel = %d \r\n", monofon_config.WIFI_channel);
	fprintf(configFile, tmp);
	memset(tmp, 0, strlen(tmp));
	sprintf(tmp, "DHCP = %d ;0-disable, 1-enable \r\n", monofon_config.DHCP);
	fprintf(configFile, tmp);
	memset(tmp, 0, strlen(tmp));
	sprintf(tmp, "ipAdress = %s \r\n", monofon_config.ipAdress);
	fprintf(configFile, tmp);
	memset(tmp, 0, strlen(tmp));
	sprintf(tmp, "netMask = %s \r\n", monofon_config.netMask);
	fprintf(configFile, tmp);
	memset(tmp, 0, strlen(tmp));
	sprintf(tmp, "gateWay = %s \r\n", monofon_config.gateWay);
	fprintf(configFile, tmp);
	memset(tmp, 0, strlen(tmp));
	sprintf(tmp, "FTP_login = %s \r\n", monofon_config.FTP_login);
	fprintf(configFile, tmp);
	memset(tmp, 0, strlen(tmp));
	sprintf(tmp, "FTP_pass = %s \r\n", monofon_config.FTP_pass);
	fprintf(configFile, tmp);
	memset(tmp, 0, strlen(tmp));
	sprintf(tmp, "mqttBrokerAdress = %s \r\n", monofon_config.mqttBrokerAdress);
	fprintf(configFile, tmp);
	memset(tmp, 0, strlen(tmp));

	sprintf(tmp, "\r\n[system] \r\n");
	fprintf(configFile, tmp);
	memset(tmp, 0, strlen(tmp));
	sprintf(tmp, "logLevel = %d ;\r\n", monofon_config.logLevel);
	fprintf(configFile, tmp);
	memset(tmp, 0, strlen(tmp));

	sprintf(tmp, "\r\n[led] \r\n");
	fprintf(configFile, tmp);
	memset(tmp, 0, strlen(tmp));
	sprintf(tmp, "brightMax = %d ; 0-255\r\n", monofon_config.brightMax);
	fprintf(configFile, tmp);
	memset(tmp, 0, strlen(tmp));
	sprintf(tmp, "brightMin = %d ; 0-255\r\n", monofon_config.brightMin);
	fprintf(configFile, tmp);
	memset(tmp, 0, strlen(tmp));
	sprintf(tmp, "RGB.r = %d ; 0-255 Red color\r\n", monofon_config.RGB.r);
	fprintf(configFile, tmp);
	memset(tmp, 0, strlen(tmp));
	sprintf(tmp, "RGB.g = %d ; 0-255 Green color\r\n", monofon_config.RGB.g);
	fprintf(configFile, tmp);
	memset(tmp, 0, strlen(tmp));
	sprintf(tmp, "RGB.b = %d ; 0-255 Blue color\r\n", monofon_config.RGB.b);
	fprintf(configFile, tmp);
	memset(tmp, 0, strlen(tmp));
	sprintf(tmp, "animate = %d \r\n", monofon_config.animate);
	fprintf(configFile, tmp);
	memset(tmp, 0, strlen(tmp));
	sprintf(tmp, "rainbow = %d \r\n", monofon_config.rainbow);
	fprintf(configFile, tmp);
	memset(tmp, 0, strlen(tmp));

	sprintf(tmp, "\r\n[sound] \r\n");
	fprintf(configFile, tmp);
	memset(tmp, 0, strlen(tmp));
	sprintf(tmp, "playerMode = %d ; 0 - disable, 1 - SD_card source, 2 - bluetooth \r\n", monofon_config.playerMode);
	fprintf(configFile, tmp);
	memset(tmp, 0, strlen(tmp));
	sprintf(tmp, "volume = %d \r\n", monofon_config.volume);
	fprintf(configFile, tmp);
	memset(tmp, 0, strlen(tmp));
	sprintf(tmp, "defaultLang = %d \r\n", monofon_config.defaultLang);
	fprintf(configFile, tmp);
	memset(tmp, 0, strlen(tmp));

	sprintf(tmp, "\r\n[sens] \r\n");
	fprintf(configFile, tmp);
	memset(tmp, 0, strlen(tmp));
	sprintf(tmp, "sensMode = %d ; 0-disable, 1-hall sensor mode, 2-button mode, 3-multilanguage mode \r\n", monofon_config.sensMode);
	fprintf(configFile, tmp);
	memset(tmp, 0, strlen(tmp));
	sprintf(tmp, "sensInverted = %d \r\n", monofon_config.sensInverted);
	fprintf(configFile, tmp);
	memset(tmp, 0, strlen(tmp));
	sprintf(tmp, "sensDebug = %d \r\n", monofon_config.sensDebug);
	fprintf(configFile, tmp);
	memset(tmp, 0, strlen(tmp));

	vTaskDelay(pdMS_TO_TICKS(100));

	FRESULT res;
	res = fclose(configFile);
	if (res != ESP_OK) {
		ESP_LOGE(TAG, "fclose() failed: %d ", res);
		return ESP_FAIL;
	}

	ESP_LOGD(TAG, "save OK");
	return ESP_OK;
}

FRESULT scan_in_dir(const char *file_extension, FF_DIR *dp, FILINFO *fno) {
	FRESULT res = FR_OK;

	while (res == FR_OK) {
		res = f_readdir(dp, fno); /* Find the first item */

		if (dp->sect == 0) {
			ESP_LOGD(TAG, "end of file in dir");
			break;
		}

		if (fno->fattrib & AM_DIR) {
			ESP_LOGD(TAG, "found subDir = %s", fno->fname);
		} else {
			ESP_LOGD(TAG, "found file = %s", fno->fname);
			char *detect = strrchr(fno->fname, '.');

			if (NULL == detect) {
				ESP_LOGD(TAG, "bad file name = %s, res:%d", fno->fname, res);
				// break;
			} else {
				ESP_LOGD(TAG, "found extension:%s", detect);
				detect++;
				if (strcasecmp(detect, file_extension) == 0) {
					ESP_LOGD(TAG, "good file_extension:%s ", fno->fname);
					return FR_OK;
				}
			}
		}
	}
	return -1;
}



void search_introIcon(const char *path) {
	FRESULT res;
	FF_DIR dir;
	FILINFO fno;

	res = f_opendir(&dir, path);
	if (res == FR_OK) {
		res = scan_in_dir((const char* ) { "bmp" }, &dir, &fno);
		if (res == FR_OK && fno.fname[0]) {
			sprintf(monofon_config.introIco, "/sdcard/%s", fno.fname);
			ESP_LOGI(TAG, "found intro image file: %s", monofon_config.introIco);
			monofon_state.introIco_error = 0;
		}
	}
	f_closedir(&dir);
	if (monofon_state.introIco_error == 1) {
		ESP_LOGD(TAG, "introIcon not found(it should be .jpeg/.jpg 240x240px)");
	}
}

uint8_t search_contenInDir(const char *path) {
	FRESULT res;
	FF_DIR dir;
	FILINFO fno;

	res = f_opendir(&dir, path);
	if (res == FR_OK) {
		ESP_LOGD(TAG, "Open dir: %s", fno.fname);
		res = scan_in_dir((const char* ) { "mp3" }, &dir, &fno);
		if (res == FR_OK) {

			memset(monofon_config.lang[monofon_state.numOfLang].audioFile, 0, 255);
			if(strcmp(path,"/")==0){
				sprintf(monofon_config.lang[monofon_state.numOfLang].audioFile, "/sdcard/%s", fno.fname);
			}else{
				sprintf(monofon_config.lang[monofon_state.numOfLang].audioFile, "/sdcard/%s/%s", path, fno.fname);
			}

			ESP_LOGI(TAG, "found audio file: %s num: %d", monofon_config.lang[monofon_state.numOfLang].audioFile, monofon_state.numOfLang);
		}
	}
	f_closedir(&dir);

	res = f_opendir(&dir, path);
	if (res == FR_OK) {
		res = scan_in_dir((const char* ) { "bmp" }, &dir, &fno);
		if (res == FR_OK) {

			memset(monofon_config.lang[monofon_state.numOfLang].icoFile, 0, 255);
			if(strcmp(path,"/")==0){
				sprintf(monofon_config.lang[monofon_state.numOfLang].icoFile, "/sdcard/%s", fno.fname);
			}else{
				sprintf(monofon_config.lang[monofon_state.numOfLang].icoFile, "/sdcard/%s/%s", path, fno.fname);
			}

			ESP_LOGI(TAG, "found image file: %s num: %d", monofon_config.lang[monofon_state.numOfLang].icoFile, monofon_state.numOfLang);

		}
	}
	f_closedir(&dir);
	if (monofon_config.lang[monofon_state.numOfLang].audioFile[0] != 0) {
		monofon_state.numOfLang++;
	}
	return monofon_state.numOfLang;
}

uint8_t loadContent(void) {
	uint32_t startTick = xTaskGetTickCount();
	uint32_t heapBefore = xPortGetFreeHeapSize();
	ESP_LOGD(TAG, "Loading content");

	monofon_state.numOfLang = 0;
	monofon_state.introIco_error = 1;

	FRESULT res;
	FF_DIR dir;
	FILINFO fno;

	search_introIcon("/");

	if (search_contenInDir("/") == 0) {
		ESP_LOGD(TAG, "No content in root dir. Try search in subdir");
		while (1) {
			res = f_readdir(&dir, &fno); /* Read a directory item */
			if (res != FR_OK || fno.fname[0] == 0)
				break;
			if (fno.fattrib & AM_DIR) {
				//char path[300];
				//sprintf(path, "/%s/",fno.fname);
				//search_contenInDir(path);
				//sprintf(fno.fname, "/%s/",fno.fname);
				search_contenInDir(fno.fname);
			}
		}
	}

	if (monofon_state.numOfLang > 0) {
		ESP_LOGI(TAG, "Load Content complete. numOfLang:%d Duration: %d ms. Heap usage: %d", monofon_state.numOfLang, (xTaskGetTickCount() - startTick) * portTICK_RATE_MS,
				heapBefore - xPortGetFreeHeapSize());
		return ESP_OK;
	} else {
		ESP_LOGE(TAG, "Load content fail");
		return ESP_FAIL;
	}
}

