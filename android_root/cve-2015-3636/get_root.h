#ifndef _GET_ROOT_H
#define _GET_ROOT_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

struct cred;
struct task_struct;
struct thread_info_head;

struct kernel_cap_struct {
	unsigned int  cap[2];
};

struct cred {
	unsigned int usage;
	uid_t uid;
	gid_t gid;
	uid_t suid;
	gid_t sgid;
	uid_t euid;
	gid_t egid;
	uid_t fsuid;
	gid_t fsgid;
	unsigned int securebits;
	struct kernel_cap_struct cap_inheritable;
	struct kernel_cap_struct cap_permitted;
	struct kernel_cap_struct cap_effective;
	struct kernel_cap_struct cap_bset;
	/* ... */
};

struct list_head {
	struct list_head *next;
	struct list_head *prev;
};

struct task_security_struct {
	unsigned long osid;
	unsigned long sid;
	unsigned long exec_sid;
	unsigned long create_sid;
	unsigned long keycreate_sid;
	unsigned long sockcreate_sid;
};


struct task_struct {
	struct list_head cpu_timers[3];
	struct cred *real_cred;
	struct cred *cred;
	struct cred *replacement_session_keyring;
	char comm[16];
};

struct thread_info_head{
	unsigned long		flags;		/* low level flags */
#ifndef __aarch64__
	int					preempt_count;
#endif
	unsigned long		addr_limit;	/* address limit */
	struct task_struct	*task;		/* main task structure */
};

extern int get_root_after_addrlimit_patched(void *_thread_info);

extern void* compute_physmap(void *usr_addr);

extern int patch_selinux_by_change_switch(void *selinux_enforcing,void *selinux_enabled);

#endif