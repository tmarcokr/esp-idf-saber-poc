#include "SmoothSwingSample.hpp"
#include "BladeSpark.hpp"
#include "BladeClash.hpp"
#include "esp_log.h"
#include "esp_timer.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace Espressif::App {

// Volume curve: lower sensitivity = full volume reached at lower speeds.
// Sharpness: 1.0 = linear, 1.75 = aggressive curve (requires high speed for
// loud). For protoboard: lowered from 450 to 250 and sharpness from 1.75
// to 1.2.
static constexpr float SWING_SENSITIVITY =
    250.0f; // deg/s for max volume (was 450)
static constexpr float SWING_SHARPNESS =
    1.2f; // Power curve exponent (was 1.75)
static constexpr float MAX_HUM_DUCKING =
    75.0f; // Percentage hum reduction at max swing
static constexpr float SWING_THRESHOLD =
    15.0f; // Minimum deg/s to trigger swing (was 20)
static constexpr float TRANSITION_WIDTH_L =
    45.0f; // Low transition zone width (degrees)
static constexpr float TRANSITION_WIDTH_H =
    160.0f; // High transition zone width (degrees)

// Accent: raised threshold and cooldown for protoboard (easy to trigger with
// wrist flicks).
static constexpr float ACCENT_THRESHOLD =
    400.0f; // deg/s to fire accent (was 250)
static constexpr uint32_t ACCENT_COOLDOWN_MS =
    500; // ms between accents (was 200)

// Linear Motion: detect lateral displacement (thrusts/stabs).
// Weight: how many "virtual degrees per second" every 1g of acceleration is
// worth.
static constexpr float LIN_ACCEL_WEIGHT = 400.0f; // 0.5g ≈ 200 virtual DPS

// Clash detection
static constexpr float CLASH_ACCEL_THRESHOLD = 2.0f; // g's
static constexpr uint32_t CLASH_COOLDOWN_MS = 600; // ms

// Volume levels: hum lowered so swings can be heard clearly.
static constexpr uint16_t MAX_VOL_14BIT = 16384;
static constexpr uint16_t BASE_HUM_VOL = 8000; // Was 11000, then 5500, now 8000
static constexpr uint16_t MAX_SWING_VOL_MTRX = 16384;

SmoothSwingSample::SmoothSwingSample(
    const Wrappers::SdCard::Config &sd_config,
    const Wrappers::Audio::AudioEngine::Config &audio_config,
    Wrappers::SmartLed::Engine &led_engine,
    gpio_num_t mpu_sda, gpio_num_t mpu_scl, gpio_num_t mpu_int)
    : m_sd_config(sd_config), m_audio_config(audio_config), m_mpu_sda(mpu_sda),
      m_mpu_scl(mpu_scl), m_mpu_int(mpu_int), m_led_engine(led_engine) {}

void IRAM_ATTR SmoothSwingSample::isrHandler(void *arg) {
  auto *sem = static_cast<QueueHandle_t>(arg);
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  xSemaphoreGiveFromISR(sem, &xHigherPriorityTaskWoken);
  if (xHigherPriorityTaskWoken == pdTRUE) {
    portYIELD_FROM_ISR();
  }
}

esp_err_t SmoothSwingSample::configureInterruptGpio(gpio_num_t pin) {
  gpio_config_t io_conf = {
      .pin_bit_mask = (1ULL << pin),
      .mode = GPIO_MODE_INPUT,
      .pull_up_en = GPIO_PULLUP_DISABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_POSEDGE, // Rising edge (latched INT)
  };
  esp_err_t ret = gpio_config(&io_conf);
  if (ret != ESP_OK)
    return ret;
  ret = gpio_install_isr_service(0);
  // Ignore ESP_ERR_INVALID_STATE if already installed by another driver
  if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE)
    return ret;
  return gpio_isr_handler_add(pin, isrHandler, m_semaphore.get());
}

esp_err_t SmoothSwingSample::setup() {
  m_sd = std::make_unique<Wrappers::SdCard>(m_sd_config);
  esp_err_t ret = m_sd->init();
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "SD Card init failed: %s", esp_err_to_name(ret));
    return ret;
  }
  ESP_LOGI(TAG, "SD Card initialized.");

  m_engine = std::make_unique<Wrappers::Audio::AudioEngine>(m_audio_config);
  ret = m_engine->init();
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "AudioEngine init failed: %s", esp_err_to_name(ret));
    return ret;
  }
  ret = m_engine->start();
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "AudioEngine start failed: %s", esp_err_to_name(ret));
    return ret;
  }
  ESP_LOGI(TAG, "Audio Engine started.");

  m_mpu = std::make_unique<Wrappers::Sensors::Mpu6050>(m_mpu_sda, m_mpu_scl,
                                                       m_mpu_int);
  ret = m_mpu->initialize();
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "MPU-6050 init failed! %s", esp_err_to_name(ret));
    return ret;
  }

  // Create Semaphore for HW Interrupts
  m_semaphore = makeSSBinarySemaphore();
  if (!m_semaphore)
    return ESP_ERR_NO_MEM;

  ret = configureInterruptGpio(m_mpu_int);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to configure INT gpio");
    return ret;
  }
  ESP_LOGI(TAG, "MPU-6050 initialized successfully.");

  return ESP_OK;
}

