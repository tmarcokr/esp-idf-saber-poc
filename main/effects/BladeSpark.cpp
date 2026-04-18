#include "BladeSpark.hpp"
#include <esp_random.h>
#include <algorithm>

namespace Espressif::App {

BladeSpark::BladeSpark(uint8_t intensity, uint32_t duration_ms)
    : m_intensity(intensity), m_duration_ms(duration_ms) {}

void BladeSpark::update(uint32_t delta_ms) {
    m_elapsed += delta_ms;
    if (m_elapsed >= m_duration_ms) {
        m_finished = true;
    }
}

void BladeSpark::render(Espressif::Wrappers::SmartLed::Canvas& canvas) {
    if (m_finished) return;

    // Calculate fade out based on remaining time
    float progress = static_cast<float>(m_elapsed) / m_duration_ms;
    uint8_t current_alpha = static_cast<uint8_t>(m_intensity * (1.0f - progress));

    for (uint16_t i = 0; i < canvas.size(); ++i) {
        // Randomly decide to spark this pixel
        if ((esp_random() % 255) < (m_intensity / 2)) {
            // Blend white spark
            canvas.blendPixel(i, Espressif::Wrappers::SmartLed::Color::White(), current_alpha);
        }
    }
}

bool BladeSpark::isFinished() const {
    return m_finished;
}

} // namespace Espressif::App
