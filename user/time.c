#include "kernel/types.h"
#include "kernel/pstat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int start, end;
  int pid, status, elapsed, cpu, percent;
  struct rusage ru;

  if (argc < 2) {
    fprintf(2, "Usage: time command [args...]\n");
    exit(1);
  }

  start = uptime();

  pid = fork();
  if (pid < 0) {
    fprintf(2, "time: fork failed\n");
    exit(1);
  }

  if (pid == 0) {
    // child: exec the command with its arguments
    exec(argv[1], argv + 1);
    fprintf(2, "time: exec %s failed\n", argv[1]);
    exit(1);
  } else {
    // parent: wait for child to finish, collecting its rusage
    wait2(&status, &ru);
    end = uptime();

    elapsed = end - start;
    cpu = ru.cputime;
    percent = (elapsed > 0) ? (cpu * 100) / elapsed : 0;

    printf("elapsed time: %d ticks, cpu time: %d ticks, %d%% CPU\n",
           elapsed, cpu, percent);
  }

  exit(0);
}
