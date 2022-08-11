#include "leds.h"
#include <ff.h>

typedef enum {
    LED_STATE_DISABLE = 0,
    LED_STATE_SD_ERROR,
    LED_STATE_CONFIG_ERROR,
    LED_STATE_CONTENT_ERROR,
	LED_STATE_SENSOR_ERROR,
    LED_STATE_WIFI_FAIL,
	LED_STATE_WIFI_OK,
    LED_STATE_STANDBY,
    LED_STATE_PLAY,
    LED_STATE_FTP_SESSION,
	LED_STATE_MSD_WORK,
} led_state_t;

typedef struct {
	uint8_t changeLang;
	uint8_t currentLang;
	uint8_t numOfLang;
	uint8_t phoneUp;
	uint8_t prevPhoneUp;

	char * wifiApClientString;

	uint8_t sd_error;
	uint8_t config_error;
	uint8_t content_error;
	uint8_t wifi_error;
	uint8_t mqtt_error;
	uint8_t introIco_error;

	led_state_t ledState;
} stateStruct;



typedef struct {
	TCHAR audioFile[1024];
	TCHAR icoFile[1024];
} langs_t;

typedef struct {
	uint8_t WIFI_mode; 
	char * WIFI_ssid;
	char * WIFI_pass;
	uint8_t WIFI_channel;
	
	uint8_t DHCP;

	char *ipAdress;
	char *netMask;
	char *gateWay;

	char *device_name;

	char *FTP_login;
	char *FTP_pass;

	char *mqttBrokerAdress;
	char *mqttLogin;
	char *mqttPass;

	char *mqttTopic_phoneUp;
	char *mqttTopic_lifetime;

	uint8_t logLevel;
	
	uint8_t playerMode;
	int volume;

	int brightMax;
	int brightMin;
	RgbColor RGB;
	uint8_t animate;
	uint8_t rainbow;

	langs_t lang[3];
	uint8_t defaultLang;

	uint8_t touchSensInverted;
	uint8_t phoneSensInverted;

	uint8_t sensMode;
	uint8_t sensDebug;
	uint16_t magnitudeLevel;

	char introIco[1024];

	char ssidT[33];

} configuration;


uint8_t loadConfig(void);
FRESULT scan_in_dir(const char *file_extension, FF_DIR *dp, FILINFO *fno);
void load_Default_Config(void);
void writeErrorTxt(const char *buff);
uint8_t loadContent(void);
int saveConfig(void);
