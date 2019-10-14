/*
 * test_basic.c
 *
 * Tests basic functionality of all system calls.
 *
 */

#include <string.h>
#include <stdlib.h>
#include <usloss.h>
#include <phase1.h>
#include <assert.h>
#include <libuser.h>

#include "tester.h"
#include "phase2Int.h"

static int passed = TRUE;

static int childPid, p2Pid, p3Pid;

/*
 * CheckName
 *
 * Verifies that the process with the specified pid has the specified name.
 * Tests Sys_GetProcInfo.
 */

static void CheckName(char *name, int pid)
{
    P1_ProcInfo info;

    int rc = Sys_GetProcInfo(pid, &info);
    TEST(rc, P1_SUCCESS);
    TEST(strcmp(info.name, name), 0);
}

/*
 * P2_Startup
 *
 * Entry point for this test. Creates the user-level process P3_Startup.
 * Tests P2_Spawn and P2_Wait.
 */

int P2_Startup(void *arg)
{
    int rc;

    P2ProcInit();
    p2Pid = P1_GetPid();
    printf("here in p2_start\n");
    rc = P2_Spawn("P3_Startup", P3_Startup, NULL, 4*USLOSS_MIN_STACK, 3, &p3Pid);
    printf("here after fork.\n");

    PASSED();
    return 0;
}

/*
 * Child
 *
 * Checks its pid and those of its ancestors.
 * Tests Sys_GetPID.
 */

int Child(void *arg) {
    int pid;

    Sys_GetPID(&pid);
    TEST(pid, childPid);
    CheckName("Child", childPid);
    CheckName("P2_Startup", p2Pid);
    CheckName("P3_Startup", p3Pid);
    printf("in child, %d" ,(int)arg);
    return (int) arg;
}

int P3_Startup(void *arg) {
    int rc;
    printf("here in P3 \n");
    rc = Sys_Spawn("Child", Child, (void *) 42, USLOSS_MIN_STACK, 3, &childPid);
    TEST(rc, P1_SUCCESS);



    Sys_Terminate(11);
    // does not get here
    return 0;
}

void test_setup(int argc, char **argv) {
    // Do nothing.
}

void test_cleanup(int argc, char **argv) {
    if (passed) {
        USLOSS_Console("TEST PASSED.\n");
    }
}
