#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>

using namespace std::chrono_literals;

int main(){
    std::mutex mutex;
    std::condition_variable cv;
    bool time_for_breakfast = false;

    auto student =[&] () ->void {
        {
            std::unique_lock<std::mutex> unique_lock(mutex);
            cv.wait(unique_lock,[&](){ return time_for_breakfast;});
        }
        std::cout << "Time for make some coffee! " << std::endl;
    };

    std::thread my_thread(student);
    std::this_thread::sleep_for(2s);

    {
        std::lock_guard<std::mutex> lock_guard(mutex);
        time_for_breakfast = true;
    }
    cv.notify_one();
    my_thread.join();
}