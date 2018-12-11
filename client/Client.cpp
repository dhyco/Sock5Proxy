#include "Client.h"
#include <string.h>
#include <unistd.h>
#include <future>
#include <functional>
#include <thread>
#include <signal.h>
#define BUFFER_SIZE 1024
//每次event事件触发，最大可读数量,這個數字太大會出現killed by SIGPIPE，後續定位
#define SOCKS_BUF (1024)
#define byte unsigned char
	
namespace SSClinet {
	static std::mutex Glock;
	Client::~Client(){
		if (m_listener) {
            evconnlistener_free(m_listener);
            m_listener = NULL;
        }   
	    if(m_base){
	        event_base_free(m_base);
	        m_base = NULL;
	    }
	}
	bool Client::Init() {
		log_i("Init");
		auto handle_pipe = [](int sig){};
		struct sigaction action;
		action.sa_handler = handle_pipe;
		sigemptyset(&action.sa_mask);
		action.sa_flags = 0;
		sigaction(SIGPIPE, &action, NULL);



		//设置socket属性
		sockaddr_in local;; // listen to local
		memset(&local, 0, sizeof(sockaddr_in));
		local.sin_family = AF_INET;
		local.sin_port = htons(this->m_conf.client_port);
		local.sin_addr.s_addr = htonl(INADDR_ANY);

		this->m_base = event_base_new();

		typedef void (*ListenerCallBackFunc)(evconnlistener *, evutil_socket_t, struct sockaddr *, int , void *);
        //对于socket操作::创建一个socket,绑定端口和地址，并监听（设置回调函数，有accpet动作触发时调用回调函数）
		this->m_listener = evconnlistener_new_bind(
				 this->m_base, (ListenerCallBackFunc)ListenerCallBack, this, LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE,16, (struct sockaddr *)&local, sizeof(local));
		 //发生错误时触发错误回调
		typedef void (*listener_errorcb_func)(evconnlistener *, void *);
		evconnlistener_set_error_cb(this->m_listener, (listener_errorcb_func)listener_errorcb);

		 event_base_dispatch(this->m_base);
		 std::cout << "Init End" << std::endl;
	}
	bool Client::Start(){
		//this->Init();
		m_future = std::async(std::launch::async, std::bind(&Client::Init, this));
		return true;
	}
	
	void Client::ListenerCallBack(evconnlistener *listener, evutil_socket_t fd, struct sockaddr *sa, int socklen, void *arg){
		Client *client_obj = (Client*)arg;
		bufferevent *bufev = NULL;
		evutil_make_socket_nonblocking(fd);
		struct linger linger;
		setsockopt(fd, SOL_SOCKET, SO_LINGER, (const void *)&linger,
                   sizeof(struct linger));
		//第一次连接上来时，需要协商SOCK5 协议		
		log_d("new client conncted ");
		event_base_once(client_obj->m_base, fd, EV_READ,(ClientReadFdCallBackFunc)ParseSock5CallBack1, (void*)client_obj->m_base,NULL);
	}
    //将socket任务投递到线程池中执行
	void Client::ParseSock5CallBack1(evutil_socket_t fd, short event, void *arg){		
		auto func = [=](){
			//Client *client_obj = (Client*)arg;
			log_t("init sock5 1");
			//資源集中管理，方便釋放
			ResourceManager ResMangObj;
			ResMangObj.m_fd_src = fd;
			int nread = 0;
			byte buf[BUFFER_SIZE] = {0};
			nread = read(fd, buf, BUFFER_SIZE);		
			if (nread < 0) {
				if (errno == EAGAIN || errno == EINTR) {
					//可能需要優化，進行重新讀
					return;
				} else {
					return;
				}
			} else if (nread == 0) {
				return;
			}

			
			log_t("read %d bytes", nread);
			if ( buf[0] != 0x05) {
				log_e("error 0x05");
				return;
			}
			byte reply_auth[2] = {0x05, 0x00};
			int nwrite;
			nwrite = write(fd, reply_auth, 2);
			if(nwrite <= 0){
				if(errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN){
					//可能會導致，連接失效
					return;
				}else{
					log_e("ParseSock5CallBack1 client loss %d", fd);
					return;
				}
			}
			log_t("ParseSock5CallBack1 write %d", nwrite);

			event_base *base = event_base_new();
			ResMangObj.m_base = base;
			event_base_once(base, fd, EV_READ,(ClientReadFdCallBackFunc)ParseSock5CallBack2, (void*)(&ResMangObj),NULL);
			event_base_dispatch(base);

			log_t("fd task finish [fd: %d, tid: %lld]end", fd, Base::GetCurrentThreadID());
		};
	    //考虑根据人物类型优化，线程
		Base::BaseClass::CThreadPool::Instance()->enqueue(func);
	}
	
