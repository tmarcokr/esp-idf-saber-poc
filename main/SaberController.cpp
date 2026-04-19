#include "SaberController.hpp"
#include "BlasterImpact.hpp"
#include "BladeDrag.hpp"
#include "esp_log.h"
#include "esp_timer.h"

static constexpr const char* TAG = "SaberController";

namespace Espressif::App {

SaberController::SaberController(BladeIgnite& blade, 
                                SmoothSwingSample& swing,
                                Espressif::Wrappers::Audio::AudioEngine& audio,
                                Espressif::Wrappers::SmartLed::Engine& led)
    : m_blade(blade), m_swing(swing), m_audio(audio), m_led(led) {}

static void smoothswing_task(void* pvParameters) {
    auto* swing = static_cast<SmoothSwingSample*>(pvParameters);
    swing->run();
    vTaskDelete(nullptr);
}

esp_err_t SaberController::begin() {
    BaseType_t ret = xTaskCreate(
        SaberController::controllerTask,
        "saber_ctrl",
        8192,
        this,
        5,
        &m_task_handle
    );

    if (ret == pdPASS) {
        xTaskCreate(smoothswing_task, "ss_task", 8192, &m_swing, 5, nullptr);
    }

    return (ret == pdPASS) ? ESP_OK : ESP_FAIL;
}

void SaberController::trigger() {
    if (m_task_handle) {
        xTaskNotify(m_task_handle, NOTIFY_TRIGGER, eSetBits);
    }
}

void SaberController::requestRetract() {
    if (m_task_handle) {
        xTaskNotify(m_task_handle, NOTIFY_RETRACT, eSetBits);
    }
}

void SaberController::requestLongPressSmall() {
    if (m_task_handle) {
        xTaskNotify(m_task_handle, NOTIFY_DRAG_START, eSetBits);
    }
}

void SaberController::releaseButton() {
    if (m_task_handle) {
        xTaskNotify(m_task_handle, NOTIFY_RELEASE, eSetBits);
    }
}

void SaberController::controllerTask(void* pvParameters) {
    auto* self = static_cast<SaberController*>(pvParameters);
    uint32_t notified_value = 0;

    while (true) {
        xTaskNotifyWait(0, 0xFFFFFFFF, &notified_value, portMAX_DELAY);

        auto state = self->m_blade.state();
        bool is_ignited = self->m_is_ignited.load();

        // 1. Handle Trigger (Click)
        if (notified_value & NOTIFY_TRIGGER) {
            self->m_last_click_time_ms = esp_timer_get_time() / 1000;
            if (!is_ignited) {
                // Ignite if OFF
                if (state != BladeIgnite::State::Igniting) {
                    ESP_LOGI(TAG, "IGNITE");
                    self->m_blade.ignite();
                    self->m_swing.startAudio();
                    self->m_is_ignited.store(true);
                }
            } else {
                // Play Blaster if ON
                ESP_LOGI(TAG, "BLASTER");
                self->m_audio.play("/sdcard/saber/blaster.wav", false, 16384);
                self->m_led.pushOverlay(std::make_unique<BlasterImpact>(400));
            }
        }

        // 2. Handle Retract (Long Press)
        if (notified_value & NOTIFY_RETRACT) {
            if (is_ignited && state != BladeIgnite::State::Retracting && !self->m_is_dragging.load()) {
                ESP_LOGI(TAG, "RETRACT");
                self->m_blade.retract();
                self->m_swing.stopAudio();
                self->m_is_ignited.store(false);
            }
        }

        // 3. Handle Drag Start
        if (notified_value & NOTIFY_DRAG_START) {
            if (is_ignited && !self->m_is_dragging.load()) {
                uint32_t now = esp_timer_get_time() / 1000;
                // Click must have happened within 1500 ms smoothly
                if ((now - self->m_last_click_time_ms) < 1500) {
                    ESP_LOGI(TAG, "DRAG START");
                    self->m_is_dragging.store(true);
                    self->m_drag_channel = self->m_audio.play("/sdcard/saber/drag1.wav", true, 16384);
                    self->m_led.pushOverlay(std::make_unique<BladeDrag>(&self->m_is_dragging));
                }
            }
        }

        // 4. Handle Release
        if (notified_value & NOTIFY_RELEASE) {
            if (self->m_is_dragging.load()) {
                ESP_LOGI(TAG, "DRAG STOP");
                self->m_is_dragging.store(false);
                if (self->m_drag_channel != Espressif::Wrappers::Audio::INVALID_CHANNEL) {
                    self->m_audio.stop(self->m_drag_channel);
                    self->m_drag_channel = Espressif::Wrappers::Audio::INVALID_CHANNEL;
                }
                self->m_audio.play("/sdcard/saber/enddrag1.wav", false, 16384);
            }
        }
    }
}

} // namespace Espressif::App
