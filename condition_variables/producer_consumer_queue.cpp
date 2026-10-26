#include <iostream>
#include <thread>
#include <mutex>
#include  <queue>
#include <chrono>
#include <condition_variable>

using namespace std::chrono_literals;

std::mutex mtx;
std::condition_variable cv_not_empty;
std::condition_variable cv_not_full;
std::queue<int> q;
constexpr int CAPACITY = 3;
bool done = false;

void producer(){
    for(int i = 0; i <=10; i++){
        {
            std::unique_lock<std::mutex> lock(mtx);
            cv_not_full.wait(lock,[]{ return q.size() < CAPACITY;});
            q.push(i);
            std::cout << "[生产者] 放入 " << i 
                        <<" (当前队列长度 " << q.size() << ")\n";
        } // 解锁
        cv_not_empty.notify_one();

        std::this_thread::sleep_for(300ms); //模拟生产耗时
    }
    // 生产完毕，通知消费者
    {
        std::lock_guard<std::mutex> lock(mtx);
        done = true;
    }

    cv_not_empty.notify_all(); // 可能有多个消费者在等
}

// 消费者