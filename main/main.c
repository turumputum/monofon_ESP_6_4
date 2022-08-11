/* Control with a touch pad playing MP3 files from SD Card

 This example code is in the Public Domain (or CC0 licensed, at your option.)

 Unless required by applicable law or agreed to in writing, this
 software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 CONDITIONS OF ANY KIND, either express or implied.
 */
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"

#include "esp_log.h"
#include "nvs_flash.h"

#include "esp_peripherals.h"
#include "periph_sdcard.h"
#include "periph_touch.h"
#include "periph_button.h"
#include "input_key_service.h"
#include "periph_adc_button.h"
#include "board.h"

#include "esp_vfs_fat.h"
#include "esp_spiffs.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "sdmmc_cmd.h"
#include "sdcard_list.h"
#include "sdcard_scan.h"

#include "driver/adc.h"
#include "esp_adc_cal.h"

#include "leds.h"

#include "stateConfig.h"

#include "audioPlayer.h"

#include "WIFI.h"
#include "mdns.h"

#include "ftp.h"

#include "st7789.h"
#include "fontx.h"
#include "decode_jpeg.h"
#include "lwip/dns.h"

#include "esp_console.h"
#include "linenoise/linenoise.h"
#include "argtable3/argtable3.h"
#include "esp_vfs_cdcacm.h"
#include "myMqtt.h"
#include "cmd.h"
#include "spisd.h"

#include "tusb.h"
#include "cdc.h"
extern void board_init(void);

#define USBD_STACK_SIZE     4096

#define MDNS_ENABLE_DEBUG 0

static const char *TAG = "MAIN";

#define SPI_DMA_CHAN 1
//#define MOUNT_POINT "/sdcard"
static const char *MOUNT_POINT = "/sdcard";

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG

int FTP_TASK_FINISH_BIT = BIT2;
EventGroupHandle_t xEventTask;

extern uint8_t FTP_SESSION_START_FLAG;
extern uint8_t FLAG_PC_AVAILEBLE;
extern uint8_t FLAG_PC_EJECT;

RTC_NOINIT_ATTR int RTC_flagMscEnabled;
int flagMscEnabled;

// static task
StackType_t usb_device_stack[USBD_STACK_SIZE];
StaticTask_t usb_device_taskdef;
// static task for cdc
//#define CDC_STACK_SZIE      (configMINIMAL_STACK_SIZE*3)
//StackType_t cdc_stack[CDC_STACK_SZIE];
//StaticTask_t cdc_taskdef;
//void usb_device_task(void *param);
//void cdc_task(void *params);

int isMscEnabled() {
	return flagMscEnabled;
}

#define RTC_FLAG_ENABLED_VALUE		0xAAAA5555

TFT_t screen;
FontxFile fx32G[2];

configuration monofon_config;
stateStruct monofon_state;

uint32_t ADC_AVERAGE;

#define RELAY_1_GPIO (47)
#define RELAY_2_GPIO (48)

void listenListener(void *pvParameters);

void RTC_IRAM_ATTR storeMscFlag() {
	RTC_flagMscEnabled = flagMscEnabled ? RTC_FLAG_ENABLED_VALUE : 0;
}

void RTC_IRAM_ATTR loadMscFlag() {
	flagMscEnabled = RTC_flagMscEnabled == RTC_FLAG_ENABLED_VALUE;

	//printf("@@@@@@@@@@@@@@@@@@@@@@@@ flagMscEnabled = %d\n", flagMscEnabled);
}

