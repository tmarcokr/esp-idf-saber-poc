#include "BlasterImpact.hpp"
#include <algorithm>

namespace Espressif::App {

BlasterImpact::BlasterImpact(uint32_t duration_ms)
    : m_duration_ms(duration_ms) {}

void BlasterImpact::update(uint32_t delta_ms) {
    m_elapsed += delta_ms;
    if (m_elapsed >= m_duration_ms) {
        m_finished = true;
    }
}

void BlasterImpact::render(Espressif::Wrappers::SmartLed::Canvas& canvas) {
    if (m_finished) return;

    // Calculate alpha (fade from 255 to 0)
    float progress = static_cast<float>(m_elapsed) / m_duration_ms;
    uint8_t alpha = static_cast<uint8_t>(255 * (1.0f - progress));

    // Fill entire blade with Red at varying alpha
    // Red color: {255, 0, 0}
    for (uint16_t i = 0; i < canvas.size(); ++i) {
        canvas.blendPixel(i, Espressif::Wrappers::SmartLed::Color{255, 0, 0}, alpha);
    }
}

bool BlasterImpact::isFinished() const {
    return m_finished;
}

} // namespace Espressif::App
