#include "sdkconfig.h"

#if CONFIG_BR_LED_ENABLED
#include "led_indicator.h"

#include "esp_check.h"
#include "esp_log.h"
#include "driver/rmt_encoder.h"
#include "driver/rmt_tx.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "led_indicator";

#define LED_STRIP_RESOLUTION_HZ 10000000 // 10MHz

static rmt_channel_handle_t s_led_chan = NULL;
static rmt_encoder_handle_t s_bytes_encoder = NULL;
static rmt_encoder_handle_t s_copy_encoder = NULL;
static uint8_t s_pixel[3];

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

static void led_indicator_color_to_pixel(uint32_t color, uint8_t *r, uint8_t *g, uint8_t *b)
{
    *r = (color >> 16) & 0xFF;
    *g = (color >> 8) & 0xFF;
    *b = color & 0xFF;

#if CONFIG_BR_LED_BRIGHTNESS < 255
    float scale = (float)CONFIG_BR_LED_BRIGHTNESS / 255.0f;
    *r = (uint8_t)(*r * scale);
    *g = (uint8_t)(*g * scale);
    *b = (uint8_t)(*b * scale);
#endif
}

esp_err_t led_indicator_init(void)
{
    ESP_LOGI(TAG, "Initializing LED indicator on GPIO %d", CONFIG_BR_LED_GPIO);

    rmt_tx_channel_config_t tx_chan_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .gpio_num = CONFIG_BR_LED_GPIO,
        .mem_block_symbols = 64,
        .resolution_hz = LED_STRIP_RESOLUTION_HZ,
        .trans_queue_depth = 4,
    };
    ESP_RETURN_ON_ERROR(rmt_new_tx_channel(&tx_chan_config, &s_led_chan), TAG,
                        "Failed to create RMT TX channel");

    rmt_bytes_encoder_config_t bytes_encoder_config = {
        .bit0 =
            {
                .level0 = 1,
                .duration0 = 0.3 * LED_STRIP_RESOLUTION_HZ / 1000000,
                .level1 = 0,
                .duration1 = 0.9 * LED_STRIP_RESOLUTION_HZ / 1000000,
            },
        .bit1 =
            {
                .level0 = 1,
                .duration0 = 0.9 * LED_STRIP_RESOLUTION_HZ / 1000000,
                .level1 = 0,
                .duration1 = 0.3 * LED_STRIP_RESOLUTION_HZ / 1000000,
            },
        .flags.msb_first = 1,
    };
    ESP_RETURN_ON_ERROR(rmt_new_bytes_encoder(&bytes_encoder_config, &s_bytes_encoder), TAG,
                        "Failed to create bytes encoder");

    rmt_copy_encoder_config_t copy_encoder_config = {};
    ESP_RETURN_ON_ERROR(rmt_new_copy_encoder(&copy_encoder_config, &s_copy_encoder), TAG,
                        "Failed to create copy encoder");

    ESP_ERROR_CHECK(rmt_enable(s_led_chan));

    return ESP_OK;
}

esp_err_t led_indicator_set_color(uint32_t color)
{
    ESP_RETURN_ON_FALSE(s_led_chan && s_bytes_encoder && s_copy_encoder, ESP_ERR_INVALID_STATE, TAG,
                        "LED strip not initialized");

    uint8_t r, g, b;
    led_indicator_color_to_pixel(color, &r, &g, &b);

    s_pixel[0] = g;
    s_pixel[1] = r;
    s_pixel[2] = b;

    rmt_transmit_config_t tx_config = {
        .loop_count = 0,
    };

    ESP_RETURN_ON_ERROR(rmt_transmit(s_led_chan, s_bytes_encoder, s_pixel, sizeof(s_pixel), &tx_config), TAG,
                        "Failed to transmit pixel data");

    rmt_symbol_word_t reset_code = {
        .level0 = 0,
        .duration0 = LED_STRIP_RESOLUTION_HZ / 1000000 * 50 / 2,
        .level1 = 0,
        .duration1 = LED_STRIP_RESOLUTION_HZ / 1000000 * 50 / 2,
    };
    ESP_RETURN_ON_ERROR(rmt_transmit(s_led_chan, s_copy_encoder, &reset_code, sizeof(reset_code), &tx_config), TAG,
                        "Failed to transmit reset code");

    ESP_RETURN_ON_ERROR(rmt_tx_wait_all_done(s_led_chan, portMAX_DELAY), TAG,
                        "Failed to wait for TX done");

    return ESP_OK;
}

esp_err_t led_indicator_set_role(otDeviceRole role)
{
    return led_indicator_set_color(led_indicator_role_to_color(role));
}

void led_indicator_deinit(void)
{
    if (s_led_chan) {
        rmt_disable(s_led_chan);
        rmt_del_channel(s_led_chan);
        s_led_chan = NULL;
    }
    if (s_bytes_encoder) {
        rmt_del_encoder(s_bytes_encoder);
        s_bytes_encoder = NULL;
    }
    if (s_copy_encoder) {
        rmt_del_encoder(s_copy_encoder);
        s_copy_encoder = NULL;
    }
}
#endif