	void Client::ParseSock5CallBack2(evutil_socket_t fd, short event, void *arg){
		ResourceManager* ResObj = (ResourceManager*)arg;
		event_base *base = ResObj->m_base;
		ResObj->m_fd_src = fd;
		log_t("init sock5 2");
		int nread = 0;
		byte buf[BUFFER_SIZE] = {0};
		nread = read(fd, buf, BUFFER_SIZE);		
		if (nread < 0) {
			if (errno == EAGAIN || errno == EINTR) {
				event_base_once(base, fd, EV_READ,(ClientReadFdCallBackFunc)ParseSock5CallBack2, arg,NULL);
				return;
			} else {
				event_base_loopbreak(base);
				return;
			}
		} else if (nread == 0) {
		    event_base_loopbreak(base);
		    return;
		}
		log_t("read %d", nread);
		if ( buf[1] != 0x01) {
			log_e("CONNECT only");
			event_base_loopbreak(base);
			return;
		}
		int n = nread;
		struct sockaddr_in ss;
		memset(&ss, 0, sizeof(ss));
		struct sockaddr_in* remote_addr = &ss;
		remote_addr->sin_family = AF_INET;
		remote_addr->sin_port = htons(buf[n - 2] << 8 | buf[n - 1]);
		
		int name_len = n - 4 - 2;
		char domain[name_len] = {0};
		log_t("luo port %d", buf[n - 2] << 8 | buf[n - 1]);
		log_t("buf[3]  %d", buf[3]);
		switch (buf[3]){
		//switch (1){
			case 0x01:
			{
				log_w("IP v4");
				in_addr_t ipv4 = buf[4] << 24 | buf[5] << 16 | buf[6] << 8 | buf[7];
				remote_addr->sin_addr.s_addr = htonl(ipv4);
				break;
			}
			case 0x03:
			{
				log_w("domain name");
				for (int i = 1; i < name_len; i++) {
					domain[i-1] = buf[i + 4];
				}
				log_t("ttt1 %s", domain);
				struct hostent *host = gethostbyname(domain);
				memcpy((char*)(&(remote_addr->sin_addr)),host->h_addr_list[0],host->h_length);
				log_t("ttt233 %s %d", inet_ntoa( remote_addr->sin_addr ), strlen(inet_ntoa( remote_addr->sin_addr )) );
				break;
			}
			default:
			log_e("ATYP error");
			event_base_loopbreak(base);
			return;
			break;
		}
		int sock = socket(AF_INET, SOCK_STREAM, 0); 
		ResObj->m_fd_dst = sock;
		int yes = 1;
		setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
		int con_result = connect(sock, (struct sockaddr*) remote_addr, sizeof(struct sockaddr));
		 if(con_result < 0){
			 log_e("remote fd error %d", con_result);
			 event_base_loopbreak(base);
			 return;
		 }
		evutil_make_socket_nonblocking(sock);
		log_t("re fd %d",  sock);
		struct linger linger;
		memset(&linger, 0, sizeof(struct linger));
		setsockopt(sock, SOL_SOCKET, SO_LINGER, (const void *)&linger,
		sizeof(struct linger));
		
		//其事件循环只能运行在一个线程中
		log_d("connected to remote server");
		ResObj->m_dst.fd = sock;
		ResObj->m_dst.base = ResObj->m_base;
        //struct event* 
        ResObj->m_fd_event_src = event_new(NULL, -1, 0, NULL, NULL);

		event_assign(ResObj->m_fd_event_src, base, fd, EV_READ|EV_PERSIST, 
			(ClientReadFdCallBackFunc)callback_func_reomte, (void*)(&(ResObj->m_dst)));
		event_add(ResObj->m_fd_event_src, NULL);
		
		
		ResObj->m_src.fd = fd;
		ResObj->m_src.base = ResObj->m_base;
		//struct event* 
		ResObj->m_fd_event_dst  = event_new(NULL, -1, 0, NULL, NULL);
		event_assign(ResObj->m_fd_event_dst, base, sock, EV_READ|EV_PERSIST, 
			(ClientReadFdCallBackFunc)callback_func_reomte, (void*)(&(ResObj->m_src)));
		event_add(ResObj->m_fd_event_dst, NULL);
		
		byte socks5_reply[] = {0x05, 0x00, 0x00, 0x01, 0x00,
					 0x00, 0x00, 0x00, 0x00, 0x00};
		int nwrite = write(fd, socks5_reply, 10);
		log_t("write socks5_reply %d bytes", nwrite); 
	
	}
	void Client::callback_func_reomte(evutil_socket_t fd, short event, void *arg){
		struct CallBackInfo* info = (struct CallBackInfo*)arg;
		int remote_fd = info->fd; 
		
		byte buf[SOCKS_BUF] = {0};
		int nread = read(fd, buf, SOCKS_BUF);
		log_t("read size %d", nread);
		if (nread < 0) {
			if (errno == EAGAIN || errno == EINTR) {
				nread = 0;
				return;
			} else {
				event_base_loopbreak(info->base);
				return;
			}
		} else if (nread == 0) {
			event_base_loopbreak(info->base);
			return;
		}
		
        for(int i=0;i<10;i++){
			int nwrite;
            nwrite = write(remote_fd, buf, nread);
			if(nwrite <= 0){
				if(errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN){
					continue;
				}else{
					log_e("client loss %d", fd);
					event_base_loopbreak(info->base);
					return;
				}
		    }
		    break;
		}
		log_t("write1112221 %d bytes", nread);	
	}
	
	void Client::listener_errorcb(evconnlistener *listener, void *arg){
			if (listener) {
            evconnlistener_free(listener);
            listener = NULL;
        } 
	}
}
