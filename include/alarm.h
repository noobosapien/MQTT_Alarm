#ifndef ALARM_H
#define ALARM_H

#include <stdbool.h>
#include <graphics.h>
#include <soc/gpio_struct.h>
#include <fonts.h>

typedef enum ALARM_STATE
{
    INIT,
    PROCESS,
    ALARM_ON
} ALARM_STATE;

typedef enum ALARM_INIT_STAGE
{
    IDLE,
    NOT_CONNECTING,
    CONNECTING_TO_WIFI,
    CONNECTING_TO_SNTP,
    CONNECTING_TO_MQTT,
    CONNECTION_DONE
} ALARM_INIT_STAGE;

typedef struct ALARM_CONFIG
{
    bool valid;
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int seconds;

    int curr_year;
    int curr_month;
    int curr_day;
    int curr_hour;
    int curr_minute;
    int curr_seconds;

    uint64_t time_before_loop;
    float delta;
    uint64_t last_time;

} ALARM_CONFIG;

bool initialize_alarm(ALARM_STATE *state);
void process_alarm(ALARM_STATE *state, ALARM_CONFIG *config);
void render_alarm(ALARM_STATE *state, ALARM_CONFIG *config);
void quit_alarm();

void start_alarm();

void paint_background(uint16_t col);
void blink(ALARM_CONFIG *config);

#endif