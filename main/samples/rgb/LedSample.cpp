#include "LedSample.hpp"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace Espressif::App {

static constexpr const char* TAG = "LedSample";

LedSample::LedSample(gpio_num_t blink_gpio)
    : _gpio(blink_gpio) {}

esp_err_t LedSample::setup() {
    _led = std::make_unique<Wrappers::RgbLed>(_gpio);
    return _led->init();
}

void LedSample::run() {
    constexpr Wrappers::Color pure_red = {255, 0, 0};
    constexpr Wrappers::Color pure_green = {0, 255, 0};
    constexpr Wrappers::Color pure_blue = {0, 0, 255};
    constexpr Wrappers::Color warm_white = {255, 200, 150};

    ESP_LOGI(TAG, "Starting infinite color transition loop.");

    while (true) {
        _led->fadeTo(pure_red, 2000);
        _led->fadeTo(pure_green, 2000);
        _led->fadeTo(pure_blue, 2000);
        _led->fadeTo(warm_white, 2000);
        _led->fadeTo({0, 0, 0}, 1000);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

} // namespace Espressif::App
