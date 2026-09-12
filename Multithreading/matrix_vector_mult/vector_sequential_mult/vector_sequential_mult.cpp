#include <iostream>
#include <cstdint>
#include <vector>
#include <thread>

#include "../../../include/hpc_helpers.hpp"

template<typename value_t,
         typename index_t>
void init(std::vector<value_t>& A,
          std::vector<value_t>& x,
          index_t m,
          index_t n){
    
    for(index_t row=0; row<m;row++){
        for(index_t col=0;col<n;col++){
            A[row*n+col] = (row>col)?1:0;
        }
    }

    for(index_t col=0;col<n;col++)
    x[col]=col;
}

// the sequential matrix vector product
template<typename value_t,
         typename index_t>
void sequential_mult(std::vector<value_t>& A,
                     std::vector<value_t>& x,
                     std::vector<value_t>& b,
                     index_t m,
                     index_t n){
    for(index_t row=0;row<m;row++){
        value_t accum = value_t(0);
        for(index_t col=0;col<n;col++){
            accum += A[row*n+col]*x[col];
        }
        b[row]=accum;
    }
}

int main(int argc,char* argv[]){
    const uint64_t n = 1UL<<15;
    const uint64_t m = 1UL<<15;

    TIMERSTART(overall)

    TIMERSTART(alloc)
    // std::vector<uint64_t> A(m*n);
    // std::vector<uint64_t> x(n);
    // std::vector<uint64_t> b(m);
    std::vector<no_init_t<uint64_t>> A(m*n);
    std::vector<no_init_t<uint64_t>> x(n);
    std::vector<no_init_t<uint64_t>> b(m);
    TIMERSTOP(alloc)

    TIMERSTART(init)
    init(A,x,m,n);
    TIMERSTOP(init)

    TIMERSTART(vector_sequential_mult)
    sequential_mult(A,x,b,m,n);
    TIMERSTOP(vector_sequential_mult)    

    TIMERSTOP(overall)

    // check if summation is correction
    for(uint64_t idx = 0;idx < m;idx++){
        if(b[idx]!=idx*(idx-1)/2)
            std::cout<<"error at position "<<idx<<std::endl;
    }
}