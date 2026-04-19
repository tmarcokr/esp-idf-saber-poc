#pragma once

#include "Canvas.hpp"
#include "IEffect.hpp"
#include "SmartLedTypes.hpp"
#include <atomic>
#include <cstdint>

namespace Espressif::App {

/**
 * @brief Overlay effect for lightsaber floor drag.
 */
class BladeDrag : public Espressif::Wrappers::SmartLed::IEffect {
public:
    explicit BladeDrag(const std::atomic<bool>* active_flag);

    void update(uint32_t delta_ms) override;
    void render(Espressif::Wrappers::SmartLed::Canvas& canvas) override;
    [[nodiscard]] bool isFinished() const override;

private:
    const std::atomic<bool>* m_active_flag;
    bool m_finished{false};
    bool m_fading_out{false};
    uint32_t m_elapsed{0};
    uint32_t m_fade_elapsed{0};
    static constexpr uint32_t FADE_DURATION_MS = 300;
};

} // namespace Espressif::App
