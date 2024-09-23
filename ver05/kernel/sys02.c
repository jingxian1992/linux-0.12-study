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

extern int sys_write_simple(const char * buf, int count)
{
	tty_write(0, buf, count);	
	return 100;
}

extern int sys_print_sys_time(void)
{
	int year, moon, day, hour, minute, second;
	second = CMOS_READ(0);
	minute = CMOS_READ(2);
	hour = CMOS_READ(4);
	day = CMOS_READ(7);
	moon = CMOS_READ(8);
	year = CMOS_READ(9);

	BCD_TO_BIN(second);
	BCD_TO_BIN(minute);
	BCD_TO_BIN(hour);
	BCD_TO_BIN(day);
	BCD_TO_BIN(moon);
	BCD_TO_BIN(year);
	
	year = year + 2000;
	
	printk("\tthe current time is: ");
	printk("%d-%d-%d   %d:%d:%d\n", year, moon, day, hour, minute, second);
	
	return 100;
}

extern int sys_print_sys_info(void)
{
	printk("\tcurrent system name: ");
	printk(thisname.sysname);
	printk("\n\tcurrent node name: ");
	printk(thisname.nodename);
	printk("\n\tcurrent release: ");
	printk(thisname.release);
	printk("\n\tcurrent version number: ");
	printk(thisname.version);
	printk("\n\tcurrent machine: ");
	printk(thisname.machine);
	printk("\n");
	
	return 100;
}

extern int sys_print_hd_info(int i)
{
	printk("hard disk %d infomation:\n", i);
	printk("cylender amount: %d\n", hd_info[i].cyl);
	printk("head amount: %d\n", hd_info[i].head);
	printk("start reduce current cylender number: %d\n", hd_info[i].rcur_cyl);
	printk("write pre-compensation cylender number: %d\n", hd_info[i].wpcom);
	printk("biggest ECC length: %d\n", hd_info[i].ECC_len);
	printk("control byte: 0x%02X\n", hd_info[i].ctl);
	printk("standard timeout value: 0x%02X\n", hd_info[i].std_timeout);
	printk("formattint timeout value: 0x%02X\n", hd_info[i].fmt_timeout);
	printk("check timeout value: 0x%02X\n", hd_info[i].check_timeout);
	printk("head loading(stop) cylender number: %d\n", hd_info[i].lzone);
	printk("sector amount per cylender: %d\n", hd_info[i].sect);
	printk("preserve byte: 0x%02X\n", hd_info[i].preserve_byte);
	printk("\n");
	
	return 100;
}

extern int sys_start_shell(void)
{
	int currcons;
	
	for (currcons = 0; currcons < NR_CONSOLES; currcons++)
	{
		change_console(currcons);
		printk(SHELL_INFO);
	}
	change_console(0);
	
	return 100;
}

