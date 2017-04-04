/*
 * libpopcnt.h - C/C++ library for counting the number of 1 bits (bit
 * population count) in an array as quickly as possible using
 * specialized CPU instructions e.g. POPCNT, AVX2.
 *
 * Copyright (c) 2016 - 2017, Kim Walisch
 * Copyright (c) 2016 - 2017, Wojciech Muła
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef LIBPOPCNT_H
#define LIBPOPCNT_H

#include <stdint.h>

#ifndef __has_builtin
  #define __has_builtin(x) 0
#endif

#ifndef __has_attribute
  #define __has_attribute(x) 0
#endif

#ifdef __GNUC__
  #define GNUC_PREREQ(x, y) \
      (__GNUC__ > x || (__GNUC__ == x && __GNUC_MINOR__ >= y))
#else
  #define GNUC_PREREQ(x, y)	0
#endif

#ifdef __clang__
  #define CLANG_PREREQ(x, y) \
      (__clang_major__ > x || (__clang_major__ == x && __clang_minor__ >= y))
#else
  #define CLANG_PREREQ(x, y)	0
#endif

#if (defined(__i386__) || \
     defined(__x86_64__) || \
     defined(_M_IX86) || \
     defined(_M_X64))
  #define X86_OR_X64
#endif

#if defined(X86_OR_X64) && \
   (defined(__cplusplus) || \
   (GNUC_PREREQ(4, 2) || \
    __has_builtin(__sync_val_compare_and_swap)))
  #define HAVE_CPUID
#endif

#if GNUC_PREREQ(4, 2) || \
    __has_builtin(__builtin_popcount)
  #define HAVE_BUILTIN_POPCOUNT
#endif

#if GNUC_PREREQ(4, 2) || \
    CLANG_PREREQ(3, 0)
  #define HAVE_ASM_POPCNT
#endif

#if defined(HAVE_CPUID) && \
   (defined(HAVE_ASM_POPCNT) || \
    defined(_MSC_VER))
  #define HAVE_POPCNT
#endif

#if defined(HAVE_CPUID) && \
    GNUC_PREREQ(4, 9)
  #define HAVE_AVX2
#endif

#if defined(HAVE_CPUID) && \
    CLANG_PREREQ(3, 8) && \
    __has_attribute(target) && \
   (!defined(_MSC_VER) || defined(__AVX2__)) && \
   (!defined(__apple_build_version__) || __apple_build_version__ >= 8000000)
  #define HAVE_AVX2
#endif

/*
 * This uses fewer arithmetic operations than any other known
 * implementation on machines with fast multiplication.
 * It uses 12 arithmetic operations, one of which is a multiply.
 * http://en.wikipedia.org/wiki/Hamming_weight#Efficient_implementation
 */
static inline uint64_t popcount64(uint64_t x)
{
  uint64_t m1 = 0x5555555555555555ll;
  uint64_t m2 = 0x3333333333333333ll;
  uint64_t m4 = 0x0F0F0F0F0F0F0F0Fll;
  uint64_t h01 = 0x0101010101010101ll;

  x -= (x >> 1) & m1;
  x = (x & m2) + ((x >> 2) & m2);
  x = (x + (x >> 4)) & m4;

  return (x * h01) >> 56;
}

#if defined(HAVE_ASM_POPCNT) && \
    defined(__x86_64__)

static inline uint64_t popcnt64(uint64_t x)
{
  __asm__ ("popcnt %1, %0" : "=r" (x) : "0" (x));
  return x;
}

#elif defined(HAVE_ASM_POPCNT) && \
      defined(__i386__)

static inline uint32_t popcnt32(uint32_t x)
{
  __asm__ ("popcnt %1, %0" : "=r" (x) : "0" (x));
  return x;
}

static inline uint64_t popcnt64(uint64_t x)
{
  return popcnt32((uint32_t) x) +
         popcnt32((uint32_t)(x >> 32));
}

#elif defined(_MSC_VER) && \
      defined(_M_X64)

