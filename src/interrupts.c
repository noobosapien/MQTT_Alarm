#include <driver/gpio.h>
#include <esp_log.h>
#include <esp_timer.h>
#include <esp_attr.h>
#include <rom/ets_sys.h>

static void IRAM_ATTR gpio_isr_handler(void* arg){
    uint32_t gpio_num = (uint32_t) arg;
    int val = gpio_get_level(gpio_num);
    static uint64_t lastkeytime;

    uint64_t time = esp_timer_get_time();
    uint64_t timesince = time - lastkeytime;
    ets_printf("gpio_isr_handler %d %d %lld\n", gpio_num, val, timesince);
    lastkeytime = time;
}

void set_interrupts(){
    gpio_set_direction(0, GPIO_MODE_INPUT);
    gpio_set_direction(35, GPIO_MODE_INPUT);

    gpio_set_intr_type(0, GPIO_INTR_POSEDGE);
    gpio_set_intr_type(35, GPIO_INTR_ANYEDGE);

    gpio_install_isr_service(0);

    gpio_isr_handler_add(0, gpio_isr_handler, (void*) 0);
    gpio_isr_handler_add(35, gpio_isr_handler, (void*) 35);
}