#pragma once

#include "ISample.hpp"
#include "AudioEngine.hpp"
#include "Mpu6050.hpp"
#include "sd_card.hpp"

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include <atomic>
#include <memory>
#include <string_view>

namespace Espressif::App {

// RAII Semaphore Wrapper (Quality_Auditor compliance)
struct SmoothSwingSemaphoreDeleter {
    void operator()(QueueDefinition* s) const {
        if (s) vSemaphoreDelete(s);
    }
};
using SSSemaphorePtr = std::unique_ptr<QueueDefinition, SmoothSwingSemaphoreDeleter>;

[[nodiscard]] inline SSSemaphorePtr makeSSBinarySemaphore() {
    return SSSemaphorePtr(xSemaphoreCreateBinary());
}

/**
 * @brief SmoothSwing integration sample.
 *
 * Implements a SmoothSwing algorithm using the MPU-6050.
 * Uses hardware interrupts to grab motion data, calculates angular velocity,
 * and dynamically maps volumes to looping audio.
 */
class SmoothSwingSample : public ISample {
public:
    /**
     * @brief Construct a new Smooth Swing Sample.
     * @param sd_config SD card configuration.
     * @param audio_config Audio engine I2S configuration.
     * @param mpu_sda MPU SDA pin.
     * @param mpu_scl MPU SCL pin.
     * @param mpu_int MPU INT pin.
     */
    SmoothSwingSample(const Wrappers::SdCard::Config& sd_config,
                      const Wrappers::Audio::AudioEngine::Config& audio_config,
                      gpio_num_t mpu_sda, gpio_num_t mpu_scl, gpio_num_t mpu_int);

    /**
     * @brief Initialize SD card, Audio Engine, and MPU driver.
     * @return esp_err_t ESP_OK on success.
     */
    esp_err_t setup() override;

    /**
     * @brief Execute the SmoothSwing motion-processing loop (blocking).
     *
     * Runs continuously but only processes sensor data when active.
     * Use startAudio() / stopAudio() to control audio lifecycle externally.
     */
    void run() override;

    /**
     * @brief Start saber audio: poweron.wav + looping hum and swing channels.
     */
    void startAudio();

    /**
     * @brief Stop saber audio: fade out channels and play poweroff.wav.
     */
    void stopAudio();

    void setActive(bool active);
    [[nodiscard]] bool isActive() const;

private:
    std::unique_ptr<Wrappers::SdCard> m_sd;
    std::unique_ptr<Wrappers::Audio::AudioEngine> m_engine;
    std::unique_ptr<Wrappers::Sensors::Mpu6050> m_mpu;

    Wrappers::SdCard::Config m_sd_config;
    Wrappers::Audio::AudioEngine::Config m_audio_config;
    gpio_num_t m_mpu_sda, m_mpu_scl, m_mpu_int;

    SSSemaphorePtr m_semaphore;

    static constexpr const char* TAG = "SmoothSwingSample";

    // Audio channels
    Wrappers::Audio::ChannelId m_ch_hum = Wrappers::Audio::INVALID_CHANNEL;
    Wrappers::Audio::ChannelId m_ch_swingl = Wrappers::Audio::INVALID_CHANNEL;
    Wrappers::Audio::ChannelId m_ch_swingh = Wrappers::Audio::INVALID_CHANNEL;

    // SmoothSwing state
    float m_virtual_position = 0.0f; 
    uint32_t m_last_accent_time_ms = 0;

    std::atomic<bool> m_active{false};

    /**
     * @brief MPU hardware interrupt handler.
     */
    static void IRAM_ATTR isrHandler(void* arg);

    /**
     * @brief Configures GPIO for the MPU INT pin.
     */
    [[nodiscard]] esp_err_t configureInterruptGpio(gpio_num_t pin);
    
    /**
     * @brief Process motion data
     * @param swing_speed_dps Magnitude of rotational velocity.
     * @param lin_accel_g Magnitude of linear acceleration (excluding gravity).
     * @param delta_time_s Time since last update.
     */
    void processMotion(float swing_speed_dps, float lin_accel_g, float delta_time_s);
};

} // namespace Espressif::App
