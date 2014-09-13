#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include "lwvmedit.h"


//hexDump is not mine.
/*void hexDump (char *desc, void *addr, int len) {
    int i;
    unsigned char buff[17];
    unsigned char *pc = (unsigned char*)addr;

    // Output description if given.
    if (desc != NULL)
        printf ("%s:\n", desc);

    // Process every byte in the data.
    for (i = 0; i < len; i++) {
        // Multiple of 16 means new line (with line offset).

        if ((i % 16) == 0) {
            // Just don't print ASCII for the zeroth line.
            if (i != 0)
                printf ("  %s\n", buff);

            // Output the offset.
            printf ("  %04x ", i);
        }

        // Now the hex code for the specific character.
        printf (" %02x", pc[i]);

        // And store a printable ASCII character for later.
        if ((pc[i] < 0x20) || (pc[i] > 0x7e))
            buff[i % 16] = '.';
        else
            buff[i % 16] = pc[i];
        buff[(i % 16) + 1] = '\0';
    }

    // Pad out last line if not exactly 16 characters.
    while ((i % 16) != 0) {
        printf ("   ");
        i++;
    }

    // And print the final ASCII bit.
    printf ("  %s\n", buff);
}*/

char *lwvm_name_to_str(char *name)
{
	char *output = malloc(LwVM_NAME_LEN + 1);
	
	output[0] = (char) name[0];
	
	if (output[0] == 0)
	{
		free(output);
		return 0;
	}
	
	int i;
	for (i = 1; i < LwVM_NAME_LEN && name[i * 2] != 0; i++)
	{
		output[i] = (char) name[i * 2];
	}
	
	output[i] = 0;
	
	return output;
}

int print_pt(FILE *img_f)
{
	printf("Listing partitions...\n");
	
	struct _LwVM *LwVM = malloc(sizeof(*LwVM));
	fread(LwVM, 1, sizeof(*LwVM), img_f);
	if (memcmp(LwVM->type, LwVMType, sizeof(LwVMType)) != 0)
	{
		printf("LwVM is probably damaged, ");
		if (ignore_errors) printf("continuing anyway...\n");
		else
		{
			printf("exiting...\n");
			return 1;
		}
	}
	
	int i;
	for (i = 0; i < 12; i++)
	{
		char *part_name = lwvm_name_to_str(&LwVM->partitions[i].partitionName);
		
		if (part_name == 0) break;
		
		printf("\nPartition %i:\n", i + 1);
		
		printf("-Name: %s\n", part_name);
		
		printf("-Encryption: ");
		if (*(&LwVM->partitions[i].attribute) == 0x1000000000000) printf("yes\n");
		else printf("no\n");
		
		if (human_readable)
		{
			printf("-Begin: %fGB (%fMB)\n", (double) *(&LwVM->partitions[i].begin) / 1024 / 1024 / 1024, (double) *(&LwVM->partitions[i].begin) / 1024 / 1024);
			printf("-End: %fGB (%fMB)\n", (double) *(&LwVM->partitions[i].end) / 1024 / 1024 / 1024, (double) *(&LwVM->partitions[i].end) / 1024 / 1024);
			printf("-Begin: %fGB (%fMB)\n", (double) (*(&LwVM->partitions[i].end) - *(&LwVM->partitions[i].begin)) / 1024 / 1024 / 1024, (double) (*(&LwVM->partitions[i].end) - *(&LwVM->partitions[i].begin)) / 1024 / 1024);
		}
		else
		{
			printf("-Begin: %lluB\n", *(&LwVM->partitions[i].begin));
			printf("-End: %lluB.\n", *(&LwVM->partitions[i].end));
			printf("-Size: %lluB.\n", *(&LwVM->partitions[i].end) - *(&LwVM->partitions[i].begin));
		}
		
		free(part_name);
	}
	
	return 0;
}

void help(const char *exec_path)
{
	printf("lwvmedit v0.1\n");
	printf("Usage: %s [OPTIONS] [FILE]\n", exec_path);
	
	printf("Options:\n");
	printf(" -h, --help		show this help text and exit.\n");
	printf(" -v, --version		show version and exit.\n");
	printf(" -l, --list		list partitions.");
	printf(" -e, --ignore-errors	ignore errors, some fatals can still interrupt execution.\n");
}

void version()
{
	printf("lwvmedit v0.1\n");
	printf("Copyright (C) 2014 Roman Zhikharevich.\n");
	printf("This software is licensed under the BSD 3-Clause license (http://opensource.org/licenses/BSD-3-Clause).\n");
	printf("This is free software: you are free to change and redistribute it.\n");
	printf("There is NO WARRANTY, to the extent permitted by law.\n");
	printf("Thanks to: iDroid project.\n");
}

int main(int argc, const char *argv[])
{
	if (argc < 2)
	{
		help(argv[0]);
		return 1;
	}
	
	int fn_arg = 0;
	
	int action = 0;
	
	int i;
	for (i = 1; i < argc; i++)
	{
		if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help"))
		{
			help(argv[0]);
			return 0;
		}
		else if (!strcmp(argv[i], "-v") || !strcmp(argv[i], "--version"))
		{
			version();
			return 0;
		}
		else if (!strcmp(argv[i], "-l") || !strcmp(argv[i], "--list"))
		{
			action = A_LIST;
		}
		else if (!strcmp(argv[i], "-e") || !strcmp(argv[i], "--ignore-errors"))
		{
			ignore_errors = true;
		}
		else if (!strcmp(argv[i], "-H") || !strcmp(argv[i], "--human-readable"))
		{
			human_readable = true;
		}
		else if (argv[i][0] != '-')
		{
			fn_arg = i;
		}
		else
		{
			printf("Unknown argument: %s\n", argv[i]);
			help(argv[0]);
			return 1;
		}
	}
	
	if (action == 0)
	{
		help(argv[0]);
		return 1;
	}
	
	FILE *img_f = fopen(argv[fn_arg], "r+");
	if (img_f == 0)
	{
		printf("Can't open file: %s\n", argv[fn_arg]);
		return 1;
	}
	
	if (action == 1)
	{
		return print_pt(img_f);
	}
	
	
	return 0;
}
