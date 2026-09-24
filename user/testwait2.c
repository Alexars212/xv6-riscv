#include "kernel/types.h"
#include "user/user.h"

int
main(void)
{
  int retime = 0;
  int pid = fork();

  if (pid < 0) {
    printf("fork failed\n");
    exit(1);
  }

  if (pid == 0) {
    // Child: do some work to consume CPU ticks
    for (volatile int i = 0; i < 10000000; i++) {}
    exit(0);
  } else {
    // Parent: wait for child and collect cputime
    int child_pid = wait2(&retime);
    printf("Child %d finished. CPU time: %d ticks\n", child_pid, retime);
  }

  exit(0);
}
