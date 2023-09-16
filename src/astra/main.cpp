#include <gw/audio/dither/no_dither.hpp>
#include <gw/audio/dither/rectangle_dither.hpp>
#include <gw/audio/dither/triangle_dither.hpp>
#include <gw/core/benchmark.hpp>
#include <gw/fft/fft.hpp>

#include <etl/algorithm.hpp>
#include <etl/array.hpp>
#include <etl/bit.hpp>
#include <etl/chrono.hpp>
#include <etl/functional.hpp>
#include <etl/linalg.hpp>
#include <etl/numeric.hpp>
#include <etl/random.hpp>

#include <daisy_patch_sm.h>

namespace astra {

auto mcu = daisy::patch_sm::DaisyPatchSM{};

}  // namespace astra

namespace etl {
template<int N, typename Benchmark>
auto timeit(char const* name, Benchmark bench)
{
    using microseconds_t = etl::chrono::duration<float, etl::micro>;

    auto runs = etl::array<float, N>{};

    bench();
    bench();
    bench();

    for (auto i{0U}; i < N; ++i) {
        auto const start = astra::mcu.system.GetUs();
        bench();
        auto const stop = astra::mcu.system.GetUs();

        runs[i] = etl::chrono::duration_cast<microseconds_t>(etl::chrono::microseconds{stop - start}).count();
    }

    auto const average = int(etl::reduce(runs.begin(), end(runs), 0.0F) / static_cast<float>(runs.size()));
    auto const dsize   = double(bench.size());
    auto const mflops  = static_cast<int>(std::lround(5.0 * dsize * std::log2(dsize) / average)) * 2;

    astra::mcu.PrintLine(
        "%30s Runs: %4d - Average: %4d us - Min: %4d us - Max: %4d us - MFLOPS: %4d us\n",
        name,
        N,
        average,
        int(*etl::min_element(runs.begin(), runs.end())),
        int(*etl::max_element(runs.begin(), runs.end())),
        mflops
    );
}
}  // namespace etl

template<typename Complex, unsigned N>
[[nodiscard]] auto make_noise(auto& rng) -> etl::array<Complex, N>
{
    using Float = Complex::value_type;
    auto buf    = etl::array<Complex, N>{};
    auto dist   = etl::uniform_real_distribution<Float>{Float(-0.5), Float(0.5)};
    auto gen    = [&rng, &dist] { return Complex{dist(rng), dist(rng)}; };
    etl::generate(buf.begin(), buf.end(), gen);
    return buf;
};

template<typename Float, int N, typename Kernel>
struct c2c_roundtrip
{
    c2c_roundtrip() = default;

    static constexpr auto size() noexcept { return N; }

    auto operator()() -> void
    {
        auto x      = etl::mdspan<etl::complex<Float>, etl::extents<etl::size_t, N>>{_buf.data()};
        auto w      = etl::mdspan<etl::complex<Float> const, etl::extents<etl::size_t, N / 2>>{_tw.data()};
        auto kernel = Kernel{};

        kernel(x, w);
        kernel(x, etl::linalg::conjugated(w));
        etl::linalg::scale(Float(1) / Float(N), x);

        gw::doNotOptimize(_buf.front());
        gw::doNotOptimize(_buf.back());
    }

private:
    etl::array<etl::complex<Float>, N / 2> _tw{gw::fft::make_twiddles_r2<Float, N>()};
    etl::array<etl::complex<Float>, N> _buf{[] {
        auto rng = etl::xoshiro128plusplus{42};
        return make_noise<etl::complex<Float>, N>(rng);
    }()};
};

template<typename Float, int N>
struct static_c2c_roundtrip
{
    static_c2c_roundtrip() = default;

    static constexpr auto size() noexcept { return N; }

    auto operator()() -> void
    {
        auto x = etl::mdspan<etl::complex<Float>, etl::extents<etl::size_t, N>>{_buf.data()};
        _plan(x, gw::fft::direction::forward);
        _plan(x, gw::fft::direction::backward);
        etl::linalg::scale(Float(1) / Float(N), x);

        gw::doNotOptimize(_buf.front());
        gw::doNotOptimize(_buf.back());
    }

private:
    gw::fft::static_fft_plan<etl::complex<Float>, N> _plan{};
    etl::array<etl::complex<Float>, N> _buf{[] {
        auto rng = etl::xoshiro128plusplus{42};
        return make_noise<etl::complex<Float>, N>(rng);
    }()};
};