extern int init_filedir_tree(void);
extern int sys_setup(void)
{
	int i,drive, sync_flags;
	unsigned char cmos_disks;
	struct partition *p;
	struct buffer_head * bh;
	int block_nr, sector_num_lba;
	char * buf;

	if (!callable)
		return -1;
	callable = 0;

	if (hd_info[1].cyl)
		NR_HD=2;
	else
		NR_HD=1;

	for (i=0 ; i<NR_HD ; i++) {
		hd[i*5].start_sect = 0;
		hd[i*5].nr_sects = hd_info[i].head * hd_info[i].sect * hd_info[i].cyl;
	}

	/*
		We querry CMOS about hard disks : it could be that 
		we have a SCSI/ESDI/etc controller that is BIOS
		compatable with ST-506, and thus showing up in our
		BIOS table, but not register compatable, and therefore
		not present in CMOS.

		Furthurmore, we will assume that our ST-506 drives
		<if any> are the primary drives in the system, and 
		the ones reflected as drive 1 or 2.

		The first drive is stored in the high nibble of CMOS
		byte 0x12, the second in the low nibble.  This will be
		either a 4 bit drive type or 0xf indicating use byte 0x19 
		for an 8 bit type, drive 1, 0x1a for drive 2 in CMOS.

		Needless to say, a non-zero value means we have 
		an AT controller hard disk for that drive.

	*/

	if ((cmos_disks = CMOS_READ(0x12)) & 0xf0)
	{
		if (cmos_disks & 0x0f)
			NR_HD = 2;
		else
			NR_HD = 1;
	}
	else
		NR_HD = 0;
	for (i = NR_HD ; i < 2 ; i++) {
		hd[i*5].start_sect = 0;
		hd[i*5].nr_sects = 0;
	}

	for (drive=0 ; drive<NR_HD ; drive++) {
		if (!(bh = bread(0x300 + drive * 5, 0))) {
			printk("Unable to read partition table of drive %d\n\r", drive);
			panic("");
		}
		if (bh->b_data[510] != 0x55 || (unsigned char)
		    bh->b_data[511] != 0xAA) {
			printk("Bad partition table on drive %d\n\r",drive);
			panic("");
		}
		p = 0x1BE + (void *)bh->b_data;
		for (i=1;i<5;i++,p++) {
			hd[i+5*drive].start_sect = p->start_sect;
			hd[i+5*drive].nr_sects = p->nr_sects;
		}
		brelse(bh);
	}

	for (i=0 ; i<5*MAX_HD ; i++)
		hd_sizes[i] = hd[i].nr_sects>>1 ;
	blk_size[MAJOR_NR] = hd_sizes;
	if (NR_HD)
		printk("Partition table%s ok.\n\r",(NR_HD>1)?"s":"");

	mount_root();
	sync_flags = init_filedir_tree();
	if (sync_flags)
	{
		sync();
	}
	else;

	return 100;
}

extern int sys_ls(int flags)
{
	struct m_inode * dir;
	int entries;
	int block, i, j;
	struct buffer_head * bh;
	struct dir_entry * de;
	struct super_block * sb;
	
	dir = current->pwd;
	entries = dir->i_size / (sizeof (struct dir_entry));
	i = 0;
	j = 0;
	
	block = dir->i_zone[0];
	if (!block)
	{
		panic("dir block 0 is 0. it's wrong.");
	}
	else;
	
	bh = bread(dir->i_dev, block);
	if (!bh)
	{
		panic("can not read the buffer head.");
	}
	else;
	
	de = (struct dir_entry *)bh->b_data;
	
	while (i < entries)
	{
		if (j >= DIR_ENTRIES_PER_BLOCK)
		{
			brelse(bh);
			bh = NULL;
			block = bmap(dir, i/DIR_ENTRIES_PER_BLOCK);
			if (!block)
			{
				i += DIR_ENTRIES_PER_BLOCK;
				continue;
			}
			else;				
			
			bh = bread(dir->i_dev, block);
			if (!bh)
			{
				panic("can not read the buffer head.");
			}
			else;
			
			j = 0;			
			de = (struct dir_entry *)bh->b_data;
		}
		else;
		
		if (de->inode)
		{
			if (!flags)
			{
				printk(de->name);
				printk("\t");
			}
			else
			{
				printk("directary entry name: %s\t\t", de->name);
				printk("inode No: %d\n", de->inode);
			}
		}
		else;
			
		de++;
		i++;	
		j++;
	}
	
	if (!flags)
	{
		printk("\n");
	}
	else;
	
	brelse(bh);
	return 100;
}

