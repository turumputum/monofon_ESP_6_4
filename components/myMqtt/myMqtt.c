#include <stdio.h>
#include "myMqtt.h"
#include "stateConfig.h"
#include "esp_log.h"
#include "mqtt_client.h"

#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
static const char *TAG = "mqtt";

extern stateStruct monofon_state;
extern configuration monofon_config;

char phonUp_State_topic[100];
char lifeTime_topic[100];

esp_mqtt_client_handle_t client;

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    //esp_mqtt_client_handle_t client = event->client;
    //int msg_id;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGD(TAG, "MQTT_EVENT_CONNECTED");
        monofon_state.mqtt_error=0;
        ESP_LOGD(TAG, "MQTT_CONNEKT_OK");
        //msg_id = esp_mqtt_client_subscribe(client, "phonState_topic", 0);
        //ESP_LOGD(TAG, "sent subscribe successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGD(TAG, "MQTT_EVENT_DISCONNECTED");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGD(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGD(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGD(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        //printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        //printf("DATA=%.*s\r\n", event->data_len, event->data);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));

        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

void mqtt_app_start(void)
{
    uint32_t startTick = xTaskGetTickCount();
    uint32_t heapBefore = xPortGetFreeHeapSize();

    if(monofon_config.mqttBrokerAdress[0]==0){
        ESP_LOGW(TAG, "Empty mqtt broker adress, mqtt disable");
        monofon_state.mqtt_error=1;
        return;
    }

    esp_mqtt_client_config_t mqtt_cfg = {
        //.host = "192.168.88.99",
        .host = monofon_config.mqttBrokerAdress,
    };

    if(strlen(monofon_config.device_name)==0){
        ESP_LOGD(TAG, "Device name is empty, set mac identifier");
        mqtt_cfg.client_id = monofon_config.ssidT;
        sprintf(phonUp_State_topic,"%s/phoneUp_state",monofon_config.ssidT);
        sprintf(lifeTime_topic,"%s/lifeTime_sek",monofon_config.ssidT);
    }else{
       sprintf(phonUp_State_topic,"%s/phoneUp_state",monofon_config.device_name); 
       sprintf(lifeTime_topic,"%s/lifeTime_sek",monofon_config.device_name);
       mqtt_cfg.client_id = monofon_config.device_name;
    }
    

    client = esp_mqtt_client_init(&mqtt_cfg);
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    ESP_ERROR_CHECK(esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL));
    ESP_ERROR_CHECK(esp_mqtt_client_start(client));
    monofon_state.mqtt_error=1;

    ESP_LOGD(TAG, "MQTT init complite. Duration: %d ms. Heap usage: %d", (xTaskGetTickCount() - startTick) * portTICK_RATE_MS, heapBefore - xPortGetFreeHeapSize());
}

void mqtt_pub(const char *topic, const char *string){
    int msg_id = esp_mqtt_client_publish(client, topic, string, 0, 0, 0);
    ESP_LOGD(TAG, "sent publish successful, msg_id=%d", msg_id);
}

