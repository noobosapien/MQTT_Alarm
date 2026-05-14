#ifndef WIFI_H
#define WIFI_H

#include <string.h>
#include <stdio.h>

#include <esp_netif_types.h>
#include <esp_netif.h>
#include <esp_wifi_default.h>
#include <esp_wifi.h>
#include <esp_wpa2.h>
#include <nvs.h>
#include <nvs_flash.h>

#include <freertos/event_groups.h>

#include <esp_netif_types.h>
#include <esp_netif.h>
#include <esp_wifi_default.h>

#include "alarm.h"
#include "mqtt_service.h"

#define ARRAY_LENGTH(array) (sizeof((array)) / sizeof((array)[0]))

typedef enum
{
    SCAN,
    STATION,
    ACCESS_POINT,
} wifi_mode_type;

int wifi_connected();
void set_event_message(const char *s);
void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
void init_wifi2(wifi_mode_type mode);

#endif