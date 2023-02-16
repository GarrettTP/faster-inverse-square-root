
#include <iostream>
#include <chrono>

#include "FastInverseSqrt.h"

constexpr int iterations = 20;
constexpr int arrSize = 50000000;
float arr1[arrSize];
float arr2[arrSize];

const char* methodNames[] = {
    "Orignal",
    "Improved Single Data SIMD",
    "Improved Multiple Data SIMD"
};

inline void Use_rsqrt() {
    for (int i = 0; i < arrSize; i++) {
        arr2[i] = Q_rsqrt(arr1[i]);
    }
}

inline void Use_rsqrt_s() {
    for (int i = 0; i < arrSize; i++) {
        arr2[i] = Q_rsqrt_s(&arr1[i]);
    }
}

inline void Use_rsqrt_256() {
    for (int i = 0; i < arrSize; i += 8) {
        _mm256_store_ps(&arr2[i], Q_rsqrt_256(&arr1[i]));
    }
}

int Q_rsqrt_test(int in, const char method) {
    for (int i = 0; i < arrSize; i++) {
        arr1[i] = in;
    }

    using milli = std::chrono::milliseconds;
    auto start = std::chrono::high_resolution_clock::now();

    switch (method)
    {
    case 0:
        Use_rsqrt();
        break;
    case 1:
        Use_rsqrt_s();
        break;
    case 2:
        Use_rsqrt_256();
        break;
    }

    auto finish = std::chrono::high_resolution_clock::now();

    return std::chrono::duration_cast<milli>(finish - start).count();
}

void RunMethod(int in, char method) {
    int sum = 0;
    int avg;

    for (int i = 0; i < iterations; i++) {
        int time = Q_rsqrt_test(in, method);
        std::cout << time << "ms ";
        sum += time;
    }
    std::cout << std::endl;
    avg = sum / iterations;

    std::cout << "result: " << arr2[0] << std::endl;
    

    std::cout
        << methodNames[method]
        << " method"
        << " took an average of "
        << avg
        << " milliseconds over "
        << iterations
        << " iterations\n\n";
}

int run_test(float result1, float result2, float result3) {
    if (abs(result1 - result2) > 0.0000001f) {
        std::cout << "ERR: Results from tests fell beyond margin of error!" << std::endl;
        std::cout << "Difference: " << result2 - result1 << std::endl;
        return 0;
    }

    if (result2 != result3) {
        std::cout << "ERR: Vector does not match!" << std::endl;
    }
}

int main() {
    // Tests
    for (float i = 0; i < 10000; i++) {
        float in[8];
        _mm256_store_ps(in, _mm256_set1_ps(i));
        
        float result1 = Q_rsqrt(i);
        float result2[8];
        _mm256_store_ps(result2, Q_rsqrt_256(in));

        float result3 = Q_rsqrt_s(&i);

        run_test(result1, result2[0], result2[7]);
        run_test(result1, result3, result3);
    }
    // (Tests)

    std::cout << "Enter a number: ";

    int in;
    std::cin >> in;

    // Warm cache
    Q_rsqrt_test(in, 0);
    
    RunMethod(in, 0);
    RunMethod(in, 1);
    RunMethod(in, 2);

    return 0;
}