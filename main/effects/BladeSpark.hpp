#pragma once

#include "Canvas.hpp"
#include "IEffect.hpp"
#include "SmartLedTypes.hpp"
#include <cstdint>
#include <vector>

namespace Espressif::App {

/**
 * @brief Overlay effect that adds random sparkles to the blade.
 * 
 * Used to react to movement intensity.
 */
class BladeSpark : public Espressif::Wrappers::SmartLed::IEffect {
public:
    /**
     * @param intensity 0-255, probability and brightness of sparks.
     * @param duration_ms How long the spark burst lasts.
     */
    explicit BladeSpark(uint8_t intensity, uint32_t duration_ms = 150);

    void update(uint32_t delta_ms) override;
    void render(Espressif::Wrappers::SmartLed::Canvas& canvas) override;
    [[nodiscard]] bool isFinished() const override;

private:
    uint8_t m_intensity;
    uint32_t m_duration_ms;
    uint32_t m_elapsed{0};
    bool m_finished{false};
};

} // namespace Espressif::App
