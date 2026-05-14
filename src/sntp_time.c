
#include "sntp_time.h"

extern EventGroupHandle_t network_event_group;
extern ALARM_INIT_STAGE init_stage;

void service_sntp()
{
    // When connected to WIFI poll the sntp pool for the correct time
    if (xEventGroupGetBits(network_event_group) & 1)
    {
        esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
        esp_sntp_setservername(0, "pool.ntp.org");
        esp_sntp_init();

        init_stage = CONNECTING_TO_MQTT;
    }
    else
    {
        printf("Not connected to WIFI yet\n");
    }
}