#include <stdbool.h>
#include <string.h>

#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/usb/cdc.h>
#include <libopencm3/usb/usbd.h>

#include "FreeRTOS.h"
#include "stream_buffer.h"
#include "task.h"

#define LED_PORT GPIOC
#define LED_PIN  GPIO13

#define USB_CDC_COMM_IN_EP   0x82
#define USB_CDC_DATA_OUT_EP  0x01
#define USB_CDC_DATA_IN_EP   0x81
#define USB_CDC_PACKET_SIZE  64
#define USB_STREAM_SIZE      256

static usbd_device *usb_device;
static StreamBufferHandle_t usb_rx_stream;
static StreamBufferHandle_t usb_tx_stream;
static volatile bool usb_configured;
static uint8_t usb_control_buffer[128];
static struct usb_cdc_line_coding line_coding = {
    .dwDTERate = 115200,
    .bCharFormat = USB_CDC_1_STOP_BITS,
    .bParityType = USB_CDC_NO_PARITY,
    .bDataBits = 8,
};

struct cdcacm_functional_descriptors {
    struct usb_cdc_header_descriptor header;
    struct usb_cdc_call_management_descriptor call_mgmt;
    struct usb_cdc_acm_descriptor acm;
    struct usb_cdc_union_descriptor cdc_union;
} __attribute__((packed));

static const struct usb_device_descriptor device_descriptor = {
    .bLength = USB_DT_DEVICE_SIZE,
    .bDescriptorType = USB_DT_DEVICE,
    .bcdUSB = 0x0200,
    .bDeviceClass = USB_CLASS_CDC,
    .bDeviceSubClass = 0,
    .bDeviceProtocol = 0,
    .bMaxPacketSize0 = 64,
    .idVendor = 0x1209,
    .idProduct = 0x0002,
    .bcdDevice = 0x0100,
    .iManufacturer = 1,
    .iProduct = 2,
    .iSerialNumber = 3,
    .bNumConfigurations = 1,
};

static const struct usb_endpoint_descriptor comm_endpoints[] = {{
    .bLength = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType = USB_DT_ENDPOINT,
    .bEndpointAddress = USB_CDC_COMM_IN_EP,
    .bmAttributes = USB_ENDPOINT_ATTR_INTERRUPT,
    .wMaxPacketSize = 16,
    .bInterval = 255,
}};

static const struct cdcacm_functional_descriptors functional_descriptors = {
    .header = {
        .bFunctionLength = sizeof(struct usb_cdc_header_descriptor),
        .bDescriptorType = CS_INTERFACE,
        .bDescriptorSubtype = USB_CDC_TYPE_HEADER,
        .bcdCDC = 0x0110,
    },
    .call_mgmt = {
        .bFunctionLength = sizeof(struct usb_cdc_call_management_descriptor),
        .bDescriptorType = CS_INTERFACE,
        .bDescriptorSubtype = USB_CDC_TYPE_CALL_MANAGEMENT,
        .bmCapabilities = 0,
        .bDataInterface = 1,
    },
    .acm = {
        .bFunctionLength = sizeof(struct usb_cdc_acm_descriptor),
        .bDescriptorType = CS_INTERFACE,
        .bDescriptorSubtype = USB_CDC_TYPE_ACM,
        .bmCapabilities = 0,
    },
    .cdc_union = {
        .bFunctionLength = sizeof(struct usb_cdc_union_descriptor),
        .bDescriptorType = CS_INTERFACE,
        .bDescriptorSubtype = USB_CDC_TYPE_UNION,
        .bControlInterface = 0,
        .bSubordinateInterface0 = 1,
    },
};

static const struct usb_interface_descriptor comm_iface[] = {{
    .bLength = USB_DT_INTERFACE_SIZE,
    .bDescriptorType = USB_DT_INTERFACE,
    .bInterfaceNumber = 0,
    .bAlternateSetting = 0,
    .bNumEndpoints = 1,
    .bInterfaceClass = USB_CLASS_CDC,
    .bInterfaceSubClass = USB_CDC_SUBCLASS_ACM,
    .bInterfaceProtocol = USB_CDC_PROTOCOL_AT,
    .iInterface = 0,
    .endpoint = comm_endpoints,
    .extra = &functional_descriptors,
    .extralen = sizeof(functional_descriptors),
}};

