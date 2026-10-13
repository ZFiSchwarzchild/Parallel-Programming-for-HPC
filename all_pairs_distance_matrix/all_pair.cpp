#include <iostream>
#include <cstdint>
#include <vector>
#include <thread>

#include "../include/hpc_helpers.hpp"
#include "../include"

template<typename index_t,
         typename value_t>
void sequential_all_pairs(
    std::vector<value_t>& mnist,
    std::vector<value_t>& all_pair,
    index_t rows,
    index_t cols
){
    // for all entries below the diagonal (i' = I)
    for(index_t i = 0; i < rows; i++){
        for(index_t I = 0; I <= i; I++){
            // compute suqared Euclidean distance
            value_t accum = value_t(0);
            for(index_t j = 0; j < cols; j++){
                value_t residue = mnist[i*cols + j]
                                - mnist[I*cols + j];
                accum += residue * residue;
            }

            // write Delta[i,i'] = Delta[i',i] = dist(i,i')
            all_pair[i*rows+I] = all_pair[I*rows + i] = accum;
        }
    }
}

int main(int argc,char* argv[]){
    
}