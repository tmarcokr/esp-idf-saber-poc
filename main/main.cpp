#include "BladeIgnite.hpp"
#include "GpioButton.hpp"
#include "SmartLed.hpp"
#include "SmoothSwingSample.hpp"
#include "esp_log.h"
#include "freertos/FreeRTOS.h" // IWYU pragma: keep
#include "freertos/task.h"
#include <atomic>
#include <memory>

static constexpr const char* TAG = "SaberPoC";

static constexpr gpio_num_t BOOT_BUTTON_GPIO = GPIO_NUM_9;
static constexpr gpio_num_t LED_STRIP_GPIO   = GPIO_NUM_0;
static constexpr uint16_t   NUM_BLADE_LEDS   = 5;

static std::atomic<bool> s_saber_on{false};
static TaskHandle_t s_controller_task = nullptr;

struct SaberContext {
    Espressif::App::BladeIgnite* blade;
    Espressif::App::SmoothSwingSample* swing;
};

/**
 * @brief Orchestrates ignition/retraction transitions.
 *
 * Waits for task notifications from the button callback,
 * ignoring presses during active animations.
 */
static void saber_controller_task(void* pvParameters) {
    auto* ctx = static_cast<SaberContext*>(pvParameters);

    while (true) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        auto state = ctx->blade->state();

        if (state == Espressif::App::BladeIgnite::State::Igniting ||
            state == Espressif::App::BladeIgnite::State::Retracting) {
            continue;
        }

        if (!s_saber_on.load()) {
            ESP_LOGI(TAG, "IGNITE");
            ctx->blade->ignite();
            ctx->swing->startAudio();
            s_saber_on.store(true);
        } else {
            ESP_LOGI(TAG, "RETRACT");
            ctx->blade->retract();
            ctx->swing->stopAudio();
            s_saber_on.store(false);
        }
    }
}

static void smoothswing_task(void* pvParameters) {
    auto* sample = static_cast<Espressif::App::SmoothSwingSample*>(pvParameters);
    sample->run();
    vTaskDelete(nullptr);
}

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "Saber PoC — electrical stabilization (1s)...");
    vTaskDelay(pdMS_TO_TICKS(1000));

    // ── SD Card ─────────────────────────────────────────────────────────
    Espressif::Wrappers::SdCard::Config sd_cfg = {
        .miso = GPIO_NUM_4,
        .mosi = GPIO_NUM_11,
        .sck  = GPIO_NUM_7,
        .cs   = GPIO_NUM_10,
        .mount_point = "/sdcard",
        .max_files = 5,
        .format_if_mount_failed = false
    };

    // ── Audio Engine (MAX98357A) ────────────────────────────────────────
    Espressif::Wrappers::Audio::AudioEngine::Config audio_cfg = {
        .bclk_pin     = GPIO_NUM_18,
        .ws_pin       = GPIO_NUM_19,
        .dout_pin     = GPIO_NUM_20,
        .sd_mode_pin  = GPIO_NUM_1,
        .sample_rate  = 44100,
        .max_channels = 9
    };

    // ── SmartLed Blade ──────────────────────────────────────────────────
    static Espressif::Wrappers::SmartLed::Engine led_engine(LED_STRIP_GPIO, NUM_BLADE_LEDS);
    ESP_ERROR_CHECK(led_engine.init());
    led_engine.setGlobalBrightness(200);

    static auto blade_effect = std::make_unique<Espressif::App::BladeIgnite>(
        Espressif::Wrappers::SmartLed::Color{255, 200, 0});

    auto* blade_ptr = blade_effect.get();
    led_engine.setBaseEffect(std::move(blade_effect));
    led_engine.start();

    // ── SmoothSwing ─────────────────────────────────────────────────────
    static Espressif::App::SmoothSwingSample ss_sample(
        sd_cfg, audio_cfg, GPIO_NUM_22, GPIO_NUM_23, GPIO_NUM_21);

    esp_err_t ss_ret = ss_sample.setup();
    if (ss_ret != ESP_OK) {
        ESP_LOGE(TAG, "SmoothSwing setup failed: %s", esp_err_to_name(ss_ret));
    }

    // ── Controller ──────────────────────────────────────────────────────
    static SaberContext ctx = {
        .blade = blade_ptr,
        .swing = &ss_sample
    };

    xTaskCreate(saber_controller_task, "saber_ctrl", 8192, &ctx, 5, &s_controller_task);

    if (ss_ret == ESP_OK) {
        xTaskCreate(smoothswing_task, "ss_task", 8192, &ss_sample, 5, nullptr);
    } else {
        ESP_LOGW(TAG, "SmoothSwing task skipped due to setup failure.");
    }

    // ── BOOT Button ─────────────────────────────────────────────────────
    static Espressif::Wrappers::GpioButton boot_btn(BOOT_BUTTON_GPIO, true);

    boot_btn.onEvent(Espressif::Wrappers::ButtonEvent::Click, []() {
        if (s_controller_task) {
            xTaskNotifyGive(s_controller_task);
        }
    });

    if (boot_btn.init() != ESP_OK) {
        ESP_LOGE(TAG, "BOOT button init failed!");
    }

    ESP_LOGI(TAG, "Ready. Press BOOT to toggle saber.");
}
