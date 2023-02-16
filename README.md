# faster-inverse-square-root
A faster version of Quake III's infamous inverse square root algorithm using compiler intrinsic operations.

The original Quake III fast inverse square root algorithm used a concept called type punning. The implementation used for type punning in the original algorithm as seen below is considered undefined behavior in C++. Although it compiles and works correctly on most machines, the compiler cannot correctly optimize the code due to its undefined nature. There are likely many fixes to this, especially in modern C++. One possible solution, although limited, is to switch to compiler intrinsic functions and handle the instruction calls more explicitly and improve performance on chips that have modern SIMD capabilities.

### Original Algorithm
```
float Q_rsqrt(float number) {
	int32_t i;
	float x2, y;
	const float threehalfs = 1.5f;
	
	x2 = number * 0.5f;
	y = number;
	i = *(int32_t*)&y;			// evil floating point bit hack
	i = 0x5f3759df - (i >> 1);		// Approximation
	y = *(float*)&i;
	y = y * (threehalfs - (x2 * y * y));	// Newtons method

	return y;
}
```
Estimates the inverse square root -- 1/sqrt(x) -- using an approximation technique.

### Improved Algorithm (without multiple data)
```
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
```

This version computes the exact same result as the first version, but up to 80% faster. This version of the algorithm uses a union based approach to type-punning. The behavior is still technically undefined, however the data types used are defined by their size, preventing errors from occuring during runtime. The intrinsic functions are correctly optimized into faster instructions. The generated assembly instructions can be found below.

### Improved Algorithm (with multiple data)
```
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
```
This version of the algorithm is almost the exact same as the above version, but it can compute the inverse square root of up to 8 packed floats in one call.


## Generated Machine Code (MSVC)
Even with full compiler optimizations enabled, including automatic use of AVX equivalent instructions on my Intel Core i7-8650U CPU, the original version of the algorithm uses significantly more instructions to accomplish the same result. The improved version compiles into a more compact and optimized set of instructions as seen below.

### Original Algorithm
```
 # float Q_rsqrt(float number)
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
```

### Improved Algorithm Equivalent
```
 # float Q_rsqrt_s(float* number)
 vpsrld      xmm0,xmm2,1
 vpsubd      xmm3,xmm5,xmm0
 vmulss      xmm1,xmm3,xmm3
 vmulss      xmm2,xmm2,xmm4
 vmovups     xmm0,xmm6
 vfnmadd231ss xmm0,xmm2,xmm1
 vmulss      xmm3,xmm3,xmm0
 vmovss      dword ptr [rcx-4],xmm3
```


## Test Case Results (Intel Core i7-8650U)
Calculated the inverse square root of 4 50,000,000 times over 20 iterations, where each time value represents the cost of 1 iteration.

```
Enter a number: 4

152ms 205ms 113ms 114ms 111ms 132ms 167ms 181ms 164ms 165ms 157ms 182ms 197ms 188ms 133ms 173ms 165ms 143ms 194ms 123ms
result: 0.499154
Orignal method took an average of 157 milliseconds over 20 iterations

70ms 89ms 118ms 119ms 124ms 81ms 81ms 71ms 82ms 100ms 91ms 92ms 104ms 121ms 131ms 90ms 87ms 95ms 98ms 68ms
result: 0.499154
Improved Single Data SIMD method took an average of 95 milliseconds over 20 iterations

34ms 42ms 42ms 39ms 42ms 37ms 39ms 36ms 37ms 35ms 32ms 38ms 43ms 42ms 43ms 42ms 31ms 34ms 33ms 39ms
result: 0.499154
Improved Multiple Data SIMD method took an average of 38 milliseconds over 20 iterations
```
