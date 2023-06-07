#include <config.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/ioctl.h>


#define CMDLINE		512
#define	EXEC_FILE	128

struct procinfo {
	int ppid;
	int tpgid;
	int cterm;			/* controling terminal */
	char exec_file[EXEC_FILE];
};	

#ifdef BSDISH
int get_login_pid(char*);
#endif
void get_info(int, struct procinfo*);
int get_ppid(int);
char *get_name(int);
char *get_cmdline(int);
char *get_command(int);

