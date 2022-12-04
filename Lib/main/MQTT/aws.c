#include "aws.h"

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "cJSON.h"

#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt_client.h"

// #include "../LED/LED.h"
#include "../include/Common.h"
#include "../NVS/NVSDriver.h"
#include "../JSON/JSON.h"
TaskHandle_t AWSTaskHandle;
enum AWS_prov_state{
    CERTIFICATE_AND_KEY,
    PROVISIONING,
    THING_ACTIVE
};
enum AWS_prov_state aws_prov_state = CERTIFICATE_AND_KEY;

cJSON *aws_prov_response;

char *cloudEnv;

char* certificateId;
char* certificatePem;
char* privateKey;
char* certificateOwnershipToken;

char* thingName;

char MQTT_SUB_TOPIC[35];
char MQTT_PUB_TOPIC[35];
char CLIENT_ID[21];

static const char * TAG = "AWS";

static void AWSHandlerConnected(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");

    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;

    int msg_id = 0;

    if(aws_prov_state==CERTIFICATE_AND_KEY){
        //
        ESP_LOGI(TAG, "Subscribe to FP_JSON_CREATE_CERT_ACCEPTED_TOPIC");
        msg_id = esp_mqtt_client_subscribe(client, FP_JSON_CREATE_CERT_ACCEPTED_TOPIC, 1);
        ESP_LOGI(TAG, "Subscribe to FP_JSON_CREATE_CERT_ACCEPTED_TOPIC successful, msg_id=%d", msg_id);

        ESP_LOGI(TAG, "Subscribe to FP_JSON_CREATE_CERT_REJECTED_TOPIC");
        msg_id = esp_mqtt_client_subscribe(client, FP_JSON_CREATE_CERT_REJECTED_TOPIC, 1);
        ESP_LOGI(TAG, "Subscribe to FP_JSON_CREATE_CERT_REJECTED_TOPIC successful, msg_id=%d", msg_id);
        //
        ESP_LOGI(TAG, "Subscribe to FP_JSON_REGISTER_ACCEPTED_TOPIC(PROVISIONING_TEMPLATE_NAME)");
        msg_id = esp_mqtt_client_subscribe(client, FP_JSON_REGISTER_ACCEPTED_TOPIC(PROVISIONING_TEMPLATE_NAME), 1);
        ESP_LOGI(TAG, "Subscribe to FP_JSON_REGISTER_ACCEPTED_TOPIC(PROVISIONING_TEMPLATE_NAME) successful, msg_id=%d", msg_id);

        ESP_LOGI(TAG, "Subscribe to FP_JSON_REGISTER_REJECTED_TOPIC(PROVISIONING_TEMPLATE_NAME)");
        msg_id = esp_mqtt_client_subscribe(client, FP_JSON_REGISTER_REJECTED_TOPIC(PROVISIONING_TEMPLATE_NAME), 1);
        ESP_LOGI(TAG, "Subscribe to FP_JSON_REGISTER_REJECTED_TOPIC(PROVISIONING_TEMPLATE_NAME) successful, msg_id=%d", msg_id);
        //
        ESP_LOGI(TAG, "Publish to FP_JSON_CREATE_CERT_TOPIC");
        msg_id = esp_mqtt_client_publish(client, FP_JSON_CREATE_CERT_PUBLISH_TOPIC, "{}", 0, 0, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);

    }else if(aws_prov_state == THING_ACTIVE){
        ESP_LOGI(TAG, "Subscribe to MQTT_SUB_TOPIC");
        msg_id = esp_mqtt_client_subscribe(client, MQTT_SUB_TOPIC, 1);
        ESP_LOGI(TAG, "Subscribe to MQTT_SUB_TOPIC successful, msg_id=%d", msg_id);
    }
}

