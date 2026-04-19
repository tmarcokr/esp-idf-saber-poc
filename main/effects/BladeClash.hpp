#pragma once

#include "Canvas.hpp"
#include "IEffect.hpp"
#include "SmartLedTypes.hpp"
#include <cstdint>

namespace Espressif::App {

/**
 * @brief Overlay effect for lightsaber collision (Hit/Clash).
 * 
 * Flashes the center LEDs bright white/yellowish and quickly fades out.
 */
class BladeClash : public Espressif::Wrappers::SmartLed::IEffect {
public:
    /**
     * @brief Construct a Clash effect with a fixed duration.
     * @param duration_ms Duration of the flash effect in milliseconds.
     */
    explicit BladeClash(uint32_t duration_ms = 300);

    void update(uint32_t delta_ms) override;
    void render(Espressif::Wrappers::SmartLed::Canvas& canvas) override;
    [[nodiscard]] bool isFinished() const override;

private:
    uint32_t m_duration_ms;
    uint32_t m_elapsed{0};
    bool m_finished{false};
};

} // namespace Espressif::App
