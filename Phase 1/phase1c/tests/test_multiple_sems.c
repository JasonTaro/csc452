#include <phase1.h>
#include <phase1Int.h>
#include <assert.h>
#include <stdio.h>

static int flag = 0;

static int
Unblocks(void *arg)
{
    int sem = (int) arg;
    int rc;

    flag = 1;
    USLOSS_Console("V on semaphore. 1\n");
    rc = P1_V(sem);
    assert(rc == P1_SUCCESS);
    // will never get here as Blocks will run and call USLOSS_Halt.
    return 12;
}

static int
Unblocks_2(void *arg)
{
    int sem = (int) arg;
    int rc;

    flag = 1;
    USLOSS_Console("V on semaphore. 1\n");
    rc = P1_V(sem);
    assert(rc == P1_SUCCESS);
    // will never get here as Blocks will run and call USLOSS_Halt.
    return 12;
}

static int
Unblocks_3(void *arg)
{
    int sem = (int) arg;
    int rc;

    flag = 1;
    USLOSS_Console("V on semaphore. 1\n");
    rc = P1_V(sem);
    assert(rc == P1_SUCCESS);
    // will never get here as Blocks will run and call USLOSS_Halt.
    return 12;
}

static int Blocks_3(void *arg){
    int sem = (int) arg;
    int rc;
    int pid;

    rc = P1_Fork("Unblocks 3", Unblocks_3, (void *) sem, USLOSS_MIN_STACK, 2, 0, &pid);
    assert(rc == P1_SUCCESS);

    USLOSS_Console("P on semaphore.\n");
    rc = P1_P(sem);
    printf("here");
    assert(rc == P1_SUCCESS);
    assert(flag == 1);

    USLOSS_Console("Test passed.\n");
    USLOSS_Halt(0);
    // should not return
    assert(0);
    return 0;
}


static int Blocks_2(void *arg){
    int sem = (int) arg;
    int rc;
    int pid1;
    int pid;

    rc = P1_Fork("Blocks3", Blocks_3, (void *) sem, USLOSS_MIN_STACK, 1, 0, &pid1);
    assert(rc == P1_SUCCESS);
    rc = P1_Fork("Unblocks 2", Unblocks_2, (void *) sem, USLOSS_MIN_STACK, 2, 0, &pid);
    assert(rc == P1_SUCCESS);

    USLOSS_Console("P on semaphore.\n");
    rc = P1_P(sem);
    printf("here");
    assert(rc == P1_SUCCESS);
    assert(flag == 1);

    USLOSS_Console("Test passed.\n");
    USLOSS_Halt(0);
    // should not return
    assert(0);
    return 0;
}

static int
Blocks(void *arg)
{
    int sem = (int) arg;
    int rc;
    int pid1;
    int pid;

    rc = P1_Fork("Blocks_2", Blocks_2, (void *) sem, USLOSS_MIN_STACK, 1, 0, &pid1);
    assert(rc == P1_SUCCESS);
    rc = P1_Fork("Unblocks", Unblocks, (void *) sem, USLOSS_MIN_STACK, 2, 0, &pid);
    assert(rc == P1_SUCCESS);

    USLOSS_Console("P on semaphore.\n");
    rc = P1_P(sem);
    printf("here");
    assert(rc == P1_SUCCESS);
    assert(flag == 1);

    USLOSS_Console("Test passed.\n");
    USLOSS_Halt(0);
    // should not return
    assert(0);
    return 0;
}

void
startup(int argc, char **argv)
{
    int pid1;
    int rc;
    int sem;
    P1SemInit();
    rc = P1_SemCreate("sem", 0, &sem);
    assert(rc == P1_SUCCESS);

    printf("1");
    // Blocks blocks then Unblocks unblocks it

    rc = P1_Fork("Blocks", Blocks, (void *) sem, USLOSS_MIN_STACK, 1, 0, &pid1);
    assert(rc == P1_SUCCESS);

    rc = P1_SemFree(sem);
    assert(rc = P1_SUCCESS);

    assert(0);
}

void dummy(int type, void *arg) {};

void test_setup(int argc, char **argv) {}

void test_cleanup(int argc, char **argv) {}

void finish(int argc, char **argv) {}