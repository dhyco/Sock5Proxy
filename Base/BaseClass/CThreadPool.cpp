#include "CThreadPool.h"
namespace Base {
	namespace BaseClass {
		// the constructor just launches some amount of workers
		CThreadPool::CThreadPool() :stop(false) {
			//m_threads.min = OverPlatform::GetCpuNum();
			std::thread *t = new std::thread(std::bind(&CThreadPool::MasterThread, this));
			t->detach();
			m_workers_map.emplace(t->get_id(), t);
			this->Start();
		}
		void  CThreadPool::MasterThread() {
			while (true) {
				std::unique_lock<std::mutex> lock(m_threadManger_mutex);
				m_threadManger_condition.wait(lock);
				log_t(("start MasterThread, workers num: " + std::to_string(m_workers_map.size())).c_str());
				auto itr = m_workers_map.begin();
				while (itr != m_workers_map.end()) {
					if (itr->second.GetState() == CThreadObj::State::exited) {
						itr = m_workers_map.erase(itr);
					}
					else {
						itr++;
					}
				}
				log_t(("End MasterThread, workers num: " + std::to_string(m_workers_map.size())).c_str());
				std::this_thread::yield();
			}
		}
		void CThreadPool::Thread_Run() {
			this->m_threads.AddThreadsNum();
			for (;;)
			{
				std::function<void()> task;   //线程中的函数对象
				{//大括号作用：临时变量的生存期，即控制lock的时间
					std::unique_lock<std::mutex> lock(this->queue_mutex);
					if ((this->m_threads.now > this->m_threads.min) && this->tasks.empty())
						break;
					this->condition.wait(lock,
						[this] { 
						if (this->stop || !this->tasks.empty()) {
							return true;
						}
						else {
							std::this_thread::yield();
							return false;
						}
					}
					); //当stop==false&&tasks.empty(),该线程被阻塞 !this->stop&&this->tasks.empty()
					if (this->stop && this->tasks.empty())
						break;
					task = std::move(this->tasks.front());
					this->tasks.pop();
				}
				task(); //调用函数，运行函数
			}
			this->m_threads.DeleteThreadsNum();	
			m_workers_map.find(std::this_thread::get_id())->second.SetState(CThreadObj::State::exited);
			m_threadManger_condition.notify_all();
		}
		void CThreadPool::CreatThreadWorker() {
			if (this->m_threads.max <= this->m_threads.now)
				return;
			//workers.emplace_back(     //以下为构造一个任务，即构造一个线程
			//	std::bind(&CThreadPool::Thread_Run, this)
			//);
			std::thread *t = new std::thread(std::bind(&CThreadPool::Thread_Run, this));
			m_workers_map.emplace(t->get_id(), t);
		}
		void CThreadPool::Start()
		{
			//size_t threads = this->m_threads;
			for (size_t i = 0;i < this->m_threads.min;++i) {
				CreatThreadWorker();
			}
		}
		 // the destructor joins all threads
		inline CThreadPool::~CThreadPool()
		{
			{
				std::unique_lock<std::mutex> lock(queue_mutex);
				stop = true;
			}
			condition.notify_all();  //通知所有wait状态的线程竞争对象的控制权，唤醒所有线程执行
			//for (std::thread &worker : workers)
			//	worker.join(); //因为线程都开始竞争了，所以一定会执行完，join可等待线程执行完
		}
	}
}
