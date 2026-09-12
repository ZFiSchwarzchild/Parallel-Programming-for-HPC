#include <iostream>
#include <cstdint>
#include <vector>
#include <thread>
#include <future>
#include <functional>

template<typename Func,
         typename ...Args,
         typename Rtrn = typename std::result_of<Func(Args...)>::type>
auto make_task(Func&& func,
               Args&& ...args) -> std::packaged_task<Rtrn(void)>{
    // basically build an auxilliary function aux(void)
    // without arguments returning func(arg0,arg1,...)
    auto aux = std::bind(std::forward<Func>(func),std::forward<Args>(args)...);
    
    // creat a task wrapping the auxilliary function:
    // task() excutes aux(void) := func(arg0,arg1,...)
    auto task = std::packaged_task<Rtrn(void)>(aux);

    // the return value of aux(void) is assign to a
    // future object accessible via task.get()_future();
    return task;
}

uint64_t fibo(uint64_t n){
    uint64_t a_0 =0;
    uint64_t a_1 =1;
    for(uint64_t idx = 0; idx < n; idx++){
        const uint64_t temp = a_0;
        a_0 = a_1; a_1 +=temp;
    }
    return a_0;
}
    

int main(){
    const uint64_t num_threads = 32;
    std::vector<std::thread> threads;
    std::vector<std::future<uint64_t>> results;

    for(uint64_t idx=0;idx<num_threads;idx++){
        auto task = make_task(fibo,idx);
        results.emplace_back(task.get_future());
        threads.emplace_back(std::move(task));
    }

    for(auto& result:results)
    std::cout<<result.get()<<std::endl;  //这里result.get()已经起到同步的作用了，
    for(auto& thread:threads)
    // thread.detach();
    thread.join();
}

// detach() 只适用于以下极其特殊的场景：
// 常驻后台服务：比如一个Web服务器的监听线程，它需要无限循环接收请求，永远不退出。
// 主线程初始化完它之后，就可以 detach() 让它变成守护线程（Daemon Thread）。
// 即发即忘（Fire and Forget）：任务的生命周期完全独立，不需要任何返回值，且保证绝对不会访问主线程的任何局部变量。
// 但即使在这些场景下，也要极度小心，保证分离线程中访问的数据是全局的、只读的或堆上长期存活的。