#include <nmmintrin.h>

static inline uint64_t popcnt64(uint64_t x)
{
  return _mm_popcnt_u64(x);
}

#elif defined(_MSC_VER) && \
      defined(_M_IX86)

#include <nmmintrin.h>

static inline uint64_t popcnt64(uint64_t x)
{
  return _mm_popcnt_u32((uint32_t) x) + 
         _mm_popcnt_u32((uint32_t)(x >> 32));
}

/* non x86 CPUs */
#elif defined(HAVE_BUILTIN_POPCOUNT)

static inline uint64_t popcnt64(uint64_t x)
{
  return __builtin_popcountll(x);
}

/* no hardware POPCNT,
 * use pure integer algorithm */
#else

static inline uint64_t popcnt64(uint64_t x)
{
  return popcount64(x);
}

#endif

static inline uint64_t popcnt64_unrolled(const uint64_t* data, uint64_t size)
{
  uint64_t i = 0;
  uint64_t limit = size - size % 4;
  uint64_t cnt = 0;

  for (; i < limit; i += 4)
  {
    cnt += popcnt64(data[i+0]);
    cnt += popcnt64(data[i+1]);
    cnt += popcnt64(data[i+2]);
    cnt += popcnt64(data[i+3]);
  }

  for (; i < size; i++)
    cnt += popcnt64(data[i]);

  return cnt;
}

/*
 * Carry-save adder (CSA).
 * @see Chapter 5 in "Hacker's Delight".
 */
static inline void CSA(uint64_t* h, uint64_t* l, uint64_t a, uint64_t b, uint64_t c)
{
  uint64_t u = a ^ b; 
  *h = (a & b) | (u & c);
  *l = u ^ c;
}

/*
 * Harley-Seal popcount (3rd iteration).
 * The Harley-Seal popcount algorithm is one of the fastest algorithms
 * for counting 1 bits in an array using only integer operations.
 * This implementation uses only 6.38 instructions per 64-bit word.
 * @see Chapter 5 in "Hacker's Delight" 2nd edition.
 */
static inline uint64_t popcnt64_hs(const uint64_t* data, uint64_t size)
{
  uint64_t cnt = 0;
  uint64_t ones = 0, twos = 0, fours = 0, eights = 0;
  uint64_t twosA, twosB, foursA, foursB;
  uint64_t limit = size - size % 8;
  uint64_t i = 0;

  for(; i < limit; i += 8)
  {
    CSA(&twosA, &ones, ones, data[i+0], data[i+1]);
    CSA(&twosB, &ones, ones, data[i+2], data[i+3]);
    CSA(&foursA, &twos, twos, twosA, twosB);
    CSA(&twosA, &ones, ones, data[i+4], data[i+5]);
    CSA(&twosB, &ones, ones, data[i+6], data[i+7]);
    CSA(&foursB, &twos, twos, twosA, twosB);
    CSA(&eights, &fours, fours, foursA, foursB);

    cnt += popcount64(eights);
  }

  cnt *= 8;
  cnt += 4 * popcount64(fours);
  cnt += 2 * popcount64(twos);
  cnt += 1 * popcount64(ones);

  for(; i < size; i++)
    cnt += popcount64(data[i]);

  return cnt;
}

#if defined(HAVE_CPUID)

#if defined(_MSC_VER)
  #include <intrin.h>
  #include <immintrin.h>
#endif

/* %ecx bit flags */
#define bit_POPCNT (1 << 23)

/* %ebx bit flags */
#define bit_AVX2 (1 << 5)

