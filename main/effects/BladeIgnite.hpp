#pragma once

#include "Canvas.hpp"
#include "IEffect.hpp"
#include "SmartLedTypes.hpp"
#include <atomic>
#include <cstdint>

namespace Espressif::App {

namespace Led = Espressif::Wrappers::SmartLed;

/**
 * @brief Lightsaber blade ignition/retraction effect for WS2812B strips.
 *
 * States: Idle → Igniting → Steady → Retracting → Idle.
 * Steady-state renders a brightness gradient (base brighter than tip)
 * with a triangular pulse on both blade endpoints.
 */
class BladeIgnite : public Led::IEffect {
public:
    enum class State : uint8_t { Idle, Igniting, Steady, Retracting };

    explicit BladeIgnite(Led::Color blade_color);

    void ignite();
    void retract();
    [[nodiscard]] State state() const;

    void update(uint32_t delta_ms) override;
    void render(Led::Canvas& canvas) override;
    [[nodiscard]] bool isFinished() const override;

private:
    Led::Color _blade_color;
    std::atomic<State> _state{State::Idle};
    uint32_t _elapsed_ms{0};
    uint32_t _steady_elapsed_ms{0};

    static constexpr uint32_t IGNITE_DURATION_MS  = 600;
    static constexpr uint32_t RETRACT_DURATION_MS = 600;
    static constexpr uint32_t TIP_PULSE_PERIOD_MS = 1500;
    static constexpr uint8_t  TOP_DIM_FACTOR      = 100;
    static constexpr uint8_t  PULSE_MIN           = 153;
    static constexpr uint8_t  PULSE_MAX           = 255;

    [[nodiscard]] Led::Color scaleColor(Led::Color c, uint8_t factor) const;
    [[nodiscard]] Led::Color gradientAt(uint16_t index, uint16_t total) const;
    [[nodiscard]] uint8_t    tipPulseFactor() const;
};

} // namespace Espressif::App
