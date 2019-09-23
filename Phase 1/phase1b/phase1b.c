/*
Phase 1b
*/

#include "phase1Int.h"
#include "usloss.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

typedef struct PCB {
    int             cid;                // context's ID
    int             cpuTime;            // process's running time
    char            name[P1_MAXNAME+1]; // process's name
    int             priority;           // process's priority
    P1_State        state;              // state of the PCB
    int             sid;                // semaphor that the process is blocked on
    int             parent;             // parent ID
    int             children[P1_MAXPROC]; // children cid's
    int             numChildren;        // num of children
    int             tag;                // process's tag
    int             cpu;                // cpu consumed in usloss
    void            (*startFunc)(void *); //process the function runs
    void            *startArg;          //arguments


    // more fields here
} PCB;

static PCB processTable[P1_MAXPROC];   // the process table
PCB readyQueue[P1_MAXPROC];
int first_proc = FALSE;

void springBoard(int pid){
    assert(processTable[pid].startFunc != NULL);
    processTable[pid].startFunc(processTable[pid].startArg);

}


void P1ProcInit(void)
{
    if ((USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) != USLOSS_PSR_CURRENT_MODE) {
        USLOSS_Console("ERROR: Call to P1ProcInit from User Mode\n");
        USLOSS_Halt(1);
    }
    P1ContextInit();
    // initialize everything including the processTable
    for(int pid = 0; pid < P1_MAXPROC; pid++){
        processTable[pid].state = P1_STATE_FREE;
    }

    //todo create centinal process

}

int P1_GetPid(void) 
{
    return 0;
}

int P1_Fork(char *name, int (*func)(void*), void *arg, int stacksize, int priority, int tag, int *pid ) 
{
    int result = P1_SUCCESS;
    int enable_interrupt_flag;
    int rc;
    // check for kernel mode
    // disable interrupts
    // check all parameters
    // create a context using P1ContextCreate
    // allocate and initialize PCB
    // if this is the first process or this process's priority is higher than the 
    //    currently running process call P1Dispatch(FALSE)
    // re-enable interrupts if they were previously enabled
    if ((USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) != USLOSS_PSR_CURRENT_MODE) {
        USLOSS_Console("ERROR: Call to P1_Fork from User Mode\n");
        USLOSS_Halt(1);
    }
    enable_interrupt_flag = P1DisableInterrupts();
    if(name == '\0'){
        printf("Null name given to fork.\n");
        return P1_NAME_IS_NULL;
    }
    if(strlen(name) > P1_MAXNAME){
        printf("Name given to fork is too long.\n");
        return P1_NAME_TOO_LONG;
    }
    if(tag != 1 && tag != 0){
        printf("Invalid tag given to fork\n");
        return P1_INVALID_TAG;
    }
    if(first_proc == TRUE){
        if(priority < 0 || priority > 5){
            printf("Error invalid priority given to fork\n");
            return P1_INVALID_PRIORITY;
        }
    } else {
        if(priority != 6){
            printf("Error first process must have priority 6.\n");
            return P1_INVALID_PRIORITY;
        }
    }
    if(stacksize < USLOSS_MIN_STACK){
        printf("Invalid stack size given to fork\n");
        return P1_INVALID_STACK;
    }
    int free = FALSE;
    for(int i = 0; i < P1_MAXPROC; i++){
        if(processTable[i].state == P1_STATE_FREE){
            if(free == FALSE){
                *pid = i;
            }
            free = TRUE;

        }
        if(strcmp(name, processTable[i].name) == 0){
            printf("Duplicate name given to fork.\n");
            return P1_DUPLICATE_NAME;
        }
    }
    if(free == FALSE){
        printf("Error no free processes\n");
        return P1_TOO_MANY_PROCESSES;
    }
    int cid;
    rc = P1ContextCreate((void *)springBoard, pid, stacksize, &cid);
    if(rc != P1_SUCCESS){
        printf("Error creating context in fork.\n");
        USLOSS_Halt(1);
    }

    if(first_proc == FALSE){
        first_proc = TRUE;
        P1Dispatch(FALSE);
    } else if(priority < readyQueue[0].priority){
        P1Dispatch(FALSE);
    }
    
    if(enable_interrupt_flag){
        P1EnableInterrupts();
    }
    return result;
}

void 
P1_Quit(int status) 
{
    // check for kernel mode
    // disable interrupts
    // remove from ready queue, set status to P1_STATE_QUIT
    // if first process verify it doesn't have children, otherwise give children to first process
    // add ourself to list of our parent's children that have quit
    // if parent is in state P1_STATE_JOINING set its state to P1_STATE_READY
    P1Dispatch(FALSE);
    // should never get here
    assert(0);
}


int 
P1GetChildStatus(int tag, int *cpid, int *status) 
{
    int result = P1_SUCCESS;
    // do stuff here

    return result;
}

int
P1SetState(int pid, P1_State state, int sid) 
{
    int result = P1_SUCCESS;
    if ((USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) != USLOSS_PSR_CURRENT_MODE) {
        USLOSS_Console("ERROR: Call to P1SetState from User Mode\n");
        USLOSS_Halt(1);
    }
    if(pid < 0 | pid > P1_MAXPROC){
        printf("Invalid pid given to setState.\n");
        return P1_INVALID_PID;
    }
    if(state != P1_STATE_READY && state != P1_STATE_JOINING && state != P1_STATE_BLOCKED && state != P1_STATE_QUIT){
        printf("Invalid state given to setState.\n");
        return P1_INVALID_STATE;
    }

    if(state == P1_STATE_BLOCKED){
        processTable[pid].state = P1_STATE_BLOCKED;
        processTable[pid].sid = sid;
    } else {
        processTable[pid].state = state;

    }
    return result;
    //TODO P1_CHILD_QUIT CHECK
}

void
P1Dispatch(int rotate)
{
    // select the highest-priority runnable process
    // call P1ContextSwitch to switch to that process
}

int
P1_GetProcInfo(int pid, P1_ProcInfo *info)
{
    int result = P1_SUCCESS;
    if(pid < 0 | pid > P1_MAXPROC){
        printf("Invalid process ID passed to P1_GetProcInfo.\n");
        return P1_INVALID_PID;
    }
    // fill in info here
  //  info->name = processTable[pid].name;
    info->state = processTable[pid].state;
    info->sid = processTable[pid].sid;
    info->priority = processTable[pid].priority;
    info->tag = processTable[pid].tag;
    info->cpu = processTable[pid].cpu;
    info->parent = processTable[pid].parent;
   // info->children = processTable[pid].children;
    info->numChildren = processTable[pid].numChildren;
    //stryncpy(processTable[pid].children, )
    printf("Printing process information for #%d \n", pid);
    printf("Name:         %s\n", processTable[pid].name);
    printf("CID:          %d\n", processTable[pid].cid);
    printf("CPUTime:      %d\n", processTable[pid].cpuTime);
    printf("Priority:     %d\n", processTable[pid].priority);
    printf("State:        %d\n", processTable[pid].state);
    printf("Parent:       %d\n", processTable[pid].parent);
    printf("NumChildren:  %d\n", processTable[pid].numChildren);
    printf("Children: ");
    for(int i = 0; i < P1_MAXPROC; i++){
        if(processTable[pid].children[i] != 0){
            printf("%d ", processTable[pid].children[i]);
        }
    }
    printf("\n");
    return result;
}







