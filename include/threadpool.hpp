// 작성자 : 곽동환
// 이메일 : arbiter1225@gmail.com
// License : GPLv3
// 요약 : 멀티코어 프로그래밍을 위한 쓰래드풀 기본형
// 최종 수정일 : 230522

#ifndef THREADPOOLIMPL_HPP
#define THREADPOOLIMPL_HPP

#define MAX_THREAD 64
#include <iostream>
#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <functional>
#include <memory>
#include <future>
#include <condition_variable>
#include <atomic>
using namespace std::literals;

// 만약 task를 function 혹은 다른 종류의 queue를 사용하고 싶으면 생성 인자에 넣어야한다.
// 고찰 : 작업순서를 우선시하던, FIFO이던 결국 한가지 방식만 사용해야한다.
// queue가 처음 설정하면 굳이 바뀔 필요는 없다.
// pool_therad 는 한번 만들면 바꿀 필요가 없다. 굳이 정책이 앞으로 더 추가된다고
// 하더라도 cuda , 일반 , RT 정도 추가될 것이다. 이를 범용성화 시킬 필요가 있나..?
// RT를 만들려면 RT thread를 결국 만들어야한다.
// 이럴거면 RT pool이랑 일반 pool이랑 분리하는 것이 맞을 듯 하다.

// ************ 해야할것 ***************//
// 각각의 thread가 작업중, 작업 대기, 스래드 사망을 알려주는 vector 하나는 만들어 주면 좋을 듯.
// 이때 작업 사망과 대기를 vector를 함부로 건들지 않는 mutex가 필요할 수도?
// 여기서 mutex보다는 atomic으로 ++와 --를 사용하여 바로 변경하도록 한다.
// 이떄 atomic bool의 list? 사용해보기?  > 이거 작업중 대기 재대로 나오는지 확인해보기

namespace kcpp
{
	template <typename TASK = std::function<void()>, typename QUEUE = std::queue<TASK>, typename THREAD = std::thread>
	class ThreadPool
	{
	private:
		std::vector<THREAD> v;
		QUEUE task_q;
		std::mutex m;
		std::condition_variable cv;
		bool fStop = false;
		bool fempty_break = true;
		std::atomic<bool> is_ready[MAX_THREAD]; // thread가 사용가능한지 확인, true 사용가능 , false 사용불가능

	public:
		ThreadPool(int cnt, bool fempty_break = true) : fempty_break(fempty_break)
		{
			if (cnt > MAX_THREAD)
				cnt = MAX_THREAD;
			for (int i = 0; i < cnt; i++)
			{
				is_ready[i].store(true, std::memory_order_release);
				v.emplace_back(&ThreadPool::_PoolThreadMain, this, i);
			}
		}
		ThreadPool(bool fempty_break = true)
		{
			int cnt = std::thread::hardware_concurrency() - 1;
			if (cnt > MAX_THREAD)
				cnt = MAX_THREAD;
			if (cnt < 1)
				cnt = 1;
			for (int i = 0; i < cnt; i++)
			{
				is_ready[i].store(true, std::memory_order_release);
				v.emplace_back(&ThreadPool::_PoolThreadMain, this, i);
			}
		}
		~ThreadPool()
		{
			{
				std::lock_guard<std::mutex> g(m);
				fStop = true;
			}
			cv.notify_all();
			for (auto &t : v)
				t.join();
		}
		//static create function
		static ThreadPool* create() {return new ThreadPool;}

		template <typename... _BoundArgs>
		static ThreadPool* create(_BoundArgs &&...args) {return new ThreadPool(std::forward<_BoundArgs>(args)...);}

		// if flag "fempty_break" is true, queue is empty => close thread;
		void Setfempty_break(bool flag) { fempty_break = flag; }
		// Stop ThreadPool
		void Stop() { delete this; }
		// get workers (thread cnt)
		int GetThreadCnt() { return v.size(); }
		// get thread status | input thread # , ex) 0,1,2,3...
		int GetThreadStatus(int cnt)
		{
			if (cnt > v.size() || cnt < 0)
				return false;
			return _IsThreadReady(cnt);
		}

		template <typename _Func, typename... _BoundArgs>
		inline decltype(auto) AddTask(_Func &&task, _BoundArgs &&...args)
		{
			auto ret = __AddTask(std::bind(task, std::forward<_BoundArgs>(args)...));
			return ret;
		}

	private:
		template <typename F, typename... ARGS>
		decltype(auto) __AddTask(F task, ARGS &&...args)
		{
			using RT = decltype(task(std::forward<ARGS>(args)...));
			auto p = std::make_shared<std::packaged_task<RT()>>(std::bind(task, std::forward<ARGS>(args)...));
			std::future<RT> ret = p->get_future();
			{
				std::lock_guard<std::mutex> g(m);
				task_q.push([p]()
							{ (*p)(); });
			}
			cv.notify_one();
			return ret;
		}

		void _PoolThreadMain(int i)
		{
			is_ready[i].store(true, std::memory_order_release);
			while (true)
			{
				is_ready[i].store(false, std::memory_order_release);
				TASK task;
				{
					std::unique_lock<std::mutex> ul(m);
					cv.wait(ul, [this]()
							{ return fStop || !task_q.empty(); });

					if (fStop == true && _IsEmptyBreak())
					{
						break;
					}
					task = task_q.front();
					task_q.pop();
				}
				task();
				is_ready[i].store(true, std::memory_order_release);
			}
		}

