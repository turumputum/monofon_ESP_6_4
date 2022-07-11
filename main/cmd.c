#include <stdio.h>
#include <string.h>
#include "esp_system.h"
#include "stateConfig.h"
#include "esp_log.h"
#include "esp_console.h" 
#include "argtable3/argtable3.h"
#include "esp_wifi.h"
#include "WIFI.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

extern configuration monofon_config;
extern stateStruct monofon_state;

#define TAG "console"
//
#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG

static struct {
    struct arg_str *parameter;
    struct arg_str *value;
    struct arg_end *end;
} set_config_arg;

static int set_config(int argc, char **argv){
    int nerrors = arg_parse(argc, argv, (void **) &set_config_arg);
    if (nerrors != 0) {
        arg_print_errors(stderr, set_config_arg.end, argv[0]);
        return 1;
    }

    const char *parameter = set_config_arg.parameter->sval[0];
    const char *value = set_config_arg.value->sval[0];
        
    ESP_LOGD(TAG,"\r\n set val parameter: %s value: %s ", parameter, value);

    if(strcmp(parameter, "device_name")==0){
        sprintf(monofon_config.device_name,"%s",value);
    }else if(strcmp(parameter, "WIFI_mode")==0){
        monofon_config.WIFI_mode=atoi(value);
    }else if(strcmp(parameter, "WIFI_ssid")==0){
        sprintf(monofon_config.WIFI_ssid,"%s",value);
    }else if(strcmp(parameter, "WIFI_pass")==0){
        sprintf(monofon_config.WIFI_pass,"%s",value);
    }else if(strcmp(parameter, "DHCP")==0){
        monofon_config.DHCP=atoi(value);
    }else if(strcmp(parameter, "ipAdress")==0){
        sprintf(monofon_config.ipAdress,"%s",value);
    }else if(strcmp(parameter, "netMask")==0){
        sprintf(monofon_config.netMask,"%s",value);
    }else if(strcmp(parameter, "gateWay")==0){
        sprintf(monofon_config.gateWay,"%s",value);
    }else if(strcmp(parameter, "FTP_login")==0){
        sprintf(monofon_config.FTP_login,"%s",value);
    }else if(strcmp(parameter, "FTP_pass")==0){
        sprintf(monofon_config.FTP_pass,"%s",value);
    }else if(strcmp(parameter, "mqttBrokerAdress")==0){
        sprintf(monofon_config.mqttBrokerAdress,"%s",value);
    }else if(strcmp(parameter, "logLevel")==0){
        monofon_config.logLevel=atoi(value);
    }else if(strcmp(parameter, "brightMax")==0){
        monofon_config.brightMax=atoi(value);
    }else if(strcmp(parameter, "brightMin")==0){
        monofon_config.brightMin=atoi(value);
    }else if(strcmp(parameter, "RGB.r")==0){
        monofon_config.RGB.r=atoi(value);
    }else if(strcmp(parameter, "RGB.g")==0){
        monofon_config.RGB.g=atoi(value);
    }else if(strcmp(parameter, "RGB.b")==0){
        monofon_config.RGB.b=atoi(value);
    }else if(strcmp(parameter, "animate")==0){
        monofon_config.animate=atoi(value);
    }else if(strcmp(parameter, "rainbow")==0){
        monofon_config.rainbow=atoi(value);
    }else if(strcmp(parameter, "playerMode")==0){
        monofon_config.playerMode=atoi(value);
    }else if(strcmp(parameter, "volume")==0){
        monofon_config.volume=atoi(value);
    }else if(strcmp(parameter, "defaultLang")==0){
        monofon_config.defaultLang=atoi(value);
    }else if(strcmp(parameter, "sensMode")==0){
        monofon_config.sensMode=atoi(value);
    }else if(strcmp(parameter, "sensInverted")==0){
        monofon_config.sensInverted=atoi(value);
    }else if(strcmp(parameter, "sensDebug")==0){
        monofon_config.sensDebug=atoi(value);
    }else{
        printf(" \r\n wrong format \r\n ");
    }
    printf(" \r\n OK \r\n ");
    saveConfig();
    return ESP_OK;
}

