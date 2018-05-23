/*unix下的计时器实现文件*/
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include "UnixTimer.h"

//static const double numTicksPerSec = (double)(sysconf(_SC_CLK_TCK));

Timer::Timer()
{
  start_time = 0.0;
  end_time = 0.0;
}

void Timer::start()
{
 struct timeval tmp;
 gettimeofday(&tmp, NULL);
 start_time = (tmp.tv_sec)*1000000+tmp.tv_usec; 
 // struct tms clkticks;
  
  
  //times(&clkticks);
  //start_time =
   // ((double) clkticks.tms_utime + (double) clkticks.tms_stime) /
    //numTicksPerSec;
}

void Timer::stop()
{
  struct timeval tmp;
  gettimeofday(&tmp, NULL);
  end_time = (tmp.tv_sec)*1000000+tmp.tv_usec;
  //struct tms clkticks;

  //times(&clkticks);
  //end_time =
   // ((double) clkticks.tms_utime + (double) clkticks.tms_stime) /
    //numTicksPerSec;
}

double Timer::get_intermediate()
{
  struct timeval tmp;
  gettimeofday(&tmp, NULL); 
  double intermediate = (tmp.tv_sec)*1000000+tmp.tv_usec; 
  //struct tms clkticks;

  //times(&clkticks);
  //double intermediate = 
   // ((double) clkticks.tms_utime + (double) clkticks.tms_stime) /
    //numTicksPerSec;
  return (intermediate - start_time)/1000;
}

double Timer::get_duration()
{
  return(end_time - start_time)/1000;
}
