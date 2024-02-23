#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>



#define INCLUDE_vTaskDelay 1
/**
 * @brief Sleep for a given time
 * 
 * @param t Time to sleep for, in *milliseconds*
 */

void ut_sleep(int t){
    vTaskDelay( t / portTICK_PERIOD_MS);
}

