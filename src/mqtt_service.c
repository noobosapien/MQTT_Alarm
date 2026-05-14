#include "mqtt_service.h"

extern EventGroupHandle_t network_event_group;
extern ALARM_INIT_STAGE init_stage;
extern esp_netif_t *network_interface;
extern char network_event[64];
extern ALARM_CONFIG alarm_config;

esp_mqtt_client_handle_t mqtt_client = NULL;
int bg_col = 0;

mqtt_callback_type mqtt_callback = 0;

void mqtt_disconnect()
{
    // Use at alarm quit to disconnect from the broker
    if (mqtt_client != NULL)
    {
        esp_mqtt_client_stop(mqtt_client);
        esp_mqtt_client_destroy(mqtt_client);
        mqtt_client = NULL;
        mqtt_callback = NULL;
    }
}

void set_mqtt_callback(mqtt_callback_type callback)
{
    // Set the callback
    mqtt_callback = callback;
}

void mqtt_connect(mqtt_callback_type callback)
{
    char client_name[32];

    if (mqtt_client != NULL)
    {
        // Disconnect if a previous client is present
        mqtt_disconnect();
    }

    srand(esp_timer_get_time());

    // Create a new client name
    sprintf(client_name, "esp32_%d", rand() % 1000);

    // MQTT config
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = "mqtt://mqtt.webhop.org",
        .credentials.client_id = client_name};

    // If connected to wifi
    if (xEventGroupGetBits(network_event_group) & 1)
    {
        // Create a new client
        mqtt_client = esp_mqtt_client_init(&mqtt_cfg);

        // Register the wifi event handler to get incoming messages
        esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, event_handler, NULL);
        // Set the callback to process the messages
        set_mqtt_callback(callback);
        // Start the client
        esp_mqtt_client_start(mqtt_client);
    }
}

static void service_mqtt_callback(int event_id, void *event_data)
{
    // Change the type of the data recieved to the handle type
    esp_mqtt_event_handle_t event = event_data;

    if (event_id == MQTT_EVENT_CONNECTED)
    {
        // When connected show the information and publish it back to the broker
        // And subscribe to the topic alarm

        esp_mqtt_client_handle_t client = event->client;
        esp_netif_ip_info_t ip_info;

        esp_netif_get_ip_info(network_interface, &ip_info);

        char buf[64];

        snprintf(buf, sizeof(buf), "Connected: " IPSTR, IP2STR(&ip_info.ip));

        esp_mqtt_client_publish(client, "esp32/alarm", buf, 0, 1, 0);
        esp_mqtt_client_subscribe(client, "esp32/alarm", 0);

        // Once connected as this is the last service the alarm can go to process mode
        init_stage = CONNECTION_DONE;
    }
    else if (event_id == MQTT_EVENT_DATA)
    {
        // When data is recieved from the broker format it and add to the alarm config
        event->data[event->data_len] = 0;
        snprintf(network_event, sizeof(network_event), "MQTT_DATA\n%s\n", event->data);

        // In the format YYYY-MM-DD HH:MM (2024-09-30 13:00)
        int year, month, day, hour, minute;
        if (sscanf(event->data, "%d-%d-%d %d:%d", &year, &month, &day, &hour, &minute))
        {
            alarm_config.year = year;
            alarm_config.month = month;
            alarm_config.day = day;
            alarm_config.hour = hour;
            alarm_config.minute = minute;
            alarm_config.valid = true;
        }
    }
}

void service_mqtt()
{
    // When connected to WIFI connect to the MQTT broker
    if (xEventGroupGetBits(network_event_group) & 1)
    {
        mqtt_connect(service_mqtt_callback);
    }
}