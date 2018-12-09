#pragma once
#include <iostream>
#include <chrono>
#include "../Base/BaseClass/CSingleton.h"
#include "Base/BaseClass/CThreadPool.h"
//#include "../../Base/include/CThreadPool.h"
//class test;
namespace TestCase {
	class test :public Base::BaseClass::CSingleton<test> {
	public:
		friend Base::BaseClass::CSingleton<test>;
		void print() {
			std::cout << "print test" << std::endl;
		}
	private:
		//public:
		test() {
			std::cout << "test" << std::endl;
		}
		//virtual ~test();//私有析构函数， 对象只能通过指针形式访问
	};

	void TestThreadPool() {
		std::cout << "Start Test ThreadPool" << std::endl;
		//Base::BaseClass::ThreadPool pool;
		Base::BaseClass::CThreadTask<std::function<int()>> tass;
		//function<int()> tt= []() {
		//	std::cout << "hello luo" << std::endl;
		//	std::this_thread::sleep_for(std::chrono::seconds(1));
		//	std::cout << "world luo" << std::endl;
		//	return 250;
		//};
		//tass.SetTaskFun([]() {
		//	std::cout << "hello luo2" << std::endl;
		//	std::this_thread::sleep_for(std::chrono::seconds(1));
		//	std::cout << "world luo2" << std::endl;
		//	return 250;
		//});
		//tass.Run();
		//auto ret = tass.GetResponse();
		std::vector< std::future<int> > results;
	/*	Base::BaseClass::CThreadPool::Instance()->enqueue([]() {
			std::cout << "hello luo"  << std::endl;
			std::this_thread::sleep_for(std::chrono::seconds(1));
			std::cout << "world luo"  << std::endl;
			return;
		});*/
		std::mutex aa;
		//aa.unlock();
		//aa.unlock();
		for (int i = 0; i < 8; ++i) {
			results.emplace_back(
				Base::BaseClass::CThreadPool::Instance()->enqueue([i]() {
				std::cout << "hello " << i << std::endl;
				std::this_thread::sleep_for(std::chrono::seconds(1));
				std::cout << "world " << i << std::endl;
				return i * i;
			})
			);
		}
		//Base::BaseClass::CThreadPool::Instance()->Start();
		for (auto && result : results)    //通过future.get()获取返回值
			std::cout << result.get() << ' ';
		std::cout << std::endl;

		getchar();
	}
}
