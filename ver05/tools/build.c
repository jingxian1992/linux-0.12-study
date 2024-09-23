#include <stdio.h>
#include <stdlib.h>

#define FIRST_PART_M	2
#define FIRST_PART_BLOCKNO (FIRST_PART_M * 1024)
#define FIRST_PART_SECTNO (FIRST_PART_M * 2048)

int main(void)
{
	FILE * fp_in, *fp_out;
	char ch;
	int in, out;
	char infile_name[60] = "../Lab/hda.img";
	char outfile_name[60] = "../bootsect.bin";
	
	fp_in = fopen(infile_name, "r");
	fp_out = fopen(outfile_name, "r+");
	if (!fp_in || !fp_out)
	{
		printf("open file fail.\n");
		exit(0);
	}
	else
	{
		printf("open file successfully.\n");
	}
	
	in = FIRST_PART_SECTNO * 512 + 446;
	out = 446;
	
	fseek(fp_in, in, SEEK_SET);
	fseek(fp_out, out, SEEK_SET);
	
	printf("start to deal with the file...\n");
	
	while (out < 510)
	{
		ch = fgetc(fp_in);
		fputc(ch, fp_out);
		out++;
	}
	
	fclose(fp_in);
	fclose(fp_out);
	
	printf("deailing with file job is ok.\n");
	return 0;	
}
