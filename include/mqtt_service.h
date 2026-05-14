#ifndef MQTT_SERVICE_H
#define MQTT_SERVICE_H

#include <mqtt_client.h>
#include <esp_timer.h>

#include "alarm.h"
#include "wifi.h"

typedef void (*mqtt_callback_type)(int event_id, void *event_data);

void service_mqtt();
void mqtt_disconnect();

#endif