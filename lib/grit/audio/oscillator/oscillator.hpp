#pragma once

#include <grit/math/range.hpp>

#include <etl/algorithm.hpp>
#include <etl/concepts.hpp>
#include <etl/numbers.hpp>

namespace grit {

enum struct oscillator_shape
{
    Sine,
    Triangle,
    Square,
};

template<etl::floating_point Float>
struct oscillator
{
    oscillator() = default;

    auto set_shape(oscillator_shape shape) -> void;
    auto set_phase(Float phase) -> void;
    auto set_frequency(Float frequency) -> void;
    auto set_sample_rate(Float sample_rate) -> void;

    auto add_phase_offset(Float offset) -> void;

    [[nodiscard]] auto operator()() -> Float;

private:
    [[nodiscard]] static auto sine(Float phase) -> Float;
    [[nodiscard]] static auto triangle(Float phase) -> Float;
    [[nodiscard]] static auto pulse(Float phase, Float width) -> Float;

    oscillator_shape _shape{oscillator_shape::Sine};
    Float _sample_rate{0};
    Float _phase{0};
    Float _phase_increment{0};
    Float _pulse_width{0.5};
};

template<etl::floating_point Float>
auto oscillator<Float>::set_shape(oscillator_shape shape) -> void
{
    _shape = shape;
}

template<etl::floating_point Float>
auto oscillator<Float>::set_phase(Float phase) -> void
{
    _phase = phase;
}

template<etl::floating_point Float>
auto oscillator<Float>::set_frequency(Float frequency) -> void
{
    _phase_increment = 1.0F / (_sample_rate / frequency);
}

template<etl::floating_point Float>
auto oscillator<Float>::set_sample_rate(Float sample_rate) -> void
{
    _sample_rate = sample_rate;
}

template<etl::floating_point Float>
auto oscillator<Float>::add_phase_offset(Float offset) -> void
{
    _phase += offset;
    _phase -= etl::floor(_phase);
}

template<etl::floating_point Float>
auto oscillator<Float>::operator()() -> Float
{
    auto output = Float{};
    switch (_shape) {
        case oscillator_shape::Sine: {
            output = sine(_phase);
            break;
        }
        case oscillator_shape::Triangle: {
            output = triangle(_phase);
            break;
        }
        case oscillator_shape::Square: {
            output = pulse(_phase, _pulse_width);
            break;
        }
        default: {
            break;
        }
    }

    add_phase_offset(_phase_increment);
    return output;
}

template<etl::floating_point Float>
auto oscillator<Float>::sine(Float phase) -> Float
{
    static constexpr auto two_pi = static_cast<Float>(etl::numbers::pi) * Float{2};
    return etl::sin(phase * two_pi);
}

template<etl::floating_point Float>
auto oscillator<Float>::triangle(Float phase) -> Float
{
    auto const x = phase <= Float{0.5} ? phase : Float{1} - phase;
    return (x - Float{0.25}) * Float{4};
}

template<etl::floating_point Float>
auto oscillator<Float>::pulse(Float phase, Float width) -> Float
{
    auto const w = etl::clamp(width, Float{0}, Float{1});
    if (phase < w) {
        return Float{-1};
    }
    return Float{1};
}

}  // namespace grit
