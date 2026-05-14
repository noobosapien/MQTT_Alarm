#include "alarm.h"
#include "interrupts.h"

ALARM_CONFIG alarm_config;

void app_main()
{
    set_interrupts();
    // Set the alarm state and initialize the config of the alarm
    ALARM_STATE alarm_state = INIT;

    alarm_config.valid = false;
    alarm_config.year = 0;
    alarm_config.month = 0;
    alarm_config.day = 0;
    alarm_config.hour = 0;
    alarm_config.minute = 0;
    alarm_config.seconds = 0;

    alarm_config.curr_year = 0;
    alarm_config.curr_month = 0;
    alarm_config.curr_day = 0;
    alarm_config.curr_hour = 0;
    alarm_config.curr_minute = 0;
    alarm_config.curr_seconds = 0;

    alarm_config.time_before_loop = 0;
    alarm_config.delta = 0.f;
    alarm_config.last_time = 0;

    initialize_alarm(&alarm_state);

    // Run the main loop of the alarm
    while (1)
    {
        process_alarm(&alarm_state, &alarm_config);
        render_alarm(&alarm_state, &alarm_config);
    }

    quit_alarm();
}