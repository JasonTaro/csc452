#include <stdlib.h>
#include <usloss.h>
#include <phase1.h>
#include <phase2.h>
#include <stdio.h>
#include <assert.h>
#include <libuser.h>
#include <usyscall.h>

#include "phase2Int.h"

#define TAG_KERNEL 0
#define TAG_USER 1

void (*USLOSS_SyscallVec[USLOSS_MAX_SYSCALLS])(void *arg) = { NULL };

static void SpawnStub(USLOSS_Sysargs *sysargs);
static void WaitStub(USLOSS_Sysargs *sysargs);
static void TerminateStub(USLOSS_Sysargs *sysargs);
static void GetProcInfoStub(USLOSS_Sysargs *sysargs);
static void GetPid (USLOSS_Sysargs *sysargs);
static void GetTimeOfDay (USLOSS_Sysargs *sysargs);

/*
 * IllegalHandler
 *
 * Handler for illegal instruction interrupts.
 *
 */

static void 
IllegalHandler(int type, void *arg)
{
    P1_ProcInfo info;
    assert(type == USLOSS_ILLEGAL_INT);

    int pid = P1_GetPid();
    int rc = P1_GetProcInfo(pid, &info);
    assert(rc == P1_SUCCESS);
    if (info.tag == TAG_KERNEL) {
        P1_Quit(1024);
    } else {
        P2_Terminate(2048);
    }
}

/*
 * SyscallHandler
 *
 * Handler for system call interrupts.
 *
 */

static void 
SyscallHandler(int type, void *arg)
//SyscallHandler(int type, USLOSS_Sysargs *arg)
{
    if ((USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) != USLOSS_PSR_CURRENT_MODE) {
        USLOSS_Console("ERROR: Call to SyscallHandler from user mode.\n");
        USLOSS_IllegalInstruction();
    }

    if (USLOSS_SyscallVec[((USLOSS_Sysargs*) arg)->number] == NULL) {
        ((USLOSS_Sysargs*) arg)->arg4 = (void*) P2_INVALID_SYSCALL;
    } else {
        USLOSS_SyscallVec[((USLOSS_Sysargs*) arg)->number](arg);
    }

}


/*
 * P2ProcInit
 *
 * Initialize everything.
 *
 */

void
P2ProcInit(void) 
{
    int rc;

    USLOSS_IntVec[USLOSS_ILLEGAL_INT] = IllegalHandler;
    USLOSS_IntVec[USLOSS_SYSCALL_INT] = SyscallHandler;

    // call P2_SetSyscallHandler to set handlers for all system calls
    rc = P2_SetSyscallHandler(SYS_SPAWN, SpawnStub);
    assert(rc == P1_SUCCESS);

    rc = P2_SetSyscallHandler(SYS_WAIT, WaitStub);
    assert(rc == P1_SUCCESS);

    rc = P2_SetSyscallHandler(SYS_TERMINATE, TerminateStub);
    assert(rc == P1_SUCCESS);

    rc = P2_SetSyscallHandler(SYS_GETPROCINFO, GetProcInfoStub);
    assert(rc == P1_SUCCESS);

    rc = P2_SetSyscallHandler(SYS_GETPID, GetPid);
    assert(rc == P1_SUCCESS);

    rc = P2_SetSyscallHandler(SYS_GETTIMEOFDAY, GetTimeOfDay);
    assert(rc == P1_SUCCESS);


}


/*
 * P2_SetSyscallHandler
 *
 * Set the system call handler for the specified system call.
 *
 */

int
P2_SetSyscallHandler(unsigned int number, void (*handler)(USLOSS_Sysargs *args))
{
    if (number < 0 || number >= USLOSS_MAX_SYSCALLS) {
        return P2_INVALID_SYSCALL;
    }

    USLOSS_SyscallVec[number] = (void *) handler;

    return P1_SUCCESS;
}

/*
 * P2_Spawn
 *
 * Spawn a user-level process.
 *
 */
int 
P2_Spawn(char *name, int(*func)(void *arg), void *arg, int stackSize, int priority, int *pid) 
{
    if(stackSize < USLOSS_MIN_STACK){
        return P1_INVALID_STACK;
    }

    if(priority > 5 || priority < 1){
        return P1_INVALID_PRIORITY;
    }

    for(int i = 0; i <  P1_MAXPROC; i++){
        //check if we have an open process
    }

    int rc;
    rc = P1_Fork(name, func, arg, stackSize, priority, 1, pid); //tag = 1: for user level
    if(rc != P1_SUCCESS){
        USLOSS_Console("Invalid process given to spawn.");
        USLOSS_Halt(1);
    }


    return P1_SUCCESS;
}

/*
 * SpawnStub
 *
 * Stub for Sys_Spawn system call.
 *
 */

static void
SpawnStub(USLOSS_Sysargs *sysargs)
{
    int (*func)(void *) = sysargs->arg1;
    void *arg = sysargs->arg2;
    int stackSize = (int) sysargs->arg3;
    int priority = (int) sysargs->arg4;
    char *name = sysargs->arg5;
    int pid;
    int rc = P2_Spawn(name, func, arg, stackSize, priority, &pid);
    if (rc == P1_SUCCESS) {
        sysargs->arg1 = (void *) pid;
    }
    sysargs->arg4 = (void *) rc;
}

/*
 * P2_Wait
 *
 * Wait for a user-level process.
 *
 */

int 
P2_Wait(int *pid, int *status) 
{
    return P1_SUCCESS;
}

/*
 * WaitStub
 *
 * Stub for Sys_Wait system call.
 *
 */

static void
WaitStub(USLOSS_Sysargs *sysargs)
{
    int pid = (int) sysargs->arg1;
    int status;

    int rc = P2_Wait(&pid, &status);
    sysargs->arg4 = (void *) rc;
}

/*
 * P2_Terminate
 *
 * Terminate a user-level process.
 *
 */

void 
P2_Terminate(int status) 
{

}

/*
 * TerminateStub
 *
 * Stub for Sys_Terminate system call.
 *
 */

static void
TerminateStub(USLOSS_Sysargs *sysargs)
{
    int terminationCode = (int) sysargs->arg1;
    P2_Terminate(terminationCode);
}

/*
 * GetProcInfo
 *
 * Returns process information for a given pid.
 *
 */

int
GetProcInfo(int pid, P1_ProcInfo *info) {
    return P1_GetProcInfo(pid, info);
}

/*
 * GetProcInfoStub
 *
 * Stub for Sys_ProcInfo system call.
 *
 */

static void
GetProcInfoStub(USLOSS_Sysargs *sysargs) {
    int pid = (int) sysargs->arg1;
    P1_ProcInfo *info = sysargs->arg2;

    int rc = GetProcInfo(pid, info);

    sysargs->arg4 = (void *) rc;
}

/*
 * GetPid
 *
 * Returns the current pid.
 *
 */

static void
GetPid (USLOSS_Sysargs *sysargs)
{
    sysargs->arg1 = (void *) P1_GetPid();
}

/*
 * GetTimeOfDay
 *
 * Returns the current USLOSS time.
 *
 */

static void
GetTimeOfDay (USLOSS_Sysargs *sysargs)
{
    int status;
    int rc = USLOSS_DeviceInput(USLOSS_CLOCK_DEV, 0, &status);
    assert(rc = USLOSS_DEV_OK);

    sysargs->arg1 = (void *) status;
}






