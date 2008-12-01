/*
 * exit()
 *
 * Copyright (C) 2008 Bahadir Balban
 */
#include <task.h>
#include <file.h>
#include <utcb.h>
#include <exit.h>
#include <test.h>
#include <vm_area.h>
#include <syscalls.h>
#include <l4lib/arch/syslib.h>
#include <l4lib/arch/syscalls.h>
#include <l4lib/exregs.h>
#include <l4lib/ipcdefs.h>
#include <lib/malloc.h>
#include <l4/api/space.h>

/*
 * Sends vfs task information about forked child, and its utcb
 */
int vfs_notify_exit(struct tcb *task, int status)
{
	int err = 0;

	// printf("%s/%s\n", __TASKNAME__, __FUNCTION__);

	l4_save_ipcregs();

	/* Write parent and child information */
	write_mr(L4SYS_ARG0, task->tid);
	write_mr(L4SYS_ARG1, status);

	if ((err = l4_sendrecv(VFS_TID, VFS_TID,
			       L4_IPC_TAG_NOTIFY_EXIT)) < 0) {
		printf("%s: L4 IPC Error: %d.\n", __FUNCTION__, err);
		goto out;
	}

	/* Check if syscall was successful */
	if ((err = l4_get_retval()) < 0) {
		printf("%s: VFS returned ipc error: %d.\n",
		       __FUNCTION__, err);
		goto out;
	}

out:
	l4_restore_ipcregs();
	return err;
}


/* Closes all file descriptors of a task */
int task_close_files(struct tcb *task)
{
	int err = 0;

	/* Flush all file descriptors */
	for (int fd = 0; fd < TASK_FILES_MAX; fd++)
		if (task->files->fd[fd].vmfile)
			if ((err = sys_close(task, fd)) < 0) {
				printf("File close error. Tid: %d,"
				       " fd: %d, error: %d\n",
				       task->tid, fd, err);
				break;
			}
	return err;
}

/* Prepare old task's environment for new task */
int execve_recycle_task(struct tcb *task)
{
	int err;
	struct task_ids ids = {
		.tid = task->tid,
		.spid = task->spid,
		.tgid = task->tgid,
	};

	/* Flush all IO on task's files and close fds */
	task_close_files(task);

	/* Vfs still knows the thread */

	/* Keep the utcb on vfs */

	/* Ask the kernel to recycle the thread */
	if ((err = l4_thread_control(THREAD_RECYCLE, &ids)) < 0) {
		printf("%s: Suspending thread %d failed with %d.\n",
		       __FUNCTION__, task->tid, err);
		return err;
	}

	/* Destroy the locally known tcb */
	tcb_destroy(task);

	return 0;
}

void do_exit(struct tcb *task, int status)
{
	struct task_ids ids = {
		.tid = task->tid,
		.spid = task->spid,
		.tgid = task->tgid,
	};

	/* Flush all IO on task's files and close fds */
	task_close_files(task);

	/* Tell vfs that task is exiting */
	vfs_notify_exit(task, status);

	/* Remove utcb shm areas from vfs */
	// printf("Unmapping 0x%p from vfs as utcb of %d\n", task->utcb, task->tid);
	utcb_unmap_from_task(task, find_task(VFS_TID));

	/* Free task's local tcb */
	tcb_destroy(task);

	/* Ask the kernel to delete the thread from its records */
	l4_thread_control(THREAD_DESTROY, &ids);

	/* TODO: Wake up any waiters about task's destruction */
#if 0
	struct tcb *parent = find_task(task->parentid);
	if (parent->waiting) {
		exregs_set_mr_return(status);
		l4_exchange_registers(parent->tid);
		l4_thread_control(THREAD_RUN, parent->tid);
	}
#endif
}

void sys_exit(struct tcb *task, int status)
{
	do_exit(task, status);
}
