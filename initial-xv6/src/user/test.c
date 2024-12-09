#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void spin(int n, int pid) {
    int i, j = 0;
    for(i = 0; i < 100000000 * n; i++) {
        j = j + i * (i % 10); // Dummy computation to keep the CPU busy
    }
    printf("Process with pid %d finished\n", pid);
}

int main(void) {
    int pid1, pid2, pid3;

    // Fork first child with 10 tickets
    if((pid1 = fork()) == 0) {
        settickets(10); // Assuming you have added a system call to set tickets
        spin(1,getpid());  // CPU-bound work
        exit(0);
    }

    // Fork second child with 20 tickets
    if((pid2 = fork()) == 0) {
        settickets(10);
        spin(1,getpid());
        exit(0);
    }

    // Fork third child with 30 tickets
    if((pid3 = fork()) == 0) {
        settickets(20);
        spin(1,getpid());
        exit(0);
    }

    wait(0);
    wait(0);
    wait(0);

    exit(0);
}
