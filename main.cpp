#include "arm_neon_processor.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <string>
#include <vector>

struct BenchRow {
    size_t size;
    double scalar_ms;
    double neon_ms;
    double unrolled_ms;
};

static std::string timestamp_utc() {
    using namespace std::chrono;
    const auto now = system_clock::now();
    const auto t = system_clock::to_time_t(now);
    std::tm tm{};
#if defined(_WIN32)
    gmtime_s(&tm, &t);
#else
    gmtime_r(&t, &tm);
#endif
    char buf[64];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d_%H-%M-%S_UTC", &tm);
    return std::string(buf);
}

template <typename Fn>
double measure_ms(Fn&& fn, int warmup, int iters) {
    for (int i = 0; i < warmup; ++i) {
        fn();
    }
    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < iters; ++i) {
        fn();
    }
    auto end = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(end - start).count() / iters;
}

bool test_correctness() {
    std::vector<size_t> sizes{0, 1, 2, 3, 4, 5, 7, 8, 15, 16, 100, 1000, 10000, 100000};
    for (size_t n : sizes) {
        alignas(16) std::vector<int32_t> data(n);
        for (size_t i = 0; i < n; ++i) {
            data[i] = static_cast<int32_t>(i % 127) - 63;
        }

        const auto scalar = process_array_scalar(data.data(), n);
        const auto neon = process_array_neon(data.data(), n);
        const auto unrolled = process_array_neon_unrolled(data.data(), n);

        if (scalar != neon || scalar != unrolled) {
            std::cerr << "Mismatch @ n=" << n << " scalar=" << scalar << " neon=" << neon
                      << " unrolled=" << unrolled << '\n';
            return false;
        }
    }
    return true;
}

int main() {
    if (!test_correctness()) {
        return 1;
    }

    std::vector<size_t> sizes{1000, 10000, 100000, 1000000, 5000000};
    std::vector<BenchRow> rows;
    rows.reserve(sizes.size());

    std::mt19937 rng(42);
    std::uniform_int_distribution<int32_t> dist(-1000, 1000);

    std::cout << "Size,Scalar(ms),NEON(ms),NEONx2(ms),Speedup\n";

    for (size_t n : sizes) {
        alignas(16) std::vector<int32_t> data(n);
        for (size_t i = 0; i < n; ++i) {
            data[i] = dist(rng);
        }

        volatile int64_t sink = 0;
        int iters = std::max(20, static_cast<int>(50000000 / std::max<size_t>(n, 1)));

        double scalar_ms = measure_ms([&]() { sink = process_array_scalar(data.data(), n); }, 5, iters);
        double neon_ms = measure_ms([&]() { sink = process_array_neon(data.data(), n); }, 5, iters);
        double unrolled_ms = measure_ms([&]() { sink = process_array_neon_unrolled(data.data(), n); }, 5, iters);
        (void)sink;

        rows.push_back({n, scalar_ms, neon_ms, unrolled_ms});
        std::cout << n << ',' << std::fixed << std::setprecision(6) << scalar_ms << ',' << neon_ms << ','
                  << unrolled_ms << ',' << (scalar_ms / neon_ms) << "\n";
    }

    const std::string base = "benchmark_" + timestamp_utc();
    std::ofstream csv(base + ".csv");
    csv << "size,scalar_ms,neon_ms,neon_unrolled_ms,speedup_vs_neon,speedup_vs_unrolled\n";
    for (const auto& r : rows) {
        csv << r.size << ',' << r.scalar_ms << ',' << r.neon_ms << ',' << r.unrolled_ms << ','
            << (r.scalar_ms / r.neon_ms) << ',' << (r.scalar_ms / r.unrolled_ms) << "\n";
    }

    std::ofstream md(base + ".md");
    md << "# ARM NEON benchmark report\n\n";
    md << "| N | Scalar ms | NEON ms | NEON x2 ms | Speedup (scalar/NEON) |\n";
    md << "|---:|---:|---:|---:|---:|\n";
    for (const auto& r : rows) {
        md << "| " << r.size << " | " << r.scalar_ms << " | " << r.neon_ms << " | " << r.unrolled_ms
           << " | " << (r.scalar_ms / r.neon_ms) << " |\n";
    }
    md << "\nCSV can be opened directly in Excel / LibreOffice.\n";

    std::cout << "Saved: " << base << ".csv and " << base << ".md\n";
    return 0;
}
