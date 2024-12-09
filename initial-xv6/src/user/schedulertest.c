#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"

#define NFORK 10 // Total processes
#define IO 5     // Number of I/O-bound processes

int main(int argc, char *argv[])
{
  int n, pid;
  int wtime, rtime;
  int twtime = 0, trtime = 0;
  //int start_time = uptime();

  for (n = 0; n < NFORK; n++)
  {
    pid = fork();
    if (pid < 0)
      break;
    if (pid == 0)
    {
      //int process_start = uptime() - start_time;
      //int p = getpid();
      //printf("START,%d,%d\n", p, process_start);
      if (n < IO)
      {
        for (int i = 0; i < 200; i++)
        {
          sleep(1);
          if (i % 10 == 0)
          {
            // int p = getpid();
            // printf("MLFQ,%d,%d,%d\n", getpid(), uptime() - start_time, getqueue(p));
          }
        }
      }
      else
      {
        for (volatile int i = 0; i < 1000000000; i++)
        {
          if (i % 10000000 == 0)
          {
            // int p = getpid();
            // printf("MLFQ,%d,%d,%d\n", p, uptime() - start_time, getqueue(p));
          }
        }
      }
      //printf("END,%d,%d\n", getpid(), uptime() - start_time);
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

  // Print average times
  printf("Average rtime %d, wtime %d\n", trtime / NFORK, twtime / NFORK);

  // Close the file
  exit(0);
}
