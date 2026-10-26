#include <iostream>
#include <cstdint>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include "../include/hpc_helpers.hpp"

int main(){
    std::vector<std::thread> threads;
    const uint64_t num_threads = 10;
    const uint64_t num_iters = 1000'000'000;

    // WARNING: this closure produces incorrect results
    auto false_max = [&] (volatile std::atomic<uint64_t>* counter,
                          const auto& id) -> void {
        for(uint64_t i = id; i < num_iters; i += num_threads){
            if(i > *counter)    // 步骤 1： 读取比较
            *counter = i;       // 步骤 2： 写入更新
        }
    };

    // Using a compare and swap loop for correct results
    auto correct_max = [&] (volatile std::atomic<uint64_t>* counter,
                            const auto& id) -> void {
            for(uint64_t i = id; i < num_iters; i += num_threads) {
                auto previous = counter->load();    // 1. 读取当前的最大值，存为previous（预期值）
                // 2. 判断与CAS重试循环
                // 如果previous比当前值i小，说明找到了更大的，尝试更新
                // 如果CAS失败(返回false)，说明其他线程刚刚修改了counter，循环重试 
                while   (previous < i && 
                         !counter -> compare_exchange_weak(previous,i)){
                            // 循环体为空，因为compare_exchange_weak失败时
                            // 会自动把counter的最新值写回到previous中
                         }
            }
        };

    TIMERSTART(incorrect_max)
    std::atomic<uint64_t> false_counter(0);
    threads.clear();
    for(uint64_t id = 0; id < num_threads; id++){
        threads.emplace_back(false_max,&false_counter,id);        
    }
    for(auto& thread:threads){
        thread.join();
    }
    TIMERSTOP(incorrect_max)

    TIMERSTART(correct_max)
    std::atomic<uint64_t> correct_counter(0);
    threads.clear();
    for(uint64_t id = 0; id < num_threads; id++){
        threads.emplace_back(correct_max,&correct_counter,id);
    }
    for(auto& thread : threads)
    {
        thread.join();
    }
    TIMERSTOP(correct_max)
    std::cout   << "false_counter "<< false_counter << std::endl;
    std::cout   << "correct_counter "<< correct_counter << std::endl;
}