extern int sys_pwd(void)
{
	struct m_inode *cur_dir, *parent_dir, *child_dir;
	int dev, block_nr;
	struct buffer_head *bh01, *bh02, *tmp_bh;
	void * buf_addr;
	char * dirname_buf;
	char ** dir_name;
	int dir_count;
	struct dir_entry *de01, *de02;
	int parent_i_num, cur_i_num, child_i_num;
	int i, j, argc, length;
	
	buf_addr = (void *)get_free_page();
	dirname_buf = (char *)buf_addr;
	dir_name = (char **)(dirname_buf + 2048);
	dirname_buf[1023] = 0;
	i = 1022;
	
	cur_dir = current->pwd;
	cur_i_num = cur_dir->i_num;
	if (cur_i_num == ROOT_INO)
	{
		dirname_buf[i] = '/';
		printk(dirname_buf + i);
		goto plane;
	}
	
	dev = cur_dir->i_dev;
	block_nr = cur_dir->i_zone[0];
	bh01 = bread(dev, block_nr);
	de01 = (struct dir_entry *)bh01->b_data;
	de01++;
	parent_i_num = de01->inode;
	brelse(bh01);
	parent_dir = iget(dev, parent_i_num);

	do
	{
		child_dir = cur_dir;
		child_i_num = child_dir->i_num;
		if (child_i_num != current->pwd->i_num)
		{
			iput(child_dir);
		}
		else;

		cur_dir = parent_dir;
		cur_i_num = cur_dir->i_num;
		dev = cur_dir->i_dev;
		block_nr = cur_dir->i_zone[0];
		bh01 = bread(dev, block_nr);
		de01 = (struct dir_entry *)bh01->b_data;
		de01++;
		parent_i_num = de01->inode;
		brelse(bh01);
		parent_dir = iget(dev, parent_i_num);
		
		bh02 = find_entry_by_inum(cur_dir, child_i_num, &de02);
		if (!bh02)
		{
			panic("can not read directary entry.");
		}
		else;
		
		length = 0;
		while (de02->name[length])
		{
			length++;
		}

#ifdef NO_TRUNCATE
	if (length > NAME_LEN)
		return NULL;
#else
	if (length > NAME_LEN)
		length = NAME_LEN;
#endif
		
		while (length > 0)
		{
			length--;
			dirname_buf[i] = de02->name[length];
			i--;
		}
		dirname_buf[i--] = '/';
		brelse(bh02);
	} while (cur_i_num != ROOT_INO || parent_i_num != ROOT_INO);
	
	iput(cur_dir);
	iput(cur_dir);
	/*****************************************************
		上面的代码释放的是根目录。注意，这仅仅是因为，在最后两次
	  do-while 循环里面，我们通过iget()函数多获取了两次i节点，增加
	  了两次根目录节点的引用计数。所以上句代码是将最后两次迭代所增加的
	  引用技术给减去，并非是说要彻底释放根节点。根节点，是在系统初始化
	  的时候，由 mount_root() 负责读取到内存中的。
		**************************************************/
	
	/*******************************************************
		普通的i节点都有上面目录来包含它的i节点号和目录项名字。唯有根节点
	  没有这样的上层目录。因为，根节点是至高无上的。对于根目录，我们需要
	  单独地来处理。遇到了，直接来保存'/'。
	  保存好了以后，就要打印路径名了。
	********************************************************/
	printk(dirname_buf + i + 1);
plane:	
	free_page(buf_addr);
	return 100;	
}

extern struct buffer_head * find_entry_by_inum(
  struct m_inode * cur_dir, int child_i_num,
  struct dir_entry ** res_dir)
{
	int entries;
	int block, i, j;
	struct buffer_head * bh;
	struct dir_entry * de;
	struct super_block * sb;

	i = 0;
	j = 0;

	entries = cur_dir->i_size / (sizeof (struct dir_entry));

	if (!(block = cur_dir->i_zone[0]))
		return NULL;
	if (!(bh = bread(cur_dir->i_dev, block)))
		return NULL;

	de = (struct dir_entry *) bh->b_data;
	while (i < entries) {
		if (j >= DIR_ENTRIES_PER_BLOCK) {
			brelse(bh);
			bh = NULL;
			if (!(block = bmap(cur_dir, i / DIR_ENTRIES_PER_BLOCK)) ||
			    !(bh = bread(cur_dir->i_dev,block))) {
				i += DIR_ENTRIES_PER_BLOCK;
				continue;
			}
			else;
			
			j = 0;
			de = (struct dir_entry *) bh->b_data;
		}
		else;
		
		if (de->inode == child_i_num) {
			*res_dir = de;
			return bh;
		}
		else;

		de++;
		i++;
		j++;
	}
	
