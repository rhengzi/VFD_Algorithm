/*计时器头文件*/

#ifndef TIMER_H
#define TIMER_H

#include <sys/time.h>

class Timer
{
  double start_time, end_time;
public:
  Timer();
  void start();
  void stop();
  double get_intermediate();
  double get_duration();
};

#endif
