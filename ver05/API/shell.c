#include <stdio.h>
#include <unistd.h>
#include <linux/tty.h>
#include <linux/sched.h>
#include <asm/segment.h>
#include <asm/system.h>

#define CTYPE_CHAR			1
#define CTYPE_SPACE			2
#define CTYPE_END			3
#define CTYPE_ORIG			100
#define IS_FRONT			1
#define NOT_FRONT			2

static void format_cmd_buf(char * buf, int * len_p)
{
	int i, j, len;
	char ch;
	int char_t_01, char_t_02; 
	
	char_t_01 = CTYPE_ORIG;
	char_t_02 = CTYPE_ORIG;
	i = 0;
	j = 1024;
	len = *len_p;
	
	while (j < (len + 1024))
	{
		ch = buf[j];
		if ((0x20 == ch) || ('\t' == ch))
		{
			j++;
			continue;
		}
		else if ('\n' == ch)
		{
			(*len_p) = 0;
			return;
		}
		else
		{
			buf[i++] = ch;
			char_t_01 = CTYPE_CHAR;
			j++;
			break;
		}
	}
	
	while (j < len + 1024)
	{
		ch = buf[j];
		if ((0x20 == ch) || ('\t' == ch))
		{
			char_t_02 = CTYPE_SPACE;
		}
		else if ('\n' != ch)
		{
			char_t_02 = CTYPE_CHAR;
		}
		else
		{
			char_t_02 = CTYPE_END;
		}
		
		if (CTYPE_SPACE == char_t_02)
		{
			char_t_01 = char_t_02;
			j++;
			continue;
		}
		else if (CTYPE_END == char_t_02)
		{
			buf[i] = 0;
			*len_p = i;
			return;
		}
		else if (CTYPE_CHAR == char_t_01)
		{
			char_t_01 = char_t_02;
			buf[i++] = ch;
			j++;
		}
		else
		{
			char_t_01 = char_t_02;
			buf[i++] = 0x20;
			buf[i++] = ch;
			j++;
		}
	}
	
	return;
}

extern void shell_manage(char * cmd_buf, int len)
{
	int  tmp;

	format_cmd_buf(cmd_buf, &len);
	if (!len)
	{
		goto fast_car;
	}
	else;
		
	if (!strcmp(cmd_buf, "mem") || !strcmp(cmd_buf, "memory"))
	{
		show_mem();
	}
	else if (!strcmp(cmd_buf, "time"))
	{
		print_sys_time();
	}
	else if (!strcmp(cmd_buf, "system") || !strcmp(cmd_buf, "sys"))
	{
		print_sys_info();
	}
	else if (!strcmp(cmd_buf, "task"))
	{
		show_state();
	}
	else if (!strcmp(cmd_buf, "clear") || !strcmp(cmd_buf, "cls"))
	{
		clear();
		return;
	}
	else if (!strcmp(cmd_buf, "hd0"))
	{
		print_hd_info(0);
	}
	else if (!strcmp(cmd_buf, "hd1"))
	{
		print_hd_info(1);
	}

	else if (!strncmp(cmd_buf, "ls", 2))
	{
		if (!cmd_buf[2])
		{
			ls(0);
		}
		else if (!strcmp(cmd_buf + 2, " -l"))
		{
			ls(1);
		}
		else if (0x20 == cmd_buf[2])
		{
			printf("command is supported, and parameter is unknown.\n");
		}
		else
		{
			printf("unknown command.\n");
		}
	}
	else if (!strncmp(cmd_buf, "dir", 3))
	{
		if (!cmd_buf[3])
		{
			ls(0);
		}
		else if (!strcmp(cmd_buf + 3, " -l"))
		{
			ls(1);
		}
		else if (0x20 == cmd_buf[3])
		{
			printf("command is supported, and parameter is unknown.\n");
		}
		else
		{
			printf("unknown command.\n");
		}
	}
	else if (!strncmp(cmd_buf, "cd ", 3) && cmd_buf[3])
	{
		chdir(cmd_buf + 3);
	}
	else if (!strncmp(cmd_buf, "chdir ", 6) && cmd_buf[6])
	{
		chdir(cmd_buf + 6);
	}
	else if (!strncmp(cmd_buf, "mkdir ", 6) && cmd_buf[6])
	{
		mkdir(cmd_buf + 6, S_IFDIR | 0777);
		kill(2, SIGCONT);
		pause();
	}
	else if (!strncmp(cmd_buf, "rmdir ", 6) && cmd_buf[6])
	{
		rmdir(cmd_buf + 6);
		kill(2, SIGCONT);
		pause();
	}
	else if (!strcmp(cmd_buf, "pwd"))
	{
		pwd();
		printf("\n");
	}
	else if (!strcmp(cmd_buf, "reboot") || !strcmp(cmd_buf,
			 "reset"))
	{
		reboot();
	}
	else
	{
		printf("unknown command.\n");
	}

fast_car:	
	printf(SHELL_INFO);
	return;
}

