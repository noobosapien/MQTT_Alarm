
#include <driver/gpio.h>

#include <esp_system.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <inttypes.h>
#include <esp_timer.h>

#include <math.h>
#include <esp_log.h>
#include <esp_sntp.h>
#include <nvs_flash.h>

#include "alarm.h"
#include "wifi.h"
#include "sntp_time.h"
#include "mqtt_service.h"

extern uint16_t *frame_buffer;
extern int display_width;
extern int display_height;
extern ALARM_CONFIG alarm_config;

ALARM_INIT_STAGE init_stage = NOT_CONNECTING;
volatile unsigned *GPIO_OUTPUT = (unsigned *)0x3ff44004;

float blink_timer = 0.f;
bool swap_colors = false;
uint16_t blink_col = 0;

bool initialize_alarm(ALARM_STATE *state)
{
    // Initialize the ESP
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    setenv("TZ", "NZST-12:00:00NZDT-13:00:00,M9.5.0,M4.1.0", 0);
    tzset();
    graphics_init();
    set_orientation(0); // Set the orientation to landscape mode

    return true;
}

void start_alarm()
{

    if (init_stage == IDLE)
        return;

    // Connect to wifi
    if (init_stage == NOT_CONNECTING)
    {
        init_wifi2(STATION);
        init_stage = IDLE;
    }

    // Get time
    if (init_stage == CONNECTING_TO_SNTP)
    {
        service_sntp();
    }

    // Subscribe to MQTT
    if (init_stage == CONNECTING_TO_MQTT)
    {
        service_mqtt();
        init_stage = IDLE;
    }

}

void process_state(ALARM_STATE *state, ALARM_CONFIG *config)
{
    // Populate the alarm config fields for current time
    time_t time_now;
    time(&time_now);
    struct tm *tm_info = localtime(&time_now);

    if (tm_info->tm_year < (2023 - 1900))
        return; // No time from sntp yet

    config->curr_year = tm_info->tm_year + 1900; // Year starts from 1900
    config->curr_month = tm_info->tm_mon + 1;    // Month starts from 0
    config->curr_day = tm_info->tm_mday;
    config->curr_hour = tm_info->tm_hour;
    config->curr_minute = tm_info->tm_min;
    config->curr_seconds = tm_info->tm_sec;

    // Check current time with the next alarm
    // If the times match change the state to ALARM_ON
    if (config->valid)
    {
        if (
            config->curr_year >= config->year &&
            config->curr_month >= config->month &&
            config->curr_day >= config->day &&
            config->curr_hour >= config->hour &&
            config->curr_minute >= config->minute)
        {
            *state = ALARM_ON;
        }
    }
}

void turn_alarm_on(ALARM_STATE *state, ALARM_CONFIG *config)
{

    // Set the GPIO out for address 0x3ff44004 as 1
    *GPIO_OUTPUT |= 1 << 0;

    // If any button is pressed
    // Change state to PROCESS
    if (!(GPIO.in & 1) || !(GPIO.in1.data & 8))
    {
        *GPIO_OUTPUT &= ~(1 << 0); // Clear the bit when leaving
        config->valid = false;     // Invalidate the previous alarm time
        *state = PROCESS;
    }
}

void process_alarm(ALARM_STATE *state, ALARM_CONFIG *config)
{
    // Calculate the delta for the blinking function
    config->time_before_loop = esp_timer_get_time();
    config->delta = (float)(config->time_before_loop - config->last_time) / 1000000;

    if (config->delta > 0.05)
        config->delta = 0.05;

    switch (*state)
    {
    case INIT:
        // Connect all services
        start_alarm();

        // When all the connections are done change to process
        if (init_stage == CONNECTION_DONE)
            *state = PROCESS;
        break;

    case PROCESS:
        process_state(state, config);
        break;

    case ALARM_ON:
        turn_alarm_on(state, config);
        break;

    default:
        break;
    }

    config->last_time = esp_timer_get_time();
}

