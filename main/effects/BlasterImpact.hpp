#pragma once

#include "Canvas.hpp"
#include "IEffect.hpp"
#include "SmartLedTypes.hpp"
#include <cstdint>

namespace Espressif::App {

/**
 * @brief Overlay effect for blaster bolt deflection.
 * 
 * Briefly flashes the blade Red and fades out.
 */
class BlasterImpact : public Espressif::Wrappers::SmartLed::IEffect {
public:
    explicit BlasterImpact(uint32_t duration_ms = 400);

    void update(uint32_t delta_ms) override;
    void render(Espressif::Wrappers::SmartLed::Canvas& canvas) override;
    [[nodiscard]] bool isFinished() const override;

private:
    uint32_t m_duration_ms;
    uint32_t m_elapsed{0};
    bool m_finished{false};
};

} // namespace Espressif::App
