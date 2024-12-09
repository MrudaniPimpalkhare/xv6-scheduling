# xv6

## System Calls

### Gotta count ‘em all

1. Create a new field value which is an array ```uint syscall_count[NPROC];``` that keeps a count of how many times a particular system call has been called (at the respective index).

2. the function ```syscall()``` increments the count of the system call at the appropriate index, using the array ```syscall[]```.

3. I exploited the code given for wait() in the orignal xv6. The
```getsyscount``` method is almost the same as wait, except that it takes two arguments, i.e one additional ```num``` argument, apart from ```addr```. And instead of returning the pid of the process concerned, it returns ```syscall_count[num]```.

4. the ```main()``` in syscount.c takes the command and mask as an argument. It uses bitshifts to get the appropriate system call number. It then forks, and the child process executes the command. 

5. The parent process calls getsyscount instead of wait, and uses that return value to print how many times the particular system call has been called. 



### Wake me up when my timer ends 

1. This function requires you to add the following field values to the structure proc 

```c
uint ticks; // no. of ticks
uint ticks_done; // ticks done
void (*alarmhandler)(); // Function to call when the     alarm goes off
struct trapframe* alarmtf; // Trapframe to store process state before calling the handler
int alarm_on; // whethersigalarm has been called
int in_handler; // whether the alarm reaches handler
```


2. ```sys_sigalarm``` and ```sys_sigreturn``` 
```c
uint64
sys_sigalarm(void)
{
  int ticks;
  uint64 handler;
  argint(0, &ticks);
  argaddr(1, &handler);
  printf("interval %d\n", ticks);
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
```

```sys_sigalarm``` sets the alarm to 1, and the ticks to 0. It makes sure alarm_tf is 0, so that it can save the context before going into the handler

```sys_sigreturn``` saves the context if non zero, and resets the variables


3. Add the following to ```usertrap()``` , in ```trap.c```, in the timer interrupt section (```whichdev == 2```)

```c
    struct proc* p = myproc();
    if (p->alarm_on > 0 && p->ticks>0 && !p->in_handler)
    {
      p->ticks_done++;
      printf("ticks %d\n", p->ticks_done);
      if (p->ticks_done == p->ticks)
      {
        p->ticks_done = 0;
        printf("here!\n");
        if(p->alarmtf==0)
        {
          p->alarmtf = kalloc();
          memmove(p->alarmtf, p->trapframe, sizeof(struct trapframe));
          p->trapframe->epc = (uint64)p->alarmhandler;
          p->in_handler = 1;
        }
      }
    }
```

this increments the ticks done, and once it reaches the maximum, it resets ticks_done to zero, and then saves the context from the trapframe. Then it sets the current context to the alarmhandler, and sets is_handler to 1.


## Scheduling 

### The process powerball

1. The scheduler for LBS looks like this : 
```c
  struct proc *p;
  struct cpu *c = mycpu();
  c->proc = 0;
  for (;;)
  {
    intr_on();
    int total = totaltickets();
    struct proc* win = 0;
    //code to make init and sh run first
    if(total > 0)
    {
      int draw = random(total);
      for(p=proc; p<&proc[NPROC];p++)
      {
        acquire(&p->lock);
        if(p->state == RUNNABLE && p->pid > 2)
        {
          if(p->tickets > draw)
          {
            if(win==0 || p->ctime < win->ctime)
            {
              if(win!=0)
                release(&win->lock);
              win = p;
            }else{
                release(&p->lock);
            }
          }else{
            release(&p->lock);
          }
        }else{
          release(&p->lock);
        }
      }
      if(win!=0)
      {
        //run the winning process
        release(&win->lock);
      }
      
    }
  }

```

the function ```totaltickets()``` calculates the total number of tickers. A random number is selected in that range. Then for each runnable process, we subtract the number of tickets from draw. The higher the number of tickets, higher the chance of draw to be negative. Once that is achieved, that number is selected as the sinning ticket, and then we check for the process with the same number of tickets but minimum creation time.







