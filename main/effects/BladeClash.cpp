#include "BladeClash.hpp"
#include <algorithm>
#include <cmath>

namespace Espressif::App {

BladeClash::BladeClash(uint32_t duration_ms)
    : m_duration_ms(duration_ms) {}

void BladeClash::update(uint32_t delta_ms) {
    m_elapsed += delta_ms;
    if (m_elapsed >= m_duration_ms) {
        m_finished = true;
    }
}

void BladeClash::render(Espressif::Wrappers::SmartLed::Canvas& canvas) {
    if (m_finished) return;

    // Calculate progress and remaining factor
    float progress = static_cast<float>(m_elapsed) / m_duration_ms;
    float remaining_factor = 1.0f - progress;
    remaining_factor = std::max(0.0f, remaining_factor);

    uint16_t num_pixels = canvas.size();
    if (num_pixels == 0) return;

    int center_index = num_pixels / 2;
    
    // Core Clash Color: Bright White/Yellow
    Espressif::Wrappers::SmartLed::Color core_color{255, 255, 200};

    for (uint16_t i = 0; i < num_pixels; ++i) {
        int dist = std::abs(static_cast<int>(i) - center_index);
        
        // Intensity drops off from center. At dist=0 -> 1.0, dist=1 -> 0.4, dist=2 -> 0.25
        float dist_factor = 1.0f / (1.0f + static_cast<float>(dist) * 1.5f);
        
        float final_intensity = remaining_factor * dist_factor;
        uint8_t alpha = static_cast<uint8_t>(255.0f * std::min(1.0f, final_intensity));

        if (alpha > 0) {
            canvas.blendPixel(i, core_color, alpha);
        }
    }
}

bool BladeClash::isFinished() const {
    return m_finished;
}

} // namespace Espressif::App
