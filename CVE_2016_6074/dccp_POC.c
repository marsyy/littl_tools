#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#ifndef SOCK_DCCP
#define SOCK_DCCP 6
#endif

#ifndef IPPROTO_DCCP
#define IPPROTO_DCCP 33
#endif

#ifndef SOL_DCCP
#define SOL_DCCP 269
#endif

#define IPV6_RECVPKTINFO 49
#define DCCP_PORT 8889

int child_main(){
	int fd;
	int ret;
	int status;
	int on = 1;
	int buf[100];
	int optval = 1;

	fd = socket(AF_INET6,SOCK_DCCP,IPPROTO_DCCP);
	if(fd == -1){
		perror("child  socket");
		return -1;
	}
	struct sockaddr_in6 target_addr;
	memset(&target_addr,0,sizeof(target_addr));
	target_addr.sin6_family = AF_INET6;
	target_addr.sin6_port = htons(DCCP_PORT);

	char *target = "0:0:0:0:0:0:0:1";
	inet_pton(AF_INET6,target,&target_addr.sin6_addr);

	ret = setsockopt(fd, SOL_DCCP, SO_REUSEADDR, (const char *) &on, sizeof(on));
	if(ret == -1){
		perror("child  setsockopt 1");
	}

	ret = setsockopt(fd,SOL_IPV6,IPV6_RECVPKTINFO,&optval,sizeof(optval));
	if(ret == -1){
		perror("child  setsockopt 2");
	}

	ret = connect(fd,(struct sockaddr *)&target_addr,sizeof(target_addr));
	if(ret == -1){
		perror("child  connect ");
	}else{
		do{
			status = send(fd,buf,30,0);
		}while((status < 0) && (errno == EAGAIN));
	}

	close(fd);
	return 0;

}

int father_main(){
	int fd;
	int ret;

	int pid = fork();

	if(pid == 0){
		sleep(1);
		child_main();
		exit(0);
	}else{


		if((fd = socket(AF_INET6,SOCK_DCCP,IPPROTO_DCCP)) == -1){
			perror("socket");
			return -1;
		}

		struct sockaddr_in6 addr , remout_addr;
		memset(&addr,0,sizeof(addr));

		addr.sin6_family = AF_INET6;
		addr.sin6_port = htons(DCCP_PORT);
		addr.sin6_addr = in6addr_any;

		int on = 1;
		ret = setsockopt(fd, SOL_DCCP, SO_REUSEADDR, (const char *) &on, sizeof(on));
		if(ret == -1){
			perror("setsockopt 1");
		}
		int optval = 1;
		ret = setsockopt(fd,SOL_IPV6,IPV6_RECVPKTINFO,&optval,sizeof(optval));
		if(ret == -1){
			perror("setsockopt 2");
		}

		ret = bind(fd,(struct sockaddr *)&addr,sizeof(addr));
		if(ret == -1){
			perror("bind");
		}

		ret = listen(fd,2);
		if(ret == -1){
			perror("listen");
		}
		int buf[20];
		int recv_buf[100];
		int client_fd = accept(fd,(struct sockaddr *)&remout_addr,0);//fack addr

		close(fd);
		waitpid(pid,NULL,0);
	}

	return 0;
}

int main(){
	int i;
	for(i=0; i<100; i++){
		printf(".");
		father_main();
	}
	return 0;
}
