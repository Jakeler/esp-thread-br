#include "led_indicator.h"

#include "esp_check.h"
#include "esp_log.h"
#include "driver/led_strip.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "led_indicator";

static led_strip_handle_t s_led_strip = NULL;

static uint32_t led_indicator_role_to_color(otDeviceRole role)
{
    switch (role) {
    case OT_DEVICE_ROLE_DISABLED:
        return 0x000000;
    case OT_DEVICE_ROLE_DETACHED:
        return 0xFF0000;
    case OT_DEVICE_ROLE_CHILD:
        return 0x0000FF;
    case OT_DEVICE_ROLE_ROUTER:
        return 0x00FF00;
    case OT_DEVICE_ROLE_LEADER:
        return 0xFFFFFF;
    default:
        return 0x000000;
    }
}

esp_err_t led_indicator_init(void)
{
    ESP_LOGI(TAG, "Initializing LED indicator on GPIO %d", CONFIG_BR_LED_GPIO);

    led_strip_config_t strip_config = {
        .strip_gpio_num = CONFIG_BR_LED_GPIO,
        .max_leds = 1,
    };

    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000,
        .flags.with_dma = false,
    };

    ESP_RETURN_ON_ERROR(led_strip_new_rmt_device(&strip_config, &rmt_config, &s_led_strip), TAG,
                        "Failed to create LED strip");

    ESP_RETURN_ON_ERROR(led_strip_clear(s_led_strip), TAG, "Failed to clear LED strip");

    return ESP_OK;
}

esp_err_t led_indicator_set_color(uint32_t color)
{
    ESP_RETURN_ON_FALSE(s_led_strip, ESP_ERR_INVALID_STATE, TAG, "LED strip not initialized");

    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;

    ESP_RETURN_ON_ERROR(led_strip_set_pixel(s_led_strip, 0, r, g, b), TAG, "Failed to set LED color");
    ESP_RETURN_ON_ERROR(led_strip_refresh(s_led_strip), TAG, "Failed to refresh LED strip");

    return ESP_OK;
}

esp_err_t led_indicator_set_role(otDeviceRole role)
{
    return led_indicator_set_color(led_indicator_role_to_color(role));
}

void led_indicator_deinit(void)
{
    if (s_led_strip) {
        led_strip_clear(s_led_strip);
        led_strip_del(s_led_strip);
        s_led_strip = NULL;
    }
}
