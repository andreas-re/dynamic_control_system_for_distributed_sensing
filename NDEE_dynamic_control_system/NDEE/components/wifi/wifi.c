/* WiFi station Example
   This example code is in the Public Domain (or CC0 licensed, at your option.)
   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
// link: https://github.com/espressif/esp-idf/blob/master/examples/wifi/getting_started/station/main/station_example_main.c

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "lwip/sockets.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/apps/sntp.h"

#include "wifi.h"

#include "controlsystem.h"
#include "configuration.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "esp_pm.h"

/* The examples use WiFi configuration that you can set via 'make menuconfig'.
   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
#define MAXIMUM_RETRY  5

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about one event 
 * - are we connected to the AP with an IP? */
const int WIFI_CONNECTED_BIT = BIT0;

const int SOCKET_CONNECTED_BIT = BIT0;

static const char *TAG = "wifi station";

static int s_retry_num = 0;

int socket_fd;

static void initialize_sntp(void);
static void obtain_time(void);

int answer (int client, answer_decoded_t *answer)
{
    char *msg;
    int size;

    encode_msgpack_answer(answer, &msg, &size);
    return send_binary(msg, size);
}

int send_binary(char *msg, int size) {
    return write(socket_fd, msg, size);
}

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch(event->event_id) {
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        ESP_LOGI(TAG, "got ip:%s",
                 ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        {
            if (s_retry_num < MAXIMUM_RETRY) {
                esp_wifi_connect();
                xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
                s_retry_num++;
                ESP_LOGI(TAG,"retry to connect to the AP");
            }
            ESP_LOGI(TAG,"connect to the AP fail\n");
            break;
        }
    default:
        break;
    }
    return ESP_OK;
}

void wifi_init_sta()
{
    s_wifi_event_group = xEventGroupCreate();

    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL) );

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = CONFIG_WIFI_SSID,
            .password = CONFIG_WIFI_PASS
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");
    ESP_LOGI(TAG, "connect to ap SSID:%s password:%s",
             CONFIG_WIFI_SSID, CONFIG_WIFI_PASS);

    ESP_LOGI(TAG, "esp_wifi_set_ps().");
    esp_wifi_set_ps(WIFI_PS_NONE);
}

void receive_requests(void *pvParam) {
    char recv_data[256];
    char *msg_buffer = NULL;
    int read_count;
    int msg_length = 0;
    
    while(true) {
        // read from client
        read_count = recv(socket_fd, recv_data, sizeof(recv_data), 0);

        // error checking
        if(read_count == 0)
        {
            ESP_LOGE(TAG, "server disconnected");
            vTaskDelete(NULL);
        }
        else if(read_count < 0)
        {
            ESP_LOGE(TAG, "error reading from server socket");
        }

        // handle received data
        // in case msg_buffer is NULL, realloc behaves like malloc
        msg_buffer = realloc(msg_buffer, msg_length+read_count);
        memcpy(msg_buffer+msg_length, recv_data, read_count);
        msg_length += read_count;
 
        msg_length = evaluate_request(msg_buffer, msg_length);
        if(msg_length == 0) {
            free(msg_buffer);
            msg_buffer = NULL;
        }
    }
}

int connect_to_server() {
    int port = 3001;
    struct sockaddr_in server_settings;

    ESP_LOGI(TAG, "waiting for WiFi to be connected");
    // wait for being connected to the WiFi
    xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT, false, true, portMAX_DELAY);
    // TODO: check and handle WiFi disconnection
    ESP_LOGI(TAG, "WiFi connected, trying to connect to server");

    socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    if(socket_fd < 0) {
        ESP_LOGE(TAG, "failed opening socket");
        return (-1);
    }
    server_settings.sin_family = AF_INET;
    server_settings.sin_port = htons(port);
    inet_pton(AF_INET, CONFIG_SERVER_ADDRESS, &server_settings.sin_addr);
    if(connect(socket_fd, (struct sockaddr*)&server_settings, sizeof(server_settings)) < 0) {
        ESP_LOGE(TAG, "failed connecting to socket");
        return (-1);
    }
    ESP_LOGI(TAG, "connected to socket");
    return 0;
}

int open_network()
{
    esp_log_level_set("wifi", ESP_LOG_NONE); // disable wifi driver logging

    wifi_init_sta();
    //obtain_time();
    if(connect_to_server() < 0) {
        // TODO: error handling
        return (-1);
    }
    xTaskCreate(&receive_requests,"requests",4096,NULL,5,NULL);

    return 0;
}

// copied from esp-idf/examples/protocols/sntp/main/sntp_example_main.c
// https://github.com/espressif/esp-idf/blob/master/examples/protocols/sntp/main/sntp_example_main.c
static void obtain_time(void)
{
    xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT, false, true, portMAX_DELAY);
    initialize_sntp();

    // wait for time to be set
    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    const int retry_count = 10;
    while(timeinfo.tm_year < (2016 - 1900) && ++retry < retry_count) {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        time(&now);
        localtime_r(&now, &timeinfo);
    }
}

static void initialize_sntp(void)
{
    ESP_LOGI(TAG, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    //sntp_setservername(0, "pool.ntp.org");
    sntp_setservername(0, CONFIG_SERVER_ADDRESS);
    sntp_init();
}
