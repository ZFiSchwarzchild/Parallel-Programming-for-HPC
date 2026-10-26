#include <iostream>
#include <cstdint>
#include <vector>
#include <atomic>
#include <thread>
#include <mutex>
#include "../include/hpc_helpers.hpp"

int main(){
    
    std::mutex mutex;
    std::vector<std::thread> threads;
    const uint64_t num_threads = 10;
    const uint64_t num_iters = 100'000'000;     // 1亿次迭代

    // 传统加锁版本
    auto lock_count = [&] (volatile uint64_t* counter, const auto& id) -> void {
        for(uint64_t i = id; i < num_iters; i += num_threads){
            std::lock_guard<std::mutex> lock_guard(mutex);
            (*counter)++;
        }
    };
    // 技术细节分析：
        // · lambda 表达式定义线程的工作函数
        // · volatile 关键字
        //      volatile 告诉编译器 “这个变量随时可能被外部因素（也就是其他线程）改变，不要把优化到寄存器里”。
        //      但请注意 volatile 在 C++ 中并不保证原子性。 它仅仅防止编译器优化。
        // · std::lock_guard(RAII机制)
        //      std::lock_guard 在构造时自动调用mutex.lock(),离开作用域时，自动调用mutex.unlok()。
        // · 底层开销：
        //      1亿次操作，每次都要经历：
        //      申请锁->获得锁（可能会休眠/唤醒，发生上下文切换）-> 内存读 -> 加1 -> 内存写 —> 释放锁。
        //      其中的系统调度和操作系统调度开销，远远大于+1这个动作本身。

    // 现代无锁版本 atomic_count
    auto atomic_count = [&] (volatile std::atomic<uint64_t>* counter, const auto& id) -> void {
        for(uint64_t i = id; i < num_iters; i += num_threads)
            (*counter)++;
    };
    // 技术细节分析：
    // · std::atomic<uint64_t> 原子类型
    //      编译器会保证对它的操作是[不可分割]的-> Indivisible
    //      即 “读->改->写” 一气呵成，中间不会被其他线程打断。
    // · 重载的 operator++ 
    //      虽然代码和普通i++一样，但是底层的(*counter)++会被编译器翻译成底层的硬件原子操作。
    // · 底层开销
    //      无系统调用、无线程休眠、无锁的申请和释放。
    //      CPU直接在硬件层面利用缓存一致性协议（如MESI）或总线锁（Bus Lock）来保证并发安全。纯CPU周期消耗，速度极快。
    
    // 性能对比实验

    TIMERSTART(mutex_multithreaded)
    uint64_t counter = 0;
    threads.clear();
    for(uint64_t id = 0; id < num_threads; id++)
        threads.emplace_back(lock_count,&counter,id);
    for(auto& thread : threads)
        thread.join();
    TIMERSTOP(mutex_multithreaded)

    TIMERSTART(atomic_multithreaded)
    std::atomic<uint64_t> atomic_counter(0);
    threads.clear();
    for(uint64_t id = 0; id < num_threads; id++)
        threads.emplace_back(atomic_count,&atomic_counter,id);
    for(auto& thread:threads)
        thread.join();
    TIMERSTOP(atomic_multithreaded) 

}