static const struct usb_endpoint_descriptor data_endpoints[] = {{
    .bLength = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType = USB_DT_ENDPOINT,
    .bEndpointAddress = USB_CDC_DATA_OUT_EP,
    .bmAttributes = USB_ENDPOINT_ATTR_BULK,
    .wMaxPacketSize = USB_CDC_PACKET_SIZE,
    .bInterval = 1,
}, {
    .bLength = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType = USB_DT_ENDPOINT,
    .bEndpointAddress = USB_CDC_DATA_IN_EP,
    .bmAttributes = USB_ENDPOINT_ATTR_BULK,
    .wMaxPacketSize = USB_CDC_PACKET_SIZE,
    .bInterval = 1,
}};

static const struct usb_interface_descriptor data_iface[] = {{
    .bLength = USB_DT_INTERFACE_SIZE,
    .bDescriptorType = USB_DT_INTERFACE,
    .bInterfaceNumber = 1,
    .bAlternateSetting = 0,
    .bNumEndpoints = 2,
    .bInterfaceClass = USB_CLASS_DATA,
    .bInterfaceSubClass = 0,
    .bInterfaceProtocol = 0,
    .iInterface = 0,
    .endpoint = data_endpoints,
}};

static const struct usb_interface interfaces[] = {{
    .num_altsetting = 1,
    .altsetting = comm_iface,
}, {
    .num_altsetting = 1,
    .altsetting = data_iface,
}};

static const struct usb_config_descriptor config_descriptor = {
    .bLength = USB_DT_CONFIGURATION_SIZE,
    .bDescriptorType = USB_DT_CONFIGURATION,
    .wTotalLength = 0,
    .bNumInterfaces = 2,
    .bConfigurationValue = 1,
    .iConfiguration = 0,
    .bmAttributes = 0x80,
    .bMaxPower = 0x32,
    .interface = interfaces,
};

static const char *const usb_strings[] = {
    "Example",
    "STM32F103 CDC ACM FreeRTOS",
    "DEMO002",
};

static void clock_setup(void)
{
    rcc_clock_setup_pll(&rcc_hse_configs[RCC_CLOCK_HSE8_72MHZ]);
    rcc_set_usbpre(RCC_CFGR_USBPRE_PLL_CLK_DIV1_5);
}

static void gpio_setup(void)
{
    rcc_periph_clock_enable(RCC_GPIOC);
    rcc_periph_clock_enable(RCC_GPIOA);

    gpio_set(LED_PORT, LED_PIN);
    gpio_set_mode(LED_PORT, GPIO_MODE_OUTPUT_2_MHZ,
                  GPIO_CNF_OUTPUT_PUSHPULL, LED_PIN);

}

static void usb_disconnect_pulse(void)
{
    gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_2_MHZ,
                  GPIO_CNF_OUTPUT_PUSHPULL, GPIO12);
    gpio_clear(GPIOA, GPIO12);
    for (uint32_t i = 0; i < 800000; i++) {
        __asm__("nop");
    }
    gpio_set_mode(GPIOA, GPIO_MODE_INPUT,
                  GPIO_CNF_INPUT_FLOAT, GPIO12);
}

static enum usbd_request_return_codes cdc_control_request(
    usbd_device *dev,
    struct usb_setup_data *request,
    uint8_t **buffer,
    uint16_t *length,
    usbd_control_complete_callback *complete)
{
    (void)dev;
    (void)complete;

    switch (request->bRequest) {
    case USB_CDC_REQ_SET_CONTROL_LINE_STATE:
        return USBD_REQ_HANDLED;
    case USB_CDC_REQ_SET_LINE_CODING:
        if (*length < sizeof(line_coding)) {
            return USBD_REQ_NOTSUPP;
        }
        memcpy(&line_coding, *buffer, sizeof(line_coding));
        return USBD_REQ_HANDLED;
    case USB_CDC_REQ_GET_LINE_CODING:
        *buffer = (uint8_t *)&line_coding;
        *length = sizeof(line_coding);
        return USBD_REQ_HANDLED;
    default:
        return USBD_REQ_NOTSUPP;
    }
}

static void cdc_data_rx(usbd_device *dev, uint8_t endpoint)
{
    uint8_t buffer[USB_CDC_PACKET_SIZE];
    const uint16_t length = usbd_ep_read_packet(dev, endpoint, buffer, sizeof(buffer));

    (void)xStreamBufferSend(usb_rx_stream, buffer, length, 0);
}

