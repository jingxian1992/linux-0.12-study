/*
 *  linux/kernel/fork.c
 *
 *  (C) 1991  Linus Torvalds
 */

/*
 *  'fork.c' contains the help-routines for the 'fork' system call
 * (see also system_call.s), and some misc functions ('verify_area').
 * Fork is rather simple, once you get the hang of it, but the memory
 * management can be a bitch. See 'mm/mm.c': 'copy_page_tables()'
 */
#include <stdio.h>
#include <errno.h>
#include <linux/sched.h>
#include <asm/segment.h>
#include <asm/system.h>

extern void write_verify(unsigned int address);

int last_pid;

extern void verify_area(void * addr,int size)
{
	unsigned int start;

	start = (unsigned int) addr;
	size += start & 0xfff;
	start &= 0xfffff000;
	start += get_base((void *)(&current->ldt[2]));
	while (size>0) {
		size -= 4096;
		write_verify(start);
		start += 4096;
	}
}

int copy_mem(int nr,struct task_struct * p)
{
	unsigned int old_data_base,new_data_base,data_limit;
	unsigned int old_code_base,new_code_base,code_limit;
	int pid;

	code_limit=get_limit(0x0f);
	data_limit=get_limit(0x17);
	old_code_base = get_base((void *)(&current->ldt[1]));
	old_data_base = get_base((void *)(&current->ldt[2]));
	
	if (old_data_base != old_code_base)
		panic("We don't support separate I&D");
	if (data_limit < code_limit)
		panic("Bad data_limit");
	new_data_base = new_code_base = nr * TASK_SIZE;
	p->start_code = new_code_base;

	set_base((void *)(&p->ldt[1]), (void *)new_code_base);
	set_base((void *)(&p->ldt[2]), (void *)new_data_base);
	
	if (copy_page_tables(old_data_base,new_data_base,data_limit)) {
		free_page_tables(new_data_base,data_limit);
		return -ENOMEM;
	}
	return 0;
}

/*
 *  Ok, this is the main fork-routine. It copies the system process
 * information (task[nr]) and sets up the necessary registers. It
 * also copies the data segment in it's entirety.
 */
int copy_process(int nr,int ebp,int edi,int esi,int gs,int none,
		int ebx,int ecx,int edx, int orig_eax, 
		int fs,int es,int ds,
		int eip,int cs,int eflags,int esp,int ss)
{
	struct task_struct *p;
	int i;
	int num;
	struct file *f;

	p = (struct task_struct *) get_free_page();
	if (!p)
		return -EAGAIN;
	task[nr] = p;

	num = sizeof(struct task_struct);     // I add this self...
//	*p = *current;	/* NOTE! this doesn't copy the supervisor stack */
	for (i = 0; i < num; i++) {
		*((char *)p + i) = *((char *)current + i);
	}

	p->state = TASK_UNINTERRUPTIBLE;
	p->pid = last_pid;
	p->counter = p->priority;
	p->signal = 0;
	p->alarm = 0;
	p->leader = 0;		/* process leadership doesn't inherit */
	p->utime = p->stime = 0;
	p->cutime = p->cstime = 0;
	p->start_time = jiffies;
	p->tss.back_link = 0;
	p->tss.esp0 = PAGE_SIZE + (int) p;
	p->tss.ss0 = 0x10;
	p->tss.eip = eip;
	p->tss.eflags = eflags;
	p->tss.eax = 0;
	p->tss.ecx = ecx;
	p->tss.edx = edx;
	p->tss.ebx = ebx;
	p->tss.esp = esp;
	p->tss.ebp = ebp;
	p->tss.esi = esi;
	p->tss.edi = edi;
	p->tss.es = es & 0xffff;
	p->tss.cs = cs & 0xffff;
	p->tss.ss = ss & 0xffff;
	p->tss.ds = ds & 0xffff;
	p->tss.fs = fs & 0xffff;
	p->tss.gs = gs & 0xffff;
	p->tss.ldt = _LDT(nr);
	p->tss.trace_bitmap = 0x80000000;

	if (copy_mem(nr,p)) {
		task[nr] = NULL;
		free_page((int) p);
		return -EAGAIN;
	}
	for (i=0; i<NR_OPEN;i++)
		if (f=p->filp[i])
			f->f_count++;
	if (current->pwd)
		current->pwd->i_count++;
	if (current->root)
		current->root->i_count++;
	if (current->executable)
		current->executable->i_count++;
	if (current->library)
		current->library->i_count++;

	if (!current->pid)
	{
		p->ldt[1].a = 0x3FFF;
		p->ldt[2].a = 0x3FFF;
	}

	set_tss_desc((void *)(gdt+(nr<<1)+FIRST_TSS_ENTRY), (void *)(&(p->tss)));
	set_ldt_desc((void *)(gdt+(nr<<1)+FIRST_LDT_ENTRY), (void *)(&(p->ldt)));
	p->p_pptr = current;
	p->p_cptr = 0;
	p->p_ysptr = 0;
	p->p_osptr = current->p_cptr;
	if (p->p_osptr)
		p->p_osptr->p_ysptr = p;
	current->p_cptr = p;
	p->state = TASK_RUNNING;	/* do this last, just in case */
	return last_pid;
}

int find_empty_process(void)
{
	int i;

	repeat:
		if ((++last_pid)<0) last_pid=1;
		for(i=0 ; i<NR_TASKS ; i++)
			if (task[i] && ((task[i]->pid == last_pid) ||
				        (task[i]->pgrp == last_pid)))
				goto repeat;
	for(i=1 ; i<NR_TASKS ; i++)
		if (!task[i])
			return i;
	return -EAGAIN;
}


