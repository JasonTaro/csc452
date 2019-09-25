/*
Phase 1b

Jack Mittelmeier, Kyle Snowden
*/

#include "phase1Int.h"
#include "usloss.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

typedef struct deadChild {
    int     deadChildPID;
    struct deadChild* next;
    int exitStatus;
} deadChild;

typedef struct PCB {
    int             cid;                // context's ID
    int             pid;                // process' pid
    int             cpuTime;            // process's running time
    char            name[P1_MAXNAME+1]; // process's name
    int             priority;           // process's priority
    P1_State        state;              // state of the PCB
    int             sid;                // semaphor that the process is blocked on
    int             parent;             // parent ID
    int             children[P1_MAXPROC]; // children cid's
    deadChild*      murkedKidsQueue;    // murdered children queue
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
int first_proc = TRUE;
static int currentPID = -1;


void P1_enQueueDeadChild (int parentPID, int childPID, int status) {
    deadChild* newChild = malloc(sizeof(deadChild));
    newChild->next = NULL;
    newChild->deadChildPID = childPID;
    newChild->exitStatus = status;
    if (processTable[parentPID].murkedKidsQueue == NULL) {
        processTable[parentPID].murkedKidsQueue = newChild;
    } else {

        deadChild* current = processTable[parentPID].murkedKidsQueue;

        while (current->next != NULL) { current = current->next; }

        current->next = newChild;
    }
}

int P1_deQueueDeadChild(int parentPID) {
    if (processTable[parentPID].murkedKidsQueue == NULL) {
        return -1;
    } else {
        deadChild* head = processTable[parentPID].murkedKidsQueue;
        int returnValue = head->deadChildPID;
        processTable[parentPID].murkedKidsQueue = head->next;
        free(head);
        return returnValue;
    }
}

void P1_RemoveFromReadyQueue(int pid) {
    PCBNode* current = ready_q_head;

    if (current == NULL) {
        return;
    } else if (current->pid == pid) {
        ready_q_head = current->next;
        free(current);
    } else {
        while (current->next != NULL && current->next->pid != pid) {
            current = current->next;
        }
        if (current->next == NULL) { return; }
        PCBNode* toFree = current->next;
        current->next = current->next->next;
        free(toFree);
    }
}

void P1_enQueue(int pid){
    if(ready_q_head == NULL){
        ready_q_head = malloc(sizeof(PCBNode));
        ready_q_head->priority = processTable[pid].priority;
        ready_q_head->pid = pid;
        ready_q_head->next = NULL;
        ready_q_head->process = &processTable[pid];
        return;
    }
    PCBNode *newNode;
    newNode = malloc(sizeof(PCBNode));
    newNode->priority = processTable[pid].priority;
    newNode->pid = pid;
    newNode->process = &processTable[pid];
    if(newNode->priority < ready_q_head->priority){
        newNode->next = ready_q_head;
        ready_q_head = newNode;
        return;
    }

    PCBNode *curNode = ready_q_head->next;
    PCBNode *prevNode = ready_q_head;
    while(curNode != NULL){
        if(newNode->priority < curNode->priority){
            prevNode->next = newNode;
            newNode->next = curNode;
            return;
        }
        prevNode = curNode;
        curNode = curNode->next;
    }
    prevNode->next = newNode;
    return;
}

PCB* P1_deQueue(int flagged){

    if(ready_q_head == NULL){
        return NULL;
    }
    if(flagged == TRUE){
        if(ready_q_head->next != NULL){
            if(ready_q_head->next->priority == ready_q_head->priority){
                PCB *aux = ready_q_head->next->process;
                PCBNode *newNext = ready_q_head->next->next;
                free(ready_q_head->next);
                ready_q_head->next = newNext;
                return aux;
            }
        }
    }
    PCB *aux = ready_q_head->process;
    PCBNode *newHead = ready_q_head->next;
    free(ready_q_head);
    ready_q_head = newHead;
    return aux;

}

static void springBoard(void* args){
    int pid = P1_GetPid();
    assert(processTable[pid].startFunc != NULL);
    int rc = processTable[pid].startFunc(args);
    P1_Quit(rc);
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

    if(interruptsEnabled) { P1EnableInterrupts(); }

}

int P1_GetPid(void) 
{
    return currentPID;
}

int P1_Fork(char *name, int (*func)(void*), void *arg, int stacksize, int priority, int tag, int *pid )
{

    //todo forking and quitting should be able to be infinite - test case

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

            processTable[*pid].startFunc = (void *) (*func);
            processTable[*pid].startArg = arg;
            int cid = 0;
            rc = P1ContextCreate((void *)springBoard, arg, stacksize, &cid);

            if (rc != P1_SUCCESS) {
                printf("Error creating context in fork.\n");
                USLOSS_Halt(1);
            }
            rc = P1SetState(*pid, P1_STATE_READY, 0);
            if(rc != P1_SUCCESS){
                printf("Unable to set process %d to ready.\n", *pid);
                USLOSS_Halt(1);
            }
            P1_enQueue(*pid);
            if (first_proc == TRUE) {
                first_proc = FALSE;
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
    int pid = P1_GetPid();

    processTable[pid].state = P1_STATE_QUIT;

    for (int indexProcess = 0; indexProcess < P1_MAXPROC; ++indexProcess) {

        // set all child processes' parent to first process
        if (processTable[indexProcess].parent == pid) { processTable[indexProcess].parent = 0; }
    }

    int parentPID = processTable[pid].parent;

    // add this process as a dead child to the parent
    P1_enQueueDeadChild(parentPID, pid, status);

    for (int indexChildren = 0; indexChildren < P1_MAXPROC; ++indexChildren) {
        if (processTable[parentPID].children[indexChildren] == pid) {

            // remove this process from its parent's list of current children
            processTable[parentPID].children[indexChildren] = 0;
            break;
        }
    }

    if (processTable[parentPID].state == P1_STATE_JOINING) {
        P1_enQueue(parentPID);
        int rc = P1SetState(parentPID, P1_STATE_READY, 0);
        if(rc != P1_SUCCESS){
            printf("Could not set process %d to ready.\n", parentPID);
            USLOSS_Halt(1);
        }
    }

    P1_RemoveFromReadyQueue(pid);

    // remove from ready queue, set status to P1_STATE_QUIT
    // if first process verify it doesn't have children, otherwise give children to first process
    // add ourself to list of our parent's children that have quit
    // if parent is in state P1_STATE_JOINING set its state to P1_STATE_READY
    if(interruptsEnabled){ P1EnableInterrupts(); }

    P1Dispatch(FALSE);
    // should never get here
    assert(0);



}


int 
P1GetChildStatus(int tag, int *cpid, int *status) 
{
    //relevant to the currently running process
    //put child's quit status into status
    int result = P1_SUCCESS;
    if(tag != 1 && tag != 0){
        result = P1_INVALID_TAG;
    } //else if(processTable[pid].murkedKidsQueue)
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


    //free child in join
    int result = P1_SUCCESS;
    if (pid < 0 | pid > P1_MAXPROC){
        printf("Invalid pid given to setState.\n");
        result = P1_INVALID_PID;
    } else if (state != P1_STATE_READY && state != P1_STATE_JOINING && state != P1_STATE_BLOCKED && state != P1_STATE_QUIT){
        printf("Invalid state given to setState.\n");
        result = P1_INVALID_STATE;
    } else {
        if (state == P1_STATE_BLOCKED){
            processTable[pid].state = P1_STATE_BLOCKED;
            processTable[pid].sid = sid;
        } else if (state == P1_STATE_JOINING && processTable[pid].murkedKidsQueue != NULL) {
            printf("Tried to set P1_Joining but process's child has already quit.\n");
            result = P1_CHILD_QUIT;
        } else {
            processTable[pid].state = state;
        }
    }


    if(interruptsEnabled){ P1EnableInterrupts(); }

    return result;
}

void
P1Dispatch(int rotate)
{
    printf("current PID before hand: %d \n", currentPID);
    if ((USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) != USLOSS_PSR_CURRENT_MODE) {
        USLOSS_Console("ERROR: Call to P1Dispatch from User Mode\n");
        USLOSS_Halt(1);
    }
    int interruptsEnabled = P1DisableInterrupts();
    PCB *new_running_process = P1_deQueue(rotate);
    if(new_running_process == NULL){
        printf("No runnable processes, halting.\n");
        USLOSS_Halt(0);
    }
    currentPID = new_running_process->pid;

    // select the highest-priority runnable process
    // call P1ContextSwitch to switch to that process
    P1ContextSwitch(new_running_process->cid);


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
    strcpy(info->name, processTable[pid].name);
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