void SmoothSwingSample::run() {
  ESP_LOGI(TAG, "=== SmoothSwing Motion Loop Started ===");

  uint64_t last_time_us = esp_timer_get_time();

  while (true) {
    if (xSemaphoreTake(m_semaphore.get(), portMAX_DELAY) == pdTRUE) {
      if (!m_active.load()) {
        // Keep timestamp fresh to avoid a burst delta on re-activation
        last_time_us = esp_timer_get_time();
        continue;
      }

      if (auto data = m_mpu->readData()) {
        uint64_t now_us = esp_timer_get_time();
        float delta_time_s =
            static_cast<float>(now_us - last_time_us) / 1000000.0f;
        last_time_us = now_us;

        float gyro_x_dps = data->gyro_x * 2000.0f / 32768.0f;
        float gyro_y_dps = data->gyro_y * 2000.0f / 32768.0f;
        float gyro_z_dps = data->gyro_z * 2000.0f / 32768.0f;

        float swing_speed_dps =
            std::sqrt(gyro_x_dps * gyro_x_dps + gyro_y_dps * gyro_y_dps +
                      gyro_z_dps * gyro_z_dps);

        float lin_accel_magnitude_g = 0.0f;
        auto lin_accel = data->getLinearAcceleration();
        lin_accel_magnitude_g =
            std::sqrt(lin_accel.x * lin_accel.x + lin_accel.y * lin_accel.y +
                      lin_accel.z * lin_accel.z);

        processMotion(swing_speed_dps, lin_accel_magnitude_g, delta_time_s);
      }
    }
  }
}

void SmoothSwingSample::startAudio() {
  m_virtual_position = 0.0f;
  m_last_accent_time_ms = 0;

  m_engine->play("/sdcard/saber/poweron.wav", false, MAX_VOL_14BIT);

  m_ch_hum = m_engine->play("/sdcard/saber/hum.wav", true, BASE_HUM_VOL);
  m_ch_swingl = m_engine->play("/sdcard/saber/swingL.wav", true, 0);
  m_ch_swingh = m_engine->play("/sdcard/saber/swingH.wav", true, 0);

  m_active.store(true);
  ESP_LOGI(TAG, "Audio started — saber ignited.");
}

void SmoothSwingSample::stopAudio() {
  m_active.store(false);

  if (m_ch_hum != Wrappers::Audio::INVALID_CHANNEL) {
    m_engine->stop(m_ch_hum);
    m_ch_hum = Wrappers::Audio::INVALID_CHANNEL;
  }
  if (m_ch_swingl != Wrappers::Audio::INVALID_CHANNEL) {
    m_engine->stop(m_ch_swingl);
    m_ch_swingl = Wrappers::Audio::INVALID_CHANNEL;
  }
  if (m_ch_swingh != Wrappers::Audio::INVALID_CHANNEL) {
    m_engine->stop(m_ch_swingh);
    m_ch_swingh = Wrappers::Audio::INVALID_CHANNEL;
  }

  m_engine->play("/sdcard/saber/poweroff.wav", false, MAX_VOL_14BIT);
  ESP_LOGI(TAG, "Audio stopped — saber retracted.");
}

void SmoothSwingSample::setActive(bool active) { m_active.store(active); }

bool SmoothSwingSample::isActive() const { return m_active.load(); }

