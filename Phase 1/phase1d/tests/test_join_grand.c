#include "phase1.h"
#include <assert.h>
#include <stdio.h>

int GrandChild(void *arg) {

    USLOSS_Console("GrandChild %d\n", (int) arg);

    return (int) arg;
}

int Child(void *arg) {
    int pid;
    int rc;
    int status;
    USLOSS_Console("Child %d\n", (int) arg);
    rc = P1_Fork("Grandchild" + (char) arg, GrandChild, arg, USLOSS_MIN_STACK, 3, 0, &pid);
    assert(rc == P1_SUCCESS);
    rc = P1_Join(0, pid, &status);
    assert(rc == P1_SUCCESS);
    printf("Successfully joined grand child %d", (int) arg);
    return (int) arg;
}

int P2_Startup(void *notused)
{
    sentinel();
    int status = 0;
    int rc;
    int pids[10];
    for(int i = 0; i < 10; i++){
        printf("Forking child %d", i);
        rc = P1_Fork("child" + (char) i, Child, (void *)i, USLOSS_MIN_STACK, 1, 0, &pids[i]);
        assert(rc == P1_SUCCESS);
    }
    for(int i = 0; i < 10; i++){
        rc = P1_Join(0, pids[i], &status);
        assert(rc == P1_SUCCESS);
        printf("Successfully joined child %d", i);
    }
    return status;
}

void test_setup(int argc, char **argv) {
   // sentinel();
}

void test_cleanup(int argc, char **argv) {
    // Do nothing.
}
