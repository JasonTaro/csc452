/*
 * Author: Jack Mittelmeier
 * Purpose: This file switches between two contexts and prints hello world 10 times
 */
#include <stdio.h>
#include "usloss.h"
#include <phase1.h>
#include <phase1Int.h>
#include <assert.h>

USLOSS_Context context0;
USLOSS_Context context_hello;
USLOSS_Context context_world;

#define SIZE (USLOSS_MIN_STACK)

char stack0[SIZE];
char stack1[SIZE];


void print_hello(){
    int rc;
    int i;
    for(i = 1; i <= 10; i++){
        USLOSS_Console("%d Hello ", i);
        rc = P1ContextSwitch(1);
        assert(rc == P1_SUCCESS);
    }
    USLOSS_Halt(0);
}

void print_world(){
    int rc;
    int i;
    int j;
    for(i = 1; i <= 10; i++){
        USLOSS_Console("World");
        for(j = 0; j < i; j++){
            USLOSS_Console("!");
        }
        USLOSS_Console("\n");
        rc = P1ContextSwitch(0);
        assert(rc == P1_SUCCESS);
    }

    USLOSS_Halt(0);
}

void
startup(int argc, char **argv)
{
    /*
     * Your code here. If you compile and run this as-is you
     * will get a simulator trap, which is a good opportunity
     * to verify that you get a core file and you can use it
     * with gdb.
     */
    int cid;
    int rc;

    printf("psr: %d\n", USLOSS_PsrGet());
    int status = USLOSS_PsrSet(USLOSS_PsrGet() & ~USLOSS_PSR_CURRENT_MODE);
    assert(status != USLOSS_ERR_INVALID_PSR);
    printf("psr: %d\n", USLOSS_PsrGet());

    P1ContextInit();
    //USLOSS_ContextInit(&context_hello, stack0, sizeof(stack0), NULL, print_hello);
  //  USLOSS_ContextInit(&context_world, stack1, sizeof(stack1), NULL, print_world);
   // USLOSS_ContextSwitch(&context0, &context_hello);
    rc = P1ContextCreate(print_hello, NULL, SIZE, &cid);
    assert(rc == P1_SUCCESS);
    rc = P1ContextCreate(print_world, NULL, SIZE, &cid);
    assert(rc == P1_SUCCESS);
    rc = P1ContextSwitch(0);
    USLOSS_Halt(0);

}

// Do not modify anything below this line.

void
finish(int argc, char **argv)
{
    USLOSS_Console("Goodbye.\n");
}

void
test_setup(int argc, char **argv)
{
    // Do nothing.
}

void
test_cleanup(int argc, char **argv)
{
    // Do nothing.
}



