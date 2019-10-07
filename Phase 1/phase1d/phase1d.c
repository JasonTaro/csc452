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

typedef struct Device {
    int wait_semaphore;
    int status;
    int aborted;
} Device;

int clock_interrupts = 0;

static Device device_list[4][USLOSS_MAX_UNITS];
static int device_units[] = {1, 1, 2, 4};

void 
startup(int argc, char **argv)
{
    int pid;
    P1SemInit();

    for (int type = 0; type < USLOSS_SYSCALL_INT; ++type) {
        for (int unit = 0; unit < USLOSS_MAX_UNITS; ++unit) {
            device_list[type][unit].status = 0;
            device_list[type][unit].aborted = 0;

            /* Crates the name of the semaphore to be a description of it's location */
            char name[P1_MAXNAME+1];
            snprintf(name, P1_MAXNAME+1, "Semaphore: Type: %d, Unit: %d", type, unit);

            int rc = P1_SemCreate(name, 0, &device_list[type][unit].wait_semaphore);
            assert(rc == P1_SUCCESS);
        }
    }


    // initialize device data structures
    // put device interrupt handlers into interrupt vector
    USLOSS_IntVec[USLOSS_CLOCK_INT] = DeviceHandler;
    USLOSS_IntVec[USLOSS_ALARM_INT] = DeviceHandler;
    USLOSS_IntVec[USLOSS_DISK_INT] = DeviceHandler;
    USLOSS_IntVec[USLOSS_TERM_INT] = DeviceHandler;
    USLOSS_IntVec[USLOSS_MMU_INT] = DeviceHandler;
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
    // disable interrupts
    // check kernel mode
    // P device's semaphore
    // set *status to device's status
    // restore interrupts

    if ((USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) != USLOSS_PSR_CURRENT_MODE) {
        USLOSS_Console("ERROR: Call to P1_WaitDevice from user mode.\n");
        USLOSS_IllegalInstruction();
        P1_Quit(1024);
    }
    int interruptsEnabled = P1DisableInterrupts();

    int result = P1_SUCCESS;
    int retVal;
    if (type < 0 || type >= USLOSS_SYSCALL_INT) {
        result = P1_INVALID_TYPE;
    } else if (unit < 0 || unit >= device_units[type]) {
        result = P1_INVALID_UNIT;
    } else {
        retVal = P1_P(device_list[type][unit].wait_semaphore);
        assert(retVal == P1_SUCCESS);
        if (device_list[type][unit].aborted == 1) {
            result = P1_WAIT_ABORTED;
        } else {
            *status = device_list[type][unit].status;
        }
    }

    if (interruptsEnabled) { P1EnableInterrupts(); }
    return result;
}

int 
P1_WakeupDevice(int type, int unit, int status, int abort) 
{
    // disable interrupts
    // check kernel mode
    // save device's status to be used by P1_WaitDevice
    // save abort to be used by P1_WaitDevice
    // V device's semaphore
    // restore interrupts

    if ((USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) != USLOSS_PSR_CURRENT_MODE) {
        USLOSS_Console("ERROR: Call to P1_WakeupDevice from user mode.\n");
        USLOSS_IllegalInstruction();
        P1_Quit(1024);
    }
    int interruptsEnabled = P1DisableInterrupts();

    int result = P1_SUCCESS;
    int retVal;
    if (type < 0 || type >= USLOSS_SYSCALL_INT) {
        result = P1_INVALID_TYPE;
    } else if (unit < 0 || unit >= device_units[type]) {
        result = P1_INVALID_UNIT;
    } else {
        device_list[type][unit].status = status;
        device_list[type][unit].aborted = abort;
        retVal = P1_V(device_list[type][unit].wait_semaphore);
        assert(retVal == P1_SUCCESS);
    }

    if (interruptsEnabled) { P1EnableInterrupts(); }
    return result;
}

static void
DeviceHandler(int type, void *arg) 
{
    // I'm writing this on the assumption that void *arg is a pointer to the unit number

    int unit = (int) arg;

    int status;
    int rc = USLOSS_DeviceInput(type, unit, &status);
    assert (rc == USLOSS_DEV_OK);
    int retVal;
    if (type == USLOSS_CLOCK_DEV) {
        ++clock_interrupts;
        if (!(clock_interrupts % 4)) {
            retVal = P1_WakeupDevice(0, 0, 0, 0);
            assert(retVal == P1_SUCCESS);
        }
        if (!(clock_interrupts % 5)) { P1Dispatch(TRUE); }
    } else {
        retVal = P1_WakeupDevice(type, unit, status, 0);
        assert(retVal == P1_SUCCESS);
    }
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
    int status;
    /* start the P2_Startup process */
    rc = P1_Fork("P2_Startup", P2_Startup, NULL, 4 * USLOSS_MIN_STACK, 2 , 0, &pid);
    assert(rc == P1_SUCCESS);
    P1EnableInterrupts();
    // enable interrupts
    // while sentinel has children
    //      get children that have quit via P1GetChildStatus (either tag)
    //      wait for an interrupt via USLOSS_WaitInt
    int has_children = 1;
    int child_id;
    int retVal;
    while(has_children == 1){
        has_children = 0;
        for(int i = 0; i < P1_MAXTAG; i++){
            while(1){
                retVal = P1GetChildStatus(i, &child_id, &status);
                if(retVal == P1_NO_QUIT){
                    has_children = 1;
                    break;
                } else if(retVal == P1_NO_CHILDREN) {
                    break;
                }
            }
        }
        if(has_children == 1){
            USLOSS_WaitInt();
        }
    }
    USLOSS_Console("Sentinel quitting.\n");
    P1_Quit(0);
    return 0;
} /* End of sentinel */

int 
P1_Join(int tag, int *pid, int *status) 
{
    // disable interrupts
    // kernel mode
    // do
    //     use P1GetChildStatus to get a child that has quit
    //     if no children have quit
    //        set state to P1_STATE_JOINING vi P1SetState
    //        P1Dispatch(FALSE)
    // until either a child quit or there are no more children

    if ((USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) != USLOSS_PSR_CURRENT_MODE) {
        USLOSS_Console("ERROR: Call to P1_WakeupDevice from user mode.\n");
        USLOSS_IllegalInstruction();
        P1_Quit(1024);
    }
    int interruptsEnabled = P1DisableInterrupts();
    int result = P1_SUCCESS;

    if (tag != 0 && tag != 1) {
        result = P1_INVALID_TAG;
    } else {
        int rc;
        while (TRUE) {
            rc = P1GetChildStatus(tag, pid, status);
            if (rc == P1_NO_CHILDREN) {
                result = P1_NO_CHILDREN;
                break;
            } else if (rc == P1_SUCCESS) {
                break;
            } else {
                int retVal;
                retVal = P1SetState(P1_GetPid(), P1_STATE_JOINING, -1);
                assert(retVal == P1_SUCCESS);
                P1Dispatch(FALSE);
            }
        }
    }

    if (interruptsEnabled) { P1EnableInterrupts(); }
    return result;
}

static void
SyscallHandler(int type, void *arg) 
{
    USLOSS_Console("System call %d not implemented.\n", (int) arg);
    USLOSS_IllegalInstruction();
}

static void IllegalInstructionHandler(int type, void *arg){
    USLOSS_IllegalInstruction();
    P1_Quit(1024);
}

void finish(int argc, char **argv) {}
