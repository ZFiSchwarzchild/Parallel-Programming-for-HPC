#include <iostream>
#include <cstdint>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>

#include "../include/hpc_helpers.hpp"

template<typename atomc_t,
         typename value_t,
         typename func_t,
         typename predc_t>
value_t binary_atomic(
    atomc_t& atomic,
    const value_t& operand,
    func_t function,
    predc_t predicate
){
    value_t expect = atomic.load();
    value_t target;

    do{
        // 1. 计算初步的新值
        target = function(expect,operand);

        // 2. 检查是否满足条件(谓词)
        // 如果不满足，立即返回当前值，不执行交换
        if(!predicate(target))
            return expect;

        // 3. 尝试原子交换：如果当前内存值 == expect，就替换为 target
    } while (!atomic.compare_exchange_weak(expect,target));

    // 4. 返回最终结果
    return expect;
}

int main(){
    std::vector<std::thread> threads;
    const uint64_t num_threads = 10;
    const uint64_t num_iters = 100'000'000;

    auto even_max = [&] (volatile std::atomic<uint64_t>* counter,
                         const auto& id) -> void {
        // 定义函数：找最大值
        auto func = [] (const auto& lhs,
                        const auto& rhs) {
            return lhs > rhs ? lhs:rhs; };
        
        // 定义谓词：必须是偶数
        auto pred = [] (const auto& val) {
            return val % 2 == 0;
        };

        // 调用封装好的bianry_atomic
        for(uint64_t i = id; i < num_iters; i += num_threads){
            binary_atomic(*counter,i,func,pred);
        }
    };

    TIMERSTART(even_max)
    std::atomic<uint64_t> even_counter(0);
    for(uint64_t id = 0; id < num_threads; id++){
        threads.emplace_back(even_max,&even_counter,id);
    }
    for(auto& thread : threads)
        thread.join();
    TIMERSTOP(even_max)

    std::cout<< "even_counter   " << even_counter<<std::endl;
}