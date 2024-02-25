

#include "esp_log.h"
#include "driver/gpio.h"

#include "wifi_types.h"
#include "sleep.h"
#include "wifi.h"
#include "nvs.h"

static const uint8_t s_LED_PIN = 2;
static const char* TAG = "[Main]";

static bool s_led = false;

void setup(){
    gpio_set_direction(s_LED_PIN, GPIO_MODE_OUTPUT);
    init_nvs();
}

void toggle_led() {
    s_led = !s_led;
    gpio_set_level(s_LED_PIN, s_led);
}

void app_main(void)
{
    setup();
    //gpio_set_level(s_LED_PIN, wifi_connect("Multiplay_AD2A","ZTEVQJNN5T03400"));
    // wifi_scan_result res = wifi_scan(); 
    // wifi_print_scan_result(res);
    
    wifi_deauth_attack(1);


    for(;;) {
        ut_sleep(1000);
    }
}

