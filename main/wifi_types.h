#pragma once
#include "esp_wifi.h" 

/**
 * @struct wifi_scan_result 
 * 
 * @brief Structure holding the result of a wifi scan
 * 
 */
typedef struct wifi_scan_result{
    bool status;
    uint8_t ap_count;
    wifi_ap_record_t* access_points;
} wifi_scan_result;


typedef uint8_t MacAddress[6];

