#include <stdio.h>
#include "WIFI.h"

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_sntp.h"
#include "esp_err.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "stateConfig.h"

#define EXAMPLE_ESP_MAXIMUM_RETRY 5

#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

static const char *TAG = "WIFI";
static EventGroupHandle_t s_wifi_event_group;

extern stateStruct monofon_state;
extern configuration monofon_config;

static int s_retry_num = 0;

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED)
    {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
        ESP_LOGD(TAG, "Connectend client " MACSTR " join, AID=%d \r\n",
                MAC2STR(event->mac), event->aid);
    }
    else if (event_id == WIFI_EVENT_AP_STADISCONNECTED)
    {
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
        ESP_LOGD(TAG, "station " MACSTR " leave, AID=%d",
                 MAC2STR(event->mac), event->aid);
        //sprintf(monofon_state.wifiApClientString, "\r\n");
    }
}

static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY)
        {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGD(TAG, "retry to connect to the AP");
        }
        else
        {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGD(TAG, "connect to the AP fail");
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        sprintf(monofon_config.ipAdress, IPSTR, IP2STR(&event->ip_info.ip));
        sprintf(monofon_config.netMask, IPSTR, IP2STR(&event->ip_info.netmask));
        sprintf(monofon_config.gateWay, IPSTR, IP2STR(&event->ip_info.gw));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}
/*
void wifi_init_softap(void)
{
    ESP_LOGI(TAG, "Start init soft AP");
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_LOGI(TAG, "NetIf init complite");
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_LOGI(TAG, "esp_event_loop_create_default init complite");

    // esp_netif_create_default_wifi_ap();
    // ESP_LOGI(TAG,"create_default_wifi_ap init complite");

    esp_netif_t *wifiAP = esp_netif_create_default_wifi_ap();
    esp_netif_ip_info_t ipInfo;

    uint8_t ipAdr[4] = {10, 0, 0, 1};
    uint8_t netMask[4];
    uint8_t gateWay[4];

    char delim[] = ".";
    char *nexttok;
    char *reserve;



    ESP_LOGI(TAG, "Parse MASK: %s", monofon_config.netMask);
    nexttok = strtok_r((char *)monofon_config.netMask, delim, &reserve);
    netMask[0] = atoi(nexttok);
    nexttok = strtok_r(NULL, delim, &reserve);
    netMask[1] = atoi(nexttok);
    nexttok = strtok_r(NULL, delim, &reserve);
    netMask[2] = atoi(nexttok);
    nexttok = strtok_r(NULL, delim, &reserve);
    netMask[3] = atoi(nexttok);

    ESP_LOGI(TAG, "Parse GATEWAY: %s", monofon_config.gateWay);
    nexttok = strtok_r((char *)monofon_config.gateWay, delim, &reserve);
    gateWay[0] = atoi(nexttok);
    nexttok = strtok_r(NULL, delim, &reserve);
    gateWay[1] = atoi(nexttok);
    nexttok = strtok_r(NULL, delim, &reserve);
    gateWay[2] = atoi(nexttok);
    nexttok = strtok_r(NULL, delim, &reserve);
    gateWay[3] = atoi(nexttok);

    IP4_ADDR(&ipInfo.ip, ipAdr[0], ipAdr[1], ipAdr[2], ipAdr[3]);
    IP4_ADDR(&ipInfo.gw, gateWay[0], gateWay[1], gateWay[2], gateWay[3]);
    IP4_ADDR(&ipInfo.netmask, netMask[0], netMask[1], netMask[2], netMask[3]);

    esp_netif_dhcps_stop(wifiAP);
    ESP_LOGI(TAG, "DHCP stoped");
    esp_netif_set_ip_info(wifiAP, &ipInfo);
    ESP_LOGI(TAG, "IP config complite");
    esp_netif_dhcps_start(wifiAP);
    ESP_LOGI(TAG, "DHCP started");

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_LOGI(TAG, "esp_wifi_init complite");

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));
    ESP_LOGI(TAG, "esp_event_handler_instance_register complite");

    char ssidT[33];
    int ssidLen;
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = "",
            .ssid_len = 0,
            .channel = monofon_config.WIFI_channel,
            .password = "",
            .max_connection = 2,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK},
    };

    if (monofon_config.WIFI_ssid[0] == 0)
    {
        ESP_LOGI(TAG, "Construct uniqSSID");
        uint8_t tmpMac[6];
        esp_read_mac(tmpMac, ESP_MAC_WIFI_SOFTAP);
        ssidLen = sprintf(ssidT, "MonofonAP-%02x:%02x:%02x:%02x:%02x:%02x", tmpMac[0], tmpMac[1], tmpMac[2], tmpMac[3], tmpMac[4], tmpMac[5]);
    }
    else
    {
        ssidLen = sprintf(ssidT, "%s", monofon_config.WIFI_ssid);
    }
    ESP_LOGI(TAG, "Construct ssid: %s, ssidLen: %d", ssidT, ssidLen);

    strcpy((char *)wifi_config.ap.ssid, (char *)ssidT);
    wifi_config.ap.ssid_len = ssidLen;
    strcpy((char *)wifi_config.ap.password, (char *)monofon_config.WIFI_pass);

    if (monofon_config.WIFI_pass[0] == 0)
    {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_LOGI(TAG, "starting softAP");
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_LOGI(TAG, "esp_wifi_set_mode complite");
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_LOGI(TAG, "esp_wifi_set_config complite");
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
             wifi_config.ap.ssid, wifi_config.ap.password, monofon_config.WIFI_channel);
}

void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "Home",
            .password = "123456q!"},
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_sta finished.");


    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           portMAX_DELAY);


    if (bits & WIFI_CONNECTED_BIT)
    {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 wifi_config.sta.ssid, wifi_config.sta.password);
    }
    else if (bits & WIFI_FAIL_BIT)
    {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 wifi_config.sta.ssid, wifi_config.sta.password);
    }
    else
    {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }

    ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler));
    ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler));
    vEventGroupDelete(s_wifi_event_group);
}
*/
void time_sync_notification_cb(struct timeval *tv)
{
    ESP_LOGI(TAG, "Notification of a time synchronization event");
}

