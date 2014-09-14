#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "lwvmedit.h"


char *lwvm_name_to_str(char *name)
{
	if (name[0] == 0 || name[1] != 0) return 0;
	
	char *output = malloc(LwVM_NAME_LEN + 1);
	
	int i;
	for (i = 0; i < LwVM_NAME_LEN && name[i * 2] != 0; i++)
	{
		output[i] = (char) name[i * 2];
	}
	
	output[i] = 0;
	
	return output;
}

void print_pt(struct _LwVM *LwVM, bool pt_no_crc)
{
	printf("LwVM info:\n");
	
	printf("-CRC: ");
	if (pt_no_crc) printf("no\n");
	else printf("yes\n");
	
	if (human_readable) printf("-Size: %fGB (%fMB)\n", (double) *(&LwVM->mediaSize) / 1024 / 1024 / 1024, (double) *(&LwVM->mediaSize) / 1024 / 1024);
	else printf("-Size: %lluB\n", *(&LwVM->mediaSize));
	
	printf("This partition table has %u partition(s).\n", *(&LwVM->numPartitions));
	
	int i;
	int disabled_partitions = 0;
	for (i = 0; i < *(&LwVM->numPartitions); i++)
	{
		printf("\nPartition %i:\n", i + 1);
		
		if (!memcmp(&LwVM->partitions[i].type, LwVMPartitionTypeDisabled, sizeof(*LwVMPartitionTypeDisabled)))
		{
			printf("Partition is disabled.\n");
			continue;
		}
		
		char *part_name = lwvm_name_to_str(&LwVM->partitions[i].partitionName[0]);
		printf("-Name: %s\n", part_name);
		
		printf("-GUID (?): %llx%llx\n", *(&LwVM->partitions[i].guid[1]), *(&LwVM->partitions[i].guid[0]));
		
		printf("-Type: ");
		if (!memcmp(LwVMPartitionTypeHFS, &LwVM->partitions[i].type, sizeof(*LwVMPartitionTypeHFS)))
		{
			printf("HFS\n");
		}
		else
		{
			printf("unrecognized.\n");
		}
		
		printf("-Encryption (?): ");
		if (*(&LwVM->partitions[i].attribute) == 0x1000000000000) printf("yes\n");
		else printf("no\n");
		
		if (human_readable)
		{
			printf("-Begin: %fGB (%fMB)\n", (double) *(&LwVM->partitions[i].begin) / 1024 / 1024 / 1024, (double) *(&LwVM->partitions[i].begin) / 1024 / 1024);
			printf("-End: %fGB (%fMB)\n", (double) *(&LwVM->partitions[i].end) / 1024 / 1024 / 1024, (double) *(&LwVM->partitions[i].end) / 1024 / 1024);
			printf("-Size: %fGB (%fMB)\n", (double) (*(&LwVM->partitions[i].end) - *(&LwVM->partitions[i].begin)) / 1024 / 1024 / 1024, (double) (*(&LwVM->partitions[i].end) - *(&LwVM->partitions[i].begin)) / 1024 / 1024);
		}
		else
		{
			printf("-Begin: %lluB\n", *(&LwVM->partitions[i].begin));
			printf("-End: %lluB.\n", *(&LwVM->partitions[i].end));
			printf("-Size: %lluB.\n", *(&LwVM->partitions[i].end) - *(&LwVM->partitions[i].begin));
		}
		
		free(part_name);
	}
}

void damage_warning()
{
	printf("WARNING: this expiremental tool is very DANGEROUS. You are going to edit the partition table with it. Use it your own risk! The copyright holder is not responsible for any damage this software can do.\n");
	
	char input[3];
	while (strcmp(input, "y\n"))
	{
		printf("Are you sure you want to conitunue? [y/n]");
		fgets(input, 3, stdin);
		if (!strcmp(input, "n\n")) exit(1);
	}
}

void edit_help()
{
	printf("? 	show help.\n");
	printf("print	show partition information.\n");
	printf("quit	quit without saving changes.\n");
	printf("write	save changes and exit.\n");
	printf("add	add an empty (disabled) partition (max: 12).");
}

