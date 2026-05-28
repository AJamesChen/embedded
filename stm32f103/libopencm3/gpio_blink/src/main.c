#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>

static volatile uint32_t milliseconds;

void sys_tick_handler(void)
{
    milliseconds++;
}

static void clock_setup(void)
{
    rcc_clock_setup_pll(&rcc_hse_configs[RCC_CLOCK_HSE8_72MHZ]);
}

static void systick_setup(void)
{
    systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);
    systick_set_reload(72000 - 1);
    systick_counter_enable();
    systick_interrupt_enable();
}

static void gpio_setup(void)
{
    rcc_periph_clock_enable(RCC_GPIOC);

    gpio_set(GPIOC, GPIO13);
    gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_2_MHZ,
                  GPIO_CNF_OUTPUT_PUSHPULL, GPIO13);
}

static void delay_ms(uint32_t delay)
{
    const uint32_t start = milliseconds;

    while ((milliseconds - start) < delay) {
        __asm__("wfi");
    }
}

int main(void)
{
    clock_setup();
    systick_setup();
    gpio_setup();

    while (1) {
        gpio_toggle(GPIOC, GPIO13);
        delay_ms(2000);
    }
}
