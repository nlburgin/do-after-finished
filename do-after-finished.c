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

//on 64-bit PC, use raw system calls in main loop to minimize cache overhead of polling to the lowest possible amount

#ifdef __amd64__
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

//stuff outside the main loop just uses regular library calls


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

//returns zero if command exists, nonzero otherwise
static int check_command_exists(const char *command){
  char buf[128];
  snprintf(buf,128,"command -v %s >/dev/null",command);
  return system(buf);
}

int main(const int argc,char *argv[]){
  
  char **argv2 = argv;

  if (argc < 5)
    return showusage();
    
  const struct timespec polldelay = parse_time(*++argv2);
  if (polldelay.tv_nsec < 0)
    return showusage();
  
  const struct timespec afterdelay = parse_time(*++argv2);
  if (afterdelay.tv_nsec < 0)
    return showusage();
    
  const long pid = strtol(*++argv2,NULL,10);
  if (pid == 0) 
    return showusage();
  if (test_pid(pid) == ESRCH){
    fprintf(stderr,"process %li doesn't exist to begin with. aborting...\n",pid);
    return 2;
  }
  
  const char *command = *++argv2;
  if(check_command_exists(command) != 0){
    fprintf(stderr,"error: command %s seems to not exist\n",command);
    return 3;
  }
  
  fprintf(stderr,"ready to run %s %s seconds after process %li has finished; checking every %s seconds\a\n",command,argv[2],pid,argv[1]);

  //main loop
  while (test_pid(pid) != ESRCH) {
    our_sleep(&polldelay);
  }

  fprintf(stderr,"process %li has finished; %s is ready to run in %s seconds\a\n",pid,command,argv[2]);
  our_sleep(&afterdelay);
  execvp(command,argv2);
  
  fprintf(stderr,"failed to execute %s!\n", command);
  return 4;
}
