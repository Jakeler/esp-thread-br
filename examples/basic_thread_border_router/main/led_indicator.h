#pragma once

#include "esp_err.h"
#include <stdint.h>
#include "openthread/thread.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t led_indicator_init(void);
esp_err_t led_indicator_set_color(uint32_t color);
esp_err_t led_indicator_set_role(otDeviceRole role);
bool led_indicator_has_peers(otInstance *instance);
void led_indicator_deinit(void);

#ifdef __cplusplus
}
#endif