void render_alarm(ALARM_STATE *state, ALARM_CONFIG *config)
{
    cls(0);
    paint_background(rgbToColour(15, 23, 42));

    switch (*state)
    {
    case INIT:
        // Initially show that the device is connecting to the services
        setFont(FONT_UBUNTU16);
        setFontColour(71, 85, 105);
        print_xy("Connecting to AP: WiFi", CENTER, LASTY + 40);
        print_xy("Connecting to the SNTP server", CENTER, LASTY + 20);
        print_xy("Connecting to the MQTT broker", CENTER, LASTY + 20);
        break;

    case PROCESS:
        char temp[64] = "";

        setFont(FONT_SMALL);
        setFontColour(71, 85, 105);
        print_xy("Current time:", CENTER, LASTY + 20);

        setFont(FONT_UBUNTU16);
        setFontColour(203, 213, 225);

        // Show the current time and whether it is AM/PM
        if (config->curr_hour > 12)
        {
            snprintf(temp, sizeof(temp), "%02d:%02d:%02d PM", config->curr_hour - 12, config->curr_minute, config->curr_seconds);
        }
        else if (config->curr_hour == 12)
        {
            snprintf(temp, sizeof(temp), "%02d:%02d:%02d PM", config->curr_hour, config->curr_minute, config->curr_seconds);
        }
        else
        {
            snprintf(temp, sizeof(temp), "%02d:%02d:%02d AM", config->curr_hour, config->curr_minute, config->curr_seconds);
        }

        print_xy(temp, CENTER, LASTY + 20);

        // Show the current date
        setFont(FONT_SMALL);
        snprintf(temp, sizeof(temp), "%02d/%02d/%d", config->curr_day, config->curr_month, config->curr_year);
        print_xy(temp, CENTER, LASTY + 20);

        setFont(FONT_SMALL);
        setFontColour(71, 85, 105);
        print_xy("Next alarm at:", CENTER, LASTY + 20);

        setFont(FONT_UBUNTU16);
        setFontColour(100, 116, 139);

        // When there is a valid alarm set from the mqtt broker
        if (config->valid)
        {
            // Show the time with AM/PM
            if (config->hour > 12)
            {
                snprintf(temp, sizeof(temp), "%02d:%02d PM", config->hour - 12, config->minute);
            }
            else if (config->hour == 12)
            {
                snprintf(temp, sizeof(temp), "%02d:%02d PM", config->hour, config->minute);
            }
            else
            {
                snprintf(temp, sizeof(temp), "%02d:%02d AM", config->hour, config->minute);
            }

            print_xy(temp, CENTER, LASTY + 20);
            setFont(FONT_SMALL);
            snprintf(temp, sizeof(temp), "%02d/%02d/%d", config->day, config->month, config->year);
            print_xy(temp, CENTER, LASTY + 20);
        }
        else
        {
            // If the alarm has already gone off show that the alarm is not set anymore
            snprintf(temp, sizeof(temp), "Alarm not set.");
            print_xy(temp, CENTER, LASTY + 20);
        }

        break;

    case ALARM_ON:
        // Show the blinking screen
        blink(config);
        break;

    default:
        break;
    }

    flip_frame();
}

void quit_alarm()
{
    // Disconnect from the MQTT broker
    mqtt_disconnect();
}

void paint_background(uint16_t col)
{
    for (int j = 0; j < display_height; j++)
    {
        uint16_t *p = frame_buffer + j * display_width;
        for (int i = 0; i < display_width; i++)
        {
            *p++ = col;
        }
    }
}

void blink(ALARM_CONFIG *config)
{

    blink_timer += config->delta;

    // For every 300ms swap the color of the background when the alarm goes off

    if (blink_timer >= 0.3)
    {
        blink_timer = 0.f;
        if (swap_colors)
        {
            blink_col = rgbToColour(220, 38, 38);
        }
        else
        {
            blink_col = rgbToColour(22, 163, 74);
        }

        swap_colors = !swap_colors;
    }

    paint_background(blink_col);
}
