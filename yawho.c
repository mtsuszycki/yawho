#include "yawho.h"
#include <utmp.h>

#ifndef UTMP_FILE
#define UTMP_FILE 	"/var/run/utmp"
#endif

#ifdef HAVE_USER_IN_UTMP
#define	__user		ut_user
#endif

#ifdef HAVE_NAME_IN_UTMP
#define __user		ut_name
#endif

#define WIDTH_TO_COMMAND 	55

struct utmp ent;

int check_args(char *login, char **argv, int i)
{
	while(argv[i]) {
		if(!strncmp(argv[i], login, UT_NAMESIZE))
			return 1;
		i++;
	}
	return 0;
}

int main(int argc, char **argv)
{
	int all = 0;
	int ssh_users, telnet_users, local_users;
    	int fd, pid, ppid, i, tab_el, longopt = 0;
    	int columns = -1;
    	char login[UT_NAMESIZE + 1], *command;    
	struct winsize win;
	struct procinfo p;
		
	void *tab[] = { 
#ifdef LINUX
		     "(sshd)",   	&ssh_users,
		     "(sshd1)", 	&ssh_users,
		     "(sshd2)",		&ssh_users,
		     "(in.telnetd)", 	&telnet_users,
		     "(init)", 		&local_users
#endif
#ifdef BSDISH
		     "telnetd",		&telnet_users,
		     "sshd", 		&ssh_users,
		     "sshd1",		&ssh_users,
		     "sshd2",		&ssh_users,
		     "init", 		&local_users
#endif
		    };
	if(argc > 1 && argv[1][0] == '-') {
		if(argv[1][1] == 'l') longopt = 1;
		else {
			fprintf(stderr,
				"Unknown option.\nUsage: yawho [-l] [user ...] \n"
				"-l\tLong display (don't cut processes names)\n");
			exit(1);
		}
	}
#ifdef HAVE_SYS_IOCTL_H
	if(ioctl(1,TIOCGWINSZ,&win) != -1) {
		columns = win.ws_col;
		if (columns < 80) {
			fprintf(stderr,"\t####### WARNING #######\n");
			fprintf(stderr,"Screen width less than 80. Output could be unreadable.\n");
		}
	}
#endif

	tab_el = sizeof tab/sizeof(void*);
	for(i = 0; i < tab_el ; i += 2) 
		*(int *)tab[i+1] = 0;
		
    	if((fd = open(UTMP_FILE ,O_RDONLY)) == -1) {
		fprintf(stderr, "Cannot open " UTMP_FILE "\n"); 
		exit(1);
	}

    	printf("DAEMON        USER      TTY    SHP    FROM             WHAT\n");
    
    	while(read(fd, &ent, sizeof(ent))) {
		strncpy(login, ent.__user, UT_NAMESIZE);
		login[UT_NAMESIZE] = '\0';	
#ifdef HAVE_PID_IN_UTMP
		pid = ent.ut_pid;
#endif

#ifdef HAVE_UPROCESS_TYPE
		if(ent.ut_type != USER_PROCESS) continue;
#else
		if(ent.__user[0] == '\0') continue;
#endif
		all++;

#ifndef HAVE_PID_IN_UTMP
		pid = get_login_pid(ent.ut_line);
#endif
		ppid = get_ppid(pid);
 		get_info(ppid, &p);
		do {
			if (p.exec_file[0] != '(') break;
			for(i = 0; i < tab_el ; i += 2)
				if (!strcmp((char *) tab[i], p.exec_file))
					goto found;
			ppid = get_ppid(ppid);
			get_info(ppid, &p);
		} while (ppid != 1);
found:
		if(ppid == 1) {
			ppid = get_ppid(pid);
			get_info(ppid, &p);
		}
		for(i = 0; i < tab_el ; i += 2)
			if (!strcmp((char *) tab[i], p.exec_file))
				( *(int *)tab[i+1] )++;
		if((argc > 1 && !longopt) || argc > 2)
			if(!check_args(ent.__user, argv, longopt + 1))
				continue;

		printf("%-13.13s %-9.9s %-6.6s %-6d %-16.16s ",
	    		p.exec_file, 
	    		login,
	    		ent.ut_line + (strncmp("tty",ent.ut_line,3)?0:3),
	    		pid, 
	    		ent.ut_host
			);
		command = get_command(pid);
		/* don't know real screen width, assume it is 80 */
		if(columns == -1) {
			printf(longopt?"%s\n":"%-24.24s\n", command);
			continue;
		}
		/* screen is really too small so we don't care */
		if(columns <= WIDTH_TO_COMMAND) {
			printf("%s\n", command);
			continue;
		}
		for(i = 0; *(command + i); i++) {
			if(!longopt && i == columns - WIDTH_TO_COMMAND) 
				break;
			printf("%c",*(command+i));
		}
		if(i != columns - WIDTH_TO_COMMAND) printf("\n");
    	}
	close(fd);
	printf("\n%d user%s",all,all?"s ":" ");
	printf ("(%d local, %d telnet, %d ssh, %d other)\n",
		local_users,
		telnet_users ,
		ssh_users, 
		all-telnet_users-ssh_users-local_users
		);
	return 0;
}