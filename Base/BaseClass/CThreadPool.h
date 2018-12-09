#pragma once
#include <vector>
#include <list>
#include <queue>
#include <thread>
#include <mutex>
#include <map> 
#include <condition_variable>
#include <future>
#include <functional>
#include <sstream>
#include "CSingleton.h"
#include "Base/OverPlatform/ComputerInfo.hpp"
#include "log/log.h"
namespace Base {
	namespace BaseClass {
		class CThreadPool :public CSingleton<CThreadPool> {
			 class CThreadObj {
			public:
				enum State {
					runing = 0,
					exited,
				};
				CThreadObj(std::thread* t) :m_t(t), m_state(State::runing) {
					m_id = m_t->get_id();
				}

				~CThreadObj() {
					if (m_t->joinable()) {
						m_t->join();
						std::stringstream ss;
						ss << m_id;
						std::string mystring = ss.str();
						log_t((mystring + " End").c_str());
						delete m_t;
						return;
					}
					delete m_t;
				}

				std::thread::id get_id() {
					return m_id;
				}
				void SetState(State s) {
					m_state = s;
				}
				State GetState() {
					return m_state;
				}
			private:
				//std::atomic<State> m_state = State::runing;
				std::atomic<State> m_state;
				std::thread::id m_id;
				std::thread* m_t;
		};
			friend CSingleton<CThreadPool>;
		public:
			CThreadPool();
			template<class F, class... Args>
			auto enqueue(F&& f, Args&&... args)   //任务管道函数
				-> std::future<typename std::result_of<F(Args...)>::type>;  //利用尾置限定符  std future用来获取异步任务的结果
			void Start();
			~CThreadPool();
		private:
			void MasterThread();
			void Thread_Run();
			void CreatThreadWorker();
			// need to keep track of threads so we can join them
			std::vector< std::thread > workers;   //追踪线程
												  // the task queue
			std::map<std::thread::id, CThreadObj> m_workers_map;
			std::mutex m_threadManger_mutex;
			std::condition_variable m_threadManger_condition;
			std::queue< std::function<void()> > tasks;    //任务队列，用于存放没有处理的任务。提供缓冲机制

														  // synchronization  同步？
			struct ThreadsNum {
				std::atomic<size_t> min;
				std::atomic<size_t>  max ;
				std::atomic<size_t>  now ;
				std::mutex mtx;
				ThreadsNum():min(3), max(20), now(0){}
				void AddThreadsNum(size_t n = 1) {
					now += n;
					//std::cout <<"rrrr: " << now << std::endl;
				}
				void DeleteThreadsNum(size_t n = 1) {
					if (min <= now) {
						now -= n;
					}
					//std::cout <<"eeee: " << now << std::endl;
				}
			};
			ThreadsNum m_threads;
			std::mutex queue_mutex;   //互斥锁
			std::condition_variable condition;   //条件变量？
			bool stop;
		};
		// add new work item to the pool
		template<class F, class... Args>
		auto CThreadPool::enqueue(F&& f, Args&&... args)  //&& 引用限定符，参数的右值引用，  此处表示参数传入一个函数
			-> std::future<typename std::result_of<F(Args...)>::type>
		{
			using return_type = typename std::result_of<F(Args...)>::type;
			//packaged_task是对任务的一个抽象，我们可以给其传递一个函数来完成其构造。之后将任务投递给任何线程去完成，通过
			//packaged_task.get_future()方法获取的future来获取任务完成后的产出值
			auto task = std::make_shared<std::packaged_task<return_type()> >(  //指向F函数的智能指针
				std::bind(std::forward<F>(f), std::forward<Args>(args)...)  //传递函数进行构造
				);
			//future为期望，get_future获取任务完成后的产出值
			std::future<return_type> res = task->get_future();   //获取future对象，如果task的状态不为ready，会阻塞当前调用者
			{
				std::unique_lock<std::mutex> lock(queue_mutex);  //保持互斥性，避免多个线程同时运行一个任务
																 // don't allow enqueueing after stopping the pool
				if (stop)
					throw std::runtime_error("enqueue on stopped ThreadPool");
				if (!this->tasks.empty()) {
					this->CreatThreadWorker();
				}
				tasks.emplace([task]() { (*task)(); });  //将task投递给线程去完成，vector尾部压入
			}
			condition.notify_one();  //选择一个wait状态的线程进行唤醒，并使他获得对象上的锁来完成任务(即其他线程无法访问对象)
			return res;
		}//notify_one不能保证获得锁的线程真正需要锁，并且因此可能产生死锁
		template<class F, class... Args>
		class CThreadTask {
			using ReType = typename std::result_of<F(Args...)>::type;
		public:
			CThreadTask():m_IsRun(false) {}
			bool SetTaskFun(F&& f, Args&&... args) {
				if (!m_TaskFun) {
					m_TaskFun = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
					m_IsRun = false;
					return true;
				}
				return false;
			};
			void Run() {
				if (!m_IsRun) {
					m_response = CThreadPool::Instance()->enqueue(m_TaskFun);
					m_IsRun = true;
				}
			}
			ReType GetResponse(){
				return m_response.get();
			}
		//获取结果std::future<int>
		private:
			std::future<ReType> m_response;
			std::function<ReType()> m_TaskFun;
			std::atomic<bool> m_IsRun;
		};
	}
}
