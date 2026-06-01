#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/usart.h>

#include "FreeRTOS.h"
#include "task.h"

#define LED_PORT GPIOC
#define LED_PIN  GPIO13

#define UART_PORT GPIOA
#define UART_TX   GPIO9
#define UART_RX   GPIO10

static void clock_setup(void)
{
    rcc_clock_setup_pll(&rcc_hse_configs[RCC_CLOCK_HSE8_72MHZ]);
}

static void gpio_setup(void)
{
    rcc_periph_clock_enable(RCC_GPIOC);
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_AFIO);

    gpio_set(LED_PORT, LED_PIN);
    gpio_set_mode(LED_PORT, GPIO_MODE_OUTPUT_2_MHZ,
                  GPIO_CNF_OUTPUT_PUSHPULL, LED_PIN);

    gpio_set_mode(UART_PORT, GPIO_MODE_OUTPUT_50_MHZ,
                  GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, UART_TX);
    gpio_set_mode(UART_PORT, GPIO_MODE_INPUT,
                  GPIO_CNF_INPUT_FLOAT, UART_RX);
}

static void usart_setup(void)
{
    rcc_periph_clock_enable(RCC_USART1);

    usart_set_baudrate(USART1, 115200);
    usart_set_databits(USART1, 8);
    usart_set_stopbits(USART1, USART_STOPBITS_1);
    usart_set_mode(USART1, USART_MODE_TX_RX);
    usart_set_parity(USART1, USART_PARITY_NONE);
    usart_set_flow_control(USART1, USART_FLOWCONTROL_NONE);

    usart_enable(USART1);
}

static void uart_write(const char *text)
{
    while (*text != '\0') {
        usart_send_blocking(USART1, (uint16_t)*text++);
    }
}

static void uart_write_digit(uint32_t digit)
{
    usart_send_blocking(USART1, (uint16_t)('0' + digit));
}

static void uart_write_2digits(uint32_t value)
{
    uart_write_digit((value / 10) % 10);
    uart_write_digit(value % 10);
}

static void uart_write_3digits(uint32_t value)
{
    uart_write_digit((value / 100) % 10);
    uart_write_digit((value / 10) % 10);
    uart_write_digit(value % 10);
}

static void uart_write_time(TickType_t ticks)
{
    const uint32_t total_ms = ticks * portTICK_PERIOD_MS;
    const uint32_t milliseconds = total_ms % 1000;
    const uint32_t total_seconds = total_ms / 1000;
    const uint32_t seconds = total_seconds % 60;
    const uint32_t total_minutes = total_seconds / 60;
    const uint32_t minutes = total_minutes % 60;
    const uint32_t hours = (total_minutes / 60) % 24;

    uart_write_2digits(hours);
    uart_write(":");
    uart_write_2digits(minutes);
    uart_write(":");
    uart_write_2digits(seconds);
    uart_write(".");
    uart_write_3digits(milliseconds);
}

static void led_task(void *argument)
{
    (void)argument;

    while (1) {
        gpio_toggle(LED_PORT, LED_PIN);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

static void uart_task(void *argument)
{
    (void)argument;

    while (1) {
        uart_write("[");
        uart_write_time(xTaskGetTickCount());
        uart_write("] FreeRTOS UART example running on USART1\r\n");
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
    usart_setup();

    nvic_set_priority(NVIC_PENDSV_IRQ, 0xff);
    nvic_set_priority(NVIC_SYSTICK_IRQ, 0xff);
    nvic_set_priority(NVIC_SV_CALL_IRQ, 0);

    xTaskCreate(led_task, "led", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    xTaskCreate(uart_task, "uart", configMINIMAL_STACK_SIZE * 2, NULL, 2, NULL);

    vTaskStartScheduler();

    while (1) {
    }
}
