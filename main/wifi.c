#include "wifi.h"

static EventGroupHandle_t wifi_event_group;

static int8_t s_retry_num = 0;

static  esp_event_handler_instance_t* wifi_handler_event_instance;
static  esp_event_handler_instance_t* got_ip_event_instance;
static const char* TAG = "WiFi";


int8_t wifi_connect(char* ssid, char* password) 
{
    ESP_LOGI(TAG, "Starting setup...");
    _init_wifi_driver();
    _register_event_handlers();
    _configure_wifi_station(ssid,password);
    int8_t rval = _start_wifi();
    _unregister_event_handlers();
    ESP_LOGI(TAG, "Setup Complete");
    return rval;
}

void _init_wifi_driver()
{
    ESP_LOGI(TAG, "Initializing driver...");
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t wifi_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_LOGI(TAG, "Station created! ");

    ESP_ERROR_CHECK(esp_wifi_init(&wifi_config));
    ESP_LOGI(TAG, "Driver Initialized.");
}

void _configure_wifi_station(char* ssid, char* password)
{
    ESP_LOGI(TAG, "Configuring the station...");
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = ssid,
            .password = password,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_LOGI(TAG, "Station config complete");
}

int8_t _start_wifi()
{
    int8_t status = WIFI_FAILURE;
    ESP_LOGI(TAG, "Wifi starting...");
    ESP_ERROR_CHECK(esp_wifi_start());
    EventBits_t bits = xEventGroupWaitBits(
        wifi_event_group,
        WIFI_SUCCESS | WIFI_FAILURE,
        pdFALSE,
        pdFALSE,
        portMAX_DELAY
    );

    if (bits & WIFI_SUCCESS) {
        ESP_LOGI(TAG, "Connected to AP!");
        status = WIFI_SUCCESS;
    } else if (bits & WIFI_FAILURE) {
        ESP_LOGI(TAG, "Failed to connect to AP.");
        status=WIFI_FAILURE;
    } else {
        ESP_LOGI(TAG, "UNEXPECTED EVENT");
        status = WIFI_FAILURE;
    }
    return status;
}

bool wifi_scan()
{
    _init_wifi_driver();
    wifi_country_t country_config = {
        .cc = "PL",
        .schan = 1,
        .nchan = 13,
        .policy = WIFI_COUNTRY_POLICY_AUTO
    };

    wifi_scan_config_t scan_config;
    scan_config.ssid = NULL; // NULL => Scan all AP's. If set, look for specific SSID
    scan_config.bssid = NULL; // NULL => Scan all AP's. If set, look for specific BSSID
    scan_config.channel = 0; // 0=> Scan all channels 1-14
    scan_config.show_hidden = true; 
    scan_config.scan_type = WIFI_SCAN_TYPE_ACTIVE;
    
    esp_wifi_set_country(&country_config);
    esp_wifi_set_mode(WIFI_MODE_STA);

    wifi_handler_event_instance = (esp_event_handler_instance_t*)malloc(sizeof(esp_event_handler_instance_t));
    wifi_event_group = xEventGroupCreate();
    esp_event_handler_instance_register(
        WIFI_EVENT,
        WIFI_EVENT_SCAN_DONE,
        &_wifi_scan_event_handler,
        NULL,
        wifi_handler_event_instance
    );

    ESP_LOGI(TAG, "Staring WI-FI Scan..");

    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_config,true));

    EventBits_t bits = xEventGroupWaitBits(
        wifi_event_group,
        SCAN_SUCCESS | SCAN_FAILURE,
        pdFALSE,
        pdFALSE,
        portMAX_DELAY
    );

    bool rval = SCAN_FAILURE;
    if (bits & SCAN_SUCCESS) {
        rval = SCAN_SUCCESS;
        ESP_LOGI(TAG, "Scan completed successfully!");
    } else if (bits & SCAN_FAILURE) {
        rval = SCAN_FAILURE;
        ESP_LOGI(TAG, "Scan failed misarebly :(");
    } else {
        rval = SCAN_FAILURE;
        ESP_LOGI(TAG, "Unexpected behaviour");
    }

    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT,WIFI_EVENT_SCAN_DONE, wifi_handler_event_instance));
    vEventGroupDelete(wifi_event_group);
    return rval;
}

