#include <stdio.h>
#include <linux/sched.h>
#include <string.h>
#include <asm/io.h>
#include <asm/system.h>
#include <asm/segment.h>
#include <unistd.h>
#include <linux/tty.h>
#include <fcntl.h>

#ifndef MAJOR_NR
#define MAJOR_NR 3
#endif
#include <linux/blk.h>

#define ORIG_SWAP_DEV *(unsigned short *)0x90000
#define ORIG_ROOT_DEV *(unsigned short *)0x90002

extern void start_kernel(void);
extern void con_init(void);
extern void trap_init(void);
extern void tty_init(void);
extern void chr_dev_init(void);
extern void time_init(void);
extern void inode_table_init(void);
extern void file_table_init(void);
extern void super_block_init(void);
extern void system_info_init(void);
extern void task_init(void);

void main(void)
{
	pg_dir = (unsigned int *)0;
	idt = (struct desc_struct *)0x13000;
	gdt = (struct desc_struct *)0x13800;
	start_kernel();
	return;
}

static int memory_end;
static int buffer_memory_end;
static int main_memory_start;

void start_kernel(void)
{
	int i, sum;
	int res;

	SWAP_DEV = ORIG_SWAP_DEV;
	ROOT_DEV = ORIG_ROOT_DEV;
	memory_end = 0x4000000;
	buffer_memory_end = 0xD00000;
	main_memory_start = buffer_memory_end;
	
	con_init();
	mem_init(main_memory_start, memory_end);
	trap_init();
	blk_dev_init();
	tty_init();
	chr_dev_init();
	time_init();
	sched_init();
	buffer_init(buffer_memory_end);
	hd_init();
	inode_table_init();
	file_table_init();
	super_block_init();
	file_table_init();
	system_info_init();
	sti();
	
	printk("\twelcome to Dumpling OS.\n");
	printk("\tmemory volume: %dM\n", memory_end/0x100000);
	sys_load_flags = 0;
	move_to_user_mode();
	
	__asm__ __volatile__ (
	"int $0x80\n\t":"=a"(res):"0"(__NR_fork):);

	if (!res)
	{
		task_init();
	}

	for(;;)
		__asm__("int $0x80"::"a" (__NR_pause));
}

extern void sync_task(void);
extern int shell_task(void);
extern void task_init(void)
{
	int i, pid;
	char * cmd_buf;
	void * tmp;
	int res, len;

	setup();
	(void) open("/dev/tty1",O_RDWR,0);
	(void) dup(0);
	(void) dup(0);
	printf("\t%d buffers = %d bytes buffer space\n\r",NR_BUFFERS,
		NR_BUFFERS*BLOCK_SIZE);
	printf("\tFree mem: %d bytes\n\r\n",memory_end-main_memory_start);
	
	while (1)
	{
		res = fork();
		if (res < 0)
		{
			printf("Fork failed in init\r\n");
			continue;
		}
		else
		{
			break;
		}
	}
	if (!res)
	{
		close(0);
		close(1);
		close(2);
		sync_task();
	}
	else;
	
	while (1) {
		if ((pid=fork())<0) {
			printf("Fork failed in init\r\n");
			continue;
		}
		if (!pid) {
			close(0);close(1);close(2);			
			setsid();
			(void) open("/dev/tty1",O_RDWR,0);
			(void) dup(0);
			(void) dup(0);
			shell_task();
		}
		while (1)
			if (pid == wait(&i))
				break;
		printf("\n\rchild %d died with code %04x\n\r",pid,i);
		sync();
	}
	_exit(0);	/* NOTE! _exit, not exit() */
}

extern void sync_task(void)
{	
	while (1)
	{
		pause();
		kill(3, SIGCONT);
		sync();
	}
}

extern int shell_task(void)
{
	char * cmd_buf;
	void * tmp;
	int res, len;

	malloc_page(&tmp);
	cmd_buf = (char *)tmp;
	start_shell();

	while(1)
	{
		res = wait_return();
		if (res)
		{
			len = read_simple(cmd_buf + 1024);
			shell_manage(cmd_buf, len);
		}
		else;
	}

	
	return -1;
}