auto main() -> int
{
    using namespace astra;

    astra::mcu.Init();

    astra::mcu.StartLog(true);
    astra::mcu.PrintLine("Daisy Patch SM started. Test Beginning");

    // etl::timeit<64>("c2c_roundtrip<float, 16, v1>      - ", c2c_roundtrip<float, 16, gw::fft::c2c_dit2_v1>{});
    // etl::timeit<64>("c2c_roundtrip<float, 32, v1>      - ", c2c_roundtrip<float, 32, gw::fft::c2c_dit2_v1>{});
    // etl::timeit<64>("c2c_roundtrip<float, 64, v1>      - ", c2c_roundtrip<float, 64, gw::fft::c2c_dit2_v1>{});
    // etl::timeit<64>("c2c_roundtrip<float, 128, v1>     - ", c2c_roundtrip<float, 128, gw::fft::c2c_dit2_v1>{});
    // etl::timeit<64>("c2c_roundtrip<float, 256, v1>     - ", c2c_roundtrip<float, 256, gw::fft::c2c_dit2_v1>{});
    // etl::timeit<64>("c2c_roundtrip<float, 512, v1>     - ", c2c_roundtrip<float, 512, gw::fft::c2c_dit2_v1>{});
    // etl::timeit<64>("c2c_roundtrip<float, 1024, v1>    - ", c2c_roundtrip<float, 1024, gw::fft::c2c_dit2_v1>{});
    // etl::timeit<64>("c2c_roundtrip<float, 2048, v1>    - ", c2c_roundtrip<float, 2048, gw::fft::c2c_dit2_v1>{});
    // etl::timeit<64>("c2c_roundtrip<float, 4096, v1>    - ", c2c_roundtrip<float, 4096, gw::fft::c2c_dit2_v1>{});
    // astra::mcu.PrintLine("");

    // etl::timeit<64>("c2c_roundtrip<float, 16, v2>      - ", c2c_roundtrip<float, 16, gw::fft::c2c_dit2_v2>{});
    // etl::timeit<64>("c2c_roundtrip<float, 32, v2>      - ", c2c_roundtrip<float, 32, gw::fft::c2c_dit2_v2>{});
    // etl::timeit<64>("c2c_roundtrip<float, 64, v2>      - ", c2c_roundtrip<float, 64, gw::fft::c2c_dit2_v2>{});
    // etl::timeit<64>("c2c_roundtrip<float, 128, v2>     - ", c2c_roundtrip<float, 128, gw::fft::c2c_dit2_v2>{});
    // etl::timeit<64>("c2c_roundtrip<float, 256, v2>     - ", c2c_roundtrip<float, 256, gw::fft::c2c_dit2_v2>{});
    // etl::timeit<64>("c2c_roundtrip<float, 512, v2>     - ", c2c_roundtrip<float, 512, gw::fft::c2c_dit2_v2>{});
    // etl::timeit<64>("c2c_roundtrip<float, 1024, v2>    - ", c2c_roundtrip<float, 1024, gw::fft::c2c_dit2_v2>{});
    // etl::timeit<64>("c2c_roundtrip<float, 2048, v2>    - ", c2c_roundtrip<float, 2048, gw::fft::c2c_dit2_v2>{});
    // etl::timeit<64>("c2c_roundtrip<float, 4096, v2>    - ", c2c_roundtrip<float, 4096, gw::fft::c2c_dit2_v2>{});
    // astra::mcu.PrintLine("");

    // etl::timeit<64>("c2c_roundtrip<float, 16, v3>      - ", c2c_roundtrip<float, 16, gw::fft::c2c_dit2_v3>{});
    // etl::timeit<64>("c2c_roundtrip<float, 32, v3>      - ", c2c_roundtrip<float, 32, gw::fft::c2c_dit2_v3>{});
    // etl::timeit<64>("c2c_roundtrip<float, 64, v3>      - ", c2c_roundtrip<float, 64, gw::fft::c2c_dit2_v3>{});
    // etl::timeit<64>("c2c_roundtrip<float, 128, v3>     - ", c2c_roundtrip<float, 128, gw::fft::c2c_dit2_v3>{});
    // etl::timeit<64>("c2c_roundtrip<float, 256, v3>     - ", c2c_roundtrip<float, 256, gw::fft::c2c_dit2_v3>{});
    // etl::timeit<64>("c2c_roundtrip<float, 512, v3>     - ", c2c_roundtrip<float, 512, gw::fft::c2c_dit2_v3>{});
    // etl::timeit<64>("c2c_roundtrip<float, 1024, v3>    - ", c2c_roundtrip<float, 1024, gw::fft::c2c_dit2_v3>{});
    etl::timeit<64>("c2c_roundtrip<float, 2048, v3>    - ", c2c_roundtrip<float, 2048, gw::fft::c2c_dit2_v3>{});
    etl::timeit<64>("c2c_roundtrip<float, 4096, v3>    - ", c2c_roundtrip<float, 4096, gw::fft::c2c_dit2_v3>{});
    astra::mcu.PrintLine("");

    etl::timeit<64>("static_c2c_roundtrip<float, 2048> - ", static_c2c_roundtrip<float, 2048>{});
    etl::timeit<64>("static_c2c_roundtrip<float, 4096> - ", static_c2c_roundtrip<float, 4096>{});
    astra::mcu.PrintLine("");

    while (true) {}
}
