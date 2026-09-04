#include <random>       // prng
#include <cstdint>      // uint32_t
#include <iostream>     // std::cout
#include <immintrin.h>  // AVX intrinsics
#include <iomanip>      // std::setw, std::setprecision
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


inline float hsum_avx(__m256 v){
    // v = [a,b,c,d,e,f,g,h]
    __m128 v_lo = _mm256_castps256_ps128(v);          // v_lo = [a,b,c,d]   _mm256_castps256_ps128将256位向量的低128位提取为128位向量
    __m128 v_hi = _mm256_extractf128_ps(v,1);         // v_hi = [e,f,g,h]   _mm256_extractf128_ps提取256位向量的高128位为128位向量
    v_lo = _mm_add_ps(v_lo,v_hi);                     // v_lo = [a+e,b+f,c+g,d+h]
    return hsum_sse3(v_lo);                           // 调用hsum_sse3对v_lo进行水平求和
}



// ============================================
// 辅助函数
// ============================================
// 打印__m256的内容
void print_m256(const char* lable,__m256 v){
    float temp[8];
    _mm256_storeu_ps(temp,v); // 将__m256类型的向量存储到float数组中
    std::cout <<lable<<":[";
    for(int i =0;i<8;i++){
        std::cout<<std::setw(8)<<std::fixed<<std::setprecision(4)<<temp[i];
        if(i<7) std::cout<<", ";
    }
    std::cout<<"]"<<std::endl;
}

// 标量求和
float hsum_scalar(const float* arr,int n){
    float sum =0.0f;
    for(int i=0;i<n;i++){
        sum += arr[i];
    }
    return sum;
}

// 浮点数比较（带容差）
bool float_equal(float a,float b,float epsilon = 1e-6f){
    return std::fabs(a-b)<epsilon;
}


// ============================================
// 测试用例1：基本功能测试（手动构造向量）
// ============================================
bool test_basic_functionality(){
    std::cout << "\n======= 测试1: 基本功能测试 =======" << std::endl;
    std::cout << "Running test_basic_functionality..." << std::endl;
    // 测试数据：8个浮点数
    float data[8] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f};   // v = [8.0,7.0,6.0,5.0,4.0,3.0,2.0,1.0]
    __m256 v = _mm256_loadu_ps(data); // 将数组加载到__m256类型的向量中
    print_m256("输入向量 v ",v);

    float sum = hsum_avx(v); // 调用水平求和函数
    float expected_sum = hsum_scalar(data,8); // 计算期望的和

    std::cout << "SIMD求和结果: " << sum << std::endl;
    std::cout << "标量求和结果: " << expected_sum << std::endl;

    bool passed = float_equal(sum, expected_sum); // 比较结果是否相等
    std::cout<<"测试结果："<<(passed?"通过":"失败")<<std::endl;
    return passed;  
}













int test_hsum_sse3(){
    __m128 v = _mm_set_ps(1.0f, 2.0f, 3.0f, 4.0f); // v = [4.0, 3.0, 2.0, 1.0]
    float sum = hsum_sse3(v);
    std::cout << "Sum: " << sum << std::endl; // 输出结果: 10.000000
    return 0;
}


int main(){
    test_hsum_sse3();
    test_basic_functionality();
    return 0;
}