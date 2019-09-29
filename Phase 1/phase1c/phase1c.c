
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
        USLOSS_Console("ERROR: Call to semaphore V from user mode.\n");
        USLOSS_IllegalInstruction();
        P1_Quit(1024);
    }
    int result = P1_SUCCESS;
    int prev_enabled = P1DisableInterrupts();
    if(*name == '\0'){
        return P1_NAME_IS_NULL;
    }
    if(strlen(name) > P1_MAXNAME){
        return P1_NAME_TOO_LONG;
    }

    for(int i = 0; i < P1_MAXSEM; i++){
        if(strcmp(sems[i].name, name) == 0){
            USLOSS_Console("ERROR semaphore name already in use.\n");
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
        return P1_TOO_MANY_SEMS;
    }
    strcpy(sems[*sid].name, name);
    sems[*sid].value = value;
    sems[*sid].free = 0;
    // check for kernel mode
    // disable interrupts
    // check parameters
    // find a free Sem and initialize it
    // re-enable interrupts if they were previously enabled
    if(prev_enabled){
        P1EnableInterrupts();
    }
    return result;
}

int P1_SemFree(int sid) 
{
    int     result = P1_SUCCESS;
    // more code here
    return result;
}

int P1_P(int sid) 
{
    int result = P1_SUCCESS;
    // check for kernel mode
    // disable interrupts
    // while value == 0
    //      set state to P1_STATE_BLOCKED
    // value--
    // re-enable interrupts if they were previously enabled
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
        USLOSS_Console("ERROR: Call to semaphore V from user mode.\n");
        USLOSS_IllegalInstruction();
        P1_Quit(1024);
    }

    int result = P1_SUCCESS;
    if(sid < 0 || sid >= P1_MAXSEM){
        return P1_INVALID_SID;
    }

    int prev_enabled = P1DisableInterrupts();
    sems[sid].value++;
    if(sems[sid].blocked_queue_head != NULL){
        //todo deQ head
        P1SetState(sems[sid].blocked_queue_head->pid, P1_STATE_READY, -1);
    }

    if(prev_enabled){
        P1EnableInterrupts();
    }

    return result;
}

int P1_SemName(int sid, char *name) {
    int result = P1_SUCCESS;
    // more code here
    return result;
}

