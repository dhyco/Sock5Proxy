#include <cstdio>
#include "Client.h"
#include "Base/BaseClass/CThreadPool.h"
int main()
{
    printf("hello from ConsoleApplication1!\n");
	SSClinet::Config conf;
	conf.client_port = 1080;
	conf.clinet_address = "192.168.23.132";
	conf.EnTpye = 1;
	SSClinet::Client test(conf);
	test.Start();
     printf("exit---------------!\n");
     //while(1)	
	getchar();
    return 0;
}
