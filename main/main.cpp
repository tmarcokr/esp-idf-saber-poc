#include "BladeIgnite.hpp"
#include "GpioButton.hpp"
#include "RgbLed.hpp"
#include "SaberController.hpp"
#include "SmartLed.hpp"
#include "SmoothSwingSample.hpp"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <memory>

static constexpr const char* TAG = "SaberPoC";

static constexpr gpio_num_t BOOT_BUTTON_GPIO = GPIO_NUM_9;
static constexpr gpio_num_t LED_STRIP_GPIO   = GPIO_NUM_0;
static constexpr gpio_num_t INTERNAL_LED_GPIO = GPIO_NUM_8;
static constexpr uint16_t   NUM_BLADE_LEDS   = 5;

extern "C" void app_main(void) {
    // ── Internal Status LED ─────────────────────────────────────────────
    static Espressif::Wrappers::RgbLed status_led(INTERNAL_LED_GPIO);
    if (status_led.init() != ESP_OK) {
        ESP_LOGE(TAG, "Internal LED init failed!");
    }

    ESP_LOGI(TAG, "Saber PoC — electrical stabilization (1s)...");
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    // Signal ready with Green color
    ESP_ERROR_CHECK(status_led.setColor({0, 255, 0})); // Green

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
        sd_cfg, audio_cfg, led_engine, GPIO_NUM_22, GPIO_NUM_23, GPIO_NUM_21);

    if (ss_sample.setup() != ESP_OK) {
        ESP_LOGE(TAG, "SmoothSwing setup failed!");
    }

    // ── Controller ──────────────────────────────────────────────────────
    static Espressif::App::SaberController controller(
        *blade_ptr, ss_sample, ss_sample.getAudioEngine(), led_engine);
    ESP_ERROR_CHECK(controller.begin());

    // ── BOOT Button ─────────────────────────────────────────────────────
    static Espressif::Wrappers::GpioButton boot_btn(BOOT_BUTTON_GPIO, true);

    boot_btn.onEvent(Espressif::Wrappers::ButtonEvent::Click, []() {
        controller.trigger();
    });

    boot_btn.onLongPress(3000, []() {
        controller.requestRetract();
    });

    boot_btn.onLongPress(500, []() {
        controller.requestLongPressSmall();
    });

    boot_btn.onEvent(Espressif::Wrappers::ButtonEvent::PressUp, []() {
        controller.releaseButton();
    });

    if (boot_btn.init() != ESP_OK) {
        ESP_LOGE(TAG, "BOOT button init failed!");
    }

    ESP_LOGI(TAG, "Ready. Press BOOT to toggle saber.");
}