static inline void run_cpuid(int eax, int ecx, int* abcd)
{
  int ebx = 0;
  int edx = 0;

#if defined(_MSC_VER)
  __cpuidex(abcd, eax, ecx);
#elif defined(__i386__) && \
      defined(__PIC__)
  /* in case of PIC under 32-bit EBX cannot be clobbered */
  __asm__ ("movl %%ebx, %%edi;"
           "cpuid;"
           "xchgl %%ebx, %%edi;"
           : "=D" (ebx),
             "+a" (eax),
             "+c" (ecx),
             "=d" (edx));
#else
  __asm__ ("cpuid;"
           : "+b" (ebx),
             "+a" (eax),
             "+c" (ecx),
             "=d" (edx));
#endif
  abcd[0] = eax;
  abcd[1] = ebx;
  abcd[2] = ecx;
  abcd[3] = edx;
}

static inline int has_POPCNT()
{
  int abcd[4];

  run_cpuid(1, 0, abcd);
  if ((abcd[2] & bit_POPCNT) != bit_POPCNT)
    return 0;

  return bit_POPCNT;
}

static inline int check_xcr0_ymm()
{
  int xcr0;
#if defined(_MSC_VER)
  xcr0 = (int) _xgetbv(0);
#else
  __asm__ ("xgetbv" : "=a" (xcr0) : "c" (0) : "%edx" );
#endif
  return (xcr0 & 6) == 6;
}

static inline int has_AVX2()
{
  int abcd[4];
  int osxsave_mask = (1 << 27);

  /* ensure OS supports extended processor state management */
  run_cpuid(1, 0, abcd);
  if ((abcd[2] & osxsave_mask) != osxsave_mask)
    return 0;

  /* ensure OS supports ZMM registers (and YMM, and XMM) */
  if (!check_xcr0_ymm())
    return 0;

  run_cpuid(7, 0, abcd);
  if ((abcd[1] & bit_AVX2) != bit_AVX2)
    return 0;

  return bit_AVX2;
}

#endif /* cpuid */

#if defined(HAVE_AVX2)

#include <immintrin.h>

__attribute__ ((target ("avx2")))
static inline void CSA256(__m256i* h, __m256i* l, __m256i a, __m256i b, __m256i c)
{
  __m256i u = a ^ b;
  *h = (a & b) | (u & c);
  *l = u ^ c;
}

__attribute__ ((target ("avx2")))
static inline __m256i popcnt256(__m256i v)
{
  __m256i lookup1 = _mm256_setr_epi8(
      4, 5, 5, 6, 5, 6, 6, 7,
      5, 6, 6, 7, 6, 7, 7, 8,
      4, 5, 5, 6, 5, 6, 6, 7,
      5, 6, 6, 7, 6, 7, 7, 8
  );

  __m256i lookup2 = _mm256_setr_epi8(
      4, 3, 3, 2, 3, 2, 2, 1,
      3, 2, 2, 1, 2, 1, 1, 0,
      4, 3, 3, 2, 3, 2, 2, 1,
      3, 2, 2, 1, 2, 1, 1, 0
  );

  __m256i low_mask = _mm256_set1_epi8(0x0f);
  __m256i lo = v & low_mask;
  __m256i hi = _mm256_srli_epi16(v, 4) & low_mask;
  __m256i popcnt1 = _mm256_shuffle_epi8(lookup1, lo);
  __m256i popcnt2 = _mm256_shuffle_epi8(lookup2, hi);

  return _mm256_sad_epu8(popcnt1, popcnt2);
}

/*
 * AVX2 Harley-Seal popcount (4th iteration).
 * The algorithm is based on the paper "Faster Population Counts
 * using AVX2 Instructions" by Daniel Lemire, Nathan Kurz and
 * Wojciech Mula (23 Nov 2016).
 * @see https://arxiv.org/abs/1611.07612
 */
