#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void
handler()
{
  printf("Alarm triggered!\n");
  sigreturn();
}

int
main(void)
{
  // Set the alarm to trigger every 10 ticks
  sigalarm(10, handler);

  for(int i = 0; i < 100000; i++) {
    printf("%d\n", i);
    // Busy wait to consume CPU time
    for(int j = 0; j < 1000000; j++);
  }

  exit(0);
}