void setMscEnabled(int ena) {
	flagMscEnabled = ena;
	storeMscFlag();
	FLAG_PC_AVAILEBLE = ena;
	ESP_LOGD(TAG, "Set USB MSD to: %d", ena);

	xTaskCreateStatic(usb_device_task, "usbd", USBD_STACK_SIZE, NULL,
	configMAX_PRIORITIES - 1, usb_device_stack, &usb_device_taskdef);
	xTaskCreateStatic(cdc_task, "cdc", CDC_STACK_SZIE, NULL,
	configMAX_PRIORITIES - 2, cdc_stack, &cdc_taskdef);

	if (ena == 1) {
		vTaskDelay(pdMS_TO_TICKS(1000));
		while (FLAG_PC_AVAILEBLE && !FLAG_PC_EJECT) {
			showState(LED_STATE_MSD_WORK);
			printf("PC_AVAILEBLE - WORK MSD+CDC \r\n");
			vTaskDelay(pdMS_TO_TICKS(1000));
		}
		ESP_LOGD(TAG, "Set USB MSD to: %d", ena);
		vTaskDelete(&usb_device_taskdef);
		vTaskDelete(&cdc_taskdef);
	}
	//esp_restart();
}

static void sensTask(void *arg) {

#define TOUCH_SENS_PIN 21
	////////////ADC config////////////////
	uint32_t startTick = xTaskGetTickCount();
	uint32_t heapBefore = xPortGetFreeHeapSize();
	static const adc_channel_t ADC_chan_1 = ADC_CHANNEL_0; //
	static const adc_channel_t ADC_chan_2 = ADC_CHANNEL_1; //
	static const adc_bits_width_t width = ADC_WIDTH_BIT_12;
	static const adc_atten_t atten = ADC_ATTEN_DB_11; // ADC_ATTEN_DB_11

	gpio_reset_pin(TOUCH_SENS_PIN);
	gpio_set_direction(TOUCH_SENS_PIN, GPIO_MODE_INPUT);
	uint8_t prevSens_state, sens_state;

	adc1_config_width(width);
	adc1_config_channel_atten(ADC_chan_1, atten);
	adc1_config_channel_atten(ADC_chan_2, atten);

	uint8_t sensInited = 0;

	int32_t adc_reading = 0;

	int32_t prev_adc_reading = 0;
	int16_t adc_delta = 0;
	float k = 0.015; // 0.03
	float kl = 0.2; // 0.03

	int ledTick = 0;
	int tick = 0;

	monofon_state.phoneUp = 0;

	ADC_AVERAGE = adc1_get_raw(ADC_chan_1);
	ADC_AVERAGE += adc1_get_raw(ADC_chan_2);

	prev_adc_reading = ADC_AVERAGE;
	int32_t average_max = ADC_AVERAGE;
	int32_t average_min = ADC_AVERAGE;

	uint8_t DELTA_DEF_THRESHOLD = 30;

	ESP_LOGD("SENS", "SNS init complite, start val:%d. Duration: %d ms. Heap usage: %d free heap:%d", average_min, (xTaskGetTickCount() - startTick) * portTICK_RATE_MS,
			heapBefore - xPortGetFreeHeapSize(), xPortGetFreeHeapSize());

	while (1) {

		adc_reading = adc1_get_raw(ADC_chan_1);
		adc_reading += adc1_get_raw(ADC_chan_2);

		ADC_AVERAGE = ADC_AVERAGE * (1 - k) + adc_reading * k;

		if (ledTick > 600) {
			ledTick = 0;
			refreshLeds();

			sens_state = gpio_get_level(TOUCH_SENS_PIN);
			if (sens_state != prevSens_state) {
				if ((sens_state) && (monofon_state.phoneUp == 1)) {
					monofon_state.changeLang = 1;
					printf("change lang\r\n");
				}
				prevSens_state = sens_state;
			}
		}

		if (tick > 3000) {
			tick = 0;

			adc_delta = prev_adc_reading - ADC_AVERAGE;
			prev_adc_reading = ADC_AVERAGE;

			if (sensInited) {
				if (abs(adc_delta) < DELTA_DEF_THRESHOLD) {
					if (monofon_state.phoneUp == 0) {
						average_max = (double) average_max * (1 - kl) + (double) ADC_AVERAGE * kl;
					} else {
						average_min = (double) average_min * (1 - kl) + (double) ADC_AVERAGE * kl;
					}
				}

				monofon_config.magnitudeLevel = average_min + (average_max - average_min) / 2;

				if (ADC_AVERAGE > monofon_config.magnitudeLevel) {
					monofon_state.phoneUp = monofon_config.phoneSensInverted;
				} else {
					monofon_state.phoneUp = !monofon_config.phoneSensInverted;
				}
			}

			if (!sensInited) {
				average_min = (double) average_min * (1 - kl) + (double) ADC_AVERAGE * kl;
				average_max = average_min;

				if ((abs(adc_delta) > DELTA_DEF_THRESHOLD) && (xTaskGetTickCount() - startTick > 100)) {
					ESP_LOGD(TAG, "First detection, val:  %d", ADC_AVERAGE);
					sensInited = 1;
					if (adc_delta > 0) {
						monofon_state.phoneUp = !monofon_config.phoneSensInverted;
					} else {
						monofon_state.phoneUp = monofon_config.phoneSensInverted;
					}
				}
			}

			if (monofon_config.sensDebug) {
				ESP_LOGI(TAG, "magnitude ADC_AVERAGE: %d DELTA: %d THRESHOLD: %d, min:%d max:%d", ADC_AVERAGE, abs(adc_delta), monofon_config.magnitudeLevel, average_min, average_max);
				//ESP_LOGI(TAG,"internaal hall sens: %d",hall_sensor_read());
				//printf( "%d %d %d \r\n",ADC_AVERAGE,adc_delta, monofon_config.magnitudeLevel);
			}
		}
		ledTick++;
		tick++;
		vTaskDelay(pdMS_TO_TICKS(6));
	}
}

