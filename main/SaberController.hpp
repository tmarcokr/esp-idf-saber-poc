#pragma once

#include "AudioEngine.hpp"
#include "BladeIgnite.hpp"
#include "Engine.hpp"
#include "SmoothSwingSample.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <atomic>
#include <memory>

namespace Espressif::App {

/**
 * @brief Orchestrates the high-level logic of the lightsaber.
 */
class SaberController {
public:
    SaberController(BladeIgnite& blade, 
                    SmoothSwingSample& swing,
                    Espressif::Wrappers::Audio::AudioEngine& audio,
                    Espressif::Wrappers::SmartLed::Engine& led);

    esp_err_t begin();

    /**
     * @brief Handles a standard trigger (click).
     * Ignite if OFF, play blaster if ON.
     */
    void trigger();

    /**
     * @brief Requests the saber to retract (long press).
     */
    void requestRetract();

    void requestLongPressSmall();
    void releaseButton();

private:
    static void controllerTask(void* pvParameters);

    BladeIgnite& m_blade;
    SmoothSwingSample& m_swing;
    Espressif::Wrappers::Audio::AudioEngine& m_audio;
    Espressif::Wrappers::SmartLed::Engine& m_led;
    
    std::atomic<bool> m_is_ignited{false};
    std::atomic<bool> m_is_dragging{false};
    uint32_t m_last_click_time_ms{0};
    Espressif::Wrappers::Audio::ChannelId m_drag_channel{Espressif::Wrappers::Audio::INVALID_CHANNEL};
    TaskHandle_t m_task_handle{nullptr};

    static constexpr uint32_t NOTIFY_TRIGGER    = (1 << 0);
    static constexpr uint32_t NOTIFY_RETRACT    = (1 << 1);
    static constexpr uint32_t NOTIFY_DRAG_START = (1 << 2);
    static constexpr uint32_t NOTIFY_RELEASE    = (1 << 3);
};

} // namespace Espressif::App
