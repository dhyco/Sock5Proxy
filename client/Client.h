#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string>

#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/event.h>
#include <event2/listener.h>
#include <event2/util.h>
#include <iostream>
#include <functional>
#include <thread>
#include <future>
#include <mutex>
#include <unistd.h>
#include "log.h"
#include "Base/BaseClass/CThreadPool.h"
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
// For inet_addr
#include <arpa/inet.h>
#include <netinet/in.h>

#include <sys/socket.h>
#endif

namespace SSClinet {
	struct Config {
		std::string clinet_address;
		unsigned int client_port;
		std::string server_address;
		unsigned int server_port;
		unsigned int EnTpye;
		std::string username;
		std::string passwd;
	};
	struct CallBackInfo{
		int fd;
		event_base * base;
	};
    class ResourceManager{
    public:
    	ResourceManager(): m_fd_src(-1), m_fd_dst(-1), m_base(NULL), m_fd_event_src(NULL), m_fd_event_dst(NULL){}
    	~ResourceManager(){
    		log_t("~ResourceManager End %lld", Base::GetCurrentThreadID());
    		if(m_base != NULL){
    			event_base_free(m_base);
    			m_base = NULL;
    		}
    		if(m_fd_src > 0){
    			close(m_fd_src);
    			m_fd_src = -1;
    		}
    		if(m_fd_dst > 0){
    			close(m_fd_dst);
    			m_fd_dst = -1;
    		}
    		if(m_fd_event_src != NULL){
    		    event_free(m_fd_event_src);
    		    m_fd_event_src = NULL;
    		}
    		if(m_fd_event_dst != NULL){
    		    event_free(m_fd_event_dst);
    		    m_fd_event_dst = NULL;
    		}
    	}
        struct CallBackInfo m_src;
        struct CallBackInfo m_dst;
    	struct event* m_fd_event_src;
    	int m_fd_src;
    	struct event* m_fd_event_dst;
    	int m_fd_dst;
    	event_base * m_base;
    };
	class Client {
	public:
		Client(Config conf) :m_conf(conf) {};
		virtual ~Client();
		bool Start();
		static void Recycling(bufferevent *bev);
	private:
		bool Init();
		bool Listen();
		Config m_conf;
		//libevent
		event_base *m_base; //提包含了events集合并选择事件类型
		evconnlistener *m_listener;//监听对象
		
		/*
		libevent库: evconnlistener_new_bind的回调函数
		arg0:监听对象
		arg1:监听时间触发时，返回的socker对象
		arg2:socket属性
		arg3:socket属性结构的长度
		arg4:回调函数入参
		*/
        typedef void (*ClientReadFdCallBackFunc)(evutil_socket_t, short , void *);		
		static void ListenerCallBack(evconnlistener *listener, evutil_socket_t fd, struct sockaddr *sa, int socklen, void *arg);
		static void ParseSock5CallBack1(evutil_socket_t fd, short event, void *arg);
		static void ParseSock5CallBack2(evutil_socket_t fd, short event, void *arg);
		static void listener_errorcb(evconnlistener *listener, void *arg);
		static void callback_func_reomte(evutil_socket_t fd, short event, void *arg);

		std::future<bool> m_future;
	};
}
