#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/reboot.h>

int main()
{
	system("/usr/sbin/lwvmedit --from-file=/usr/lib/lwvmedit/ptmaps/dualboot.map /dev/disk0");
	
	system("/usr/sbin/nvram auto-boot=true"); //Set auto-boot to true to fix booting (otherwise an idevice can go to recovery mode).
	
	reboot(RB_AUTOBOOT); //Reboot.
	
	
	return 0; //Pedantic, but useless.
}
