#pragma once
#include <mutex>
#include <memory>
#include <iostream>
namespace Base {
	namespace BaseClass {
		template<class T>
		class CSingleton {
		public:
			//static void aaa();
			static T* Instance();
			static void destroy();
		protected:
			CSingleton();
			virtual ~CSingleton();
		private:

			CSingleton(const CSingleton<T>&) {};
			CSingleton<T>& operator=(const CSingleton<T>&) {};
			static std::unique_ptr<T> m_Instance;
			static std::mutex g_mtx;
		};
		template<class T>
		std::unique_ptr<T> CSingleton<T>::m_Instance;
		template<class T>
		std::mutex CSingleton<T>::g_mtx;

		template<class T>
		CSingleton<T>::CSingleton()
		{
			//do something  
		}
		template<class T>
		CSingleton<T>::~CSingleton()
		{
			//do something  
		}

		template<class T>
		T* CSingleton<T>::Instance() {
			if (NULL == m_Instance) {
				g_mtx.lock();
				if (NULL == m_Instance) {
					//m_Instance = new Singleton();
					std::unique_ptr<T> p(new T());
					m_Instance = std::move(p);
				}
				g_mtx.unlock();
			}
			return m_Instance.get();
		}

		template<class T>
		void CSingleton<T>::destroy()
		{
			if (m_Instance) {
				g_mtx.lock();
				if (m_Instance) {
					m_Instance.release();
					//m_Instance = NULL;
				}
				g_mtx.unlock();
			}
		}
	}
}
