#pragma once

#include <gw/core/mdspan.hpp>
#include <gw/math/fast_lerp.hpp>
#include <gw/math/hermite_interpolation.hpp>

namespace gw {

struct BufferInterpolation
{
    struct None
    {
        template<etl::linalg::in_vector Vec, typename SampleType = typename Vec::value_type>
        [[nodiscard]] constexpr auto operator()(Vec buffer, etl::size_t readPos, SampleType fracPos) -> SampleType
        {
            etl::ignore_unused(fracPos);
            return buffer(readPos % buffer.size());
        }
    };

    struct Linear
    {
        template<etl::linalg::in_vector Vec, typename SampleType = typename Vec::value_type>
        [[nodiscard]] constexpr auto operator()(Vec buffer, etl::size_t readPos, SampleType fracPos) -> SampleType
        {
            auto const x0 = buffer(readPos % buffer.size());
            auto const x1 = buffer((readPos + 1) % buffer.size());
            return fast_lerp(x0, x1, fracPos);
        }
    };

    struct Hermite
    {
        template<etl::linalg::in_vector Vec, typename SampleType = typename Vec::value_type>
        [[nodiscard]] constexpr auto operator()(Vec buffer, etl::size_t readPos, SampleType fracPos) -> SampleType
        {
            auto const pos = readPos + buffer.size();
            auto const xm1 = buffer((pos - 1) % buffer.size());
            auto const x0  = buffer(pos % buffer.size());
            auto const x1  = buffer((pos + 1) % buffer.size());
            auto const x2  = buffer((pos + 2) % buffer.size());
            return hermite_interpolation(xm1, x0, x1, x2, fracPos);
        }
    };
};

}  // namespace gw
