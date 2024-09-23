#include <stdio.h>
#include <linux/sys.h>
#include <linux/tty.h>
#include <linux/sched.h>
#include <sys/utsname.h>
#include <asm/segment.h>
#include <string.h>
#include <errno.h>
#include <linux/config.h>
#include <sys/stat.h>

#ifndef MAJOR_NR
#define MAJOR_NR 3
#endif
#include <linux/blk.h>

extern int sys_reboot(void)
{
	printk("\n\n\treboot the system...\n\n");
	hard_reset_now();
	return 100;
}







