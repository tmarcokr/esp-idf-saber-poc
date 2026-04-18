#include "BladeIgnite.hpp"
#include <algorithm>

namespace Espressif::App {

using namespace Espressif::Wrappers::SmartLed;

BladeIgnite::BladeIgnite(Color blade_color)
    : _blade_color(blade_color) {}

void BladeIgnite::ignite() {
    _elapsed_ms = 0;
    _steady_elapsed_ms = 0;
    _state.store(State::Igniting);
}

void BladeIgnite::retract() {
    _elapsed_ms = 0;
    _state.store(State::Retracting);
}

BladeIgnite::State BladeIgnite::state() const {
    return _state.load();
}

void BladeIgnite::update(uint32_t delta_ms) {
    State current = _state.load();

    switch (current) {
        case State::Igniting:
            _elapsed_ms += delta_ms;
            if (_elapsed_ms >= IGNITE_DURATION_MS) {
                _elapsed_ms = 0;
                _steady_elapsed_ms = 0;
                _state.store(State::Steady);
            }
            break;

        case State::Steady:
            _steady_elapsed_ms += delta_ms;
            break;

        case State::Retracting:
            _elapsed_ms += delta_ms;
            if (_elapsed_ms >= RETRACT_DURATION_MS) {
                _elapsed_ms = 0;
                _state.store(State::Idle);
            }
            break;

        case State::Idle:
            break;
    }
}

void BladeIgnite::render(Canvas& canvas) {
    State current = _state.load();
    uint16_t n = canvas.size();
    canvas.clear();

    switch (current) {
        case State::Idle:
            break;

        case State::Igniting: {
            uint32_t progress_x256 = std::min(uint32_t(256),
                _elapsed_ms * 256 / IGNITE_DURATION_MS);
            uint32_t lit_x256 = progress_x256 * n;
            uint16_t full_lit = static_cast<uint16_t>(lit_x256 / 256);
            uint8_t frontier_alpha = static_cast<uint8_t>(lit_x256 & 0xFF);

            for (uint16_t i = 0; i < full_lit && i < n; ++i) {
                canvas.setPixel(i, gradientAt(i, n));
            }

            if (full_lit < n && frontier_alpha > 0) {
                canvas.setPixel(full_lit,
                    scaleColor(gradientAt(full_lit, n), frontier_alpha));
            }
            break;
        }

        case State::Steady: {
            for (uint16_t i = 0; i < n; ++i) {
                canvas.setPixel(i, gradientAt(i, n));
            }

            uint8_t pulse = tipPulseFactor();
            canvas.setPixel(0, scaleColor(gradientAt(0, n), pulse));
            canvas.setPixel(n - 1, scaleColor(gradientAt(n - 1, n), pulse));
            break;
        }

        case State::Retracting: {
            uint32_t progress_x256 = std::min(uint32_t(256),
                _elapsed_ms * 256 / RETRACT_DURATION_MS);
            // Turn off from the top (pixel N-1) down to the base (pixel 0)
            uint32_t off_x256 = progress_x256 * n;
            uint16_t full_off = static_cast<uint16_t>(off_x256 / 256);
            uint8_t frontier_fade = static_cast<uint8_t>(off_x256 & 0xFF);

            uint16_t remaining = (full_off < n) ? (n - full_off) : 0;

            for (uint16_t i = 0; i < remaining; ++i) {
                Color grad = gradientAt(i, n);
                // Fade the frontier pixel (last remaining, about to turn off)
                if (i == remaining - 1 && frontier_fade > 0) {
                    canvas.setPixel(i, scaleColor(grad,
                        static_cast<uint8_t>(255 - frontier_fade)));
                } else {
                    canvas.setPixel(i, grad);
                }
            }
            break;
        }
    }
}

bool BladeIgnite::isFinished() const {
    return false;
}

Color BladeIgnite::scaleColor(Color c, uint8_t factor) const {
    return {
        static_cast<uint8_t>(uint16_t(c.r) * factor / 255),
        static_cast<uint8_t>(uint16_t(c.g) * factor / 255),
        static_cast<uint8_t>(uint16_t(c.b) * factor / 255)
    };
}

Color BladeIgnite::gradientAt(uint16_t index, uint16_t total) const {
    if (total <= 1) return _blade_color;
    // Pixel 0 = full brightness, pixel N-1 = TOP_DIM_FACTOR/255 brightness
    uint8_t brightness = static_cast<uint8_t>(
        255 - (uint16_t(255 - TOP_DIM_FACTOR) * index / (total - 1)));
    return scaleColor(_blade_color, brightness);
}

uint8_t BladeIgnite::tipPulseFactor() const {
    // Triangular wave between PULSE_MIN and PULSE_MAX
    uint32_t phase = _steady_elapsed_ms % TIP_PULSE_PERIOD_MS;
    uint32_t half = TIP_PULSE_PERIOD_MS / 2;
    uint8_t range = PULSE_MAX - PULSE_MIN;

    if (phase < half) {
        return PULSE_MIN + static_cast<uint8_t>(uint32_t(range) * phase / half);
    }
    return PULSE_MAX - static_cast<uint8_t>(uint32_t(range) * (phase - half) / half);
}

} // namespace Espressif::App