void _register_event_handlers()
{
    ESP_LOGI(TAG, "Initializing event handlers...");
    wifi_event_group = xEventGroupCreate();
    wifi_handler_event_instance = (esp_event_handler_instance_t*)malloc(sizeof(esp_event_handler_instance_t));
    esp_event_handler_instance_register(WIFI_EVENT,
    ESP_EVENT_ANY_ID,
    &_wifi_event_handler,
    NULL,
    wifi_handler_event_instance
    );

    got_ip_event_instance = (esp_event_handler_instance_t*)malloc(sizeof(esp_event_handler_instance_t));
    esp_event_handler_instance_register(
        IP_EVENT,
        IP_EVENT_STA_GOT_IP,
        &_wifi_got_ip,
        NULL,
        got_ip_event_instance
    );

     ESP_LOGI(TAG, "Event handlers initialized.");
}

void _unregister_event_handlers()
{
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT,IP_EVENT_STA_GOT_IP,*got_ip_event_instance));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT,ESP_EVENT_ANY_ID, *wifi_handler_event_instance));
    vEventGroupDelete(wifi_event_group);
}

void _wifi_event_handler(void* arg, esp_event_base_t event_base,
                        int32_t event_id, void* event_data)
{
    char* TAG = "[Wi-Fi][Event]";
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "Connecting to the AP...");
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < MAX_RETRIES) {
            ESP_LOGI(TAG, "Reconnectiong to the AP...");
            esp_wifi_connect();
            s_retry_num++;
        } else {
            xEventGroupSetBits(wifi_event_group,WIFI_FAILURE);
        }
    }
}

void _wifi_got_ip(void* arg, esp_event_base_t event_base,
                        int32_t event_id, void* event_data)
{
    char* TAG = "[Wi-Fi][IP]";
    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "STA IP: " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(wifi_event_group, WIFI_SUCCESS);
    }
    ESP_LOGI(TAG, "Got IP");
}


void _wifi_scan_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data){
   if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_SCAN_DONE) {
    xEventGroupSetBits(wifi_event_group,SCAN_SUCCESS);
    
    uint16_t ap_num = 0;
    wifi_ap_record_t* ap_records = 0;

    esp_wifi_scan_get_ap_num(&ap_num);
    if (ap_num == 0) {
        ESP_LOGI(TAG, "No AP's Found");
    } else {
        ap_records = (wifi_ap_record_t*)malloc(sizeof(wifi_ap_record_t) * ap_num);
        esp_wifi_scan_get_ap_records(&ap_num, ap_records);
    }
    ESP_LOGI(TAG, "----- AP's Found: -----");
    for (size_t i = 0; i < ap_num; i++) {
        char* country = malloc(sizeof(char)*3);
        country[0] = ap_records[i].country.cc[0];
        country[1] = ap_records[i].country.cc[1];
        country[2] = 0;
        ESP_LOGI(
            TAG, 
            "[%s] SSID: %s Channel: %d MAC: %xh-%xh-%xh-%xh-%xh-%xh",
            country,
            ap_records[i].ssid,
            ap_records[i].primary,
            ap_records[i].bssid[0],
            ap_records[i].bssid[1],
            ap_records[i].bssid[2],
            ap_records[i].bssid[3],
            ap_records[i].bssid[4],
            ap_records[i].bssid[5]

        );
    }
    

   } else {
    xEventGroupSetBits(wifi_event_group,SCAN_FAILURE);
    ESP_LOGI(TAG, "Scan failed");

   }

}
