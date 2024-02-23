#pragma once
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "driver/gpio.h"


#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"


#define WIFI_SUCCESS 1
#define WIFI_FAILURE 0

#define SCAN_SUCCESS 1
#define SCAN_FAILURE 0

#define MAX_RETRIES  10

/**
 * @brief Connect to a Wi-Fi network  
 * 
 *  This function will initialize the wifi driver in STA mode
 *  and try to connect to the AP with the given SSID. If the connection
 *  is not successfull there will be 10 retries, after the 10'th one fails
 *  the function will return
 * 
 * @attention Make sure the nvs is initalized before any wifi-related operations
 * 
 * @param ssid the SSID of the network to connect to 
 * @param password the password to the network
 * @return int8_t, 1 if connected successfully, 0 otherwise
 */
int8_t wifi_connect(char* ssid, char* password);


/**
 * @brief Scan and list nearby AP's
 * 
 * This fucntion will initialize the wifi driver in STA mode and
 * perfrm a full range network scan for nearby AP's on all channels.
 * After the scan is complete the found AP's will be printed to the Serial port
 * ant the function will return.
 * 
 * 
 * @attention  1.The scan takes a long time and this function blocks untill its completed.
 * @attention  2.Make sure the nvs is initalized before any wifi-related operations

 * @return int8_t 1 if scan is successfull, 0 if the scan fails 
 */
bool wifi_scan();


void _init_wifi_driver();
void _register_event_handlers();
void _unregister_event_handlers();
void _configure_wifi_station(char*, char*);
int8_t _start_wifi();
void _connect_wifi();
void _wifi_got_ip(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
void _wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
void _wifi_scan_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

