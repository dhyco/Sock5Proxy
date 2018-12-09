#include "Client.h"
#include <string.h>
#include <unistd.h>
#include <future>
#include <functional>
#include <thread>
#define BUFFER_SIZE 1024
#define SOCKS_BUF 1024
#define byte unsigned char
	
namespace SSClinet {
	static std::mutex Glock;
	Client::~Client(){
		if (m_listener) {
            evconnlistener_free(m_listener);
        }   
	    if(m_base){
	        event_base_free(m_base);
	    }
	}
	bool Client::Init() {
		log_i("Init");
		//设置socket属性
		sockaddr_in& local = this->m_clentSocket.attribute; // listen to local
		memset(&local, 0, sizeof(sockaddr_in));
		local.sin_family = AF_INET;
		local.sin_port = htons(this->m_conf.client_port);
		local.sin_addr.s_addr = htonl(INADDR_ANY);


		//event_base *base = NULL;
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
		this->Init();
		//m_future = std::async(std::launch::async, std::bind(&Client::Init, this));
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
		//bufev = bufferevent_socket_new(client_obj->m_base, fd, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS);
		
		log_d("new client conncted ");
		//struct event *evfifo2 = event_new(client_obj->m_base, fd, EV_READ|EV_PERSIST, (ClientReadFdCallBackFunc)callback_func_reomte, NULL);
		
		//event_add(evfifo2, NULL);
		event_base_once(client_obj->m_base, fd, EV_READ,(ClientReadFdCallBackFunc)ParseSock5CallBack1, (void*)client_obj->m_base,NULL);
        //设置回调，socket可读或可写
		//typedef void (*ClientReadCallBackFunc)(bufferevent *, void *);	
	    //typedef void (*ClientReadErrorCallBackFunc)(bufferevent *, short, void *);
		//bufferevent_setcb(bufev, (ClientReadCallBackFunc)ParseSock5CallBack1, NULL, (ClientReadErrorCallBackFunc)ClientReadErrorCallBack, (void*)client_obj);
		//bufferevent_enable(bufev, EV_READ | EV_WRITE);
	}

	void Client::ParseSock5CallBack1(evutil_socket_t fd, short event, void *arg){
		//std::thread(std::bind(&CThreadPool::MasterThread, this))
		
		auto func = [=](){
			//Client *client_obj = (Client*)arg;
			log_t("init sock5 1");
			int nread = 0;
			byte buf[BUFFER_SIZE] = {0};
			nread = read(fd, buf, BUFFER_SIZE);		
			if (nread < 0) {
				if (errno == EAGAIN || errno == EINTR) {
					nread = 0;
				} else {
					close(fd);
					return;
				}
			} else if (nread == 0) {
			close(fd);
			return;
			}

			
			log_t("read %d bytes", nread);
			if ( buf[0] != 0x05) {
				log_e("error 0x05");
				close(fd);
				return;
			}
			byte reply_auth[2] = {0x05, 0x00};
			int nwrite;
			nwrite = write(fd, reply_auth, 2);
			if(nwrite <= 0){
				if(errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN){
					
				}else{
					log_e("client loss %d", fd);
					close(fd);
					return;
				}
			}
			log_t("ParseSock5CallBack1 write %d", nwrite);
			event_base *base = event_base_new();
			
			event_base_once(base, fd, EV_READ,(ClientReadFdCallBackFunc)ParseSock5CallBack2, (void*)base,NULL);
			event_base_dispatch(base);
		};
		event_base_loopbreak((event_base *)arg);
		//bufferevent_setcb(bufev, (ClientReadCallBackFunc)ParseSock5CallBack2, NULL, (ClientReadErrorCallBackFunc)ClientReadErrorCallBack, (void*)client_obj);
		//bufferevent_enable(bufev, EV_READ | EV_WRITE);
		//std::thread t(func);
		//t.detach();
	   	
	}
	