__attribute__ ((target ("avx2")))
static inline uint64_t popcnt_avx2(const __m256i* data, uint64_t size)
{
  __m256i cnt = _mm256_setzero_si256();
  __m256i ones = _mm256_setzero_si256();
  __m256i twos = _mm256_setzero_si256();
  __m256i fours = _mm256_setzero_si256();
  __m256i eights = _mm256_setzero_si256();
  __m256i sixteens = _mm256_setzero_si256();
  __m256i twosA, twosB, foursA, foursB, eightsA, eightsB;

  uint64_t i = 0;
  uint64_t limit = size - size % 16;
  uint64_t* cnt64;

  for(; i < limit; i += 16)
  {
    CSA256(&twosA, &ones, ones, data[i+0], data[i+1]);
    CSA256(&twosB, &ones, ones, data[i+2], data[i+3]);
    CSA256(&foursA, &twos, twos, twosA, twosB);
    CSA256(&twosA, &ones, ones, data[i+4], data[i+5]);
    CSA256(&twosB, &ones, ones, data[i+6], data[i+7]);
    CSA256(&foursB, &twos, twos, twosA, twosB);
    CSA256(&eightsA, &fours, fours, foursA, foursB);
    CSA256(&twosA, &ones, ones, data[i+8], data[i+9]);
    CSA256(&twosB, &ones, ones, data[i+10], data[i+11]);
    CSA256(&foursA, &twos, twos, twosA, twosB);
    CSA256(&twosA, &ones, ones, data[i+12], data[i+13]);
    CSA256(&twosB, &ones, ones, data[i+14], data[i+15]);
    CSA256(&foursB, &twos, twos, twosA, twosB);
    CSA256(&eightsB, &fours, fours, foursA, foursB);
    CSA256(&sixteens, &eights, eights, eightsA, eightsB);

    cnt = _mm256_add_epi64(cnt, popcnt256(sixteens));
  }

  cnt = _mm256_slli_epi64(cnt, 4);
  cnt = _mm256_add_epi64(cnt, _mm256_slli_epi64(popcnt256(eights), 3));
  cnt = _mm256_add_epi64(cnt, _mm256_slli_epi64(popcnt256(fours), 2));
  cnt = _mm256_add_epi64(cnt, _mm256_slli_epi64(popcnt256(twos), 1));
  cnt = _mm256_add_epi64(cnt, popcnt256(ones));

  for(; i < size; i++)
    cnt = _mm256_add_epi64(cnt, popcnt256(data[i]));

  cnt64 = (uint64_t*) &cnt;

  return cnt64[0] +
         cnt64[1] +
         cnt64[2] +
         cnt64[3];
}

/* Align memory to 32 bytes boundary */
static inline void align_avx2(const uint8_t** p, uint64_t* size, uint64_t* cnt)
{
  for (; (uintptr_t) *p % 8; (*p)++)
  {
    *cnt += popcnt64(**p);
    *size -= 1;
  }
  for (; (uintptr_t) *p % 32; (*p) += 8)
  {
    *cnt += popcnt64(
        *(const uint64_t*) *p);
    *size -= 8;
  }
}

#endif /* avx2 */

/* x86 CPUs */
#if defined(X86_OR_X64)

/*
 * Count the number of 1 bits in the data array
 * @data: An array
 * @size: Size of data in bytes
 */
static inline uint64_t popcnt(const void* data, uint64_t size)
{
  uint64_t i;
  uint64_t cnt = 0;
  const uint8_t* buf = (const uint8_t*) data;

#if defined(HAVE_CPUID)
  #if defined(__cplusplus)
    /* C++11 thread-safe singleton */
    static const int cpuid =
        has_POPCNT() | has_AVX2();
  #else
    static int cpuid = -1;
    int cpuid2;
    if (cpuid == -1)
    {
      cpuid2 = has_POPCNT() | has_AVX2();
      __sync_val_compare_and_swap(&cpuid, -1, cpuid2);
    }
  #endif
#endif

#if defined(HAVE_AVX2)

  /* AVX2 requires arrays >= 512 bytes */
  if ((cpuid & bit_AVX2) &&
      size >= 512)
  {
    align_avx2(&buf, &size, &cnt);
    cnt += popcnt_avx2((const __m256i*) buf, size / 32);
    buf += size - size % 32;
    size = size % 32;
  }

#endif

#if defined(HAVE_POPCNT)

  if (cpuid & bit_POPCNT)
  {
    cnt += popcnt64_unrolled((const uint64_t*) buf, size / 8);
    buf += size - size % 8;
    size = size % 8;
    for (i = 0; i < size; i++)
      cnt += popcnt64(buf[i]);

    return cnt;
  }

#endif

  /* pure integer algorithm */
  cnt += popcnt64_hs((const uint64_t*) buf, size / 8);
  buf += size - size % 8;
  size = size % 8;
  for (i = 0; i < size; i++)
    cnt += popcount64(buf[i]);

  return cnt;
}

