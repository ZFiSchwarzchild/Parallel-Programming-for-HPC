#include <iostream>
#include <cstdint>
#include <vector>
#include <thread>
#include <future>

template<typename value_t,
         typename index_t>
void fibo(value_t n,
          std::promise<value_t>&& result){
    
    value_t a_0 = 0;
    value_t a_1 = 1;
    
    for(value_t indx = 0; indx<n; indx++){
        const value_t temp = a_0;
              a_0=a_1;
              a_1 +=temp;
    }
    result.set_value(a_0);
}

int main(int argc, char* argv[]){
    const uint64_t num_thread = 32;
    std::vector<std::thread> threads;
    std::vector<std::future<uint64_t>> results;
    
    for(uint64_t idx = 0; idx < num_thread; idx++){
        std::promise<uint64_t> promise;
        results.emplace_back(promise.get_future());        
        // threads.emplace_back(fibo<uint64_t,uint64_t>,idx,&(results[idx]));
        threads.emplace_back(fibo<uint64_t,uint64_t>,idx,std::move(promise));
    }

    for(auto& result : results){
        std::cout<<result.get()<<std::endl;
    }
    for(auto& thread : threads){
        thread.join();
    }
}