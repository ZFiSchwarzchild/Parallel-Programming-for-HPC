#ifndef HPC_HELPERS_HPP
#define HPC_HELPERS_HPP

#include <iostream> 
#include <cstdint>

#ifndef __CUDACC__
    #include <chrono>
#endif

#ifndef __CUDACC__
    #define TIMERSTART(label)                                                   \
    std::chrono::time_point<std::chrono::system_clock> a##label, b##label;      \
    a##label = std::chrono::system_clock::now();
#else
    #define TIMERSTART(label)                                                   \
            cudaEvent_t start##label, stop##label;                              \
            float time##label;                                                  \
            cudaEventCreate(&start##label);                                     \
            cudaEventCreate(&stop##label);                                      \
            cudaEventRecord(start##label, 0);
#endif

#ifndef __CUDACC__
    #define TIMERSTOP(label)                                                    \
    b##label = std::chrono::system_clock::now();                                \
    std::chrono::duration<double> elapsed_seconds##label = b##label - a##label; \
    std::cout << "Elapsed time for (" << #label << "): "                        \
              << elapsed_seconds##label.count() << "s" << std::endl;
#else
    #define TIMERSTOP(label)                                                    \
            cudaEventRecord(stop##label, 0);                                    \
            cudaEventSynchronize(stop##label);                                  \
            cudaEventElapsedTime(&time##label, start##label, stop##label);      \
            std::cout << "Elapsed time for (" << #label << "): "                \
                      << time##label << "ms" << std::endl;                      \
            cudaEventDestroy(start##label);                                     \
            cudaEventDestroy(stop##label);
#endif

#ifdef __CUDACC__
    #define CUERR()                                                             \
    {                                                                           \
        cudaError_t err = cudaGetLastError();                                   \
        if (err != cudaSuccess) {                                               \
            std::cerr << "CUDA error: " << cudaGetErrorString(err)              \
                                    <<" : " << __FILE__  << ", at line "        \
                                    << __LINE__ << std::endl;                   \
            exit(EXIT_FAILURE);                                                 \
        }                                                                       \
    }

    // transfer constants
    #define H2D(cudaMemcpyHostToDevice)
    #define D2H(cudaMemcpyDeviceToHost)
    #define D2D(cudaMemcpyDeviceToDevice)
#endif

// safe division
#define SDIV(x,y) (((x)+(y)-1)/(y))

// no_init_t
#include <type_traits>      //C++ 标准库中的类型特性（Type Traits）库，它提供了编译时类型检查、转换和查询的工具

template <class T>
class  no_init_t
{
private:
    T v_;
public:
    static_assert(std::is_fundamental<T>::value &&
                  std::is_arithmetic<T>::value,
                  "warpped type must be a fundamental, numeric type");  //限制 T 必须是基本算术类型。
    // do noting
    constexpr no_init_t() noexcept{}    // 什么都不做的构造函数

    // convertible from a T
    constexpr no_init_t(T value) noexcept : v_(value) {}

    // act as a T in all conversion contexts
    constexpr operator T() const noexcept { return v_; }

    // negation on value and bit level
    constexpr no_init_t& operator -() noexcept { v_ = -v_; return *this; }
    constexpr no_init_t& operator ~() noexcept { v_ = ~v_; return *this; }

    // perfix increment and decrement operators
    constexpr no_init_t& operator ++() noexcept { v_++; return *this;}
    constexpr no_init_t& operator --() noexcept { v_--; return *this;}

    // perfix increment and decrement operators
    constexpr no_init_t operator ++(int) noexcept {
        auto old(*this);
        v_++;
        return old;
    }

    constexpr no_init_t operator --(int) noexcept {
        auto old(*this);
        v_--;
        return old;
    }

    // assignment opertors
    constexpr no_init_t& operator +=(T v) noexcept { v_ += v; return *this; }
    constexpr no_init_t& operator -=(T v) noexcept { v_ -= v; return *this; }
    constexpr no_init_t& operator *=(T v) noexcept { v_ *= v; return *this; }
    constexpr no_init_t& operator /=(T v) noexcept { v_ /= v; return *this; }

    // bit-wise operators
    constexpr no_init_t& operator &=(T v) noexcept { v_ &= v; return *this; }
    constexpr no_init_t& operator |=(T v) noexcept { v_ |= v; return *this; }
    constexpr no_init_t& operator ^=(T v) noexcept { v_ ^= v; return *this; }       
    constexpr no_init_t& operator <<=(T v) noexcept { v_ <<= v; return *this; }
    constexpr no_init_t& operator >>=(T v) noexcept { v_ >>= v; return *this; }
};

#endif // HPC_HELPERS_HPP