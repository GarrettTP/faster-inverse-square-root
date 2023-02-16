#pragma once

#include <intrin.h>

/*
 - METHOD 1
 sar         ecx,1
 sub         eax,ecx
 mov         dword ptr [rsp+20h],eax
 mov         eax,5F3759DFh
 vmovss      xmm2,dword ptr [rsp+20h]
 vmulss      xmm0,xmm0,xmm2
 vmulss      xmm1,xmm0,xmm2
 vsubss      xmm1,xmm4,xmm1
 vmulss      xmm0,xmm1,xmm2
 vmovss      dword ptr [rdx+rdi+4],xmm0
 vmovss      xmm0,dword ptr [rdx+rbp+8]
 vmovss      dword ptr [rsp+20h],xmm0
 mov         ecx,dword ptr [rsp+20h]
 vmulss      xmm0,xmm0,xmm3

 - METHOD 2
 vpsrld      xmm0,xmm2,1
 vpsubd      xmm3,xmm5,xmm0
 vmulss      xmm1,xmm3,xmm3
 vmulss      xmm2,xmm2,xmm4
 vmovups     xmm0,xmm6
 vfnmadd231ss xmm0,xmm2,xmm1
 vmulss      xmm3,xmm3,xmm0
 vmovss      dword ptr [rcx-4],xmm3

 - METHOD 3
 vmovups     ymm1,ymmword ptr [rax+r14]
 vpsrld      ymm0,ymm1,1
 vpsubd      ymm3,ymm5,ymm0
 vmulps      ymm1,ymm1,ymm4
 vmulps      ymm0,ymm3,ymm3
 vmovups     ymm2,ymm6
 vfnmadd231ps ymm2,ymm0,ymm1
 vmulps      ymm1,ymm2,ymm3
 vmovups     ymmword ptr [rax+rdi],ymm1
*/

// Using union (undefined behavior, works)
//float Q_rsqrt(float number) {
//	union Number {
//		float y;
//		long i;
//	};
//
//	long i;
//	float x2, y;
//	const float threehalfs = 1.5f;
//
//	Number num = { number };
//
//	x2 = num.y * 0.5f;
//	num.i = 0x5f3759df - (num.i >> 1);
//	y = num.y * (threehalfs - (x2 * num.y * num.y));	// Newtons method
//
//	return y;
//}

// Using address casting trick (type punning)
float Q_rsqrt(float number) {
	int32_t i;
	float x2, y;
	const float threehalfs = 1.5f;
	
	x2 = number * 0.5f;
	y = number;
	i = *(int32_t*)&y;						// evil floating point bit hack
	i = 0x5f3759df - (i >> 1);				// Approximation
	y = *(float*)&i;
	y = y * (threehalfs - (x2 * y * y));	// Newtons method

	return y;
}

float Q_rsqrt_s(float* number) {
	union __m128types {
		__m128 m128;
		__m128i m128i;
	};

	const __m128 threehalfs = _mm_set1_ps(1.5f);
	const __m128 half = _mm_set1_ps(0.5f);

	__m128types n = { _mm_load_ss(number) };
	__m128 x2 = _mm_mul_ss(n.m128, half);

	n.m128i = _mm_sub_epi32(_mm_set1_epi32(0x5f3759df), _mm_srli_epi32(n.m128i, 1));

	__m128 ysqr = _mm_mul_ss(n.m128, n.m128);

	float result;
	_mm_store_ss(&result, _mm_mul_ss(n.m128, _mm_sub_ss(threehalfs, _mm_mul_ss(x2, ysqr))));

	return result;
}

__m256 Q_rsqrt_256(float* number) {
	union __m256types {
		__m256 m256;
		__m256i m256i;
	};

	const __m256 threehalfs = _mm256_set1_ps(1.5f);
	const __m256 half = _mm256_set1_ps(0.5f);
	
	__m256types n = { _mm256_load_ps(number) };
	__m256 x2 = _mm256_mul_ps(n.m256, half);
	
	n.m256i = _mm256_sub_epi32(_mm256_set1_epi32(0x5f3759df), _mm256_srli_epi32(n.m256i, 1));

	__m256 ysqr = _mm256_mul_ps(n.m256, n.m256);
	n.m256 = _mm256_mul_ps(n.m256, _mm256_sub_ps(threehalfs, _mm256_mul_ps(x2, ysqr)));

	return n.m256;
}