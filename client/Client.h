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
#include "log.h"
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
		struct event* evfifo;
		event_base * base;
	};
	struct Ssocket
	{
		int fd;
		struct sockaddr_in attribute;
	};
	struct Road{
		//std::mutex m_lock_head;
		bufferevent *m_head;
		bool m_flag;
		int m_type_flag;
		bufferevent *m_tail;
	};
	struct LockBuf{
	bufferevent *m_bev;
	std::mutex m_lock;
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
		Ssocket m_clentSocket;
		Ssocket m_serverSocket;
		//libevent
		event_base *m_base; //提包含了events集合并选择事件类型
		evconnlistener *m_listener;//监听对象
		//bufferevent *m_bufev;//检测网络套接字是否已经就绪，可以进行读写操作
		
		/*
		libevent库: evconnlistener_new_bind的回调函数
		arg0:监听对象
		arg1:监听时间触发时，返回的socker对象
		arg2:socket属性
		arg3:socket属性结构的长度
		arg4:回调函数入参
		*/
		typedef void (*ClientReadCallBackFunc)(bufferevent *, void *);	
	    typedef void (*ClientReadErrorCallBackFunc)(bufferevent *, short, void *);	
        typedef void (*ClientReadFdCallBackFunc)(evutil_socket_t, short , void *);		
		static void ListenerCallBack(evconnlistener *listener, evutil_socket_t fd, struct sockaddr *sa, int socklen, void *arg);
		static void ParseSock5CallBack1(evutil_socket_t fd, short event, void *arg);
		static void ParseSock5CallBack2(evutil_socket_t fd, short event, void *arg);
		static void ClientReadCallBack(bufferevent *bev, void *arg);	
		static void ClientReadErrorCallBack(bufferevent *bev, short events, void *arg);
		static void listener_errorcb(evconnlistener *listener, void *arg);
		static void callback_func_reomte(evutil_socket_t fd, short event, void *arg);

		
		
		std::future<bool> m_future;
	};
}