	brelse(bh);
	return NULL;
}

extern int sys_malloc_page(void ** addr)
{
	unsigned int * page_dir;
	unsigned int * page_table;
	int i, j;
	unsigned int base_addr, line_addr, phy_addr;
	unsigned int tmp;
	
	base_addr = current->pid * 0x40;
	page_dir = (unsigned int *)base_addr;
	tmp = *(unsigned int *)0x40;
	
	for (i = 0; i < 16; i++)
	{
		if (!page_dir[i])
		{
			tmp = get_free_page();
			if (!tmp)
			{
				free_page(tmp);
				oom();
			}
			else;
			
			tmp |= 7;
			page_dir[i] = tmp;
		}
		else;
		
		phy_addr = page_dir[i] & 0xFFFFF000;
		page_table = (unsigned int *)phy_addr;
		for (j = 0; j < 1024; j++)
		{
			if (!page_table[j])
			{
				tmp = get_free_page();
				if (!tmp)
				{
					free_page(tmp);
					oom();
				}
				else;
			
				tmp |= 7;
				page_table[j] = tmp;

				line_addr = (i << 22) + (j << 12);
				verify_area((void *)addr, 4);
				put_fs_long(line_addr, (unsigned int *)addr);
				return 100;
			}
			else;
		}
	}
	
	oom();
}

extern int sys_wait_return(void)
{
	int currcons;
	struct tty_queue * queue;
	char ch;
	
	currcons = fg_console;
	queue = tty_table[currcons].secondary;
	
	if (EMPTY(queue))
	{
		return 0;
	}
	else;

	ch = LAST(queue);
	if ('\n' != ch)
	{
		return 0;
	}
	else
	{
		return 100;
	}
}

extern int sys_read_simple(char * user_buf)
{
	int currcons, length;
	struct tty_queue * queue;
	char ch;
	int i;

	currcons = fg_console;
	queue = tty_table[currcons].secondary;
	length = CHARS(queue);
	verify_area(user_buf, length);
	
	for (i = 0; i < length; i++)
	{
		GETCH(queue,ch);
		put_fs_byte(ch, user_buf + i);
	}
	
	return length;
}

extern int sys_clear(void)
{
	csi_J(fg_console, 1);
	gotoxy(fg_console, 0, 0);
	set_cursor(fg_console);
	printk(SHELL_INFO);
}

extern int init_filedir_tree(void)
{
	int res, i;
	
	res = 0;

	i = sys_mkdir("bin", S_IFDIR | 0777);
	if (!i)
	{
		res = 1;
	}
	else;

	i = sys_mkdir("boot", S_IFDIR | 0777);
	if (!i)
	{
		res = 1;
	}
	else;
	
	i = sys_mkdir("var", S_IFDIR | 0777);
	if (!i)
	{
		res = 1;
	}
	else;
	
	i = sys_mkdir("opt", S_IFDIR | 0777);
	if (!i)
	{
		res = 1;
	}
	else;

	i = sys_mkdir("usr", S_IFDIR | 0777);
	if (!i)
	{
		res = 1;
	}
	else;

	i = sys_mkdir("dev", S_IFDIR | 0777);
	if (!i)
	{
		res = 1;
	}
	else;

	i = sys_mkdir("etc", S_IFDIR | 0777);
	if (!i)
	{
		res = 1;
	}
	else;

	i = sys_mkdir("home", S_IFDIR | 0777);
	if (!i)
	{
		res = 1;
	}
	else;

	i = sys_mknod("/dev/hda", S_IFBLK | 0777, 0x0300);
	if (!i)
	{
		res = 1;
	}
	else;

	i = sys_mknod("/dev/tty1", S_IFCHR | 0777, 0x400);
	if (!i)
	{
		res = 1;
	}
	else;
	
	return res;	
}
