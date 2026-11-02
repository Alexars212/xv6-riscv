#include "kernel/param.h"
#include "kernel/types.h"
#include "user/user.h"

#define WORKERS 4

struct result {
  int pid;
  int priority;
};

// Create workers in an order different from the expected priority order.
static int priorities[WORKERS] = {5, 15, 25, 35};

static void
fail(char *message)
{
  printf("sched4: FAIL: %s\n", message);
  exit(1);
}

// Wait for release, then report when this worker first runs.
static void
worker(int priority, int readyfd, int gofd, int orderfd)
{
  char token = 'R';
  struct result row;

  if (setpriority(priority) != 0 ||
      write(readyfd, &token, 1) != 1 ||
      read(gofd, &token, 1) != 1)
    fail("worker setup");

  row.pid = getpid();
  row.priority = getpriority();
  if (write(orderfd, &row, sizeof(row)) != sizeof(row))
    fail("result write");
  exit(0);
}

int
main(void)
{
  int ready[2], go[2], order[2];
  int pids[WORKERS], status;
  char token;
  char release[WORKERS] = {'G', 'G', 'G', 'G'};
  struct result rows[WORKERS];

  if (SCHED_POLICY != SCHED_PRIORITY)
    fail("select SCHED_PRIORITY in kernel/param.h");

  if (pipe(ready) < 0 || pipe(go) < 0 || pipe(order) < 0)
    fail("pipe setup");

  // Fork the lowest-priority worker first.
  for (int i = 0; i < WORKERS; i++) {
    pids[i] = fork();
    if (pids[i] < 0)
      fail("fork");
    if (pids[i] == 0)
      worker(priorities[i], ready[1], go[0], order[1]);
  }

  // Release the workers after all four have blocked.
  for (int i = 0; i < WORKERS; i++)
    if (read(ready[0], &token, 1) != 1)
      fail("worker readiness");
  if (write(go[1], release, sizeof(release)) != sizeof(release))
    fail("worker release");

  // Pipe records arrive in the order the workers first ran.
  for (int i = 0; i < WORKERS; i++)
    if (read(order[0], &rows[i], sizeof(rows[i])) != sizeof(rows[i]))
      fail("result collection");
  for (int i = 0; i < WORKERS; i++)
    if (wait(&status) < 0 || status != 0)
      fail("worker exit");

  printf("run order\tpid\tpriority\n");
  for (int i = 0; i < WORKERS; i++) {
    int expected = WORKERS - 1 - i;
    printf("%d\t\t%d\t%d\n", i + 1, rows[i].pid, rows[i].priority);
    if (rows[i].pid != pids[expected] ||
        rows[i].priority != priorities[expected])
      fail("priority order");
  }

  printf("sched4: PASS\n");
  exit(0);
}
