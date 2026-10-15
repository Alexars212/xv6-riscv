#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/pstat.h"
#include "user/user.h"

int
main(int argc, char **argv)
{
  struct pstat uproc[NPROC];
  int nprocs;
  int i, now, age;
  char *state;
  static char *states[] = {
    [UNUSED]   = "unused  ",
    [USED]     = "used    ",
    [SLEEPING] = "sleeping",
    [RUNNABLE] = "runnable",
    [RUNNING]  = "running ",
    [ZOMBIE]   = "zombie  "
  };

  nprocs = getprocs(uproc);
  if (nprocs < 0)
    exit(-1);

  now = uptime();

  printf("pid\tstate\t\tsize\tage\tpriority\tcputime\tppid\tname\n");
  for (i = 0; i < nprocs; i++) {
    state = states[uproc[i].state];
    if (uproc[i].state == RUNNABLE)
      age = now - (int)uproc[i].readytime;
    else
      age = -1;

    if (age >= 0)
      printf("%d\t%s\t%d\t%d\t%d\t\t%d\t%d\t%s\n", uproc[i].pid, state,
             (int)uproc[i].size, age, uproc[i].priority, uproc[i].cputime,
             uproc[i].ppid, uproc[i].name);
    else
      printf("%d\t%s\t%d\t\t%d\t\t%d\t%d\t%s\n", uproc[i].pid, state,
             (int)uproc[i].size, uproc[i].priority, uproc[i].cputime,
             uproc[i].ppid, uproc[i].name);
  }
  exit(0);
}
