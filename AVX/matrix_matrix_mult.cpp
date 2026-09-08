#include <random>       // prng
#include <cstdint>      // uint32_t
#include <iostream>     // std::cout
#include <immintrin.h>  // AVX intrinsics
#include <iomanip>      // std::setw, std::setprecision
#include <functional>   // for std::function
#include <fstream>      // for std::ofstream
#include <vector>
#include <string>

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

void plain_dmm(float* A,
               float* B,
               float* C,
               uint64_t M,
               uint64_t L,
               uint64_t N,
               bool parallel){

    #pragma omp parallel for collapse(2) if(parallel)
    for(uint64_t i = 0; i < M ; i++){
        for(uint64_t j = 0; j < N ; j++){
            float sum = 0.0f;
            for(uint64_t k = 0; k < L ; k++){
                sum += A[i*L+k] * B[j*L+k];         // 注意B的访问方式，假设B是按列主序存储的                
            }    
            C[i*N+j] = sum;     
        }
    }
}

void avx_dmm(float* A,
             float* B,
             float* C,
             uint64_t M,
             uint64_t L,
             uint64_t N,
             bool parallel){

    #pragma omp parallel for collapse(2) if(parallel) 
    for(uint64_t i = 0; i < M ; i++){
        for(uint64_t j = 0; j < N ; j++){
            __m256 sum_vec = _mm256_setzero_ps();                   // 初始化一个256位的向量为零
            for(uint64_t k = 0; k < L ; k+=8){
                __m256 a_vec = _mm256_loadu_ps(&A[i*L+k]);          // 加载A的8个元素到向量
                __m256 b_vec = _mm256_loadu_ps(&B[j*L+k]);          // 加载B的8个元素到向量
                sum_vec = _mm256_fmadd_ps(a_vec, b_vec, sum_vec);   // 执行FMA操作: sum_vec += a_vec * b_vec
            }
            C[i*N+j] = hsum_avx(sum_vec);                           // 对sum_vec进行水平求和并存储到C中
        }
    }
}

void avx_dmm_unroll_2(float* A,
                      float* B,
                      float* C,
                      uint64_t M,
                      uint64_t L,
                      uint64_t N,
                      bool parallel){

    #pragma omp parallel for collapse(2) if(parallel)
    for(uint64_t i =0;i<M;i++){
        for(uint64_t j=0;j<N;j++){

            __m256 X = _mm256_setzero_ps();
            __m256 Y = _mm256_setzero_ps();
            for(uint64_t k=0;k<L;k+=16){
                const __m256 AVX = _mm256_load_ps(A+i*L+k+0);
                const __m256 BVX = _mm256_load_ps(B+j*L+k+0);
                const __m256 AVY = _mm256_load_ps(A+i*L+k+8);
                const __m256 BVY = _mm256_load_ps(B+j*L+k+8);
                X = _mm256_add_ps(X,_mm256_mul_ps(AVX,BVX));
                Y = _mm256_add_ps(Y,_mm256_mul_ps(AVY,BVY));
            }
            C[i*N+j] = hsum_avx(X)+hsum_avx(Y);
        }
    }
 }


void plain_tmm(float* A,
               float* B,
               float* C,
               uint64_t M,
               uint64_t L,
               uint64_t N){
    for(uint64_t i = 0; i < M; i++)
        for(uint64_t j = 0; j < N; j++){
            float accum = float(0);
            for(uint64_t k = 0; k < L; k++){
                accum += A[i*L+k]*B[j*L+k];
            }
            C[i*N+j] = accum;
        }
 }

 void avx2_tmm(float* A,
               float* B,
               float* C,
               uint64_t M,
               uint64_t L,
               uint64_t N){
    
    for(uint64_t i = 0; i < M; i++){
        for(uint64_t j = 0; j < N; j++){

            __m256 X = _mm256_setzero_ps();
            for(uint64_t k = 0; k < L; k+=8){
                const __m256 AV = _mm256_load_ps(A+i*L+k);
                const __m256 BV = _mm256_load_ps(B+j*L+k);
                X = _mm256_fmadd_ps(AV,BV,X);
            }
            C[i*N+j] = hsum_avx(X);
        }
    }
}