static void AWSHandlerData(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGI(TAG, "MQTT_EVENT_DATA");
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id=0;
    printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
    printf("DATA=%.*s\r\n", event->data_len, event->data);
    switch(aws_prov_state){
        case CERTIFICATE_AND_KEY:
            if(strcmp(event->topic,"$aws/certificates/create/json/accepted") == 0)
            {
                aws_prov_response = cJSON_Parse((char *)event->data);
                certificateId = cJSON_GetObjectItem(aws_prov_response,"certificateId")->valuestring;
                certificatePem = cJSON_GetObjectItem(aws_prov_response,"certificatePem")->valuestring;
                privateKey = cJSON_GetObjectItem(aws_prov_response,"privateKey")->valuestring;
                certificateOwnershipToken = cJSON_GetObjectItem(aws_prov_response,"certificateOwnershipToken")->valuestring;

                NVSDriverOpen(NVS_NAMESPACE_AWS);
                NVSDriverWriteString(NVS_KEY_CERT_ID, certificateId);
                NVSDriverWriteString(NVS_KEY_CERT_PEM, certificatePem);
                NVSDriverWriteString(NVS_KEY_PRIVATE_KEY, privateKey);
                NVSDriverClose();

                char cPayload[2048];
                sprintf(cPayload, "{\"certificateOwnershipToken\":\"%s\",\"parameters\":{\"SerialNumber\":\"%s\"}}", certificateOwnershipToken, HWData.snstr);
                ESP_LOGI(TAG,"Payload : %s",cPayload);
                ESP_LOGI(TAG,"strlen(Payload) %d", strlen(cPayload));

                ESP_LOGI(TAG, "Publish to FP_JSON_REGISTER_PUBLISH_TOPIC(PROVISIONING_TEMPLATE_NAME)");
                msg_id = esp_mqtt_client_publish(client, FP_JSON_REGISTER_PUBLISH_TOPIC(PROVISIONING_TEMPLATE_NAME), cPayload, 0, 1, 0);
                ESP_LOGI(TAG, "sent publish FP_JSON_REGISTER_PUBLISH_TOPIC(PROVISIONING_TEMPLATE_NAME) successful, msg_id=%d", msg_id);
            }
            else if(strcmp(event->topic,"$aws/provisioning-templates/Mini2_FP/provision/json/accepted") == 0){
                ESP_LOGI(TAG,"THING was created sucessfully");

                aws_prov_response = cJSON_Parse((char *)event->data);
                thingName = cJSON_GetObjectItem(aws_prov_response, "thingName")->valuestring;

                NVSDriverOpen(NVS_NAMESPACE_AWS);
                NVSDriverWriteString(NVS_KEY_THING_NAME, thingName);
                NVSDriverClose();
                
                /* Restart device to use updated certificate and private key */
                aws_prov_state = THING_ACTIVE;
            }else{
                abort();
            }
            break;
        default:
            JSON_Deserialize((char *)event->data);
            break;
    }
}
void AWSTask(void *arg)
{   
    NVSDriverOpen(NVS_NAMESPACE_AWS);
    NVSDriverReadString(NVS_KEY_CERT_ID, &certificateId);
    NVSDriverReadString(NVS_KEY_CERT_PEM, &certificatePem);
    NVSDriverReadString(NVS_KEY_PRIVATE_KEY, &privateKey);
    NVSDriverReadString(NVS_KEY_THING_NAME, &thingName);
    NVSDriverReadString(NVS_KEY_CLOUD_ENV, &cloudEnv);
    NVSDriverClose();

    sprintf(MQTT_SUB_TOPIC,"%s%s%s%s%s", "Mini2", "/sub/", "Mini2", "_", HWData.snstr);
    printf("MQTT_SUB_TOPIC=%s\n",MQTT_SUB_TOPIC);
    printf("MQTT_SUB_TOPIC length =%d\n",strlen(MQTT_SUB_TOPIC));

    sprintf(MQTT_PUB_TOPIC,"%s%s%s%s%s", "Mini2", "/pub/", "Mini2", "_", HWData.snstr);
    printf("MQTT_PUB_TOPIC=%s\n",MQTT_PUB_TOPIC);
    printf("MQTT_PUB_TOPIC length =%d\n",strlen(MQTT_PUB_TOPIC));

    sprintf(CLIENT_ID,"%s%s%s", "Mini2", "_", HWData.snstr);
    printf("CLIENT_ID=%s\n",CLIENT_ID);
    printf("CLIENT_ID length =%d\n",strlen(CLIENT_ID));

    esp_mqtt_client_handle_t client;

    if(certificatePem==NULL||privateKey==NULL||thingName==NULL){
        aws_prov_state=CERTIFICATE_AND_KEY;
        // if(cloudEnv==NULL){
        //     //prod
        //     ESP_LOGI(TAG, "Device not provisioned, initiating using claim certificate and key to production");
        //     const esp_mqtt_client_config_t mqtt_cfg = {
        //         .uri = "mqtts://a37xp5gb4zzal5-ats.iot.ap-southeast-1.amazonaws.com",
        //         .client_cert_pem = (const char *)client_prod_cert_pem_start,
        //         .client_key_pem = (const char *)client_prod_key_pem_start,
        //         .cert_pem = (const char *)root_cert_auth_pem_start,
        //         .client_id = CLIENT_ID,
        //         .buffer_size = 4096,
        //     };
        //     client = esp_mqtt_client_init(&mqtt_cfg);
        // }else {
            //stag
            ESP_LOGI(TAG, "Device not provisioned, initiating using claim certificate and key to staging");
            const esp_mqtt_client_config_t mqtt_cfg = {
                .uri = "mqtts://a14vk91kdzkp3p-ats.iot.ap-southeast-1.amazonaws.com",
                .client_cert_pem = (const char *)client_stag_cert_pem_start,
                .client_key_pem = (const char *)client_stag_key_pem_start,
                .cert_pem = (const char *)root_cert_auth_pem_start,
                .client_id = CLIENT_ID,
                .buffer_size = 4096,
            };
            client = esp_mqtt_client_init(&mqtt_cfg);
        //}
        
    }else{
        aws_prov_state=THING_ACTIVE;
        // if(cloudEnv==NULL){
        //     //prod
        //     ESP_LOGI(TAG, "Device already provisioned, retreiving certificate and key from NVS to production");
        //     const esp_mqtt_client_config_t mqtt_cfg = {
        //         .uri = "mqtts://a37xp5gb4zzal5-ats.iot.ap-southeast-1.amazonaws.com",
        //         .client_cert_pem = (const char *)certificatePem,
        //         .client_key_pem = (const char *)privateKey,
        //         .cert_pem = (const char *)root_cert_auth_pem_start,
        //         .client_id = CLIENT_ID,
        //         .buffer_size = 2048,
        //     };
        //     client = esp_mqtt_client_init(&mqtt_cfg);
        // }else{
            //stag
            ESP_LOGI(TAG, "Device already provisioned, retreiving certificate and key from NVS to staging");
            const esp_mqtt_client_config_t mqtt_cfg = {
                .uri = "mqtts://a14vk91kdzkp3p-ats.iot.ap-southeast-1.amazonaws.com",
                .client_cert_pem = (const char *)certificatePem,
                .client_key_pem = (const char *)privateKey,
                .cert_pem = (const char *)root_cert_auth_pem_start,
                .client_id = CLIENT_ID,
                .buffer_size = 2048,
            };
            client = esp_mqtt_client_init(&mqtt_cfg);
        //}
    }
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(client, MQTT_EVENT_CONNECTED, AWSHandlerConnected, NULL);
    esp_mqtt_client_register_event(client, MQTT_EVENT_DATA, AWSHandlerData, NULL);
    esp_mqtt_client_start(client);
    
    while(1){
        // SetHS(AMBI_PRIMARY_HUE, AMBI_PRIMARY_SAT);
        // SetOn();
        ESP_LOGI(TAG,"========================RAM left %d==============================", esp_get_free_heap_size());
        if(aws_prov_state == THING_ACTIVE){
            int msg_id=0;
            char *data_json;

            cJSON *pub;
            pub = cJSON_CreateObject();

            char *fw=malloc(5*sizeof(char));
            sprintf(fw,"%d%d%d", FW_VERSION_MAJOR, FW_VERSION_MINOR, FW_VERSION_BUILD);

            cJSON_AddStringToObject(pub, "mac_id", HWData.macstr);
            cJSON_AddStringToObject(pub, "real_device_id", HWData.uidstr);
            cJSON_AddStringToObject(pub, "SN", HWData.snstr);
            cJSON_AddStringToObject(pub, "firmware", fw);
            cJSON_AddNumberToObject(pub, "wifi", SensorData.signal_quality);

            cJSON *lux;
            lux= cJSON_CreateObject();

            cJSON_AddNumberToObject(lux, "overall_lux", SensorData.lux);
            cJSON_AddNumberToObject(lux, "full_spectrum", SensorData.fspec);
            cJSON_AddNumberToObject(lux, "infrared_spectrum", SensorData.inspec);

            cJSON_AddNumberToObject(pub,"TP", SensorData.temperature_sht);
            cJSON_AddNumberToObject(pub,"HM", SensorData.humidity_sht);
            cJSON_AddItemToObject(pub, "LU", lux);
            cJSON_AddNumberToObject(pub,"CO", SensorData.co2);

            memset(&data_json,0,sizeof(data_json));

            data_json = cJSON_Print(pub);
 
            ESP_LOGI(TAG,"data serialized %s", data_json);
            ESP_LOGI(TAG,"strlen(data) %d", strlen(data_json));

            ESP_LOGI(TAG, "Publish to MQTT_PUB_TOPIC");
            msg_id = esp_mqtt_client_publish(client, MQTT_PUB_TOPIC, data_json, 0, 1, 0);
            ESP_LOGI(TAG, "sent publish MQTT_PUB_TOPIC successful, msg_id=%d", msg_id);

            cJSON_Delete(pub);
            free(fw);
            free(data_json);
        }
        vTaskDelay(30000/portTICK_PERIOD_MS);
    }
}
/*-----------------------------------------------------------*/
void AWSRun(void){
    xTaskCreate(&AWSTask, "AWSTask", 3*1024, NULL, 5, &AWSTaskHandle);
}
/*-----------------------------------------------------------*/
void AWSStop(void){
    vTaskDelete(AWSTaskHandle);
}
/*-----------------------------------------------------------*/