static void initialize_sntp(void)
{
    ESP_LOGI(TAG, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    // ESP_LOGI(TAG, "Your NTP Server is %s", CONFIG_NTP_SERVER);
    // sntp_setservername(0, CONFIG_NTP_SERVER);
    sntp_set_time_sync_notification_cb(time_sync_notification_cb);
    sntp_init();
}

void obtain_time(void)
{
    initialize_sntp();
    // wait for time to be set
    int retry = 0;
    const int retry_count = 10;
    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count)
    {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

void wifi_scan(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    uint16_t number = 20;
    wifi_ap_record_t ap_info[number];
    uint16_t ap_count = 0;
    memset(ap_info, 0, sizeof(ap_info));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    esp_wifi_scan_start(NULL, true);
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
    printf("Avalable networks = %u \r\n", ap_count);
    for (int i = 0; (i < number) && (i < ap_count); i++)
    {
        printf("SSID \t%s", ap_info[i].ssid);
        printf("\tRSSI \t%d", ap_info[i].rssi);
        printf("\tChannel \t%d \n", ap_info[i].primary);
    }
}

uint8_t wifiInit()
{
    uint32_t startTick = xTaskGetTickCount();
    uint32_t heapBefore = xPortGetFreeHeapSize();

    if (monofon_config.WIFI_mode == 1)
    {
        ESP_LOGI(TAG, "Start init soft AP");
        ESP_ERROR_CHECK(esp_netif_init());
        ESP_LOGD(TAG, "NetIf init complite");
        ESP_ERROR_CHECK(esp_event_loop_create_default());
        ESP_LOGD(TAG, "esp_event_loop_create_default init complite");
        esp_netif_t *wifiAP = esp_netif_create_default_wifi_ap();
        esp_netif_ip_info_t ipInfo;

        if (monofon_config.DHCP == 0)
        {

            uint8_t ipAdr[4] = {10, 0, 0, 1};
            uint8_t netMask[4];
            uint8_t gateWay[4];

            char delim[] = ".";
            char *nexttok;
            char *reserve;

            ESP_LOGD(TAG, "Parse IP: %s", monofon_config.ipAdress);
            nexttok = strtok_r((char *)monofon_config.ipAdress, delim, &reserve);
            ipAdr[0] = atoi(nexttok);
            nexttok = strtok_r(NULL, delim, &reserve);
            ipAdr[1] = atoi(nexttok);
            nexttok = strtok_r(NULL, delim, &reserve);
            ipAdr[2] = atoi(nexttok);
            nexttok = strtok_r(NULL, delim, &reserve);
            ipAdr[3] = atoi(nexttok);

            ESP_LOGD(TAG, "Parse MASK: %s", monofon_config.netMask);
            nexttok = strtok_r((char *)monofon_config.netMask, delim, &reserve);
            netMask[0] = atoi(nexttok);
            nexttok = strtok_r(NULL, delim, &reserve);
            netMask[1] = atoi(nexttok);
            nexttok = strtok_r(NULL, delim, &reserve);
            netMask[2] = atoi(nexttok);
            nexttok = strtok_r(NULL, delim, &reserve);
            netMask[3] = atoi(nexttok);

            ESP_LOGD(TAG, "Parse GATEWAY: %s", monofon_config.gateWay);
            nexttok = strtok_r((char *)monofon_config.gateWay, delim, &reserve);
            gateWay[0] = atoi(nexttok);
            nexttok = strtok_r(NULL, delim, &reserve);
            gateWay[1] = atoi(nexttok);
            nexttok = strtok_r(NULL, delim, &reserve);
            gateWay[2] = atoi(nexttok);
            nexttok = strtok_r(NULL, delim, &reserve);
            gateWay[3] = atoi(nexttok);

            IP4_ADDR(&ipInfo.ip, ipAdr[0], ipAdr[1], ipAdr[2], ipAdr[3]);
            IP4_ADDR(&ipInfo.gw, gateWay[0], gateWay[1], gateWay[2], gateWay[3]);
            IP4_ADDR(&ipInfo.netmask, netMask[0], netMask[1], netMask[2], netMask[3]);

            esp_netif_dhcps_stop(wifiAP);
            ESP_LOGD(TAG, "DHCP stoped");
            esp_netif_set_ip_info(wifiAP, &ipInfo);
            ESP_LOGD(TAG, "IP config complite");
            esp_netif_dhcps_start(wifiAP);
            ESP_LOGD(TAG, "DHCP started");
        }

        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        cfg.nvs_enable = false;
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));
        ESP_LOGD(TAG, "esp_wifi_init complite");

        ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                            ESP_EVENT_ANY_ID,
                                                            &wifi_event_handler,
                                                            NULL,
                                                            NULL));
        ESP_LOGD(TAG, "esp_event_handler_instance_register complite");

        int ssidLen;
        wifi_config_t wifi_config = {
            .ap = {
                .ssid = "",
                .ssid_len = 0,
                .channel = monofon_config.WIFI_channel,
                .password = "",
                .max_connection = 2,
                .authmode = WIFI_AUTH_WPA_WPA2_PSK},
        };

        if (strlen(monofon_config.WIFI_ssid) == 0)
        {
            ESP_LOGD(TAG, "Construct uniqSSID");
            uint8_t tmpMac[6];
            esp_read_mac(tmpMac, ESP_MAC_WIFI_SOFTAP);
            ssidLen = sprintf(monofon_config.ssidT, "MonofonAP-%02x:%02x:%02x:%02x:%02x:%02x", tmpMac[0], tmpMac[1], tmpMac[2], tmpMac[3], tmpMac[4], tmpMac[5]);
        }
        else
        {
            ssidLen = sprintf(monofon_config.ssidT, "%s", monofon_config.WIFI_ssid);
        }
        ESP_LOGD(TAG, "Construct ssid: %s, ssidLen: %d", monofon_config.ssidT, ssidLen);

        memcpy(wifi_config.ap.ssid, monofon_config.ssidT, strlen(monofon_config.ssidT));
        wifi_config.ap.ssid_len = ssidLen;
        // memcpy(wifi_config.ap.password,monofon_config.WIFI_pass, strlen(monofon_config.WIFI_pass));
        memcpy(wifi_config.sta.password, monofon_config.WIFI_pass, sizeof(wifi_config.sta.ssid));
        ESP_LOGD(TAG, "Pass is %s", wifi_config.sta.password);

        if (strlen(monofon_config.WIFI_pass) == 0)
        {
            wifi_config.ap.authmode = WIFI_AUTH_OPEN;
        }

        ESP_LOGD(TAG, "starting softAP");
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
        ESP_LOGD(TAG, "esp_wifi_set_mode complite");
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
        ESP_LOGD(TAG, "esp_wifi_set_config complite");
        ESP_ERROR_CHECK(esp_wifi_start());

        esp_netif_get_ip_info(wifiAP, &ipInfo);

        ESP_LOGI(TAG, "My IP: " IPSTR, IP2STR(&ipInfo.ip));
        sprintf(monofon_config.ipAdress, IPSTR, IP2STR(&ipInfo.ip));
        sprintf(monofon_config.netMask, IPSTR, IP2STR(&ipInfo.netmask));
        sprintf(monofon_config.gateWay, IPSTR, IP2STR(&ipInfo.gw));
        ESP_LOGI(TAG, "wifi_init_softap finished.  SSID:%s password:%s channel:%d",
                 wifi_config.ap.ssid, wifi_config.ap.password, monofon_config.WIFI_channel);
    }
    else if (monofon_config.WIFI_mode == 2)
    {
        ESP_LOGI(TAG, "Start init wifi in station mode");

        if (monofon_config.WIFI_ssid[0] == 0)
        {
            ESP_LOGE(TAG, "WIFI ssid is emty");
            writeErrorTxt("WIFI ssid is emty");
            return ESP_FAIL;
        }

        if (monofon_config.WIFI_pass[0] == 0)
        {
            ESP_LOGE(TAG, "WIFI pass is emty");
            writeErrorTxt("WIFI pass is emty");
            return ESP_FAIL;
        }

        s_wifi_event_group = xEventGroupCreate();

        ESP_ERROR_CHECK(esp_netif_init());

        ESP_ERROR_CHECK(esp_event_loop_create_default());
        esp_netif_t *wifiSta = esp_netif_create_default_wifi_sta();

        if (monofon_config.DHCP == 0)
        {
            ESP_ERROR_CHECK(esp_netif_dhcpc_stop(wifiSta));
            esp_netif_ip_info_t ipInfo;

            uint8_t ipAdr[4] = {10, 0, 0, 1};
            uint8_t netMask[4];
            uint8_t gateWay[4];

            char delim[] = ".";
            char *nexttok;
            char *reserve;

            ESP_LOGD(TAG, "Parse IP: %s", monofon_config.ipAdress);
            nexttok = strtok_r((char *)monofon_config.ipAdress, delim, &reserve);
            ipAdr[0] = atoi(nexttok);
            nexttok = strtok_r(NULL, delim, &reserve);
            ipAdr[1] = atoi(nexttok);
            nexttok = strtok_r(NULL, delim, &reserve);
            ipAdr[2] = atoi(nexttok);
            nexttok = strtok_r(NULL, delim, &reserve);
            ipAdr[3] = atoi(nexttok);

            ESP_LOGD(TAG, "Parse MASK: %s", monofon_config.netMask);
            nexttok = strtok_r((char *)monofon_config.netMask, delim, &reserve);
            netMask[0] = atoi(nexttok);
            nexttok = strtok_r(NULL, delim, &reserve);
            netMask[1] = atoi(nexttok);
            nexttok = strtok_r(NULL, delim, &reserve);
            netMask[2] = atoi(nexttok);
            nexttok = strtok_r(NULL, delim, &reserve);
            netMask[3] = atoi(nexttok);

            ESP_LOGD(TAG, "Parse GATEWAY: %s", monofon_config.gateWay);
            nexttok = strtok_r((char *)monofon_config.gateWay, delim, &reserve);
            gateWay[0] = atoi(nexttok);
            nexttok = strtok_r(NULL, delim, &reserve);
            gateWay[1] = atoi(nexttok);
            nexttok = strtok_r(NULL, delim, &reserve);
            gateWay[2] = atoi(nexttok);
            nexttok = strtok_r(NULL, delim, &reserve);
            gateWay[3] = atoi(nexttok);

            IP4_ADDR(&ipInfo.ip, ipAdr[0], ipAdr[1], ipAdr[2], ipAdr[3]);
            IP4_ADDR(&ipInfo.gw, gateWay[0], gateWay[1], gateWay[2], gateWay[3]);
            IP4_ADDR(&ipInfo.netmask, netMask[0], netMask[1], netMask[2], netMask[3]);

            //
            // ESP_LOGD(TAG, "DHCP stoped");
            ESP_ERROR_CHECK(esp_netif_set_ip_info(wifiSta, &ipInfo));
            ESP_LOGD(TAG, "IP config complite");
            // esp_netif_dhcps_start(wifiSta);
            // ESP_LOGD(TAG, "DHCP started");
        }

        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        cfg.nvs_enable = false;
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));

        ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
        ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

        wifi_config_t wifi_config = {
            .sta.ssid = "",
            .sta.password = ""};

        memcpy(wifi_config.sta.ssid, monofon_config.WIFI_ssid, sizeof(wifi_config.sta.ssid));
        memcpy(wifi_config.sta.password, monofon_config.WIFI_pass, sizeof(wifi_config.sta.ssid));
        // ESP_LOGD(TAG, "srt pass %s len %d", monofon_config.WIFI_pass, strlen(monofon_config.WIFI_pass));


        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
        ESP_ERROR_CHECK(esp_wifi_start());

        /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
         * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
        EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                               WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                               pdFALSE,
                                               pdFALSE,
                                               portMAX_DELAY);

        /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
         * happened. */
        if (bits & WIFI_CONNECTED_BIT)
        {
            ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                     wifi_config.sta.ssid, wifi_config.sta.password);
            uint8_t tmpMac[6];
            esp_read_mac(tmpMac, ESP_MAC_WIFI_STA);
            sprintf(monofon_config.ssidT, "Monofon-%02x:%02x:%02x:%02x:%02x:%02x", tmpMac[0], tmpMac[1], tmpMac[2], tmpMac[3], tmpMac[4], tmpMac[5]);
        }
        else if (bits & WIFI_FAIL_BIT)
        {
            ESP_LOGE(TAG, "Failed to connect to SSID:%s, password:%s",
                     wifi_config.sta.ssid, wifi_config.sta.password);
            char tmpString[200];
            sprintf(tmpString, "Failed to connect to SSID:%s, password:%s", wifi_config.sta.ssid, wifi_config.sta.password);
            writeErrorTxt(tmpString);
            return ESP_FAIL;
        }
        else
        {
            ESP_LOGE(TAG, "UNEXPECTED EVENT");
        }
        ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler));
        ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler));
        vEventGroupDelete(s_wifi_event_group);
    }
    else
    {
        ESP_LOGD(TAG, "WIFI disable");
        return ESP_OK;
    }

    ESP_LOGD(TAG, "WIFI init complite. Duration: %d ms. Heap usage: %d free heap:%d", (xTaskGetTickCount() - startTick) * portTICK_RATE_MS, heapBefore - xPortGetFreeHeapSize(), xPortGetFreeHeapSize());
    return ESP_OK;
}
