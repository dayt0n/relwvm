#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "lwvmedit.h"


char *lwvm_name_to_str(char *name)
{
	//if (name[0] == 0 || name[1] != 0) return 0;
	
	char *output = malloc(LwVM_NAME_LEN + 1);
	
	int i;
	for (i = 0; i < LwVM_NAME_LEN && name[i * 2] != 0; i++)
	{
		output[i] = (char) name[i * 2];
	}
	
	output[i] = 0;
	
	return output;
}

char *str_to_lwvm_name(char *str)
{
	char *output = malloc(LwVM_NAME_LEN * 2);
	memset(output, 0, LwVM_NAME_LEN * 2);
	
	int i;
	for(i = 0; i < (LwVM_NAME_LEN * 2) && str[i] != 0; i++)
	{
		output[i * 2] = (char) str[i];
	}
	
	return output;
}

int pt_splice(struct _LwVM *LwVM, int part_num)
{
	if (*(&LwVM->numPartitions) < part_num)
	{
		return E_NOPART;
	}
	
	int i;
	for(i = part_num - 1; i < (*(&LwVM->numPartitions) - 1); i++)
	{
		memcpy(&LwVM->partitions[i], &LwVM->partitions[i + 1], sizeof(LwVMPartitionRecord));
	}
	
	memset(&LwVM->partitions[i], 0, sizeof(LwVMPartitionRecord));
	
	*(&LwVM->numPartitions) = *(&LwVM->numPartitions) - 1;
	
	return 0;
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

uint64_t get_param_input()
{
	char input[14];
	fgets(&input[0], 14, stdin);
	
	if (!strcmp(&input[strlen(input) - 3], "MB\n"))
	{
		input[strlen(input) - 3] = 0;
		return atoi(input) * 1024 * 1024;
	}
	else if (!strcmp(&input[strlen(input) - 3], "GB\n"))
	{
		input[strlen(input) - 3] = 0;
		return atoi(input) * 1024 * 1024 * 1024;
	}
	
	return atoi(input);
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
	printf("add	add an empty (disabled) partition (max: 12).\n");
	printf("rm	remove a partition.\n");
	printf("edit	edit a partition entry.");
}

void available_part_types()
{
	printf("Available partition types:\n");
	printf("-HFS\n");
	printf("-Disabled\n");
	printf("\nNew types may be added soon.\n");
}

int edit_pt(struct _LwVM *LwVM, bool pt_no_crc)
{
	char input[CLI_INPUT_BUFF_SZ];
	while(1)
	{
		printf("Command (? for help): ");
		
		fgets(&input[0], CLI_INPUT_BUFF_SZ, stdin);
		
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
				memset(&LwVM->partitions[*(&LwVM->numPartitions)], 0, sizeof(LwVMPartitionRecord));
				
				uint64_t new_guid[2];
				FILE *random_f = fopen("/dev/random", "r");
				while(fread(&new_guid, 1, 16, random_f) != 16);
				
				memcpy(&LwVM->partitions[*(&LwVM->numPartitions)].guid[0], &new_guid[0], 16);
				*(&LwVM->numPartitions) = *(&LwVM->numPartitions) + 1;
				printf("Done.\n");
			}
			else printf("Can't create more than 12 partitions.\n");
		}
		else if (!strcmp(input, "rm\n") || !strcmp(input, "r\n"))
		{
			if (*(&LwVM->numPartitions) == 0)
			{
				printf("Nothing to delete.\n\n");
				continue;
			}
			
			printf("Type the number of the partition you want to remove [1-%u]: ", *(&LwVM->numPartitions));
			
			fgets(&input[0], CLI_INPUT_BUFF_SZ, stdin);
			
			int part_num = atoi(input);
			
			if (pt_splice(LwVM, part_num) == E_NOPART)
			{
				printf("No such partition.\n");
			}
			else
			{
				printf("Done.\n");
			}
		}
		else if (!strcmp(input, "edit\n") || !strcmp(input, "e\n"))
		{
			printf("Type the number of the partition you want to edit [1-%u]: ", *(&LwVM->numPartitions));
			fgets(&input[0], CLI_INPUT_BUFF_SZ, stdin);
			
			int part_to_edit = atoi(input);
			part_to_edit--;
			if (part_to_edit < 12 && part_to_edit >= 0 && part_to_edit < *(&LwVM->numPartitions))
			{
				printf("What parameter do you want to edit? [begin/end/type/name] ");
				fgets(&input[0], CLI_INPUT_BUFF_SZ, stdin);
				
				if (!strcmp(input, "begin\n"))
				{
					printf("Begin [0-%llu(MB/GB)]: ", *(&LwVM->mediaSize));
					
					*(&LwVM->partitions[part_to_edit].begin) = get_param_input();
				}
				else if (!strcmp(input, "end\n"))
				{
					printf("End [0-%llu(MB/GB)]: ", *(&LwVM->mediaSize));
					
					*(&LwVM->partitions[part_to_edit].end) = get_param_input();
				}
				else if (!strcmp(input, "type\n"))
				{
					while(1)
					{
						printf("Enter partition type you want (? for help): ");
						fgets(&input[0], CLI_INPUT_BUFF_SZ, stdin);
						
						if (!strcmp(input, "?\n"))
						{
							available_part_types();
						}
						else if (!strcmp(input, "HFS\n") || !strcmp(input, "hfs\n"))
						{
							memcpy(&LwVM->partitions[part_to_edit].type, LwVMPartitionTypeHFS, sizeof(*LwVMPartitionTypeHFS));
							break;
						}
						else if (!strcmp(input, "disabled\n") || !strcmp(input, "Disabled\n"))
						{
							memcpy(&LwVM->partitions[part_to_edit].type, LwVMPartitionTypeDisabled, sizeof(*LwVMPartitionTypeDisabled));
							break;
						}
						else
						{
							printf("Unknown type: %s\n", input);
							available_part_types();
						}
					}
				}
				else if (!strcmp(input, "name\n"))
				{
					printf("Type new name of the partition (max of 36 characters): ");
					fgets(&input[0], CLI_INPUT_BUFF_SZ, stdin);
					
					input[strlen(input) - 1] = 0; //Remove the \n byte.
					char *lwvm_name = str_to_lwvm_name(input);
					memcpy(&LwVM->partitions[part_to_edit].partitionName, lwvm_name, 0x48);
					
					free(lwvm_name);
				}
				else
				{
					printf("Invalid parameter name.\n");
				}
			}
			else
			{
				printf("Invalid number.\n");
			}
		}
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

void errno_print()
{
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
	else if (errno == EBADF)
	{
		printf("Error: bad file descriptor.\n");
	}
	else
	{
		printf("Unknown error occured (%i).\n", errno);
	}
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
	
	FILE *img_f = fopen(argv[fn_arg], "rb+");
	
	if (img_f == 0)
	{
		printf("Can't open file: %s\n", argv[fn_arg]);
		
		errno_print();
		
		return 1;
	}
	
	struct _LwVM *LwVM = malloc(sizeof(*LwVM));
	fseek(img_f, 0L, SEEK_SET);
	if (fread(LwVM, 1, sizeof(*LwVM), img_f) != 4096)
	{
		printf("Can't read file: %s\n", argv[fn_arg]);
		
		errno_print();
		
		cleanup(img_f, LwVM);
		
		return 1;
	}
	
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
			fseek(img_f, 0L, SEEK_SET);
			if (fwrite(LwVM, 1, sizeof(*LwVM), img_f) != 4096)
			{
				errno_print();
			}
		}
	}
	
	cleanup(img_f, LwVM);
	
	
	return 0;
}
