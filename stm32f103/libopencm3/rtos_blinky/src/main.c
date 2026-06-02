#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>

#include "FreeRTOS.h"
#include "task.h"

#define LED_PORT GPIOC
#define LED_PIN  GPIO13

static void clock_setup(void)
{
    rcc_clock_setup_pll(&rcc_hse_configs[RCC_CLOCK_HSE8_72MHZ]);
}

static void gpio_setup(void)
{
    rcc_periph_clock_enable(RCC_GPIOC);

    gpio_set(LED_PORT, LED_PIN);
    gpio_set_mode(LED_PORT, GPIO_MODE_OUTPUT_2_MHZ,
                  GPIO_CNF_OUTPUT_PUSHPULL, LED_PIN);
}

static void fast_led_task(void *argument)
{
    (void)argument;

    while (1) {
        gpio_toggle(LED_PORT, LED_PIN);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

static void slow_led_task(void *argument)
{
    (void)argument;

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void vApplicationMallocFailedHook(void)
{
    taskDISABLE_INTERRUPTS();
    while (1) {
    }
}

void vApplicationStackOverflowHook(TaskHandle_t task, char *task_name)
{
    (void)task;
    (void)task_name;

    taskDISABLE_INTERRUPTS();
    while (1) {
    }
}

int main(void)
{
    clock_setup();
    gpio_setup();

    nvic_set_priority(NVIC_PENDSV_IRQ, 0xff);
    nvic_set_priority(NVIC_SYSTICK_IRQ, 0xff);
    nvic_set_priority(NVIC_SV_CALL_IRQ, 0);

    xTaskCreate(fast_led_task, "fast", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    xTaskCreate(slow_led_task, "slow", configMINIMAL_STACK_SIZE, NULL, 1, NULL);

    vTaskStartScheduler();

    while (1) {
    }
}
