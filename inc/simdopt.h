#pragma once
#include <nosdef.h>
#include <intrin.h>
#include <immintrin.h>
#include <zmmintrin.h>


static __forceinline void _Simd256BroadcastQ_epi64(__m128i* Val, __m256i* out);
static __forceinline void _Simd256Srlv_epi64(__m256i* a, __m256i* count);
static __forceinline void _Simd256_and_pd(void* a, void* b);
// Optimizing defs

#ifdef __EX_AVX2__
static __forceinline void _Simd256BroadcastQ_epi64(UINT64 Val, __m256i* out) {
    *out = _mm256_broadcastq_epi64(*(__m128i*)&Val);
}
static __forceinline void _Simd256Srlv_epi64(__m256i* a, __m256i* count) {
    *a = _mm256_srlv_epi64(*a, *count);
}

static __forceinline void _Simd256_and_pd(void* a, void* b) {
    *(__m256d*)a = _mm256_and_pd(*(__m256d*)a, *(__m256d*)b);
}

#else



static __forceinline void _Simd256BroadcastQ_epi64(__m128i* Val, __m256i* out) {
    *((__m128i*)out) = _mm_broadcastq_epi64(*Val);
    *((__m128i*)out + 1) = _mm_broadcastq_epi64(*Val);
}
static __forceinline void _Simd256Srlv_epi64(__m256i* a, __m256i* count) {
    *((__m128i*)a) = _mm_srl_epi64(*(__m128i*)a, *(__m128i*)count);
    *((__m128i*)a + 1) = _mm_srl_epi64(*((__m128i*)a + 1), *((__m128i*)count + 1));
}

static __forceinline void _Simd256_and_pd(void* a, void* b) {
    *((__m128d*)a) = _mm_and_pd(*(__m128d*)a, *(__m128d*)b);
    *((__m128d*)a + 1) = _mm_and_pd(*((__m128d*)a + 1), *((__m128d*)b + 1));
}
#endif