#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <signal.h>
enum {
	IOPRIO_CLASS_NONE,
	IOPRIO_CLASS_RT,
	IOPRIO_CLASS_BE,
	IOPRIO_CLASS_IDLE,
};

enum {
	IOPRIO_WHO_PROCESS = 1,
	IOPRIO_WHO_PGRP,
	IOPRIO_WHO_USER,
};

int read_fd;
int write_fd;

#define change_pgid() setpgid(getpid(),0)

static int _ioprio_set(int which, int who, int prio ){
	return syscall(__NR_ioprio_set,which,who,prio);
}

static int _ioprio_get(int which ,int who){
	return syscall(__NR_ioprio_get,which,who);
}

// one child : fork and exit 
// one child : get
// one child : change pgid --> fork and exit


static void child1(){
	int pid;
	int ret;
	for(;;){
		pid = fork();
		if(pid == 0){
			exit(0);
		}else{
			waitpid(pid,NULL,0);
			//assert(ret == pid);
		}
	}
	exit(0);
}

static void child2(){
	int prio;
	int OK = 1;
	time_t time_start = time(NULL),time_end;
	for(;;){
		prio = _ioprio_get(IOPRIO_WHO_PGRP,0);
	//	printf("child prio : %x\n",prio);
		if(prio != 0x6000  && prio !=4){
			time_end = time(NULL);
			printf("%d\n",time_end-time_start);
			write(write_fd,&OK,sizeof(OK));
		}
		//usleep(100);
	}
	exit(0);
}

static void child3(){
	int ret = change_pgid();
	int pid;
	if(ret == -1){
		perror("child3");
	}
	_ioprio_set(IOPRIO_WHO_PROCESS,0,0);
	for(;;){
		pid = fork();
		if(pid == 0){
			exit(0);
		}else{
			ret = wait(NULL);
			waitpid(pid,NULL,0);
			//assert(ret == pid);
		}
	}
	exit(0);
}

static void child4(){
	int count = 0;
	for(;;count++){
		sleep(1);
		printf("%d\n",count);
	}
	exit(0);
}

int main(){
	int prio;
	int pid[6] = {0};
	int pipe_fd[2];
	if(pipe(pipe_fd) == -1){
		perror("pipe");
		return -1;
	}
	read_fd = pipe_fd[0];
	write_fd = pipe_fd[1];
	_ioprio_set(IOPRIO_WHO_PROCESS,0,0x6000);

	pid[0] = fork();
	if(pid[0] == 0){
		child1();
		//sleep(1);
		// exit(0);
	}

	pid[1] = fork();
	if(pid[1] == 0){
		child2();
		//sleep(1);
		// exit(0);
	}

	pid[2] = fork();
	if(pid[2] == 0){
		child2();
		// exit(0);
	}

	pid[3] = fork();
	if(pid[3] == 0){
		child3();
		//exit(0);
	}

	pid[4] = fork();
	if(pid[4] == 0){
		child3();
		//exit(0);
	}

	pid[5] = fork();
	if(pid[5] == 0){
		child3();
		//exit(0);
	}

	int val,i;
	read(read_fd,&val,sizeof(val));
	printf("read pipe %d\n",val);
	if(val == 1){
		printf("done!\n");


	}
	for(i=0;i<6;i++)
		kill(pid[i],SIGKILL);
	close(read_fd);
	close(write_fd);
	// time_t start,end;
	// start = time(NULL);
	// while(1){
	// 	//_ioprio_set(IOPRIO_WHO_PROCESS,0,0);
	// 	prio = _ioprio_get(IOPRIO_WHO_PGRP,0);
	// 	//printf("father prio :%x\n",prio);
	// 	if(prio != 4 && prio != 0x6000){
	// 		end = time(NULL);
	// 		printf("time spent:%d\n",end-start);
	// 	}
	// 	//usleep(100);
	// }
	return 0;
}