int list_config(void){

	ESP_LOGD(TAG,"_DEBUG_ ");
    printf("\r\n\r\n"); 
    printf("List config: \r\n");
    printf("device_name: %s\r\n",monofon_config.device_name);
    printf("\r\n");
    printf("WIFI_mode: %d  // 0-disable, 1-AP mode, 2-station mode\r\n",monofon_config.WIFI_mode);
    if(strlen(monofon_config.WIFI_ssid)==0){
        printf("WIFI_ssid: %s\r\n", monofon_config.ssidT);
    }else{
        printf("WIFI_ssid: %s\r\n", monofon_config.WIFI_ssid);
    }
    printf("WIFI_pass: %s\r\n", monofon_config.WIFI_pass);
    printf("DHCP: %d \r\n",monofon_config.DHCP);
    printf("IP adress: %s \r\n",monofon_config.ipAdress);
    printf("Net mask: %s \r\n",monofon_config.netMask);
    printf("Gateway: %s \r\n",monofon_config.gateWay);
    printf("FTP_login: %s \r\n",monofon_config.FTP_login);
    printf("FTP_pass: %s \r\n",monofon_config.FTP_pass);
    printf("mqtt Broker Adress:%s\r\n",monofon_config.mqttBrokerAdress);

    printf("\r\n");
    printf("logLevel:%d // 0-none, 1-error, 2-warn, 3-info, 4-debug\r\n", monofon_config.logLevel);

    printf("\r\n");
    printf("brightMax:%d \r\n", monofon_config.brightMax);
    printf("brightMin:%d \r\n", monofon_config.brightMin);
    printf("RGB.r:%d \r\n", monofon_config.RGB.r);
    printf("RGB.g:%d \r\n", monofon_config.RGB.g);
    printf("RGB.b:%d \r\n", monofon_config.RGB.b);
    printf("animate:%d \r\n", monofon_config.animate);
    printf("rainbow:%d \r\n", monofon_config.rainbow);

    printf("\r\n");
    printf("playerMode:%d \r\n", monofon_config.playerMode);
    printf("volume:%d \r\n", monofon_config.volume);
    printf("defaultLang:%d \r\n", monofon_config.defaultLang);

    printf("\r\n");
    printf("sensMode:%d // 0-disable, 1-hall sensor mode, 2-button mode\r\n", monofon_config.sensMode);
    printf("sensInverted:%d \r\n", monofon_config.sensInverted);
    printf("sensDebug:%d \r\n", monofon_config.sensDebug);

    //saveConfig();
    return ESP_OK;
}

void wifiStatus(void){
    if(monofon_state.wifi_error==1){
        printf("Wifi connection error \r\n");
        wifi_scan();
    }else{
        if(monofon_config.WIFI_mode==0){
            printf("Wifi disabled \r\n");
        }else if(monofon_config.WIFI_mode==1){
            printf("Wifi softap mode.  SSID:%s password:%s channel:%d \r\n",
                 monofon_config.ssidT, monofon_config.WIFI_pass, monofon_config.WIFI_channel);
            //printf("Wifi clients: %s \r\n", monofon_state.wifiApClientString);
        }else if(monofon_config.WIFI_mode==2){
            wifi_ap_record_t ap;
            esp_wifi_sta_get_ap_info(&ap);
            printf("Wifi client mode.  SSID:%s password:%s rssi:%d\r\n",
                 monofon_config.WIFI_ssid, monofon_config.WIFI_pass, ap.rssi);
        }
    }
    
}

int status(void){
    printf("free Heap size %d \r\n", xPortGetFreeHeapSize());
    if(monofon_state.sd_error==1){
        printf("SD_card error\r\n");
    }
    if(monofon_state.config_error==1){
        printf("Config error\r\n");
    }
    if(monofon_state.content_error==1){
        printf("Content error\r\n");
    }
    if(monofon_state.wifi_error==1){
        printf("wifi error\r\n");
    }
    if(monofon_state.mqtt_error==1){
        printf("mqtt error\r\n");
    }


    printf( "Task Name\tStatus\tPrio\tHWM\tTask\tAffinity\n");
    char stats_buffer[1024];
    vTaskList(stats_buffer);
    printf("%s\n", stats_buffer);

    wifiStatus();
    return ESP_OK;

}

void resetConfig(void){
    if (remove("/sdcard/config.ini")!=ESP_OK)
    {
         ESP_LOGD(TAG, "/sdcard/config.ini delete failed");
         esp_restart();
		//return ESP_FAIL;
    }
}

esp_console_cmd_t cmd_status={
    .command = "status",
        .help = "Show current status",
        .hint = NULL,
        .func = &status,
        .argtable = NULL
    };

esp_console_cmd_t cmd_list_config={
    .command = "list",
        .help = "Show current config",
        .hint = NULL,
        .func = &list_config,
        .argtable = NULL
    };

esp_console_cmd_t cmd_set_config={
    .command = "set",
        .help = "set parameter value \n"
        "  Examples: set WIFI_mode 2 \n",
        .hint = NULL,
        .func = &set_config,
        .argtable = &set_config_arg
    };

esp_console_cmd_t cmd_restart={
    .command = "restart",
        .help = "restart device",
        .hint = NULL,
        .func = &esp_restart,
        .argtable = NULL
    };

esp_console_cmd_t cmd_resetConfig={
    .command = "reset_default",
        .help = "reset configuration, to default settings",
        .hint = NULL,
        .func = &resetConfig,
        .argtable = NULL
    };



void register_console_cmd(void){
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd_list_config));

    set_config_arg.parameter = arg_str1(NULL, NULL,"<parameter>", "use 'list_config' to see available options");
    set_config_arg.value = arg_str1(NULL, NULL, "<value>", "value to be stored");
    set_config_arg.end = arg_end(1);

    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd_status));
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd_set_config));
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd_resetConfig));
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd_restart));
}
