#include <iostream>
#include <cstdint>
#include <vector>
#include <thread>
#include <mutex>

std::mutex cout_mutex;
// this function will be called by the threads (should be void)
void say_hello(uint64_t id){
    std::lock_guard<std::mutex> lock(cout_mutex);
    std::cout<<"Hello form thread: "<<id<<std::endl;
}


// this runs master thread 
int main(int argc,char* argv[]){
    const uint64_t num_threads = 4;
    std::vector<std::thread> threads;

    // for all threads
    for(uint64_t id=0;id<num_threads;id++){
        // threads.emplace_back(std::thread(say_hello,id));  //  std::thread(say_hello, id)  这个构造函数会立即启动线程,这里发生了：一次线程构造 + 一次移动构造
        threads.emplace_back(say_hello,id);                  //  emplace_back 是可变参数模板，它会将参数完美转发给 std::thread 的构造函数，直接在 vector 尾部原地构造 std::thread 对象,只发生：一次线程构造，没有临时对象，也没有移动构造.
    }

    for(auto& thread:threads)
    thread.join();
}