		// check fempty_break
		bool _IsEmptyBreak() { return fempty_break && task_q.empty(); }
		void _Initialize_is_ready()
		{
			for (int i = 0; i < MAX_THREAD; i++)
				is_ready[i].store(false, std::memory_order_release);
		}
		bool _IsThreadReady(int cnt)
		{
			if (cnt >= MAX_THREAD)
				return false;
			if (cnt < 0)
				cnt = 0;
			return is_ready[cnt].load(std::memory_order_acquire);
		}

	}; // ThreadPool
}

#endif

// THREADPOOLIMPL_HPP
//****************************************************************************************//
// your work function
/*
#include<threadpool.hpp>
int add(int a, int b)
{
   std::this_thread::sleep_for(5s);
   return a + b;
}
using namespace kcpp;
*/
//****************************************************************************************//
// example 1 get result ASAP
// 기본사용법. 스래드 풀은 작업이 없으면 작업완료와 함께 죽어버립니다. 
/*
int main()
{
   ThreadPool tp(3);

   std::future<int> ft = tp.AddTask(add, 1, 2);  //add 함수 int a = 1, int b = 2를 넣어준다.
   std::cout << "continue main" << std::endl;
   int ret = ft.get(); //std::future<int>에서 get 객채를 이용하여 데이터 획득
   std::cout << ret << std::endl;
   ft = tp.AddTask(add, 1, 2);
   ret = ft.get();
   std::cout << ret << std::endl;
}
*/

//****************************************************************************************//
// example 2 get result vectors
// 결과를 vector 형태로 받아서 사용하기 .
/*
int main()
{
	ThreadPool tp;
	int a = 1;
	std::vector<std::future<int>> vout;
	std::cout << "input number : " << std::endl;
	while(a != 0)
	{
		std::cin >> a;
		if(a != 0 )	{ vout.emplace_back(tp.AddTask(add, a, 2)); }
	}
	for (auto& m:vout) 	{ std::cout << m.get() << std::endl; }
}
*/
//****************************************************************************************//
// example 3 threadpool is not die until main function is die
// 스래드 풀이 메인스래드가 죽거나 Stop을 부르기 전까지는 안죽습니다. 
/*
int main()
{
	ThreadPool tp(false);
	int a = 1;
	std::cout << "total thread cnt : " << tp.GetThreadCnt() << std::endl;

	while (true)
	{
		std::cout << "insert number" << std::endl;
		{
			std::vector<std::future<int>> vout;
			while (a != 0)
			{
				std::cin >> a;
				if (a != 0)
				{
					vout.emplace_back(tp.AddTask(add, a, 2));
				}
			}
			for (auto &m : vout)
			{
				std::cout << "result :" << m.get() << std::endl;
			}
		}
		std::cout << "again? (if want start enter 0) : ";
		std::cin >> a;
		if (a == 0)
			break;
	}
}
*/
//****************************************************************************************//
// example 4 with class memberfunction
// 클래스 맴버 함수를 작업으로 등록시키는방법
/*
#include <functional>
#include "headers.h"
#include <iostream>
class add_class
{
	int a, b;
	public:
	add_class() { a= 0; b = 0;}
	add_class(int a, int b) : a(a), b(b) {}

	int add(int x, int y)
	{
		std::this_thread::sleep_for(2s);
		return x + y;
	}
};
int main()
{
	ThreadPool tp(false);
	int a = 1;
	add_class add_instance;

	std::cout << "total thread cnt : " << tp.GetThreadCnt() << std::endl;
	std::cout <<  tp.GetThreadStatus(2) << std::endl; //number 2 thread status , 0 = false 1= true;

	while (true)
	{
		std::cout << "insert number" << std::endl;
		{
			std::vector<std::future<int>> vout;
			while (a != 0)
			{
				std::cin >> a;
				std::cin >> a;
				if (a != 0)
				{
					vout.emplace_back(tp.AddTask(&add_class::add, &add_instance,2,a));
					//vout.emplace_back(tp.AddTask(add,2,a));
				}
			}
			for (auto &m : vout)
			{
				std::cout << "result :" << m.get() << std::endl;
			}
		}
		std::cout << "again? (if want start enter 0) : ";
		std::cin >> a;
		if (a == 0)
			break;
	}
}
*/
//****************************************************************************************//
// example 5 use of static member pointer 
// static 으로 객체를 생성하고 싶을때 <>를 써줘야 한다. 
/*
#include "headers.h"
using namespace kcpp;
int main()
{
	ThreadPool<>* tp = ThreadPool<>::create(false);
	int a = 1;
	add_class add_instance;

	std::cout << "total thread cnt : " << tp->GetThreadCnt() << std::endl;
	std::cout <<  tp->GetThreadStatus(2) << std::endl; //number 2 thread status , 0 = false 1= true;

	while (true)
	{
		std::cout << "insert number" << std::endl;
		{
			std::vector<std::future<int>> vout;
			while (a != 0)
			{
				std::cin >> a;
				std::cin >> a;
				if (a != 0)
				{
					vout.emplace_back(tp->AddTask(&add_class::add, &add_instance,2,a));
					//vout.emplace_back(tp.AddTask(add,2,a));
				}
			}
			for (auto &m : vout)
			{
				std::cout << "result :" << m.get() << std::endl;
			}
		}
		std::cout << "again? (if want start enter 0) : ";
		std::cin >> a;
		if (a == 0)
			break;
	}
}
*/