static void playerTask(void *arg) {
	uint8_t nextImageNum;

	ESP_LOGD(TAG, "Create fatfs stream to read data from sdcard");
	audioInit();

	char payload[15];
	xTaskCreatePinnedToCore(listenListener, "audio_listener", 1024 * 3, NULL, 1,
	NULL, 0);

	while (1) {
		//listenListener();

		vTaskDelay(pdMS_TO_TICKS(100));

		if (monofon_config.playerMode == 1) {
			if (monofon_state.phoneUp != monofon_state.prevPhoneUp) {

				printf("phoneUp %d \r\n", monofon_state.phoneUp);
				usbprintf("phoneUp %d \r\n", monofon_state.phoneUp);

				if (monofon_state.phoneUp == 1) {

					monofon_state.currentLang = monofon_config.defaultLang;
					vTaskDelay(pdMS_TO_TICKS(500));

					if (monofon_config.lang[monofon_state.currentLang].icoFile[0] != 0) {
						JPEGTest(&screen, monofon_config.lang[monofon_state.currentLang].icoFile, 240, 240);
					}

					audioPlay();

					gpio_set_level(RELAY_1_GPIO, 1);
					gpio_set_level(RELAY_2_GPIO, 0);

					nextImageNum = monofon_state.currentLang + 1;
					if (nextImageNum >= monofon_state.numOfLang) {
						nextImageNum = 0;
					}
				} else {
					gpio_set_level(RELAY_1_GPIO, 0);
					gpio_set_level(RELAY_2_GPIO, 1);

					audioStop();
					if (monofon_config.introIco[0] != 0) {
						JPEGTest(&screen, monofon_config.introIco, 240, 240);
					}

				}

				monofon_state.prevPhoneUp = monofon_state.phoneUp;
			}

			if (monofon_state.changeLang == 1) {
				monofon_state.changeLang = 0;
				monofon_state.currentLang++;
				if (monofon_state.currentLang >= monofon_state.numOfLang) {
					monofon_state.currentLang = 0;
				}
				audioStop();


				if (monofon_config.lang[monofon_state.currentLang].icoFile[0] != 0) {
					JPEGTest(&screen, monofon_config.lang[monofon_state.currentLang].icoFile, 240, 240);
					nextImageNum = monofon_state.currentLang + 1;
					if (nextImageNum >= monofon_state.numOfLang) {
						nextImageNum = 0;
					}
				}

				audioPlay();
				//vTaskDelay(pdMS_TO_TICKS(100));

			}
		}
	}
	free(payload);
}

void ftp_task(void *pvParameters);

