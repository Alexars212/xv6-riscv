#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int start, end;
  int pid, status;

  if (argc < 2) {
    fprintf(2, "Usage: time1 command [args...]\n");
    exit(1);
  }

  start = uptime();

  pid = fork();
  if (pid < 0) {
    fprintf(2, "time1: fork failed\n");
    exit(1);
  }

  if (pid == 0) {
    // child: exec the command with its arguments
    exec(argv[1], argv + 1);
    // exec only returns on failure
    fprintf(2, "time1: exec %s failed\n", argv[1]);
    exit(1);
  } else {
    // parent: wait for child to finish
    wait(&status);
    end = uptime();
    printf("elapsed time: %d ticks\n", end - start);
  }

  exit(0);
}
