#include "kernel/param.h"
#include "kernel/types.h"
#include "user/user.h"

#define WORKERS 4
#define WORK_TICKS 120

struct sample {
  int slot;
  int pid;
  int priority;
  uint started;
  uint finished;
};

// Create workers in increasing base-priority order.
static int priorities[WORKERS] = {0, 15, 30, 49};

static void
fail(char *message)
{
  printf("aging4: FAIL: %s\n", message);
  exit(1);
}

// Report the first run, perform timed work, and report completion.
static void
worker(int slot, int readyfd, int gofd, int startfd, int donefd)
{
  char token = 'R';
  struct sample result;

  if (setpriority(priorities[slot]) != 0 ||
      write(readyfd, &token, 1) != 1 ||
      read(gofd, &token, 1) != 1)
    fail("worker setup");

  result.slot = slot;
  result.pid = getpid();
  result.priority = getpriority();
  result.started = uptime();

  if (write(startfd, &slot, sizeof(slot)) != sizeof(slot))
    fail("start report");

  // Keep the worker runnable long enough for aging to affect selection.
  while ((uint)(uptime() - result.started) < WORK_TICKS)
    ;

  result.finished = uptime();
  if (write(donefd, &result, sizeof(result)) != sizeof(result))
    fail("completion report");
  exit(0);
}

int
main(void)
{
  int ready[2], go[2], starts[2], done[2];
  int pids[WORKERS], start_order[WORKERS];
  int seen_start[WORKERS] = {0};
  int seen_done[WORKERS] = {0};
  int status;
  char token;
  char release[WORKERS] = {'G', 'G', 'G', 'G'};
  struct sample results[WORKERS];
  uint released, total_response = 0, total_turnaround = 0;

  if (SCHED_POLICY != SCHED_AGING &&
      SCHED_POLICY != SCHED_PRIORITY)
    fail("select SCHED_AGING or SCHED_PRIORITY");

  if (pipe(ready) < 0 || pipe(go) < 0 ||
      pipe(starts) < 0 || pipe(done) < 0)
    fail("pipe setup");

  for (int i = 0; i < WORKERS; i++) {
    pids[i] = fork();
    if (pids[i] < 0)
      fail("fork");
    if (pids[i] == 0)
      worker(i, ready[1], go[0], starts[1], done[1]);
  }

  // Release the workers together once all four are blocked.
  for (int i = 0; i < WORKERS; i++)
    if (read(ready[0], &token, 1) != 1)
      fail("worker readiness");

  released = uptime();
  if (write(go[1], release, sizeof(release)) != sizeof(release))
    fail("worker release");

  // Record the order in which workers begin running.
  for (int i = 0; i < WORKERS; i++) {
    int slot;
    if (read(starts[0], &slot, sizeof(slot)) != sizeof(slot) ||
        slot < 0 || slot >= WORKERS || seen_start[slot])
      fail("start order");
    seen_start[slot] = 1;
    start_order[i] = slot;
  }

  // Collect start and finish times for each worker.
  for (int i = 0; i < WORKERS; i++) {
    struct sample sample;
    if (read(done[0], &sample, sizeof(sample)) != sizeof(sample) ||
        sample.slot < 0 || sample.slot >= WORKERS)
      fail("completion collection");
    if (seen_done[sample.slot] ||
        sample.pid != pids[sample.slot] ||
        sample.priority != priorities[sample.slot])
      fail("worker identity");
    seen_done[sample.slot] = 1;
    results[sample.slot] = sample;
  }

  for (int i = 0; i < WORKERS; i++)
    if (wait(&status) < 0 || status != 0)
      fail("worker exit");

  printf("policy: %s\n",
         SCHED_POLICY == SCHED_AGING ? "aging" : "priority");
  printf("order\tpid\tbase\tage~\teffective~\tresponse\tturnaround\n");

  for (int i = 0; i < WORKERS; i++) {
    struct sample *row = &results[start_order[i]];
    uint response = row->started - released;
    uint turnaround = row->finished - released;

    // The common release tick estimates the initial ready-queue wait.
    uint age = response;
    uint effective = row->priority;
    if (SCHED_POLICY == SCHED_AGING) {
      if (age >= MAXEFFPRIORITY - effective)
        effective = MAXEFFPRIORITY;
      else
        effective += age;
    }

    printf("%d\t%d\t%d\t%d\t%d\t\t%d\t\t%d\n",
           i + 1, row->pid, row->priority, age, effective,
           response, turnaround);
    total_response += response;
    total_turnaround += turnaround;
  }

  printf("average response: %d + %d/4 ticks\n",
         total_response / WORKERS, total_response % WORKERS);
  printf("average turnaround: %d + %d/4 ticks\n",
         total_turnaround / WORKERS, total_turnaround % WORKERS);

  // Aging should start the lowest-priority worker before the
  // highest-priority worker finishes.
  if (SCHED_POLICY == SCHED_AGING &&
      results[0].started >= results[3].finished)
    fail("lowest-priority worker did not benefit from aging");

  printf("aging4: PASS\n");
  exit(0);
}