uint8_t sd_card_init() {
#define PIN_NUM_MISO 6
#define PIN_NUM_MOSI 7
#define PIN_NUM_CLK 15
#define PIN_NUM_CS 16

	uint32_t startTick = xTaskGetTickCount();
	uint32_t heapBefore = xPortGetFreeHeapSize();

	esp_err_t ret;

	esp_vfs_fat_sdmmc_mount_config_t mount_config = { .format_if_mount_failed =
	true, .max_files = 5, .allocation_unit_size = 16 * 1024 };

	sdmmc_card_t *card;
	ESP_LOGD(TAG, "try connect sdSpi");
	sdmmc_host_t host = SDSPI_HOST_DEFAULT();
	host.max_freq_khz = SDMMC_FREQ_52M;

	spi_bus_config_t bus_cfg = { .mosi_io_num = PIN_NUM_MOSI, .miso_io_num =
	PIN_NUM_MISO, .sclk_io_num = PIN_NUM_CLK, .quadwp_io_num = -1, .quadhd_io_num = -1, .max_transfer_sz = 4000, };
	ret = spi_bus_initialize(host.slot, &bus_cfg, SPI_DMA_CH_AUTO);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "Failed to initialize bus. Error: %s", esp_err_to_name(ret));
		return ret;
	} else {
		ESP_LOGD(TAG, "sdSpi init OK");
	}
	// This initializes the slot without card detect (CD) and write protect (WP) signals.
	// Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
	sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
	slot_config.gpio_cs = PIN_NUM_CS;
	slot_config.host_id = host.slot;

	ESP_LOGD(TAG, "try mount fs");
	ret = esp_vfs_fat_sdspi_mount(MOUNT_POINT, &host, &slot_config, &mount_config, &card);
	if (ret != ESP_OK) {
		if (ret == ESP_FAIL) {
			ESP_LOGE(TAG, "Failed to mount filesystem. " "If you want the card to be formatted, set the EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
		} else {
			ESP_LOGE(TAG, "Failed to initialize the card (%s). " "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
		}
		return ret;
	} else {
		ESP_LOGD(TAG, "SD_card init complite. Duration: %d ms. Heap usage: %d free heap:%d", (xTaskGetTickCount() - startTick) * portTICK_RATE_MS, heapBefore - xPortGetFreeHeapSize(),
				xPortGetFreeHeapSize());
	}
	return ret;
}

void nvs_init() {
	uint32_t startTick = xTaskGetTickCount();
	uint32_t heapBefore = xPortGetFreeHeapSize();
	esp_err_t ret;
	ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);

	ESP_LOGD(TAG, "NVS init complite. Duration: %d ms. Heap usage: %d free heap:%d", (xTaskGetTickCount() - startTick) * portTICK_RATE_MS, heapBefore - xPortGetFreeHeapSize(), xPortGetFreeHeapSize());
}

void st7789_init() {
#define CONFIG_WIDTH 240
#define CONFIG_HEIGHT 240

#define CONFIG_MOSI_GPIO 9
#define CONFIG_SCLK_GPIO 12
#define CONFIG_CS_GPIO 11
#define CONFIG_DC_GPIO 13
#define CONFIG_RESET_GPIO 14
#define CONFIG_BL_GPIO -1

	uint32_t startTick = xTaskGetTickCount();
	uint32_t heapBefore = xPortGetFreeHeapSize();

	spi_master_init(&screen, CONFIG_MOSI_GPIO, CONFIG_SCLK_GPIO, CONFIG_CS_GPIO,
	CONFIG_DC_GPIO, CONFIG_RESET_GPIO, CONFIG_BL_GPIO);
	lcdInit(&screen, CONFIG_WIDTH, CONFIG_HEIGHT, 0, 0);

	InitFontx(fx32G, "/spiffs/ILGH32XB.FNT", ""); // 16x32Dot Gothic

	lcdFillScreen(&screen, GREEN);

	ESP_LOGD(TAG, "Screen init complite. Duration: %d ms. Heap usage: %d free heap:%d", (xTaskGetTickCount() - startTick) * portTICK_RATE_MS, heapBefore - xPortGetFreeHeapSize(),
			xPortGetFreeHeapSize());
}

