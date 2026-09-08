#include <iostream>
#include <cstdint>
#include <vector>
#include <immintrin.h>
#include "../include/hpc_helpers.hpp"


int main(){

    // matrix shapes 
    const uint64_t m = 1 << 13;
    const uint64_t n = 1 << 13;
    const uint64_t l = 1 << 11;

    TIMERSTART(init)
    std::vector<float> A(m*l,0);
    std::vector<float> B(l*n,0);
    std::vector<float> Bt(n*l,0);
    std::vector<float> C(m*n,0);
    TIMERSTOP(init)

    TIMERSTART(simple_mult)
    for(uint64_t i = 0; i < m; i++){
        for(uint64_t j = 0; j < n; j++){
            float sum = float(0);
            for (uint64_t k = 0; k < l; k++){
                sum += A[i*l+k] * B[k*n + j];
            }
            C[i*n+j]=sum;
        }
    }
    TIMERSTOP(simple_mult)

    TIMERSTART(transpose_and_mult)
    TIMERSTART(transpose_matrix)
    for(uint64_t k = 0; k < l; k++){
        for(uint64_t j = 0; j < n; j++){
            Bt[j*l+k] = B[k*n+j];       //写入通常比读取开销小,因此选择让写入不连续，而让读取数据连续
        }
    }
    TIMERSTOP(transpose_matrix)
    for(uint64_t i = 0; i < m; i++){
        for (uint64_t j = 0; j < n; j++){
            float sum = float(0);
            for(uint64_t k = 0; k < l; k++){
                sum += A[i*l+k]*Bt[j*l+k];
            }
            C[i*n+j]=sum;
        }
    }
    TIMERSTOP(transpose_and_mult)
}