void SmoothSwingSample::processMotion(float swing_speed_dps, float lin_accel_g,
                                      float delta_time_s) {
  if (m_ch_swingl == Wrappers::Audio::INVALID_CHANNEL ||
      m_ch_swingh == Wrappers::Audio::INVALID_CHANNEL) {
    return; // Need active swing channels to do smoothswing
  }

  uint32_t current_time_ms = esp_timer_get_time() / 1000;

  // 0. Clash Check (Linear Acceleration)
  if (lin_accel_g > CLASH_ACCEL_THRESHOLD) {
    if ((current_time_ms - m_last_clash_time_ms) > CLASH_COOLDOWN_MS) {
      ESP_LOGI(TAG, "Clash Triggered! Accel: %.2fg", lin_accel_g);
      m_engine->play("/sdcard/saber/clash1.wav", false, MAX_VOL_14BIT);
      m_led_engine.pushOverlay(std::make_unique<BladeClash>(300));
      m_last_clash_time_ms = current_time_ms;
    }
  }

  // 1. Accent Swings Check
  if (swing_speed_dps > ACCENT_THRESHOLD) {
    if ((current_time_ms - m_last_accent_time_ms) > ACCENT_COOLDOWN_MS) {
      ESP_LOGI(TAG, "Accent Swing Triggered! Speed: %.2f dps", swing_speed_dps);
      m_engine->play("/sdcard/saber/swng01.wav", false, MAX_VOL_14BIT);
      m_last_accent_time_ms = current_time_ms;
    }
  }

  // 2. Smooth gate: instead of a hard on/off cut, we let the math curve
  //    naturally produce near-zero swing volume at low speeds.
  //    Below threshold, swing_volume_factor will be extremely small (~0.01)
  //    so swings fade out organically and hum restores smoothly.

  // 3. Normalized Swing Strength (0.0 to 1.0)
  //    We combine rotational speed with linear acceleration to ensure
  //    lateral movements (stabs) contribute to the audio intensity.
  float combined_speed = swing_speed_dps + (lin_accel_g * LIN_ACCEL_WEIGHT);

  // Apply a dead zone below threshold to avoid mic-noise triggering.
  float effective_speed = std::max(0.0f, combined_speed - SWING_THRESHOLD);
  float base_strength = effective_speed / (SWING_SENSITIVITY - SWING_THRESHOLD);
  base_strength = std::min(1.0f, base_strength);

  // 4. Apply Power Curve (Sharpness) -> determines ultimate volume
  float swing_volume_factor = std::pow(base_strength, SWING_SHARPNESS);

  // 4b. Trigger Reactive LED Sparkles
  if (swing_volume_factor > 0.4f) {
      if ((current_time_ms - m_last_spark_time_ms) > 100) {
          uint8_t spark_intensity = static_cast<uint8_t>(swing_volume_factor * 255);
          m_led_engine.pushOverlay(std::make_unique<BladeSpark>(spark_intensity, 150));
          m_last_spark_time_ms = current_time_ms;
      }
  }

  // 5. Update phase position
  m_virtual_position += (swing_speed_dps * delta_time_s);
  while (m_virtual_position > 360.0f)
    m_virtual_position -= 360.0f;
  while (m_virtual_position < 0.0f)
    m_virtual_position += 360.0f;

  // 6. Calculate Crossfade between L and H based on Position
  // Proportional mixing regions for cross-fading.
  // Low zone: 0->180, High zone: 180->360
  float mix_ab = 0.0f;
  if (m_virtual_position >= 0.0f && m_virtual_position < 180.0f) {
    // We are crossing the Low domain
    // Compute fade via transition width
    if (m_virtual_position < TRANSITION_WIDTH_L) {
      mix_ab = m_virtual_position /
               TRANSITION_WIDTH_L; // 0.0 to 1.0 going to High (if we consider
                                   // A=High, B=Low)
    } else if (m_virtual_position > (180.0f - TRANSITION_WIDTH_L)) {
      mix_ab = (180.0f - m_virtual_position) / TRANSITION_WIDTH_L; // 1.0 to 0.0
    } else {
      mix_ab = 1.0f;
    }
  } else {
    // We are crossing High domain (180-360)
    // For sample simplicity, map mix_ab to 0.0 for High domain unless in
    // transition.
    mix_ab = 0.0f;
    float pos_h = m_virtual_position - 180.0f;
    if (pos_h < TRANSITION_WIDTH_H) {
      mix_ab = 1.0f - (pos_h / TRANSITION_WIDTH_H); // 1.0 to 0.0
    } else if (pos_h > (180.0f - TRANSITION_WIDTH_H)) {
      mix_ab = (pos_h - (180.0f - TRANSITION_WIDTH_H)) /
               TRANSITION_WIDTH_H; // 0.0 to 1.0
    }
  }

  // mix_ab represents balance: 1.0 = Low purely, 0.0 = High purely
  mix_ab = std::max(0.0f, std::min(1.0f, mix_ab));

  // 7. Calculate final channel volumes
  uint16_t total_swing_vol =
      static_cast<uint16_t>(swing_volume_factor * MAX_SWING_VOL_MTRX);
  uint16_t vol_l = static_cast<uint16_t>(total_swing_vol * mix_ab);
  uint16_t vol_h = static_cast<uint16_t>(total_swing_vol * (1.0f - mix_ab));

  m_engine->setChannelVolume(m_ch_swingl, vol_l);
  m_engine->setChannelVolume(m_ch_swingh, vol_h);

  // 8. Hum Ducking logic
  uint16_t hum_vol = BASE_HUM_VOL;
  if (m_ch_hum != Wrappers::Audio::INVALID_CHANNEL) {
    float ducking_amount = (swing_volume_factor * (MAX_HUM_DUCKING / 100.0f));
    float hum_ratio = std::max(0.0f, 1.0f - ducking_amount);
    hum_vol = static_cast<uint16_t>(BASE_HUM_VOL * hum_ratio);
    m_engine->setChannelVolume(m_ch_hum, hum_vol);
  }

  // 9. Telemetry (every ~0.5s at 100Hz = every 50 frames)
  static uint32_t log_counter = 0;
  if (++log_counter >= 50) {
    ESP_LOGI(
        TAG,
        "Spd:%.0f | Lin:%.2f | Str:%.2f | SwL:%u SwH:%u | Hum:%u | Pos:%.0f",
        swing_speed_dps, lin_accel_g, swing_volume_factor, vol_l, vol_h,
        hum_vol, m_virtual_position);
    log_counter = 0;
  }
}

} // namespace Espressif::App