void spiffs_init() {

	uint32_t startTick = xTaskGetTickCount();
	uint32_t heapBefore = xPortGetFreeHeapSize();
	esp_err_t ret;
	esp_vfs_spiffs_conf_t conf = { .base_path = "/spiffs", .partition_label =
	NULL, .max_files = 10, .format_if_mount_failed = true };
	ret = esp_vfs_spiffs_register(&conf);
	size_t total = 0, used = 0;
	ret = esp_spiffs_info(NULL, &total, &used);

	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
	} else {
		ESP_LOGD(TAG, "SPIFFS init complite. Duration: %d ms. Heap usage: %d free heap:%d", (xTaskGetTickCount() - startTick) * portTICK_RATE_MS, heapBefore - xPortGetFreeHeapSize(),
				xPortGetFreeHeapSize());
		ESP_LOGD(TAG, "Partition size: total: %d, used: %d", total, used);
	}
}

void relayGPIO_init() {
	uint32_t startTick = xTaskGetTickCount();
	uint32_t heapBefore = xPortGetFreeHeapSize();

	gpio_reset_pin(RELAY_1_GPIO);
	gpio_reset_pin(RELAY_2_GPIO);
	gpio_set_direction(RELAY_1_GPIO, GPIO_MODE_OUTPUT);
	gpio_set_direction(RELAY_2_GPIO, GPIO_MODE_OUTPUT);

	ESP_LOGD(TAG, "GPIO init complite. Duration: %d ms. Heap usage: %d free heap:%d", (xTaskGetTickCount() - startTick) * portTICK_RATE_MS, heapBefore - xPortGetFreeHeapSize(),
			xPortGetFreeHeapSize());
}

int my_mega_fn(void) {

	return 1;
}

void mdns_start(void) {
	uint32_t startTick = xTaskGetTickCount();
	uint32_t heapBefore = xPortGetFreeHeapSize();
	char mdnsName[80];

	// set mDNS hostname (required if you want to advertise services)
	if (strlen(monofon_config.device_name) == 0) {
		sprintf(mdnsName, "%s", (char*) monofon_config.ssidT);
		strcpy(mdnsName, monofon_config.ssidT);
		ESP_LOGD(TAG, "Set mdns name: %s  device_name len:%d ", mdnsName, strlen(monofon_config.device_name));
	} else {
		sprintf(mdnsName, "%s", monofon_config.device_name);
		ESP_LOGD(TAG, "Set mdns name: %s  device_name len:%d ", mdnsName, strlen(monofon_config.device_name));
	}

	ESP_ERROR_CHECK(mdns_init());
	ESP_ERROR_CHECK(mdns_hostname_set(mdnsName));
	ESP_ERROR_CHECK(mdns_instance_name_set("monofon-instance"));
	mdns_service_add(NULL, "_ftp", "_tcp", 21, NULL, 0);
	mdns_service_instance_name_set("_ftp", "_tcp", "Monofon FTP server");
	strcat(mdnsName, ".local");
	ESP_LOGI(TAG, "mdns hostname set to: %s", mdnsName);
	mdns_txt_item_t serviceTxtData[1] = { { "URL", strdup(mdnsName) }, };
	//sprintf()
	mdns_service_txt_set("_ftp", "_tcp", serviceTxtData, 1);

	ESP_LOGD(TAG, "mdns_start complite. Duration: %d ms. Heap usage: %d free heap:%d", (xTaskGetTickCount() - startTick) * portTICK_RATE_MS, heapBefore - xPortGetFreeHeapSize(),
			xPortGetFreeHeapSize());
}

