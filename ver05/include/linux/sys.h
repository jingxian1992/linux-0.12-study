#ifndef _SYS_H_
#define _SYS_H_

#include <sys/types.h>

extern int sys_fork(void);
extern int sys_write_simple(const char * buf, int count);
extern int sys_pause(void);
extern int sys_print_sys_time(void);
extern int sys_print_sys_info(void);
extern int sys_print_hd_info(int i);
extern int sys_start_shell(void);
extern int sys_setup(void);
extern int sys_ls(int flags);
extern int sys_pwd(void);
extern int sys_chdir(const char * filename);
extern int sys_mkdir(const char * pathname, int mode);
extern int sys_rmdir(const char * name);
extern int sys_sync(void);
extern int sys_malloc_page(void ** addr);
extern int sys_wait_return(void);
extern int sys_read_simple(char * user_buf);
extern int sys_clear(void);
extern int sys_show_state(void);
extern int sys_show_mem(void);
extern int sys_kill(int pid,int sig);
extern int sys_write(int fd, const char * buf, off_t count);
extern int sys_exit(int error_code);
extern int sys_close(int fd);
extern int sys_dup(int fd);
extern int sys_execve(const char * file, char ** argv, char ** envp);
extern int sys_open(const char * filename,int flag,int mode);
extern pid_t sys_setsid(void);
extern pid_t sys_waitpid(pid_t pid, int * wait_stat, int options);
extern int sys_reboot(void);

#endif
