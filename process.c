#include "yawho.h"

#ifdef BSDISH
#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/proc.h>
#include <sys/stat.h>
#endif

#ifdef HAVE_LIBKVM
#include <kvm.h>
#endif

#ifdef HAVE_KINFO_IN_USER_H
#include <sys/user.h>
#endif

#define elemof(x)	(sizeof (x) / sizeof*(x))
#define endof(x)	((x) + elemof(x))

/*
 * Get process info
 */
#ifdef LINUX
void get_info(int pid, struct procinfo *p)
{
    	char buf[32];
    	FILE *f;

	p->ppid = -1;
	p->cterm = -1;
	strcpy(p->exec_file, "can't access");
    	snprintf(buf, sizeof buf, "/proc/%d/stat", pid);
    	if (!(f = fopen(buf,"rt"))) 
    		return;
    	if(fscanf(f,"%*d %128s %*c %d %*d %*d %*d %d",
    			p->exec_file, &p->ppid, &p->tpgid) != 3)
    		return;
    	fclose(f);	
	p->exec_file[EXEC_FILE-1] = '\0';
}
#endif

#ifdef BSDISH
int fill_kinfo(struct kinfo_proc *info, int pid)
{
	int mib[] = { CTL_KERN, KERN_PROC, KERN_PROC_PID, pid };
	int len = sizeof *info;
	if(sysctl(mib, 4, info, &len, 0, 0) == -1) 
		return -1;
	if(len == 0) 
		 return -1;
	return 0;
}
		
void get_info(int pid, struct procinfo *p)
{
	struct kinfo_proc info;	
	p->ppid = -1;
	p->cterm = -1;
	p->tpgid = -1;
	strcpy(p->exec_file, "can't access");
	
	if(fill_kinfo(&info, pid) == -1) return;
	
    	p->ppid = info.kp_eproc.e_ppid;
    	p->tpgid = info.kp_eproc.e_tpgid;
    	strncpy(p->exec_file, info.kp_proc.p_comm, EXEC_FILE);
    	p->cterm = info.kp_eproc.e_tdev;
	p->exec_file[EXEC_FILE-1] = '\0';
}
#endif

/*
 * Get parent pid
 */
int get_ppid(int pid)
{
	struct procinfo p;
	get_info(pid, &p);
	return p.ppid;
}

#ifdef BSDISH
/*
 * Get terminal
 */
int get_term(char *tty)
{
	struct stat s;
	char buf[32];
	sprintf(buf, "/dev/%s", tty);
	if(stat(buf, &s) == -1) return -1;
	return s.st_rdev;
}

/*
 * Find pid of the process which parent doesn't have control terminal.
 * Hopefully it is a pid of the login shell (ut_pid in Linux)
 */
int get_login_pid(char *tty)
{
	int mib[4], len, t, el, i, pid;
	struct kinfo_proc *info;
	struct procinfo p;
	
	t = get_term(tty);
	if(t == -1) return -1;
	mib[0] = CTL_KERN;
	mib[1] = KERN_PROC;
	mib[2] = KERN_PROC_TTY;
	mib[3] = t;
	if(sysctl(mib, 4, 0, &len, 0, 0) == -1)
		return -1;
	info = malloc(len);
	if(!info) return -1;
	el = len/sizeof(struct kinfo_proc);
	if(sysctl(mib, 4, info, &len, 0, 0) == -1)
		return -1;
	for(i = 0; i < el; i++) {
		pid = info[i].kp_proc.p_pid;
		get_info(get_ppid(pid), &p);
		if(p.cterm == -1 || p.cterm != t) {
			free(info);
			return pid;
		}
	}
	free(info);
	return -1;
}
#endif BSDISH

/* 
 * Return the complete command line for the process
 */
#ifdef LINUX
char *get_cmdline(int pid)
{
        static char buf[512];
	struct procinfo p;
        FILE *f;
        int i = 0;
        sprintf(buf, "/proc/%d/cmdline",pid);
        if (!(f = fopen(buf, "rt")))
                return "-";
        while (fread(buf+i,1,1,f) == 1){
	        if (buf[i] == '\0') buf[i] = ' ';
                if (i == sizeof buf - 1) break;
                i++;
        }
        fclose(f);
	buf[i] = '\0';
	if(!i) {
		get_info(pid, &p);
		bzero(buf, sizeof buf);
		strncpy(buf, p.exec_file, sizeof buf - 1);
	}	
        return buf;
}
#endif

#ifdef BSDISH
char *get_cmdline(int pid)
{
	static char buf[512];
	struct kinfo_proc info;
	
#ifdef HAVE_LIBKVM
	kvm_t *kd;
#endif
	char **p, *s = buf;
	bzero(buf, sizeof buf);
	if(fill_kinfo(&info, pid) == -1)
		return "-";
	strncpy(buf, info.kp_proc.p_comm, sizeof buf - 1);

#ifdef HAVE_LIBKVM
	kd = kvm_openfiles(0, 0, 0, O_RDONLY, 0);
	if(!kd) return buf;
	p = kvm_getargv(kd, &info, 0);
	if(!p) 	return buf;
	for(; *p; p++) {
		*s++ = ' ';	
		strncpy(s, *p, endof(buf) - s);
		s += strlen(*p);
		if(s >= endof(buf) - 1) break;
	}
	buf[sizeof buf - 1] = 0; 
	return buf + 1;
#else
	return buf;
#endif
}
#endif
	 
/* 
 * Get process group ID of the process which currently owns the tty
 * that the process is connected to and return its command line.
 */
char *get_command(int pid)
{
	struct procinfo p;
	get_info(pid, &p);
        return get_cmdline(p.tpgid);
}

