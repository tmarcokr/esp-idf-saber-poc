#include "BladeDrag.hpp"
#include <cmath>
#include <algorithm>

namespace Espressif::App {

BladeDrag::BladeDrag(const std::atomic<bool>* active_flag)
    : m_active_flag(active_flag) {}

void BladeDrag::update(uint32_t delta_ms) {
    m_elapsed += delta_ms;

    if (!m_fading_out && !m_active_flag->load()) {
        m_fading_out = true;
    }

    if (m_fading_out) {
        m_fade_elapsed += delta_ms;
        if (m_fade_elapsed >= FADE_DURATION_MS) {
            m_finished = true;
        }
    }
}

void BladeDrag::render(Espressif::Wrappers::SmartLed::Canvas& canvas) {
    if (m_finished) return;

    float fade_factor = 1.0f;
    if (m_fading_out) {
        fade_factor = 1.0f - (static_cast<float>(m_fade_elapsed) / static_cast<float>(FADE_DURATION_MS));
        fade_factor = std::max(0.0f, fade_factor);
    }

    uint16_t num_pixels = canvas.size();
    if (num_pixels == 0) return;

    for (uint16_t i = 0; i < num_pixels; ++i) {
        // Create an oscillating wave moving across LEDs using bit of math
        float wave = std::sin(static_cast<float>(m_elapsed) / 100.0f + static_cast<float>(i) * 0.5f);
        
        // Colors: mix White, Yellow, Red
        // wave from -1 to 1.
        uint8_t r = 255;
        uint8_t g = static_cast<uint8_t>(std::clamp((wave + 1.0f) * 127.0f, 0.0f, 255.0f));
        uint8_t b = static_cast<uint8_t>(std::clamp(wave * 127.0f, 0.0f, 255.0f));

        Espressif::Wrappers::SmartLed::Color color{r, g, b};
        uint8_t alpha = static_cast<uint8_t>(255.0f * fade_factor);
        canvas.blendPixel(i, color, alpha);
    }
}

bool BladeDrag::isFinished() const {
    return m_finished;
}

} // namespace Espressif::App
