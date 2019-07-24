#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <math.h>

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

inline static int showusage() {fputs("usage: do-after-finished polldelay afterdelay PID command [arg]...\n",stderr); return 1;}


static struct timespec parse_time(char *str){
  if (strcmp(str,"0") == 0 || strcmp(str,"0.0") == 0){
    const struct timespec retval = {0,100000000};
    return retval;
  } 
  const double dtime = strtod(str,NULL);
  if (dtime == 0.0){
    const struct timespec retval = {0,-1}; //this designates an error
    return retval;
  }
  double dint = 0.0;
  const double dfract = modf(dtime,&dint);
  const struct timespec retval = {(time_t)dint,(long)(dfract * 1000000000.0)};
  return retval;
}

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
    
  const struct timespec polldelay = parse_time(*++argv);
  if (polldelay.tv_nsec < 0)
    return showusage();
  
  const struct timespec afterdelay = parse_time(*++argv);
  if (afterdelay.tv_nsec < 0)
    return showusage();
    
  const long pid = our_strtol(*++argv);
  if (pid < 0) 
    return showusage();
  
  const char *command = *++argv;
  
  if (test_pid(pid) == ESRCH){
    fprintf(stderr,"process %li doesn't exist to begin with. aborting...\n",pid);
    return 2;
  }
  
  fprintf(stderr,"ready to run %s %li seconds after process %li has finished; checking every %li seconds\a\n",command,afterdelay.tv_sec,pid,polldelay.tv_sec);

  //main loop
  while (test_pid(pid) != ESRCH) {
    our_sleep(&polldelay);
  }

  fprintf(stderr,"process %li has finished; %s is ready to run in %li seconds\a\n",pid,command,afterdelay.tv_sec);
  our_sleep(&afterdelay);
  execvp(command,argv);
  
  fprintf(stderr,"failed to execute %s!", command);
  return 3;
}