// ============================================
// 辅助函数
// ============================================
// 打印__m256的内容
void print_m256(const char* label,__m256 v){
    float temp[8];
    _mm256_storeu_ps(temp,v); // 将__m256类型的向量存储到float数组中
    std::cout <<label<<":[";
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

// 计算GFLOPS:  2 * M * L * N / (time_sec * 1e9)
double compute_gflops(uint64_t M,uint64_t L,uint64_t N,double time_sec){
    double ops =  2.0 * M * L * N;      // 乘加各一次
    return ops / (time_sec * 1e9);
}

// 测试单个参数组合
void test_one_config(uint64_t M, uint64_t L,uint64_t N,
                    const std::string& label,
                    std::ostream& out){
    std::cout << "\n--- Testing " << label << std::endl;
    std::cout << "M=" << M << std::endl;
    std::cout << "N=" << N << std::endl;
    std::cout << "L=" << L << std::endl;
    std::cout << "---\n";

    // 分配内存
    float* A = static_cast<float*>(_mm_malloc(M*L*sizeof(float),32));
    float* B = static_cast<float*>(_mm_malloc(N*L*sizeof(float),32));
    float* C = static_cast<float*>(_mm_malloc(M*N*sizeof(float),32));

    if(!A || !B || !C){
        std::cerr<<"Memory allocation failed!\n";
        exit(1);  
    }
    init(A,M*L);
    init(B,N*L);

    // 定义函数指针数组和名称
    struct FuncEntry
    {
        std::string name;
        std::function<void()> func;
        bool parallel;  //是否使用并行
    };
    
    std::vector<FuncEntry> funcs ={
        {"plain_dmm_single",        [&](){plain_dmm(A,B,C,M,L,N,false);},false},
        {"plain_dmm_multi",         [&](){plain_dmm(A,B,C,M,L,N,true);},true},
        {"avx_dmm_single",          [&](){avx_dmm(A,B,C,M,L,N,false);},false},
        {"avx_dmm_multi",           [&](){avx_dmm(A,B,C,M,L,N,true);},true},
        {"avx_dmm_unroll_2_single", [&](){avx_dmm_unroll_2(A,B,C,M,L,N,false);},false},
        {"avx_dmm_unroll_2_multi",  [&](){avx_dmm_unroll_2(A,B,C,M,L,N,true);},true},
        {"plain_tmm",               [&](){plain_tmm(A,B,C,M,L,N);},false},
        {"avx2_tmm",                [&](){avx2_tmm(A,B,C,M,L,N);},false}
    };
   
    // 采用更精确的手动计时
    for(auto& entry:funcs){
        auto start = std::chrono::high_resolution_clock::now();
        entry.func();
        auto end = std::chrono::high_resolution_clock::now();
        double elapsed = std::chrono::duration<double>(end-start).count();
        double gflops = compute_gflops(M,L,N,elapsed);
        out << M << "," << L << "," << N << ","
            << entry.name << "," << (entry.parallel ? "true":"false") << ","
            << std::fixed << std::setprecision(6) << elapsed << ","
            << std::setprecision(3) << gflops << std::endl;
        std::cout<<entry.name << " elapsed: " << elapsed << " s, GFLOPS: " << gflops << std::endl;
    }

    _mm_free(A);
    _mm_free(B);
    _mm_free(C);

}

// 主测试函数： 遍历参数组合
void test_performance_scaling(){
    std::ofstream out("perf_data.csv");
    if(!out){
        std::cerr<<"Cannot open perf_data.csv for writing\n";
        return;
    }
    // 输出表头（csv）
    out << "M,L,N,function,parallel,time_s,GFLOPS\n";
    std::cout << "=== Performance Scaling Tests ===\n";

    // 测试范围：可以调整
    std::vector<uint64_t> sizes = {128,256,512,1024,2048};      //注意：2048*2048*4 = 16 MB,可接受
    // 为了研究L的影响，固定M=N=1024,变化L
    // 研究M的影响，固定L=1024,N=1024,变化M
    // 研究N的影响，固定M=1024,L=1024,变化N    

    // 1.固定 M=N=1024,变化L
    const uint64_t baseM = 1024, baseN = 1024;
    for(uint64_t L:{64,128,256,512,1024,2048}){
        test_one_config(baseM,L,baseN,"vary_L",out);
    }

    // 2.固定 L=N=1024,变化M
    const uint64_t baseL = 1024,baseN2 = 1024;
    for(uint64_t M:{128,256,512,1024,2048}){
        test_one_config(M,baseL,baseN2,"vary_M",out);
    }

    // 3.固定 M=L=1024,变化N
    const uint64_t baseM2 =1024, baseL2=1024;
    for(uint64_t N:{128,256,512,1024,2048}){
        test_one_config(baseM2,baseL2,N,"vary_N",out);
    }

    out.close();
    std::cout<<"\nPerformance data written to perf_data.csv\n";
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


void test_matrix_matrix_mult(){
    std::cout << "\n======= 测试: 矩阵相乘功能测试 =======" << std::endl;
    std::cout << "Running test_matrix_matrix_mult..." << std::endl;
    // matrix shapes
    const uint64_t M = 1UL << 10;
    const uint64_t L = 1UL << 11;
    const uint64_t N = 1UL << 12;

    TIMERSTART(alloc_memory)
    auto A = static_cast<float*>(_mm_malloc(M*L*sizeof(float),32));
    auto B = static_cast<float*>(_mm_malloc(N*L*sizeof(float),32));
    auto C = static_cast<float*>(_mm_malloc(M*N*sizeof(float),32));
    TIMERSTOP(alloc_memory)

    TIMERSTART(init)
    init(A,M*L);
    init(B,N*L);
    TIMERSTOP(init)

    TIMERSTART(plain_dmm_single)
    plain_dmm(A,B,C,M,L,N,false);
    TIMERSTOP(plain_dmm_single)

    TIMERSTART(avx_dmm_single)
    avx_dmm(A,B,C,M,L,N,false);
    TIMERSTOP(avx_dmm_single)

    TIMERSTART(avx_dmm_multi)
    avx_dmm(A,B,C,M,L,N,true);
    TIMERSTOP(avx_dmm_multi)

    TIMERSTART(avx_dmm_unroll_2_single)
    avx_dmm_unroll_2(A,B,C,M,L,N,false);
    TIMERSTOP(avx_dmm_unroll_2_single)

    TIMERSTART(avx_dmm_unroll_2_multi)
    avx_dmm_unroll_2(A,B,C,M,L,N,true);
    TIMERSTOP(avx_dmm_unroll_2_multi)

    TIMERSTART(free_memory)
    _mm_free(A);
    _mm_free(B);
    _mm_free(C);
    TIMERSTOP(free_memory)
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
    test_matrix_matrix_mult();
    test_performance_scaling();     // 性能测试
    return 0;
}