int edit_pt(struct _LwVM *LwVM, bool pt_no_crc)
{
	char input[10];
	while(1)
	{
		printf("Command (? for help): ");
		
		fgets(&input[0], 10, stdin);
		
		if (!strcmp(input, "?\n") || !strcmp(input, "help\n") || !strcmp(input, "h\n"))
		{
			edit_help();
		}
		else if (!strcmp(input, "print\n") || !strcmp(input, "p\n"))
		{
			print_pt(LwVM, pt_no_crc);
		}
		else if (!strcmp(input, "quit\n") || !strcmp(input, "q\n"))
		{
			break;
		}
		else if (!strcmp(input, "write\n") || !strcmp(input, "w\n"))
		{
			damage_warning();
			return SAVE_CHANGES;
		}
		else if (!strcmp(input, "add\n") || !strcmp(input, "a\n"))
		{
			if (*(&LwVM->numPartitions) < 12)
			{
				*(&LwVM->numPartitions) = *(&LwVM->numPartitions) + 1;
				printf("Done.\n");
			}
			else printf("Can't create more than 12 partitions.\n");
		}
		/*else if (!strcmp(input, "edit\n") || !strcmp(input, "e\n"))
		{
			printf("Type the number of the partition you want to edit [1-12]: ");
			fgets(&input[0], 10, stdin);
			
			int part_to_edit = atoi(input);
			part_to_edit--;
			if (part_to_edit < 12 && part_to_edit >= 0)
		}*/
		else if (strcmp(input, " \n") && strcmp(input, "\n"))
		{
			edit_help();
		}
		
		printf("\n");
	}
	
	return DISCARD_CHANGES;
}

void cleanup(FILE *img_f, struct _LwVM *LwVM)
{
	free(LwVM);
	fclose(img_f);
}

void help(const char *exec_path)
{
	printf("lwvmedit v0.1\n");
	printf("Usage: %s [OPTIONS] [FILE]\n", exec_path);
	
	printf("Options:\n");
	printf(" -h, --help		show this help text and exit.\n");
	printf(" -v, --version		show version and exit.\n");
	printf(" -l, --list		list partitions.\n");
	printf(" -E, --ignore-errors	ignore errors, some fatals can still interrupt execution.\n");
	printf(" -H, --human-readable	output all values in human-readable format.\n");
	printf(" -e, --edit		enter interactive mode.\n");
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
		else if (!strcmp(argv[i], "-E") || !strcmp(argv[i], "--ignore-errors"))
		{
			ignore_errors = true;
		}
		else if (!strcmp(argv[i], "-H") || !strcmp(argv[i], "--human-readable"))
		{
			human_readable = true;
		}
		else if (!strcmp(argv[i], "-e") || !strcmp(argv[i], "--edit"))
		{
			action = A_EDIT;
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
	
	FILE *img_f = fopen(argv[fn_arg], "r+b");
	
	if (img_f == 0)
	{
		printf("Can't open file: %s\n", argv[fn_arg]);
		
		if (errno == EBUSY)
		{
			printf("Error: device or resource busy.\n");
		}
		else if (errno == EPERM)
		{
			printf("Error: operation not permitted.\n");
		}
		else if (errno == EACCES)
		{
			printf("Error: permission denied.\n");
		}
		else
		{
			printf("Unknown error occured (%i).\n", errno);
		}
		
		
		return 1;
	}
	
	struct _LwVM *LwVM = malloc(sizeof(*LwVM));
	fseek(img_f, 0L, SEEK_SET);
	fread(LwVM, 1, sizeof(*LwVM), img_f);
	
	bool pt_no_crc = !memcmp(LwVM->type, LwVMType_noCRC, sizeof(*LwVMType_noCRC));
	if (memcmp(LwVM->type, LwVMType, sizeof(*LwVMType)) != 0 && !pt_no_crc)
	{
		printf("LwVM has unknown type or was damaged, ");
		if (ignore_errors) printf("continuing anyway...\n");
		else
		{
			printf("exiting...\n");
			cleanup(img_f, LwVM);
			return 1;
		}
	}
	
	if (action == A_LIST)
	{
		print_pt(LwVM, pt_no_crc);
	}
	else if (action == A_EDIT)
	{
		int status = edit_pt(LwVM, pt_no_crc);
		if (status == SAVE_CHANGES)
		{
			printf("Writing new LwVM table.\n");
			//fseek(img_f, 0L, SEEK_SET);
			printf("wrote: %lu\n", fwrite(LwVM, 1, sizeof(*LwVM), img_f));
			printf("errno == %i\n", errno);
		}
		else if (status == DISCARD_CHANGES)
		{
			printf("Discarding changes.\n");
		}
	}
	
	cleanup(img_f, LwVM);
	
	
	return 0;
}
