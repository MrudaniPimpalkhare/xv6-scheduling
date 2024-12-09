#include "kernel/types.h"
#include "kernel/syscall.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"

char *sysname(int syscall_num)
{
  switch (syscall_num)
  {
  case SYS_fork:
    return "fork";
  case SYS_exit:
    return "exit";
  case SYS_wait:
    return "wait";
  case SYS_pipe:
    return "pipe";
  case SYS_read:
    return "read";
  case SYS_kill:
    return "kill";
  case SYS_exec:
    return "exec";
  case SYS_fstat:
    return "fstat";
  case SYS_chdir:
    return "chdir";
  case SYS_dup:
    return "dup";
  case SYS_getpid:
    return "getpid";
  case SYS_sbrk:
    return "sbrk";
  case SYS_sleep:
    return "sleep";
  case SYS_uptime:
    return "uptime";
  case SYS_open:
    return "open";
  case SYS_write:
    return "write";
  case SYS_close:
    return "close";
  default:
    return "unknown";
  }
}

int main(int argc, char *argv[])
{
  if (argc < 3)
  {
    fprintf(2, "Usage: syscount <mask> command [args]\n");
    exit(1);
  }

  int mask = atoi(argv[1]);
  int pid = fork();
  int syscall_num = 0;
  while (mask > 1)
  {
    mask >>= 1;
    syscall_num++;
  }

  if (pid < 0)
  {
    fprintf(2, "fork failed\n");
    exit(1);
  }
  else if (pid == 0)
  {
    // Child process
    exec(argv[2], &argv[2]);
    fprintf(2, "exec failed\n");
    exit(1);
  }
  else
  {
    // Parent process

    int count = getsyscount(0,syscall_num);

    // Find the syscall number from the mask

    char *syscall_name = sysname(syscall_num);
    printf("PID %d called %s %d times\n", pid, syscall_name, count);
  }
  exit(0);
}