	void Client::ParseSock5CallBack2(evutil_socket_t fd, short event, void *arg){
		
		event_base *base = (event_base *)arg;
		log_t("init sock5 2");
		int nread = 0;
		byte buf[BUFFER_SIZE] = {0};
		nread = read(fd, buf, BUFFER_SIZE);		
		if (nread < 0) {
			if (errno == EAGAIN || errno == EINTR) {
				nread = 0;
			} else {
				close(fd);
				return;
			}
		} else if (nread == 0) {
		    close(fd);
		    return;
		}
		log_t("read %d", nread);
		if ( buf[1] != 0x01) {
			log_e("CONNECT only");
			close(fd);
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
			return;
			break;
		}
		int sock = socket(AF_INET, SOCK_STREAM, 0); 
		
		int yes = 1;
		setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
		int con_result = connect(sock, (struct sockaddr*) remote_addr, sizeof(struct sockaddr));
		 if(con_result < 0){
			 log_e("remote fd error %d", con_result);
			 close(fd);
			 close(sock);
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
		log_t("thread ID: %d", getpid());
		struct CallBackInfo* info1 = new CallBackInfo();
		info1->fd = sock;
		
        struct event* evfifo1 = event_new(NULL, -1, 0, NULL, NULL);
		info1->evfifo =  evfifo1;
		event_assign(evfifo1, base, fd, EV_READ|EV_PERSIST, (ClientReadFdCallBackFunc)callback_func_reomte, (void*)info1);
		event_add(evfifo1, NULL);
		
		
		struct CallBackInfo* info2 = new CallBackInfo();
		info2->fd = fd;
		struct event* evfifo2 = event_new(NULL, -1, 0, NULL, NULL);
		info2->evfifo =  evfifo2;
		event_assign(evfifo2, base, sock, EV_READ|EV_PERSIST, (ClientReadFdCallBackFunc)callback_func_reomte, (void*)info2);
		event_add(evfifo2, NULL);
		
		byte socks5_reply[] = {0x05, 0x00, 0x00, 0x01, 0x00,
					 0x00, 0x00, 0x00, 0x00, 0x00};
		int nwrite = write(fd, socks5_reply, 10);
		log_t("write socks5_reply %d bytes", nwrite);
		/*  // disable linger
		int remotefd = bufferevent_getfd(remote_bev);
		log_t("re fd %d",  remotefd);
		struct linger linger;
		memset(&linger, 0, sizeof(struct linger));
		setsockopt(remotefd, SOL_SOCKET, SO_LINGER, (const void *)&linger,
		sizeof(struct linger));
		//int *fd1 = new int();
		//int *fd2 = new int();
		//*fd1 = bufferevent_getfd(bufev);
		//*fd2 = remotefd;
		//struct event *evfifo1 = event_new(client_obj->m_base, remotefd, EV_READ|EV_PERSIST, (ClientReadFdCallBackFunc)callback_func_reomte, fd1);
		//struct event *evfifo2 = event_new(client_obj->m_base, remotefd, EV_READ|EV_PERSIST, (ClientReadFdCallBackFunc)callback_func_reomte, fd2);
		//event_add(evfifo1, NULL);
		//event_add(evfifo2, NULL);
		bufferevent_setcb(bufev, (ClientReadCallBackFunc)ClientReadCallBack, NULL, (ClientReadErrorCallBackFunc)ClientReadErrorCallBack, (void*)remote_bev);
		bufferevent_enable(bufev, EV_READ | EV_WRITE);
		bufferevent_disable(bufev, EV_PERSIST);

		
		bufferevent_setcb(remote_bev, (ClientReadCallBackFunc)ClientReadCallBack, NULL, (ClientReadErrorCallBackFunc)ClientReadErrorCallBack, (void*)bufev);
		bufferevent_enable(remote_bev, EV_READ| EV_WRITE);
		bufferevent_disable(remote_bev, EV_PERSIST);
		byte socks5_reply[] = {0x05, 0x00, 0x00, 0x01, 0x00,
					 0x00, 0x00, 0x00, 0x00, 0x00};
		n = bufferevent_write(bufev, socks5_reply, 10);
		log_t("write socks5_reply %d bytes", n);		*/	 
	
	}
	void Client::callback_func_reomte(evutil_socket_t fd, short event, void *arg){
		//int errno =0;
		struct CallBackInfo* info = (struct CallBackInfo*)arg;
		int remote_fd = info->fd; 
		log_t("thread ID: %d", getpid());
		log_t("luo test!!");
		while(1){
			byte buf[SOCKS_BUF] = {0};
			int nread = read(fd, buf, 1024);
			log_t("read size %d", nread);
			if (nread < 0) {
				if (errno == EAGAIN || errno == EINTR) {
					nread = 0;
				} else {
					event_free(info->evfifo);
					close(fd);
					close(info->fd);
					return;
				}
			} else if (nread == 0) {
				event_free(info->evfifo);
				close(info->fd);
				close(fd);
				return;
			}
			//printf("%s", buf);
			int nwrite;
            nwrite = write(remote_fd, buf, nread);
			if(nwrite <= 0){
				if(errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN){
					
				}else{
					log_e("client loss %d", fd);
					event_del(info->evfifo);
					event_free(info->evfifo);
				    close(info->fd);
				    close(fd);
					break;
				}
		    }
			//if(nread <=0 && (errno != EINTR || errno != EWOULDBLOCK || errno != EAGAIN)){
				//close(remote_fd);
			//}
			log_t("write1112221 %d bytes", nread);
			break;
		}
	}
	void Client::ClientReadCallBack(bufferevent *bev, void *arg){
		log_t("thread ID: %d", getpid());
		bufferevent *src = bev;
		//Road* tmp = (Road*)arg;
		bufferevent *dst = (bufferevent *)arg;
		//log_t("fd %d to fd %d",  bufferevent_getfd(src), bufferevent_getfd(dst));
	    size_t n = 0;
	    bool first_time = true;
		int remote_fd = bufferevent_getfd(dst);
	    while (1) {
		    byte buf[SOCKS_BUF] = {0};
		    n = bufferevent_read(src, buf, SOCKS_BUF);
		    log_t("ClientReadCallBack read %d bytes", n);
		    if (n <= 0) {
		        log_t("no more data1");
		        if (first_time) {
					log_t("no more start Recycling!!!!");
				    Client::Recycling(src);
					Client::Recycling(dst);
			        return;
		        }
				
				log_t("no more data2");
				break;
		    }
						
		    first_time = false;
			
			log_t("last %d， %d", bufferevent_getfd(dst), n);
			//n = write(remote_fd, buf, n);	
			n = bufferevent_write(dst, buf, n);
				//Client::Recycling(dst);
		    log_t("write1112221 %d bytes", n);
	    }
	}
	
	void Client::ClientReadErrorCallBack(bufferevent *bev, short events, void *arg){
	    if (events && (BEV_EVENT_EOF | BEV_EVENT_ERROR)) {
			log_i("ClientReadErrorCallBack");
            // event_base *base = arg;
            //Client *client = (Client *)arg;
            if (events & BEV_EVENT_EOF) {
                log_d("connection %d closed", bufferevent_getfd(bev));
            } else if (events & BEV_EVENT_ERROR) {
                log_e("got an error on the connection");
            }
			
			if (bev) {
				//LockBuf* tmp = (LockBuf*)arg;
		        //bufferevent *dst = arg->m_bev;
                //Client* cc = (Client*)arg;
				//Client::Recycling(bev); // client->local_bev			
				//Client::Recycling((bufferevent *)arg); 
                //bufferevent_clear_free(client->remote_bev);
                //if (client) {
                // free(client);
                //}
            }
        }
	}
	
	void Client::listener_errorcb(evconnlistener *listener, void *arg){
			if (listener) {
            evconnlistener_free(listener);
        } 
	}
	void Client::Recycling(bufferevent *bev){
		log_t("Client::Recycling %d", bufferevent_getfd(bev));
		if (bev) {
            bufferevent_setcb(bev, NULL, NULL, NULL, NULL);
            bufferevent_free(bev);
			bev = NULL;
		}
	}
}
