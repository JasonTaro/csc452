/*
 * Authors: Jack Mittelmeier, Kyle Snowden
 *
 * Basic interrupt functions, process control block implementation, and ability
 * to create and switch between contexts.
 */

#include "phase1Int.h"
#include "usloss.h"
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

extern  USLOSS_PTE  *P3_AllocatePageTable(int cid);
extern  void        P3_FreePageTable(int cid);

typedef struct Context {
    int             free;
    void            (*startFunc)(void *);
    void            *startArg;
    char            *stack;
    USLOSS_Context  context;
} Context;

static Context   contexts[P1_MAXPROC];

static int currentCid = -1;

float num[10];

/*
 * Helper function to call func passed to P1ContextCreate with its arg.
 */
static void launch(void)
{
    if ((USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) != USLOSS_PSR_CURRENT_MODE) {
        USLOSS_Console("ERROR: Call to launch from User Mode\n");
        USLOSS_Halt(1);
    }

    assert(contexts[currentCid].startFunc != NULL);
    contexts[currentCid].startFunc(contexts[currentCid].startArg);
}

void P1ContextInit(void)
{
    if ((USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) != USLOSS_PSR_CURRENT_MODE) {
        USLOSS_Console("ERROR: Call to P1ContextInit from User Mode\n");
        USLOSS_Halt(1);
    }

    for (int contextID = 0; contextID < P1_MAXPROC; ++contextID) { contexts[contextID].free = 1; }
}

int P1ContextCreate(void (*func)(void *), void *arg, int stacksize, int *cid) {
    if ((USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) != USLOSS_PSR_CURRENT_MODE) {
        USLOSS_Console("ERROR: Call to P1ContextCreate from User Mode\n");
        USLOSS_Halt(1);
    }

    P1DisableInterrupts();

    int result = P1_SUCCESS;

    if (stacksize < USLOSS_MIN_STACK) { result = P1_INVALID_STACK; }
    else {
        int contextID = 0;
        while (contexts[contextID].free == 0) { ++contextID; }

        if (contextID == P1_MAXPROC) { result = P1_TOO_MANY_CONTEXTS; }
        else {
            contexts[contextID].free = 0;
            contexts[contextID].startFunc = func;
            contexts[contextID].startArg = arg;
            contexts[contextID].stack = malloc(stacksize);

            USLOSS_ContextInit(&contexts[contextID].context, contexts[contextID].stack,
                               stacksize, P3_AllocatePageTable(contextID), (void *)launch);

            *cid = contextID;
        }
    }

    // find a free context and initialize it
    // allocate the stack, specify the startFunc, etc.

    P1EnableInterrupts();
    return result;
}

int P1ContextSwitch(int cid) {
    if ((USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) != USLOSS_PSR_CURRENT_MODE) {
        USLOSS_Console("ERROR: Call to P1ContextSwitch from User Mode\n");
        USLOSS_Halt(1);
    }

    P1DisableInterrupts();

    int result = P1_SUCCESS;
    // switch to the specified context
    if (cid < 0 || cid >= P1_MAXPROC) { result = P1_INVALID_CID; }
    else if (contexts[cid].free == 1) { result = P1_INVALID_CID; }
    else {
        int previousCid = currentCid;
        currentCid = cid;

        USLOSS_ContextSwitch(&contexts[previousCid].context, &contexts[cid].context);
    }

    P1EnableInterrupts();
    return result;
}

int P1ContextFree(int cid) {
    if ((USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) != USLOSS_PSR_CURRENT_MODE) {
        USLOSS_Console("ERROR: Call to P1ContextFree from User Mode\n");
        USLOSS_Halt(1);
    }

    P1DisableInterrupts();

    int result = P1_SUCCESS;
    // switch to the specified context
    if (cid < 0 || cid >= P1_MAXPROC) { result = P1_INVALID_CID; }
    else if (contexts[cid].free == 1) { result = P1_INVALID_CID; }
    else {
        Context currentContext = contexts[cid];
        free(currentContext.stack);
        currentContext.free = 1;

        P3_FreePageTable(cid);
    }

    P1EnableInterrupts();
    return result;
}


void 
P1EnableInterrupts(void) 
{
    if ((USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) != USLOSS_PSR_CURRENT_MODE) {
        USLOSS_Console("ERROR: Call to P1EnableInterrupts from User Mode\n");
        USLOSS_Halt(1);
    }

    // set the interrupt bit in the PSR
    int status;
    status = USLOSS_PsrSet(USLOSS_PsrGet() | USLOSS_PSR_CURRENT_INT);
    if (status != USLOSS_ERR_OK) { USLOSS_Halt(status); }
}

/*
 * Returns true if interrupts were enabled, false otherwise.
 */
int 
P1DisableInterrupts(void)
{
    // set enabled to TRUE if interrupts are already enabled
    // clear the interrupt bit in the PSR
    if ((USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) != USLOSS_PSR_CURRENT_MODE) {
        USLOSS_Console("ERROR: Call to P1DisableInterrupts from User Mode\n");
        USLOSS_Halt(1);
    }

    int enabled = FALSE;
    int status;
    if ((USLOSS_PsrGet() & USLOSS_PSR_CURRENT_INT) == USLOSS_PSR_CURRENT_INT) { enabled = TRUE; }
    status = USLOSS_PsrSet(USLOSS_PsrGet() & ~USLOSS_PSR_CURRENT_INT);
    if (status != USLOSS_ERR_OK) { USLOSS_Halt(status); }
    return enabled;
}