void console_init() {
	uint32_t startTick = xTaskGetTickCount();
	uint32_t heapBefore = xPortGetFreeHeapSize();

	esp_console_repl_t *repl = NULL;
	esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
	esp_console_dev_uart_config_t uart_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
	/* Prompt to be printed before each line.
	 * This can be customized, made dynamic, etc.
	 */
	repl_config.prompt = "";
	//repl_config.max_cmdline_length = 150;

	esp_console_register_help_command();
	ESP_ERROR_CHECK(esp_console_new_repl_uart(&uart_config, &repl_config, &repl));

	//esp_console_dev_usb_cdc_config_t cdc_config = ESP_CONSOLE_DEV_CDC_CONFIG_DEFAULT();
	//ESP_ERROR_CHECK(esp_console_new_repl_usb_cdc(&cdc_config,&repl_config, &repl));

	register_console_cmd();

	ESP_LOGD(TAG, "Console init complite. Duration: %d ms. Heap usage: %d free heap:%d", (xTaskGetTickCount() - startTick) * portTICK_RATE_MS, heapBefore - xPortGetFreeHeapSize(),
			xPortGetFreeHeapSize());
	ESP_ERROR_CHECK(esp_console_start_repl(repl));
}

void setLogLevel(uint8_t level) {
	if (level == 3) {
		level = ESP_LOG_INFO;
	} else if (level == 4) {
		level = ESP_LOG_DEBUG;
	} else if (level == 2) {
		level = ESP_LOG_WARN;
	} else if (level == 1) {
		level = ESP_LOG_ERROR;
	} else if (level == 0) {
		level = ESP_LOG_NONE;
	} else if (level == 5) {
		level = ESP_LOG_VERBOSE;
	}

	esp_log_level_set("*", ESP_LOG_ERROR);
	esp_log_level_set("stateConfig", level);
	esp_log_level_set("console", level);
	esp_log_level_set("MAIN", level);
	esp_log_level_set(TAG, level);
	esp_log_level_set("AUDIO", level);
	esp_log_level_set("AUDIO_ELEMENT", ESP_LOG_ERROR);
	esp_log_level_set("MP3_DECODER", ESP_LOG_ERROR);
	esp_log_level_set("CODEC_ELEMENT_HELPER:", ESP_LOG_ERROR);
	esp_log_level_set("FATFS_STREAM", ESP_LOG_ERROR);
	esp_log_level_set("AUDIO_PIPELINE", ESP_LOG_ERROR);
	esp_log_level_set("I2S_STREAM", ESP_LOG_ERROR);
	esp_log_level_set("RSP_FILTER", ESP_LOG_ERROR);
	esp_log_level_set("WIFI", level);
	esp_log_level_set("esp_netif_handlers", level);
	esp_log_level_set("[Ftp]", level);
	esp_log_level_set("system_api", level);
	esp_log_level_set("MDNS", level);
	esp_log_level_set("[Ftp]", level);
	esp_log_level_set("mqtt", level);
	esp_log_level_set("leds", level);
	esp_log_level_set("ST7789", level);
	esp_log_level_set("JPED_Decoder", level);
	esp_log_level_set("SENS", level);
	esp_log_level_set("SDSPI", level);

}