#elif defined(__ARM_NEON) || \
      defined(__aarch64__)

#include <arm_neon.h>

static inline uint64_t popcnt_neon(const uint8_t* ptr, uint64_t size)
{
  uint32_t tmp[4];
  uint64_t cnt = 0;
  uint64_t chunk_size = 128;
  uint64_t n = size / chunk_size;
  uint64_t k = size % chunk_size;
  uint64_t i;

  uint8x16_t t0;
  uint16x8_t t1;
  uint8x16x4_t input0;
  uint8x16x4_t input1;

  uint32x4_t sum = vcombine_u32(vcreate_u32(0), vcreate_u32(0));

  for (i = 0; i < n; i++, ptr += chunk_size)
  {
    input0 = vld4q_u8(ptr);
    input1 = vld4q_u8(ptr + 64);

    t0 = vcntq_u8(input0.val[0]);
    t0 = vaddq_u8(t0, vcntq_u8(input0.val[1]));
    t0 = vaddq_u8(t0, vcntq_u8(input0.val[2]));
    t0 = vaddq_u8(t0, vcntq_u8(input0.val[3]));
    t0 = vaddq_u8(t0, vcntq_u8(input1.val[0]));
    t0 = vaddq_u8(t0, vcntq_u8(input1.val[1]));
    t0 = vaddq_u8(t0, vcntq_u8(input1.val[2]));
    t0 = vaddq_u8(t0, vcntq_u8(input1.val[3]));
    t1 = vpaddlq_u8(t0);

    sum = vpadalq_u16(sum, t1);
  }

  vst1q_u32(tmp, sum);
  for (i = 0; i < 4; i++)
    cnt += tmp[i];

  for (i = 0; i < k - k % 8; i += 8)
    cnt += popcount64(*(const uint64_t*) &ptr[i]);

  for (; i < k; i++)
    cnt += popcount64(ptr[i]);

  return cnt;
}

/*
 * Count the number of 1 bits in the data array
 * @data: An array
 * @size: Size of data in bytes
 */
static inline uint64_t popcnt(const void* data, uint64_t size)
{
  const uint8_t* ptr = (const uint8_t*) data;
  uint64_t uint32_max = (1ull << 32) - 1;
  uint64_t cnt = 0;
  uint64_t bytes;

  for (; size > 0; size -= bytes)
  {
    bytes = (size < uint32_max) ? size : uint32_max;
    cnt += popcnt_neon(ptr, bytes);
    ptr += bytes;
  }

  return cnt;
}

/* all other CPUs */
#else

/* Align memory to 8 bytes boundary */
static inline void align(const uint8_t** p, uint64_t* size, uint64_t* cnt)
{
  for (; *size > 0 && (uintptr_t) *p % 8; (*p)++)
  {
    *cnt += popcnt64(**p);
    *size -= 1;
  }
}

/*
 * Count the number of 1 bits in the data array
 * @data: An array
 * @size: Size of data in bytes
 */
static inline uint64_t popcnt(const void* data, uint64_t size)
{
  uint64_t i;
  uint64_t cnt = 0;

  const uint8_t* buf = (const uint8_t*) data;
  align(&buf, &size, &cnt);

  cnt += popcnt64_unrolled((const uint64_t*) buf, size / 8);
  buf += size - size % 8;
  size = size % 8;

  for (i = 0; i < size; i++)
    cnt += popcnt64(buf[i]);

  return cnt;
}

#endif

#endif /* LIBPOPCNT_H */
