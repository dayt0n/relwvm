#include <stdlib.h>
#include <stdio.h>

void *gen_guid()
{
	void *guid = malloc(16);
	FILE *random_f = fopen("/dev/random", "r");
	while(fread(guid, 1, 16, random_f) != 16);
	fclose(random_f);
	
	return guid;
}
