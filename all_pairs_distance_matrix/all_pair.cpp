#include <iostream>
#include <cstdint>
#include <vector>
#include <thread>

#include "../include/hpc_helpers.hpp"
#include "../include/binary_IO.hpp"

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
            // compute squared Euclidean distance
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

template<typename index_t,
         typename value_t>
void parallel_all_pairs(
    std::vector<value_t>& mnist,
    std::vector<value_t>& all_pair,
    index_t rows,
    index_t cols,
    index_t num_threads=64,
    index_t chunck_size=64/sizeof(value_t)
){
    auto block_cyclic = [&](const index_t& id) -> void {

        // precompute offset and stride
        const index_t off = id * chunck_size;
        const index_t str = num_threads * chunck_size;

        // for each block of size chunck_size in cyclic order
        for(index_t lower = off; lower < rows; lower += str){

            // compute the upper border of the block (exclusive)
            const index_t upper = std::min(lower+chunck_size,rows);

            // for all entries below the diagonal (i' =I)
            for(index_t i = lower; i < upper; i++){
                for(index_t I = 0; I <= i; I++){

                    // compute squared Euclidean distance
                    value_t accum = value_t(0) ;
                    for(index_t j = 0; j < cols; j++){
                        value_t residue = mnist[i*cols + j]
                                        - mnist[I*cols + j];
                        accum += residue * residue;
                    }

                    // write Delta[i,i'] = Delta[i',i]
                    all_pair[i*rows + I] = 
                    all_pair[I*rows + i] = accum;
                }
            }
        }
    };

        // business as usual
        std::vector<std::thread> threads;

        for(index_t id = 0; id < num_threads; id++){
            threads.emplace_back(block_cyclic,id);
        }

        // wait for all threads to finish
        for(auto& thread:threads){
            thread.join();
        }
}

#include <mutex>
#include <atomic>
std::mutex mutex;

template<typename index_t,
         typename value_t>
void dynamic_all_pairs(
    std::vector<value_t>& mnist,
    std::vector<value_t>& all_pair,
    index_t rows,
    index_t cols,
    index_t num_threads=64,
    index_t chunck_size=64/sizeof(value_t)
){
    // declare mutex and current lower index
    index_t global_lower = 0;

    auto dynamic_block_cyclic = [&] (const index_t& id) -> void {

        // assume we have not done anything
        index_t lower = 0;

        // while there are still rows to compute
        while (lower < rows) {

            // update lower row with global lower row
            {
                std::lock_guard<std::mutex> lock_guard(mutex);
                        lower = global_lower;
                global_lower += chunck_size;
            }

            // compute the upper border of the block (exclusive)
            const index_t upper = std::min(lower+chunck_size,rows);

            // for all entries below the diagonal (i' =I)
            for(index_t i = lower; i < upper; i++){
                for(index_t I = 0; I <= i; I++){

                    // compute squared Euclidean distance
                    value_t accum = value_t(0) ;
                    for(index_t j = 0; j < cols; j++){
                        value_t residue = mnist[i*cols + j]
                                        - mnist[I*cols + j];
                        accum += residue * residue;
                    }

                    // write Delta[i,i'] = Delta[i',i]
                    all_pair[i*rows + I] = 
                    all_pair[I*rows + i] = accum;
                }
            }
        }
    };

    // business as usual
    std::vector<std::thread> threads;

    for(index_t id = 0; id < num_threads; id++){
        threads.emplace_back(dynamic_block_cyclic,id);
    }

    for(auto& thread:threads){
        thread.join();
    }
}


int main(int argc,char* argv[]){

    // used data types
    typedef no_init_t<float>    value_t;
    typedef uint64_t            index_t;

    // number of images and pixels
    const index_t rows = 65000;
    const index_t cols = 28*28;

    // load MNIST data from binary file
    TIMERSTART(load_data_from_disk);
    std::vector<value_t> mnist(rows*cols);
    load_binary(mnist.data(),rows*cols,"./data/mnist_65000_28_28_32.bin");
    TIMERSTOP(load_data_from_disk);

    // compute all-pairs distance matrix
    TIMERSTART(compute_distance);
    std::vector<value_t> all_pair(rows*rows);
    // sequential_all_pairs(mnist,all_pair,rows,cols);
    // parallel_all_pairs(mnist,all_pair,rows,cols);
    dynamic_all_pairs(mnist,all_pair,rows,cols);
    TIMERSTOP(compute_distance);
    
}