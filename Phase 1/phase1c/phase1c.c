
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <string.h>
#include "usloss.h"
#include "phase1Int.h"
typedef struct blocked_queue {
    int                     pid;
    struct blocked_queue*   next;
} blocked_queue;

typedef struct Sem {
    char        name[P1_MAXNAME+1];
    u_int       value;
    int         free;
    struct blocked_queue*        blocked_queue_head;
    // more fields here
} Sem;


static Sem sems[P1_MAXSEM];

void addBlockedSemaphore(Sem* semaphore, int pid) {
    blocked_queue* newBlocked = malloc(sizeof(blocked_queue));
    newBlocked->next = NULL;
    newBlocked->pid = pid;

    if (semaphore->blocked_queue_head == NULL) {
        semaphore->blocked_queue_head = newBlocked;
    } else {
        blocked_queue *current = semaphore->blocked_queue_head;
        while (current->next != NULL) { current = current->next; }
        current->next = newBlocked;
    }
}

int getBlockedSemaphore(Sem* semaphore) {
    if (semaphore->blocked_queue_head == NULL) {
        return -1;
    } else {
        int returnVal = semaphore->blocked_queue_head->pid;
        blocked_queue *tmp = semaphore->blocked_queue_head;
        semaphore->blocked_queue_head = semaphore->blocked_queue_head->next;
        free(tmp);
        return returnVal;
    }
}


void 
P1SemInit(void) 
{
    P1ProcInit();
    // initialize sems here
    for(int i = 0; i < P1_MAXSEM; i++){
        sems[i].value = -1;
        sems[i].free = 1;
        sems[i].blocked_queue_head = NULL;
    }

}

int P1_SemCreate(char *name, unsigned int value, int *sid) {
    if ((USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) != USLOSS_PSR_CURRENT_MODE) {
        USLOSS_Console("ERROR: Call to P1_SemCreate from user mode.\n");
        USLOSS_IllegalInstruction();
        P1_Quit(1024);
    }
    int interruptsEnabled = P1DisableInterrupts();

    int result = P1_SUCCESS;
    if (*name == '\0'){
        result = P1_NAME_IS_NULL;
    } else if (strlen(name) > P1_MAXNAME){
        result = P1_NAME_TOO_LONG;
    } else {
        for(int i = 0; i < P1_MAXSEM; i++){
            if(strcmp(sems[i].name, name) == 0){
                USLOSS_Console("ERROR semaphore name already in use.\n");
                if(interruptsEnabled) { P1EnableInterrupts(); }
                return P1_DUPLICATE_NAME;
            }
        }

        int free = 0;
        for(int i = 0; i < P1_MAXSEM; i++){
            if(sems[i].free == 1){
                free = 1;
                *sid = i;
                break;
            }
        }
        if(free == 0){
            result = P1_TOO_MANY_SEMS;
        } else {
            strcpy(sems[*sid].name, name);
            sems[*sid].value = value;
            sems[*sid].free = 0;
            strcpy(sems[*sid].name, "\0");
        }
    }



    // check for kernel mode
    // disable interrupts
    // check parameters
    // find a free Sem and initialize it
    // re-enable interrupts if they were previously enabled


    return result;
}

int P1_SemFree(int sid) {
    int result = P1_SUCCESS;
    if (sid < 0 || sid >= P1_MAXSEM) {
        result = P1_INVALID_SID;
    } else if (sems[sid].blocked_queue_head != NULL) {
        result = P1_BLOCKED_PROCESSES;
    } else {
        sems[sid].free = 1;
        sems[sid].value = -1;
        strcpy(sems[sid].name, "\0");
    }
    return result;
}

int P1_P(int sid) {
    if ((USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) != USLOSS_PSR_CURRENT_MODE) {
        USLOSS_Console("ERROR: Call to P1_P from user mode.\n");
        USLOSS_IllegalInstruction();
        P1_Quit(1024);
    }
    int interruptsEnabled = P1DisableInterrupts();

    int result = P1_SUCCESS;
    if (sid < 0 || sid >= P1_MAXSEM){
        result = P1_INVALID_SID;
    } else {

        while(sems[sid].value == 0) {
            addBlockedSemaphore(&sems[sid], P1_GetPid());
            P1SetState(P1_GetPid(), P1_STATE_BLOCKED, sid);
            P1Dispatch(0);
        }

        sems[sid].value--;
    }


    // check for kernel mode
    // disable interrupts
    // while value == 0
    //      set state to P1_STATE_BLOCKED
    // value--
    // re-enable interrupts if they were previously enabled

    if (interruptsEnabled) { P1EnableInterrupts(); }
    return result;
}

int P1_V(int sid) 
{
    // check for kernel mode
    // disable interrupts
    // value++
    // if a process is waiting for this semaphore
    //      set the process's state to P1_STATE_READY
    // re-enable interrupts if they were previously enabled
    if ((USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) != USLOSS_PSR_CURRENT_MODE) {
        USLOSS_Console("ERROR: Call to P1_V from user mode.\n");
        USLOSS_IllegalInstruction();
        P1_Quit(1024);
    }

    int interruptsEnabled = P1DisableInterrupts();

    int result = P1_SUCCESS;
    if (sid < 0 || sid >= P1_MAXSEM){
        result = P1_INVALID_SID;
    } else {
        sems[sid].value++;
        int blockedSemPID = getBlockedSemaphore(&sems[sid]);
        if (blockedSemPID != -1) {
            P1SetState(blockedSemPID, P1_STATE_READY, -1);
        }
    }

    if(interruptsEnabled){ P1EnableInterrupts(); }
    return result;
}

int P1_SemName(int sid, char *name) {
    int result = P1_SUCCESS;
    if (sid < 0 || sid >= P1_MAXSEM) {
        result = P1_INVALID_SID;
    } else if (strcmp(sems[sid].name, "\0") == 0) {
        result = P1_NAME_IS_NULL;
    } else {
        strcpy(name, sems[sid].name);
    }
    return result;
}

