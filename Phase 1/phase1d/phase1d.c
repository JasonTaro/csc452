#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include "usloss.h"
#include "phase1.h"
#include "phase1Int.h"

static void DeviceHandler(int type, void *arg);
static void SyscallHandler(int type, void *arg);
static void IllegalInstructionHandler(int type, void *arg);

static int sentinel(void *arg);

void 
startup(int argc, char **argv)
{
    int pid;
    P1SemInit();

    // initialize device datastructures
    // put device interrupt handlers into interrupt vector
    USLOSS_IntVec[USLOSS_SYSCALL_INT] = SyscallHandler;
    USLOSS_IntVec[USLOSS_ILLEGAL_INT] = IllegalInstructionHandler;

    /* create the sentinel process */
    int rc = P1_Fork("sentinel", sentinel, NULL, USLOSS_MIN_STACK, 6 , 0, &pid);
    assert(rc == P1_SUCCESS);
    // should not return
    assert(0);
    return;

} /* End of startup */

int 
P1_WaitDevice(int type, int unit, int *status) 
{
    int     result = P1_SUCCESS;
    // P device's semaphore
    // set *status to device's status
    return result;
}

int 
P1_WakeupDevice(int type, int unit, int status, int abort) 
{
    int     result = P1_SUCCESS;
    // save device's status
    // V device's semaphore 
    return result;
}

static void
DeviceHandler(int type, void *arg) 
{
    // if clock device
    //      P1_WakeupDevice every 4 ticks
    //      P1Dispatch(TRUE) every 5 ticks
    // else
    //      P1_WakeupDevice
}

static int
sentinel (void *notused)
{
    int     pid;
    int     rc;

    /* start the P2_Startup process */
    rc = P1_Fork("P2_Startup", P2_Startup, NULL, 4 * USLOSS_MIN_STACK, 2 , 0, &pid);
    assert(rc == P1_SUCCESS);

    // enable interrupts
    // while sentinel has children
    //      get children that have quit via P1GetChildStatus
    //      wait for an interrupt via USLOSS_WaitInt
    USLOSS_Console("Sentinel quitting.\n");
    return 0;
} /* End of sentinel */

int 
P1_Join(int tag, int *pid, int *status) 
{
    int result = P1_SUCCESS;
    // check for kernel mode
    // disable interrupts
    // do
    //  get a child that has quit via P1GetChildStatus 
    //  if no children have quit
    //      set state to P1_STATE_JOINING vi P1SetState
    //      P1Dispatch(FALSE)
    // until either a child quit or there are no more children
    return result;
}

static void
IllegalInstructionHandler(int type, void *arg)
{
    P1_Quit(1024); // quit process with arbitrary status 1024
}

static void
SyscallHandler(int type, void *arg) 
{
    USLOSS_Console("System call %d not implemented.\n", (int) arg);
    USLOSS_IllegalInstruction();
}

void finish(int argc, char **argv) {}
