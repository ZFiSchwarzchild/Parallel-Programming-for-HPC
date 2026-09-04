#include <random>       // prng
#include <cstdint>      // uint32_t
#include <iostream>     // std::cout
#include <immintrin.h>  // AVX intrinsics

#include "../include/hpc_helpers.hpp"   // timers 

void init(float * data , uint64_t length){
    std::mt19937 engine(42); // seed the generator
    std::uniform_real_distribution<float> density(-1.0f, 1.0f); // define the range
    for(uint64_t i = 0; i < length ; i++){
        data[i] = density(engine);
    }
}

inline float hsum_sse3(__m128 v){
    // v =[a,b,c,d]
    __m128 shuf = _mm_movehdup_ps(v);       // shuf = [b,b,d,d]
    __m128 sums = _mm_add_ps(v,shuf);       // sums = [a+b,b+b,c+d,d+d]
    shuf = _mm_movehl_ps(shuf,sums);        // shuf = [c+d,d+d,d,d]
    sums = _mm_add_ss(sums,shuf);           // sums = [a+b+c+d,b+b,c+d,d+d] , 只加最低位
    return _mm_cvtss_f32(sums);             // 提取最低浮点数并转换为C的float类型
}

int test_hsum_sse3(){
    __m128 v = _mm_set_ps(1.0f, 2.0f, 3.0f, 4.0f); // v = [4.0, 3.0, 2.0, 1.0]
    float sum = hsum_sse3(v);
    std::cout << "Sum: " << sum << std::endl; // 输出结果: 10.000000
    return 0;
}


int main(){
    test_hsum_sse3();
    return 0;
}