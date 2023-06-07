# yawho

Yawho (Yet Another Who) is a who-like program.

It was written back in the day when some people were still using telnet for remote sessions.

Program shows standard informations about users currently on the machine
(current process, username, tty, "from")
but has two additional columns not seen in other 'w' programs:

DAEMON - shows connection type ie. ssh, telnet
SHP - pid of the login shell (helpful if you are an ugly administrator
	that whishes to kill someone's sesssion ;-)

Compile:
./configure && make
```
Usage: yawho [-l] [username ...]
	-l	long display - don't cut processes names
 
```
