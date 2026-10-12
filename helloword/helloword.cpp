#include  <iostream>
#include  <cstdint>
#include  <vector>
#include  <thread>

// this function will be called by the threads (should be void)
void say_hello(uint64_t id){
    std::cout << "Hello from thread: " << id << std::endl;
}

// this runs in the master thread 
int main(int argc, char* argv[]){
    const uint64_t num_threads = 4;
    std::vector<std::thread> threads;
    threads.reserve(num_threads); // reserve space for threads to avoid reallocations  // 关键：仅预留空间
    // for all threads
    for(uint64_t id = 0; id < num_threads; id++){
        // emplace the thread object in vector threads
        // using argument forwarding, this avoids unnecessary
        // move operations to the vector after thread creation
        // threads.emplace_back(say_hello,id);
        threads.emplace_back(std::thread(say_hello,id)); // 关键：直接创建线程对象并传递参数

        // join each thread at the end
        for(auto& thread: threads){
            if (thread.joinable())  // 增加防御性检查
            thread.join();
        }
    }

    return 0;
}

