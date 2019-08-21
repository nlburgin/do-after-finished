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

inline static long syscall3(const long callnumber,const long arg1,const long arg2,const long arg3){
  register long *number asm ("rax") = callnumber;
  register long *one asm ("rdi") = arg1;
  register long *two asm ("rsi") = arg2;
  register long *three asm ("rdx") = arg3;
  asm volatile ("syscall"
    : "=r" (number)
    : "r" (number),"r"(one),"r"(two),"r"(three)
    : "rcx","r8","r9","r11","r10"
  );
  return number;
}
#endif

inline static void beep(const char *beepchar){
#ifdef __amd64__
 syscall3(SYS_write,1,(long)beepchar,1);
#else
  write(1,&beepchar,1);
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


inline static int showusage() {fputs("usage: alarm pre-delay beep-delay repetitions\nzero repetitions means beep infinitely\n",stderr); return 1;}


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

static void beep_forever(const long lolnope,const char *beepchar,const struct timespec *delay){
  for(;;){
    beep(beepchar);
    our_sleep(delay);
  }
}

static void beep_x_times(const long x,const char *beepchar,const struct timespec *delay){
  for(long i = 0; i < x; i++){
    beep(beepchar);
    our_sleep(delay);
  }
  exit(0);
}


int main(const int argc,char *argv[]){
  
  char **argv2 = argv;

  if (argc < 4)
    return showusage();
    
  const struct timespec pre_delay = parse_time(*++argv2);
  if (pre_delay.tv_nsec < 0)
    return showusage();
  
  const struct timespec beep_delay = parse_time(*++argv2);
  if (beep_delay.tv_nsec < 0)
    return showusage();
    
  const char beepchar = 0x07;
  static void (*beep_fn)(const long repetitions,const char *beepchar,const struct timespec *delay);  
  const long repetitions = our_strtol(*++argv2);
  if (repetitions == 0)
    beep_fn = beep_forever;
  else if (repetitions > 0)
    beep_fn = beep_x_times;
  else
    return showusage();
    
  our_sleep(&pre_delay);
  beep_fn(repetitions,&beepchar,&beep_delay);
}
