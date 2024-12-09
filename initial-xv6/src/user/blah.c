#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"

#define NFORK 10
#define IO 5

void log_mlfq_data(int pid, int elapsed_time) {
  int priority = getpriority();
  printf("MLFQ,%d,%d,%d\n", pid, elapsed_time, priority);
}

int main()
{
  int n, pid;
  int wtime, rtime;
  int twtime = 0, trtime = 0;
  int start_time = uptime();

  for (n = 0; n < NFORK; n++)
  {
    pid = fork();
    if (pid < 0)
      break;
    if (pid == 0)
    {
      int process_start = uptime() - start_time;
      printf("START,%d,%d\n", getpid(), process_start);

      if (n < IO)
      {
        for (int i = 0; i < 200; i++) {
          sleep(1); // IO bound processes
          if (i % 10 == 0) {
            log_mlfq_data(getpid(), uptime() - start_time);
          }
        }
      }
      else
      {
        for (volatile int i = 0; i < 1000000000; i++)
        {
          if (i % 10000000 == 0) {
            log_mlfq_data(getpid(), uptime() - start_time);
          }
        } // CPU bound process
      }
      printf("END,%d,%d\n", getpid(), uptime() - start_time);
      exit(0);
    }
  }
  for (; n > 0; n--)
  {
    if (waitx(0, &wtime, &rtime) >= 0)
    {
      trtime += rtime;
      twtime += wtime;
    }
  }
  printf("Average rtime %d,  wtime %d\n", trtime / NFORK, twtime / NFORK);
  exit(0);
}