static void usb_set_config(usbd_device *dev, uint16_t value)
{
    if (value == 0) {
        usb_configured = false;
        return;
    }

    usbd_ep_setup(dev, USB_CDC_DATA_OUT_EP, USB_ENDPOINT_ATTR_BULK,
                  USB_CDC_PACKET_SIZE, cdc_data_rx);
    usbd_ep_setup(dev, USB_CDC_DATA_IN_EP, USB_ENDPOINT_ATTR_BULK,
                  USB_CDC_PACKET_SIZE, NULL);
    usbd_ep_setup(dev, USB_CDC_COMM_IN_EP, USB_ENDPOINT_ATTR_INTERRUPT,
                  16, NULL);

    usbd_register_control_callback(dev,
                                   USB_REQ_TYPE_CLASS | USB_REQ_TYPE_INTERFACE,
                                   USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT,
                                   cdc_control_request);

    usb_configured = true;
}

static void usb_reset(void)
{
    usb_configured = false;
}

static void usb_setup(void)
{
    usb_disconnect_pulse();
    usb_device = usbd_init(&st_usbfs_v1_usb_driver,
                           &device_descriptor,
                           &config_descriptor,
                           usb_strings,
                           3,
                           usb_control_buffer,
                           sizeof(usb_control_buffer));
    usbd_register_reset_callback(usb_device, usb_reset);
    usbd_register_set_config_callback(usb_device, usb_set_config);
}

static void usb_task(void *argument)
{
    (void)argument;

    uint8_t packet[USB_CDC_PACKET_SIZE];
    uint16_t pending_length = 0;

    while (1) {
        usbd_poll(usb_device);

        if (usb_configured) {
            if (pending_length == 0) {
                pending_length = (uint16_t)xStreamBufferReceive(usb_tx_stream,
                                                                packet,
                                                                sizeof(packet),
                                                                0);
            }

            if (pending_length > 0) {
                const uint16_t sent = usbd_ep_write_packet(usb_device,
                                                           USB_CDC_DATA_IN_EP,
                                                           packet,
                                                           pending_length);
                if (sent == pending_length) {
                    pending_length = 0;
                }
            }

            vTaskDelay(pdMS_TO_TICKS(1));
        } else {
            taskYIELD();
        }
    }
}

static void stream_send_all(StreamBufferHandle_t stream, const uint8_t *data, size_t length)
{
    size_t sent = 0;

    while (sent < length) {
        sent += xStreamBufferSend(stream, data + sent, length - sent, portMAX_DELAY);
    }
}

static void echo_task(void *argument)
{
    (void)argument;

    uint8_t buffer[USB_CDC_PACKET_SIZE];

    while (1) {
        const size_t length = xStreamBufferReceive(usb_rx_stream,
                                                   buffer,
                                                   sizeof(buffer),
                                                   portMAX_DELAY);
        if (length > 0) {
            stream_send_all(usb_tx_stream, buffer, length);
        }
    }
}

static void led_task(void *argument)
{
    (void)argument;

    while (1) {
        gpio_toggle(LED_PORT, LED_PIN);
        vTaskDelay(pdMS_TO_TICKS(500));
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
    usb_setup();

    nvic_set_priority(NVIC_PENDSV_IRQ, 0xff);
    nvic_set_priority(NVIC_SYSTICK_IRQ, 0xff);
    nvic_set_priority(NVIC_SV_CALL_IRQ, 0);

    usb_rx_stream = xStreamBufferCreate(USB_STREAM_SIZE, 1);
    usb_tx_stream = xStreamBufferCreate(USB_STREAM_SIZE, 1);

    if ((usb_rx_stream == NULL) || (usb_tx_stream == NULL)) {
        vApplicationMallocFailedHook();
    }

    xTaskCreate(usb_task, "usb", configMINIMAL_STACK_SIZE * 2, NULL, 3, NULL);
    xTaskCreate(echo_task, "echo", configMINIMAL_STACK_SIZE * 2, NULL, 2, NULL);
    xTaskCreate(led_task, "led", configMINIMAL_STACK_SIZE, NULL, 1, NULL);

    vTaskStartScheduler();

    while (1) {
    }
}
