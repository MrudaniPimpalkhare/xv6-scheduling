#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  exit(n);
  return 0; // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return wait(p);
}

uint64
sys_getsyscount(void)
{
  uint64 p;
  int num;
  argaddr(0, &p);
  argint(1, &num);
  return getsyscount(p, num);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int n;

  argint(0, &n);
  addr = myproc()->sz;
  if (growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  acquire(&tickslock);
  ticks0 = ticks;
  while (ticks - ticks0 < n)
  {
    if (killed(myproc()))
    {
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

uint64
sys_waitx(void)
{
  uint64 addr, addr1, addr2;
  uint wtime, rtime;
  argaddr(0, &addr);
  argaddr(1, &addr1); // user virtual memory
  argaddr(2, &addr2);
  int ret = waitx(addr, &wtime, &rtime);
  struct proc *p = myproc();
  if (copyout(p->pagetable, addr1, (char *)&wtime, sizeof(int)) < 0)
    return -1;
  if (copyout(p->pagetable, addr2, (char *)&rtime, sizeof(int)) < 0)
    return -1;
  return ret;
}

uint64
sys_sigalarm(void)
{
  int ticks;
  uint64 handler;
  argint(0, &ticks);
  argaddr(1, &handler);
  //printf("interval %d\n", ticks);
  struct proc *p = myproc();
  p->ticks = ticks;
  p->ticks_done = 0;
  p->alarmhandler = (void (*)())handler;
  p->in_handler = 0;
  p->alarm_on = (ticks>0?1:0);
  if(p->alarmtf)
  {
    kfree(p->alarmtf);
    p->alarmtf = 0;
  }
  return 0;
}

uint64
sys_sigreturn(void)
{
  struct proc *p = myproc();
  if (p->alarmtf && p->in_handler)
  {
    memmove(p->trapframe, p->alarmtf, sizeof(struct trapframe));
    kfree(p->alarmtf);
    p->alarmtf = 0;
    p->in_handler = 0;
    p->ticks_done = 0;
  }
  return 0;
}

uint64
sys_settickets(void)
{
  int num;
  struct proc *p = myproc();
  argint(0, &num);
  if (num < 0)
  {
    return -1;
  }
  p->tickets = num;
  // printf("here %d %d\n",p->pid, p->tickets);
  return 0;
}


uint64
sys_yield(void)
{
  struct proc *p = myproc();
  // If the process yields voluntarily, keep it in the same priority queue
  p->time_slice = TIMESLICE_0 + p->priority * (TIMESLICE_1 - TIMESLICE_0);
  yield();
  return 0;
}

uint64
sys_getqueue(void)
{
  int pid;
  argint(0, &pid);
  return getqueue(pid);
}



