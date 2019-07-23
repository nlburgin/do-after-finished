#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <string.h>

//usage: do-after-finished polldelay afterdelay command

#ifdef __amd64__
//on 64-bit PC, use raw system calls in main loop to minimize cache overhead of polling to the lowest possible amount
inline static long syscall2(const long callnumber,const long arg1,const long arg2){
	register long *number asm ("rax") = callnumber;
	register long *one asm ("rdi") = arg1;
	register long *two asm ("rsi") = arg2;
	asm volatile ("syscall"
		: "=r" (number)
		: "r" (number),"r"(one),"r"(two)
		: "rcx","r8","r9","r11","rdx","r10"
	);
	return number;
}
#endif

inline static int test_pid(const long pid){
#ifdef __amd64__
  return -((int) syscall2(SYS_kill,pid,0));
#else
  kill((pid_t)pid,0);
  return errno;
#endif
}

inline static void our_sleep(const struct timespec *seconds){
#ifdef __amd64__
  syscall2(SYS_nanosleep,(long)seconds,NULL);
#else
  nanosleep(seconds,NULL);
#endif
}

inline static int showusage() {puts("usage: do-after-finished polldelay afterdelay PID command [arg]..."); return 1;}

//unlike regular strtol, returns -1 on error.
static long our_strtol(char *str){
  if (strcmp(str,"0") == 0)
    return 0;
  
  const long retval = strtol(str,NULL,10);
  if (retval == 0)
    return -1;
  else 
    return retval;
}

int main(const int argc,char *argv[]){
	if (argc < 5)
    return showusage();
    
	const long polldelay_l = our_strtol(*++argv);
  if (polldelay_l < 0)
    return showusage();
	const struct timespec polldelay = {(time_t)polldelay_l,100000000};
  
  const long afterdelay_l = our_strtol(*++argv);
  if (afterdelay_l < 0)
    return showusage();
	const struct timespec afterdelay = {(time_t)afterdelay_l,100000000};
    
  const long pid = our_strtol(*++argv);
  if (pid < 0) 
    return showusage();
  
  const char *command = *++argv;
  
  printf("ready to run %s %li seconds after process %li has finished; checking every %li seconds\a\n",command,afterdelay.tv_sec,pid,polldelay.tv_sec);

  //main loop
	while (test_pid(pid) != ESRCH) {
		our_sleep(&polldelay);
	}

	printf("process %li has finished; %s is ready to run in %li seconds\a\n",pid,command,afterdelay.tv_sec);
	our_sleep(&afterdelay);
	execvp(command,argv);
  
	printf("failed to execute %s!", command);
	return 2;
}
