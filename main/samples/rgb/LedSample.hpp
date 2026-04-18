#pragma once

#include "ISample.hpp"
#include "RgbLed.hpp"
#include <memory>

namespace Espressif::App {

/**
 * @brief LED Sample implementing the color fading loop.
 */
class LedSample : public ISample {
public:
    /**
     * @brief Construct a new Led Sample.
     * @param blink_gpio GPIO pin for the status LED.
     */
    explicit LedSample(gpio_num_t blink_gpio);
    
    /**
     * @brief Initialize the RGB LED driver.
     * @return esp_err_t ESP_OK on success.
     */
    esp_err_t setup() override;
    
    /**
     * @brief Start the infinite fading loop.
     */
    void run() override;

private:
    std::unique_ptr<Wrappers::RgbLed> _led;
    gpio_num_t _gpio;
};

} // namespace Espressif::App
