

#include "wifi.h"

#define CONNECTED_BIT 1
#define AUTH_FAIL 2

esp_netif_t *network_interface = NULL;
esp_netif_t *network_interface_ap = NULL;

const char *ssid = "WIFI_SSID";
const char *password = "PASSWORD";
const char *username = "";

char network_event[64];
const char *ip_messages[] = {
    "STA_GOT_IP", "STA_LOST_IP", "AP_STA_IPASSIGNED", "GOT_IP6", "ETH_GOT_IP", "PPP_GOT_IP", "PPP_LOST_IP"};

const char *mqtt_messages[] = {
    "MQTT_ERROR", "MQTT_CONNECTED", "MQTT_DISCONNECTED", "MQTT_SUBSCRIBED",
    "MQTT_UNSUBSCRIBED", "MQTT_PUBLISHED", "MQTT_DATA", "MQTT_BEFORE_CONNECT"};

EventGroupHandle_t network_event_group;

wifi_mode_type wifi_mode = STATION;

extern ALARM_INIT_STAGE init_stage;
extern mqtt_callback_type mqtt_callback;

nvs_handle_t storage_open(nvs_open_mode_t mode)
{
    // Handle the NVS storage read operations
    esp_err_t err;
    nvs_handle_t my_handle;
    err = nvs_open("storage", mode, &my_handle);
    if (err != 0)
    {
        nvs_flash_init();
        err = nvs_open("storage", mode, &my_handle);
        printf("err1: %d\n", err);
    }
    return my_handle;
}

void storage_read_string(char *name, char *def, char *dest, int len)
{
    // Read a given string from the NVS
    nvs_handle_t handle = storage_open(NVS_READONLY);
    strncpy(dest, def, len);
    size_t length = len;
    nvs_get_str(handle, name, dest, &length);
    nvs_close(handle);
    printf("Read %s = %s\n", name, dest);
}

int wifi_connected()
{
    // Return the WIFI connection status
    if (network_event_group)
    {
        printf("connected: %ld, fail: %ld\n", xEventGroupGetBits(network_event_group) & CONNECTED_BIT, xEventGroupGetBits(network_event_group) & AUTH_FAIL);
        return xEventGroupGetBits(network_event_group) & CONNECTED_BIT;
    }
    return 0;
}

void set_event_message(const char *s)
{
    // Set the message recieved to the network_event
    snprintf(network_event, sizeof(network_event), "%s\n", s);
}

void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    // Create a new event group if one is not present
    if (network_event_group == NULL)
        network_event_group = xEventGroupCreate();

    if (event_base == WIFI_EVENT)
    {
        wifi_event_sta_disconnected_t *disconnect_data;

        switch (event_id)
        {
            // Whent the WIFI event has started
        case WIFI_EVENT_STA_START:
            // Clear the bits that are necassary
            xEventGroupClearBits(network_event_group, AUTH_FAIL | CONNECTED_BIT);
            esp_wifi_connect();

            break;

        case WIFI_EVENT_STA_DISCONNECTED:
            // If disconnected from WIFI show the reason for it
            disconnect_data = event_data;

            printf("WiFi Disconnect: %d\n", disconnect_data->reason);
            // Clear the bit that shows it is connected
            xEventGroupClearBits(network_event_group, CONNECTED_BIT);

            // If the authentication step failed set the relevant bit
            if (disconnect_data->reason == WIFI_REASON_AUTH_FAIL ||
                disconnect_data->reason == WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT)
                xEventGroupSetBits(network_event_group, AUTH_FAIL);
            break;

        case WIFI_EVENT_SCAN_DONE:
            esp_wifi_scan_start(NULL, false);
            break;

        default:
            break;
        }
    }

    if (event_base == IP_EVENT)
    {
        // When the connection is successful
        set_event_message(ip_messages[event_id % sizeof(ip_messages)]);

        if ((event_id == IP_EVENT_STA_GOT_IP) ||
            (event_id == IP_EVENT_AP_STAIPASSIGNED) || (event_id == IP_EVENT_ETH_GOT_IP))
        {
            xEventGroupSetBits(network_event_group, CONNECTED_BIT);

            // Go to the next stage of the alarm when connected
            init_stage = CONNECTING_TO_SNTP;
        }
    }

    // For the MQTT messages
    if (!strcmp(event_base, "MQTT_EVENTS"))
    {
        // Check what type of MQTT message it is and add to the event message
        set_event_message(mqtt_messages[event_id % ARRAY_LENGTH(mqtt_messages)]);

        // If the MQTT callback is present call it with the id and the data
        if (mqtt_callback)
            mqtt_callback(event_id, event_data);
    }
}

void init_wifi2(wifi_mode_type mode)
{

    // If WIFI is already connected set the bit and return
    if (wifi_mode == mode && network_interface != NULL &&
        (xEventGroupGetBits(network_event_group) & CONNECTED_BIT))
        return;

    // If there is no event group create a new one
    if (network_event_group == NULL)
        network_event_group = xEventGroupCreate();

    // Clear the required fields before connection
    xEventGroupClearBits(network_event_group, AUTH_FAIL | CONNECTED_BIT);
    wifi_mode = mode;

    // If there is already a interface present stop the connection
    if (network_interface != NULL)
    {
        esp_wifi_stop();
    }

    // When there is no connection
    if (network_interface == NULL)
    {
        // Initialize the TCP/IP stack
        esp_netif_init();

        // Create a default event loop
        ESP_ERROR_CHECK(esp_event_loop_create_default());

        // Create Access Point(not required) and a Station
        network_interface_ap = esp_netif_create_default_wifi_ap();
        network_interface = esp_netif_create_default_wifi_sta();

        // Initialize with the WIFI init config
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));

        // Set the event handlers for both WIFI and and IP_EVENT (both same)
        ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
        ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
        ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    }

    // Set the protocol for the WIFI
    uint8_t protocol = (WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N); //|WIFI_PROTOCOL_LR);

    // When the mode is AP
    if (mode == 2)
    {
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
        esp_wifi_set_protocol(ESP_IF_WIFI_AP, protocol);

#define SSID "ESP32"

        wifi_config_t wifi_config = {
            .ap = {
                .ssid = SSID,
                .ssid_len = strlen(SSID),
                .channel = 3,
                .password = "",
                .max_connection = 8,
                .authmode = WIFI_AUTH_OPEN},
        };
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    }
    else
    {
        // When the initialize mode is Station
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        esp_wifi_set_protocol(ESP_IF_WIFI_STA, protocol);

        // char ssid[32];
        // storage_read_string("ssid", "WIFI SSID", ssid, sizeof(ssid));
        // char password[64];
        // storage_read_string("password", "", password, sizeof(password));
        // char username[64];
        // storage_read_string("username", "", username, sizeof(username));

        // Create a new WIFI config and copy the SSID, password and set the config
        wifi_config_t wifi_config = {0};
        strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
        strncpy((char *)wifi_config.sta.password, password, sizeof(wifi_config.sta.password));
        ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
        if (strlen(username) != 0)
        {
            // If the username is present set the username and the password
            // And connect EAP authentication
            ESP_ERROR_CHECK(esp_eap_client_set_username((uint8_t *)username, strlen(username)));
            ESP_ERROR_CHECK(esp_eap_client_set_password((uint8_t *)password, strlen(password)));
            ESP_ERROR_CHECK(esp_wifi_sta_enterprise_enable());
        }
    }

    // Start the services
    ESP_ERROR_CHECK(esp_wifi_start());
}