void app_main(void) {
	setLogLevel(4);
	initLeds();
	board_init();

	spiffs_init();
	nvs_init();

	st7789_init();

	ESP_LOGI(TAG, "try init sdCard");
	//if (sd_card_init() != ESP_OK) {
	if (spisd_init() != ESP_OK) {
		lcdDrawString(&screen, fx32G, 25, 140, (uint8_t*) "SD card FAIL", RED);
		ESP_LOGE(TAG, "spisd_init FAIL");
		showState(LED_STATE_SD_ERROR);
		monofon_state.sd_error = 1;
		esp_restart();
	} else {
		//spisd_mount_fs();
		monofon_state.sd_error = 0;
		if (remove("/sdcard/error.txt")) {
			ESP_LOGD(TAG, "/sdcard/error.txt delete failed");
		}
		setMscEnabled(1);
		setMscEnabled(0);

		load_Default_Config();
		int res = loadConfig();
		if (res == ESP_OK) {
			monofon_state.config_error = 0;
			setLogLevel(monofon_config.logLevel);
		} else {
			lcdDrawString(&screen, fx32G, 20, 140, (uint8_t*) "Load config FAIL", RED);
			showState(LED_STATE_CONFIG_ERROR);
			char tmpString[40];
			sprintf(tmpString, "Load config FAIL in line: %d", res);
			writeErrorTxt(tmpString);
			monofon_state.config_error = 1;
		}

		if (loadContent() == ESP_OK) {
			monofon_state.content_error = 0;

			JPEGTest(&screen, monofon_config.introIco, 240, 240);

		} else {
			ESP_LOGD(TAG, "Load Content FAIL");
			lcdFillScreen(&screen, BLACK);
			lcdDrawString(&screen, fx32G, 15, 140,(uint8_t*) "Load content FAIL", RED);
			showState(LED_STATE_CONTENT_ERROR);
			writeErrorTxt("Load content FAIL");
			monofon_state.content_error = 1;
		}

		if (wifiInit() != ESP_OK) {
			monofon_state.wifi_error = 1;
			lcdFillScreen(&screen, BLACK);
			lcdDrawString(&screen, fx32G, 100, 50, (uint8_t*) "WIFI connect FAIL", RED);
			showState(LED_STATE_WIFI_FAIL);
			wifi_scan();
		} else {

			if (monofon_config.WIFI_mode != 0) {

				xTaskCreatePinnedToCore(ftp_task, "FTP", 1024 * 6, NULL, 2, NULL, 0);

				vTaskDelay(pdMS_TO_TICKS(100));
				mdns_start();
				//mqtt_app_start();
			}
		}

	}

	xTaskCreatePinnedToCore(playerTask, "player", 1024 * 8, NULL, 1, NULL, 0);
	vTaskDelay(pdMS_TO_TICKS(100));
	xTaskCreatePinnedToCore(sensTask, "sens", 1024 * 4, NULL, 24, NULL, 1);

	console_init();

	vTaskDelay(pdMS_TO_TICKS(100));
	ESP_LOGI(TAG, "Load complite, start working. free Heap size %d", xPortGetFreeHeapSize());

	while (1) {

		vTaskDelay(pdMS_TO_TICKS(100));
		if (monofon_state.mqtt_error == 0) {
			char tmpString[10];
			sprintf(tmpString, "%d", pdTICKS_TO_MS(xTaskGetTickCount()) / 1000);
			//mqtt_pub(lifeTime_topic,tmpString);
		}
		if (monofon_state.changeLang == 1) {
			ESP_LOGI(TAG, "Change lang to:%d", monofon_state.currentLang);
			//monofon_state.changeLang = 0;
		}

	}
}

/*
 void app_main_2(void) {

 board_init();

 setLogLevel(4);
 ESP_LOGD(TAG, "Start up");
 ESP_LOGD(TAG, "free Heap size %d", xPortGetFreeHeapSize());

 loadMscFlag();

 if (isMscEnabled()) {
 spisd_init();
 spisd_mount_fs();

 xTaskCreateStatic(usb_device_task, "usbd", USBD_STACK_SIZE, NULL,
 configMAX_PRIORITIES - 1, usb_device_stack, &usb_device_taskdef);
 xTaskCreateStatic(cdc_task, "cdc", CDC_STACK_SZIE, NULL,
 configMAX_PRIORITIES - 2, cdc_stack, &cdc_taskdef);

 while (1) {
 vTaskDelay(pdMS_TO_TICKS(100));
 }
 }
 //else
 //default_main_app();
 }

 */
// USB Device Driver task
// This top level thread process all usb events and invoke callbacks
extern void reconnectUsb();

void usb_device_task(void *param) {
	(void) param;

	// init device stack on configured roothub port
	// This should be called after scheduler/kernel is started.
	// Otherwise it could cause kernel issue since USB IRQ handler does use RTOS queue API.
	tud_init(BOARD_TUD_RHPORT);

	reconnectUsb();

	// RTOS forever loop
	while (1) {
		// put this thread to waiting state until there is new events
		tud_task();

		// following code only run if tud_task() process at least 1 event
		tud_cdc_write_flush();
	}
}
