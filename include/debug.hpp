// 작성자 : 곽동환
// 이메일 : arbiter1225@gmail.com
// License : GPLv3
// 요약 : 디버깅용 매크로 , 시간측정기
// 최초 작성일 : 230520
// 최종 수정일 : 230522

#ifndef KDH_DEBUG_HPP
#define KDH_DEBUG_HPP

#include <iostream>
#include <functional>
#include <chrono>
#include <sstream>
#include <iostream>

//++++++ 디버그 메시지 함수
#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL 4
#endif

// DEBUG LEVEL보다 낮은 숫자이면 출력
// 따라서 저수준 함수일 수록 DEBUG LEVEL을 낮추어야한다.
// 일반적인 결과 출력은 디버그 래밸 2로 설정. (과거 단순 printf 등을 사용하는 부분)
// 실행의 성공 및 실패 등등을 알려주는 부분은 디버그 래밸 1로 설정
// msg가 ""인부분을 제거 하였다. 
#if defined(NODEBUG)
#define PRINT_DEBUG(msg, level) 0
#else
#define PRINT_DEBUG(msg, level)                           \
    do                                                    \
    {                                                     \
        if (msg != "" && DEBUG_LEVEL >= level)            \
        {                                                 \
            std::cout << "DEBUG : " << #msg << std::endl; \
        }                                                 \
    } while (0)
#define PRINT_DEBUG2(msg, data, level)                      \
    do                                                    \
    {                                                     \
        if (msg != "" && DEBUG_LEVEL >= level)           \
        {                                                 \
            std::cout << "DEBUG - " << #msg << data << std::endl; \
        }                                                 \
    } while (0)
#endif

// 시간과 같이 디버그 함수 생성
#if defined(NODEBUG)
#define CLOCK_DEBUG_ON(msg, level) 0
#else
#define CLOCK_DEBUG_ON(msg, level) \
    {                              \
        StopWatch *kdh_clock_debug = new StopWatch(DEBUG_LEVEL >= level, #msg)
#endif

#if defined(NODEBUG)
#define CLOCK_DEBUG_STOP(msg, level) 0
#else
#define CLOCK_DEBUG_STOP()  \
    delete kdh_clock_debug; \
    }
#endif

namespace kcpp
{
    //++++++ 실행 시간 확인 함수
    class StopWatch
    {
        std::chrono::system_clock::time_point start;
        std::string log_str;
        bool log_at_finish;

    public:
        StopWatch()
        {
            this->log_at_finish = false;
            start = std::chrono::system_clock::now();
        }
        StopWatch(bool b) : log_at_finish(b)
        {
            start = std::chrono::system_clock::now();
        }
        StopWatch(bool b, std::string log_str_) : log_at_finish(b), log_str(log_str_)
        {
            start = std::chrono::system_clock::now();
        }
        ~StopWatch()
        {
            if (log_at_finish)
                log();
        }
        std::chrono::duration<double> stop()
        {
            std::chrono::system_clock::time_point end = std::chrono::system_clock::now();
            return std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
        }
        void log()
        {
            std::cout << this->log_str << stop().count() << " seconds." << std::endl;
        }
    };
    template <typename F, typename... ARGS>
    decltype(auto) chronometry(F &&f, ARGS &&...args) // 함수의 동작 시간을 확인하는 코드
    {
        StopWatch sw(true);
        return std::invoke(std::forward<F>(f), std::forward<ARGS>(args)...);
    }
}
#endif // KDH_DEBUG_HPP

//****************************************************************************************//
// 사용 예시
/*
#include<debug.hpp>
void test()
{
    std::this_thread::sleep_for(2s);
    return;
}
using namespace kcpp;
int main()
{

    chronometry(test);  //void 함수의 경우 수행 시간 확인

    // 좀더 일반적인 함수의 수행 시간 확인 이떄  {} 로 묶어서 소멸자 생성 가능하다.
    {
    StopWatch c1(true,"c1 : ");
    test();
    c1.stop();
    }

    // 디버그 메시지 출력, 이때 디버그 단계 설정 가능
    PRINT_DEBUG(디버그 확인 , 1);
    PRINT_DEBUG(디버그 확인2 , 1);

    // 시간 함수 디버그 메시지 반드시 ON이 입력되었을 경우 STOP과 같이 써야한다.
    CLOCK_DEBUG_ON(clock 확인 :  , 1);
    test();
    CLOCK_DEBUG_STOP();

    CLOCK_DEBUG_ON(clock 확인2 :  , 1);
    test();
    test();
    CLOCK_DEBUG_STOP();
}
*/
//****************************************************************************************//