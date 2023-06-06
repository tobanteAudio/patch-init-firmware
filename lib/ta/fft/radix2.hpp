#pragma once

#include <etl/array.hpp>
#include <etl/cmath.hpp>
#include <etl/complex.hpp>
#include <etl/numbers.hpp>
#include <etl/span.hpp>
#include <etl/utility.hpp>

#include <cmath>

namespace etl
{
template<typename T>
inline auto polar(T const& rho, T const& theta) -> complex<T>
{
    return complex<T>(rho * std::cos(theta), rho * std::sin(theta));
}
}  // namespace etl

namespace ta::fft
{

template<typename T>
auto power(T base, T exponent) -> T
{
    T result = 1;
    for (T i = 0; i < exponent; i++) { result *= base; }
    return result;
}

template<auto Base>
auto power(decltype(Base) exponent) -> decltype(Base)
{
    return power(Base, exponent);
}

template<typename Float, unsigned Size>
auto make_twiddle_factors(bool inverse = false) -> etl::array<etl::complex<Float>, Size / 2>
{
    auto const sign = inverse ? Float(1) : Float(-1);
    auto table      = etl::array<etl::complex<Float>, Size / 2>{};
    for (unsigned i = 0; i < Size / 2; ++i)
    {
        auto const angle = sign * Float(2) * static_cast<Float>(etl::numbers::pi) * Float(i) / Float(Size);
        table[i]         = etl::polar(Float(1), angle);
    }
    return table;
}

template<typename Float, etl::size_t Extent>
auto radix2_inplace(etl::span<etl::complex<Float>, Extent> x, etl::span<etl::complex<Float> const, Extent / 2> w)
    -> void
{
    // bit-reverse ordering
    auto const len   = x.size();
    auto const order = static_cast<int>(std::lround(std::log2(len)));

    // // Rearrange the input in bit-reversed order
    // unsigned j = 0;
    // for (unsigned i = 0; i < len - 1U; ++i)
    // {
    //     if (i < j) { etl::swap(x[i], x[j]); }
    //     unsigned k = len / 2;
    //     while (k <= j)
    //     {
    //         j -= k;
    //         k /= 2;
    //     }
    //     j += k;
    // }

    // butterfly computation
    for (auto stage = 0; stage < order; ++stage)
    {

        auto const stage_length = power<2>(stage);
        auto const stride       = power<2>(stage + 1);
        auto const tw_stride    = power<2>(order - stage - 1);

        for (auto k = 0; etl::cmp_less(k, len); k += stride)
        {
            for (auto pair = 0; pair < stage_length; ++pair)
            {
                auto const tw = w[pair * tw_stride];

                auto const i1 = k + pair;
                auto const i2 = k + pair + stage_length;

                auto const temp = x[i1] + tw * x[i2];
                x[i2]           = x[i1] - tw * x[i2];
                x[i1]           = temp;
            }
        }
    }
}

}  // namespace ta::fft
