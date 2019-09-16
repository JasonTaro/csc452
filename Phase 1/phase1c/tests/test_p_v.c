#include <phase1.h>
#include <phase1Int.h>
#include <assert.h>

static int flag = 0;

static int
Unblocks(void *arg)
{
    int sem = (int) arg;
    int rc;

    USLOSS_Console("Unblocks running.\n");
    flag = 1;
    rc = P1_V(sem);
    assert(rc == P1_SUCCESS);
    USLOSS_Console("Unblocks quitting.\n");
    return 12;
}

static int
Blocks(void *arg) 
{
    int sem = (int) arg;
    int rc;
    int pid;

    USLOSS_Console("Blocks running.\n");
    rc = P1_Fork("Unblocks", Unblocks, (void *) sem, USLOSS_MIN_STACK, 2, 0, &pid);
    assert(rc == P1_SUCCESS);
    rc = P1_P(sem);
    assert(rc == P1_SUCCESS);
    assert(flag == 1);
    USLOSS_Console("Blocks quitting.\n");
    P1_Quit(11);
    // should not return
    assert(0);
    return 0;
}

void
startup(int argc, char **argv)
{
    int pid;
    int rc;
    int sem;

    P1SemInit();
    rc = P1_SemCreate("sem", 0, &sem);
    assert(rc == P1_SUCCESS);
    // Blocks blocks then Unblocks unblocks it
    rc = P1_Fork("Blocks", Blocks, (void *) sem, USLOSS_MIN_STACK, 1, 0, &pid);
    assert(rc == P1_SUCCESS);
    assert(0);
}

void test_setup(int argc, char **argv) {}

void test_cleanup(int argc, char **argv) {}

void finish(int argc, char **argv) {}