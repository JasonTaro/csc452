#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include <usloss.h>
#include <phase1.h>

#include "phase2Int.h"


static int      ClockDriver(void *);
static void     SleepStub(USLOSS_Sysargs *sysargs);

// User made functions
int AddSleepingProcess(int pid, int waitingTime);
int GetSleepingProcess() __attribute__((warn_unused_result));
int getCurrentTime();
#define NO_PROCESS_SLEEPING -1


/*
 * P2ClockInit
 *
 * Initialize the clock data structures and fork the clock driver.
 */
void
P2ClockInit(void)
{
    if ((USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) != USLOSS_PSR_CURRENT_MODE) {
        USLOSS_Console("ERROR: Call to P2ClockInit from user mode.\n");
        USLOSS_IllegalInstruction();
    }

    int rc;

    P2ProcInit();

    // initialize data structures here
    // JACK I made it a queue so we don't have have to initialize it.... maybe....

    rc = P2_SetSyscallHandler(SYS_SLEEP, SleepStub);
    assert(rc == P1_SUCCESS);

    int pid;
    rc = P1_Fork("Clock Driver", ClockDriver, NULL, 4*USLOSS_MIN_STACK, 2, 0, &pid);
    assert(rc == P1_SUCCESS);
}

/*
 * P2ClockShutdown
 *
 * Clean up the clock data structures and stop the clock driver.
 */

void
P2ClockShutdown(void)
{
    if ((USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) != USLOSS_PSR_CURRENT_MODE) {
        USLOSS_Console("ERROR: Call to P2ClockShutdown from user mode.\n");
        USLOSS_IllegalInstruction();
    }

    int rc = P1_WakeupDevice(0, 0, 0, TRUE);
    assert(rc == P1_SUCCESS);
}

/*
 * ClockDriver
 *
 * Kernel process that manages the clock device and wakes sleeping processes.
 */
static int
ClockDriver(void *arg)
{
    if ((USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) != USLOSS_PSR_CURRENT_MODE) {
        USLOSS_Console("ERROR: Call to ClockDriver from user mode.\n");
        USLOSS_IllegalInstruction();
    }

    while(1) {
        int rc;
        int now;

        // wait for the next interrupt
        rc = P1_WaitDevice(USLOSS_CLOCK_DEV, 0, &now);
        if (rc == P1_WAIT_ABORTED) {
            break;
        }
        assert(rc == P1_SUCCESS);

        // wakeup any sleeping processes whose wakeup time has arrived
        int blockedSemID;
        while((blockedSemID = GetSleepingProcess()) != NO_PROCESS_SLEEPING) {
            rc = P1_V(blockedSemID);
            assert(rc == P1_SUCCESS);
        };


    }
    return P1_SUCCESS;
}

/*
 * P2_Sleep
 *
 * Causes the current process to sleep for the specified number of seconds.
 */
int
P2_Sleep(int seconds)
{
    if ((USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) != USLOSS_PSR_CURRENT_MODE) {
        USLOSS_Console("ERROR: Call to P2_Sleep from user mode.\n");
        USLOSS_IllegalInstruction();
    }

    int rc;

    if (seconds < 0) {
        return P2_INVALID_SECONDS;
    } else if (seconds == 0) {
        return P1_SUCCESS;
    } else {
        // add current process to data structure of sleepers
        int semID = AddSleepingProcess(P1_GetPid(), getCurrentTime() + (seconds * 1000000));
        // wait until sleep is complete
        rc = P1_P(semID);
        assert(rc == P1_SUCCESS);

        rc = P1_SemFree(semID);
        assert(rc == P1_SUCCESS);
        return P1_SUCCESS;
    }
}

/*
 * SleepStub
 *
 * Stub for the Sys_Sleep system call.
 */
static void
SleepStub(USLOSS_Sysargs *sysargs)
{
    if ((USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) != USLOSS_PSR_CURRENT_MODE) {
        USLOSS_Console("ERROR: Call to SleepStub from user mode.\n");
        USLOSS_IllegalInstruction();
    }

    int seconds = (int) sysargs->arg1;
    int rc = P2_Sleep(seconds);
    sysargs->arg4 = (void *) rc;
}


// Simple helper function which grabs the current time

int getCurrentTime() {
    int currentTime;
    int rc = USLOSS_DeviceInput(USLOSS_CLOCK_DEV, 0, &currentTime);
    assert(rc == USLOSS_DEV_OK);
    return currentTime;
}

// QUEUEUEUEUEUEUEUEUEUEUEUEUEU

struct SleepNode {
    int pid;
    int waitingTime;
    struct SleepNode* next;
    int semID;
} typedef SleepNode;

SleepNode* SleepQueueHead;
char meaninglessName = (char) 33;

// Adds a sleeping process to the queue, in increasing order of wakeup time.
// Return the id of the semaphore the process will be blocked on
int AddSleepingProcess(int pid, int waitingTime) {
    SleepNode* newNode = (SleepNode*) malloc(sizeof(SleepNode));
    newNode->pid = pid;
    newNode->waitingTime = waitingTime;
    newNode->next = NULL;
    int rc = P1_SemCreate((&meaninglessName), 0, &newNode->semID);
    meaninglessName++;
    assert(rc == P1_SUCCESS);

    if (SleepQueueHead == NULL) {
        SleepQueueHead = newNode;
    } else {
        SleepNode* current = SleepQueueHead;
        while (current->next != NULL && waitingTime > current->next->waitingTime) {
            current = current->next;
        }

        if (current->next == NULL) {
            // adding to end of queue
            current->next = newNode;
        } else {
            // adding into sorted location in queue
            newNode->next = current->next;
            current->next = newNode;
        }
    }
    return newNode->semID;
}

// Checks whether any process should be woken up
// If so, returns the semID a sleeping process is blocked on
// Otherwise, returns NO_PROCESS_SLEEPING
int GetSleepingProcess() {
    if (SleepQueueHead == NULL || SleepQueueHead->waitingTime > getCurrentTime()) {
        return NO_PROCESS_SLEEPING;
    } else {
        int blockedSem = SleepQueueHead->semID;
        SleepNode* tmp = SleepQueueHead;
        SleepQueueHead = SleepQueueHead->next;
        free(tmp);
        return blockedSem;
    }
}