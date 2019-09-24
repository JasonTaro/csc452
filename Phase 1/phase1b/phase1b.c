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
    int             (*startFunc)(void *); //process the function runs
    void            *startArg;          //arguments


    // more fields here
} PCB;

typedef struct PCBNode{
    int pid;
    struct PCBNode *next;
    int priority;
    PCB *process;
} PCBNode;

static PCB processTable[P1_MAXPROC];   // the process table
PCBNode *ready_q_head = NULL;
int first_proc = FALSE;
static int currentPID = -1;


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

    int interruptsEnabled = P1DisableInterrupts();

    P1ContextInit();
    // initialize everything including the processTable
    for(int pid = 0; pid < P1_MAXPROC; pid++){
        processTable[pid].state = P1_STATE_FREE;
    }

    //todo create centinal process

    if(interruptsEnabled) { P1EnableInterrupts(); }

}

int P1_GetPid(void) 
{
    return currentPID;
}

int P1_Fork(char *name, int (*func)(void*), void *arg, int stacksize, int priority, int tag, int *pid ) 
{

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

    int interruptsEnabled = P1DisableInterrupts();

    int result = P1_SUCCESS;
    int rc;
    if (name == '\0'){
        printf("Null name given to fork.\n");
        result = P1_NAME_IS_NULL;
    } else if (strlen(name) > P1_MAXNAME){
        printf("Name given to fork is too long.\n");
        result = P1_NAME_TOO_LONG;
    } else if (tag != 1 && tag != 0){
        printf("Invalid tag given to fork\n");
        result = P1_INVALID_TAG;
    } else if (first_proc == FALSE && (priority < 1 || priority > 5)){
        printf("Error invalid priority given to fork\n");
        result = P1_INVALID_PRIORITY;
    } else if (first_proc == TRUE && priority != 6) {
        printf("Error first process must have priority 6.\n");
        result = P1_INVALID_PRIORITY;
    } else if(stacksize < USLOSS_MIN_STACK) {
        printf("Invalid stack size given to fork\n");
        result = P1_INVALID_STACK;
    } else {

        int free = FALSE;
        for (int i = 0; i < P1_MAXPROC; i++) {
            if (processTable[i].state == P1_STATE_FREE) {
                if (free == FALSE) {
                    *pid = i;
                }
                free = TRUE;

            }
            if (strcmp(name, processTable[i].name) == 0) {
                printf("Duplicate name given to fork.\n");
                if(interruptsEnabled){ P1EnableInterrupts(); }
                return P1_DUPLICATE_NAME;
            }
        }
        if (free == FALSE) {
            printf("Error no free processes\n");
            result = P1_TOO_MANY_PROCESSES;
        } else {
            int cid;
            processTable[*pid].startFunc = func;
            processTable[*pid].startArg = arg;
            rc = P1ContextCreate((void *) springBoard, pid, stacksize, &cid);
            if (rc != P1_SUCCESS) {
                printf("Error creating context in fork.\n");
                USLOSS_Halt(1);
            }

            if (first_proc == FALSE) {
                first_proc = TRUE;
                P1Dispatch(FALSE);
            } else if (priority <  processTable[currentPID].priority) {
                P1Dispatch(FALSE);
            }
        }

    }
    
    if(interruptsEnabled){ P1EnableInterrupts(); }
    return result;
}

void 
P1_Quit(int status) 
{

    if ((USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) != USLOSS_PSR_CURRENT_MODE) {
        USLOSS_Console("ERROR: Call to P1_Quit from User Mode\n");
        USLOSS_Halt(1);
    }

    int interruptsEnabled = P1DisableInterrupts();

    // remove from ready queue, set status to P1_STATE_QUIT
    // if first process verify it doesn't have children, otherwise give children to first process
    // add ourself to list of our parent's children that have quit
    // if parent is in state P1_STATE_JOINING set its state to P1_STATE_READY
    P1Dispatch(FALSE);
    // should never get here
    assert(0);

    if(interruptsEnabled){ P1EnableInterrupts(); }

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
    if ((USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) != USLOSS_PSR_CURRENT_MODE) {
        USLOSS_Console("ERROR: Call to P1SetState from User Mode\n");
        USLOSS_Halt(1);
    }
    int interruptsEnabled = P1DisableInterrupts();



    int result = P1_SUCCESS;
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

    if(interruptsEnabled){ P1EnableInterrupts(); }

    return result;
    //TODO P1_CHILD_QUIT CHECK
}

void
P1Dispatch(int rotate)
{
    if ((USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) != USLOSS_PSR_CURRENT_MODE) {
        USLOSS_Console("ERROR: Call to P1Dispatch from User Mode\n");
        USLOSS_Halt(1);
    }
    int interruptsEnabled = P1DisableInterrupts();

    // select the highest-priority runnable process
    // call P1ContextSwitch to switch to that process

    if(interruptsEnabled){ P1EnableInterrupts(); }

}

int
P1_GetProcInfo(int pid, P1_ProcInfo *info)
{

    int interruptsEnabled = P1DisableInterrupts();

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

    if(interruptsEnabled){ P1EnableInterrupts(); }

    return result;
}
/*
void enQueue(pid){
    PCBNode *newNode;
    newNode->pid = pid;
    newNode->priority = processTable[pid].priority;
    if(ready_q_head == NULL){
        ready_q_head = newNode;
        ready_q_head->next = NULL;
        return;
    } else{
        PCBNode *curNode = ready_q_head;
        PCBNode *prevNode;
        int priority = processTable[pid].priority;
        int placed = FALSE;
        while(curNode->next != NULL){
            if(priority < curNode->priority){
                if(curNode == ready_q_head){
                    prevNode = *ready_q_head;
                    ready_q_head = &newNode;
                    ready_q_head->next = &prevNode;
                    return;
                } else{
                    prevNode.next = &newNode;
                    newNode.next = &curNode;
                    return;
                }
            }
            curNode = curNode.next;
        }
        curNode.next = newNode;
    }
}

*/







