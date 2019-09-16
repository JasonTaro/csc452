/*
  Test case for basic Semaphores. Ensures that P1_P blocks, then child executes and prints 0, then child unblocks parent and parent prints 1

  should print :

  ---------Starting Test SEM ----------
 0
 1
 ---------Ending Test SEM ----------

*/

#include "phase1.h"
#include <stdio.h>
#include <assert.h>

int sem1;

int child(void *arg) {
    int rc;
    USLOSS_Console("child");
    rc = P1_V(sem1);
    assert(rc == P1_SUCCESS);
    return 0;
}

int P2_Startup(void *arg){

    int rc;
    int pid;
    USLOSS_Console(" \n---------Starting Test SEM ----------\n");
    rc = P1_Fork("child", child, NULL, USLOSS_MIN_STACK, 4 , 0, &pid);
    assert(rc == P1_SUCCESS);
    rc = P1_SemCreate("semaphore" , 0, &sem1 );
    if (rc)
    {
        USLOSS_Console("Could not create semaphore, result: %d\n", rc );
    }
    
    rc = P1_P(sem1);

    USLOSS_Console(" %d \n", 1);
    assert(rc == P1_SUCCESS);
    USLOSS_Console(" ---------Ending Test SEM ----------\n");
    return 0;
}

void test_setup(int argc, char **argv) {
    // Do nothing.
}

void test_cleanup(int argc, char **argv) {
    // Do nothing.
}
