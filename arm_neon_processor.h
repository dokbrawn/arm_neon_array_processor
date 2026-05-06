#ifndef ARM_NEON_PROCESSOR_H
#define ARM_NEON_PROCESSOR_H

#include <cstddef>
#include <cstdint>
#include <stdexcept>

#ifdef __ARM_NEON
#include <arm_neon.h>
#endif

inline int64_t process_array_scalar(const int32_t* data, size_t n) {
    if (data == nullptr && n > 0) {
        throw std::invalid_argument("Null pointer with non-zero size");
    }

    int64_t sum = 0;
    for (size_t i = 0; i < n; ++i) {
        int32_t val = data[i];
        if (val > 0) {
            sum += val;
        } else if (val < 0) {
            sum -= static_cast<int64_t>(val);
        }
    }
    return sum;
}

#ifdef __ARM_NEON
inline int64_t horizontal_add_s32x4_to_s64(int32x4_t v) {
#if defined(__aarch64__)
    return static_cast<int64_t>(vaddvq_s32(v));
#else
    int64x2_t pair_sum = vpaddlq_s32(v);
    int64x1_t total = vadd_s64(vget_low_s64(pair_sum), vget_high_s64(pair_sum));
    return vget_lane_s64(total, 0);
#endif
}

inline int64_t process_array_neon(const int32_t* data, size_t n) {
    if (data == nullptr && n > 0) {
        throw std::invalid_argument("Null pointer with non-zero size");
    }
    if (n == 0) return 0;

    int32x4_t acc = vdupq_n_s32(0);
    const int32x4_t zero = vdupq_n_s32(0);

    size_t i = 0;
    const size_t simd_limit = n & ~static_cast<size_t>(3);
    for (; i < simd_limit; i += 4) {
        int32x4_t vec = vld1q_s32(data + i);

        uint32x4_t mask_pos = vcgtq_s32(vec, zero);
        uint32x4_t mask_neg = vcltq_s32(vec, zero);

        int32x4_t sign = vshrq_n_s32(vec, 31);
        int32x4_t abs_val = veorq_s32(vec, sign);
        abs_val = vsubq_s32(abs_val, sign);

        int32x4_t pos_part = vandq_s32(vec, vreinterpretq_s32_u32(mask_pos));
        int32x4_t neg_part = vandq_s32(abs_val, vreinterpretq_s32_u32(mask_neg));
        int32x4_t contrib = vorrq_s32(pos_part, neg_part);

        acc = vaddq_s32(acc, contrib);
    }

    int64_t sum = horizontal_add_s32x4_to_s64(acc);
    for (; i < n; ++i) {
        int32_t val = data[i];
        if (val > 0) sum += val;
        else if (val < 0) sum -= static_cast<int64_t>(val);
    }
    return sum;
}

inline int64_t process_array_neon_unrolled(const int32_t* data, size_t n) {
    if (data == nullptr && n > 0) {
        throw std::invalid_argument("Null pointer with non-zero size");
    }
    if (n == 0) return 0;

    int32x4_t acc1 = vdupq_n_s32(0);
    int32x4_t acc2 = vdupq_n_s32(0);
    const int32x4_t zero = vdupq_n_s32(0);

    size_t i = 0;
    const size_t unroll_limit = n & ~static_cast<size_t>(7);
    for (; i < unroll_limit; i += 8) {
        __builtin_prefetch(data + i + 16, 0, 3);

        int32x4_t vec1 = vld1q_s32(data + i);
        int32x4_t vec2 = vld1q_s32(data + i + 4);

        uint32x4_t mask_pos1 = vcgtq_s32(vec1, zero);
        uint32x4_t mask_neg1 = vcltq_s32(vec1, zero);
        uint32x4_t mask_pos2 = vcgtq_s32(vec2, zero);
        uint32x4_t mask_neg2 = vcltq_s32(vec2, zero);

        int32x4_t sign1 = vshrq_n_s32(vec1, 31);
        int32x4_t abs_val1 = vsubq_s32(veorq_s32(vec1, sign1), sign1);

        int32x4_t sign2 = vshrq_n_s32(vec2, 31);
        int32x4_t abs_val2 = vsubq_s32(veorq_s32(vec2, sign2), sign2);

        int32x4_t contrib1 = vorrq_s32(
            vandq_s32(vec1, vreinterpretq_s32_u32(mask_pos1)),
            vandq_s32(abs_val1, vreinterpretq_s32_u32(mask_neg1)));

        int32x4_t contrib2 = vorrq_s32(
            vandq_s32(vec2, vreinterpretq_s32_u32(mask_pos2)),
            vandq_s32(abs_val2, vreinterpretq_s32_u32(mask_neg2)));

        acc1 = vaddq_s32(acc1, contrib1);
        acc2 = vaddq_s32(acc2, contrib2);
    }

    int64_t sum = horizontal_add_s32x4_to_s64(vaddq_s32(acc1, acc2));
    for (; i < n; ++i) {
        int32_t val = data[i];
        if (val > 0) sum += val;
        else if (val < 0) sum -= static_cast<int64_t>(val);
    }
    return sum;
}
#else
inline int64_t process_array_neon(const int32_t* data, size_t n) {
    return process_array_scalar(data, n);
}

inline int64_t process_array_neon_unrolled(const int32_t* data, size_t n) {
    return process_array_scalar(data, n);
}
#endif

#endif
