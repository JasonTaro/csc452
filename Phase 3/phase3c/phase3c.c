/*
 * phase3c.c
 *
 */

#include <assert.h>
#include <phase1.h>
#include <phase2.h>
#include <usloss.h>
#include <string.h>
#include <libuser.h>
#include "phase3.h"
#include "phase3Int.h"

#ifdef DEBUG
int debugging3 = 1;
#else
int debugging3 = 0;
#endif

void debug3(char *fmt, ...)
{
    va_list ap;

    if (debugging3) {
        va_start(ap, fmt);
        USLOSS_VConsole(fmt, ap);
    }
}

// This allows the skeleton code to compile. Remove it in your solution.

#define UNUSED __attribute__((unused))


typedef struct Frame {
    USLOSS_PTE* page;
    int num;
    int inUse;
} Frame;
Frame* framesInUse;
static int Pager(void *arg);
SID faultMutex;
SID pagerMutex;
static int pager_init;
int pager_count = 1;
/*
 *----------------------------------------------------------------------
 *
 * P3FrameInit --
 *
 *  Initializes the frame data structures.
 *
 * Results:
 *   P3_ALREADY_INITIALIZED:    this function has already been called
 *   P1_SUCCESS:                success
 *
 *----------------------------------------------------------------------
 */
int
P3FrameInit(int pages, int frames)
{
    int result = P1_SUCCESS;
    int rc;
    if (framesInUse != NULL) { return P3_ALREADY_INITIALIZED; }

    framesInUse = malloc(sizeof(Frame) * frames);
    for (int i = 0; i < frames; i++) {
        framesInUse[i].inUse = 0;
        framesInUse[i].num = i;
        framesInUse[i].page = NULL;
    }

    P3_vmStats.freeFrames = frames;
    P3_vmStats.pages = pages;
    P3_vmStats.frames = frames;

    rc = P1_SemCreate("Fault Mutex", 1, &faultMutex); assert(rc == P1_SUCCESS);
    // initialize the frame data structures, e.g. the pool of free frames
    // set P3_vmStats.freeFrames

    return result;
}
/*
 *----------------------------------------------------------------------
 *
 * P3FrameShutdown --
 *
 *  Cleans up the frame data structures.
 *
 * Results:
 *   P3_NOT_INITIALIZED:    P3FrameInit has not been called
 *   P1_SUCCESS:            success
 *
 *----------------------------------------------------------------------
 */
int
P3FrameShutdown(void)
{
    int result = P1_SUCCESS;

    if (framesInUse == NULL) { return P3_NOT_INITIALIZED; }

    free(framesInUse);

    // clean things up

    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * P3FrameFreeAll --
 *
 *  Frees all frames used by a process
 *
 * Results:
 *   P3_NOT_INITIALIZED:    P3FrameInit has not been called
 *   P1_SUCCESS:            success
 *
 *----------------------------------------------------------------------
 */

int
P3FrameFreeAll(int pid)
{
    int result = P1_SUCCESS;

    if (framesInUse == NULL) { return P3_NOT_INITIALIZED; }
    else if (pid < 0 || pid > P1_MAXPROC) { return P1_INVALID_PID; }

    USLOSS_PTE* table;
    int rc = P3PageTableGet(pid, &table);
    assert (rc == P1_SUCCESS);

    int i;
    for (i = 0; i < P3_vmStats.pages; i++) {
        if (table[i].incore == 1) {
            framesInUse[table[i].frame].inUse = 0;
            framesInUse[table[i].frame].page = NULL;
        }
    }

    // free all frames in use by the process (P3PageTableGet)

    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * P3FrameMap --
 *
 *  Maps a frame to an unused page and returns a pointer to it.
 *
 * Results:
 *   P3_NOT_INITIALIZED:    P3FrameInit has not been called
 *   P3_OUT_OF_PAGES:       process has no free pages
 *   P1_INVALID_FRAME       the frame number is invalid
 *   P1_SUCCESS:            success
 *
 *----------------------------------------------------------------------
 */
int
P3FrameMap(int frame, void **ptr) 
{
    if (framesInUse == NULL) { return P3_NOT_INITIALIZED; }
    else if (frame < 0 || frame >= P3_vmStats.frames) { return P3_INVALID_FRAME; }

    int i, pid, rc;
    pid = P1_GetPid();
    USLOSS_PTE* table;
    rc =  P3PageTableGet(pid, &table);
    for (i = 0; i < P3_vmStats.pages; i++) {
        if (table[i].incore == 0) {
            table[i].incore = 1;
            table[i].frame = frame;
            *ptr = &table[i];

            framesInUse[frame].inUse = 1;
            framesInUse[frame].page = &table[i];

            rc = USLOSS_MmuSetPageTable(table); assert(rc == USLOSS_MMU_OK);
            return P1_SUCCESS;
        }
    }

    return P3_OUT_OF_PAGES;

    // get the page table for the process (P3PageTableGet)
    // find an unused page
    // update the page's PTE to map the page to the frame
    // update the page table in the MMU (USLOSS_MmuSetPageTable)
}
/*
 *----------------------------------------------------------------------
 *
 * P3FrameUnmap --
 *
 *  Opposite of P3FrameMap. The frame is unmapped.
 *
 * Results:
 *   P3_NOT_INITIALIZED:    P3FrameInit has not been called
 *   P3_FRAME_NOT_MAPPED:   process didn’t map frame via P3FrameMap
 *   P1_INVALID_FRAME       the frame number is invalid
 *   P1_SUCCESS:            success
 *
 *----------------------------------------------------------------------
 */
int
P3FrameUnmap(int frame) 
{
    if (framesInUse == NULL) { return P3_NOT_INITIALIZED; }
    else if (frame < 0 || frame >= P3_vmStats.frames) { return P3_INVALID_FRAME; }

    int i, pid, rc;
    pid = P1_GetPid();
    USLOSS_PTE* table;
    rc = P3PageTableGet(pid, &table);
    for (i = 0; i < P3_vmStats.pages; i++) {
        if (table[i].frame == frame && table[i].incore == 1) {

            table[i].incore = 0;

            framesInUse[frame].inUse = 0;
            framesInUse[frame].page = NULL;

            rc = USLOSS_MmuSetPageTable(table); assert(rc == USLOSS_MMU_OK);
            return P1_SUCCESS;
        }
    }

    return P3_FRAME_NOT_MAPPED;

    // get the process's page table (P3PageTableGet)
    // verify that the process mapped the frame
    // update page's PTE to remove the mapping
    // update the page table in the MMU (USLOSS_MmuSetPageTable)
}

// information about a fault. Add to this as necessary.

typedef struct Fault {
    PID         pid;
    int         offset;
    int         page;
    int         cause;
    SID         wait;
    // other stuff goes here
    struct Fault*      nextFault;
} Fault;

Fault* faultQueueHead;

int faultNum = 0;

/*
 *----------------------------------------------------------------------
 *
 * FaultHandler --
 *
 *  Page fault interrupt handler
 *
 *----------------------------------------------------------------------
 */

static void
FaultHandler(int type, void *arg)
{
    int rc;

    rc = P1_P(faultMutex); assert(rc == P1_SUCCESS);

    // FILL FAULT STRUCT
    Fault   fault UNUSED;
    fault.offset = (int) arg;
    fault.page = fault.offset / USLOSS_MmuPageSize();
    fault.pid = P1_GetPid();
    fault.cause = type;
    fault.nextFault = NULL;

    // CREATE SEMAPHORE FOR FAULT TO WAIT
    char waitName[32];
    snprintf(waitName, 32, "Wait Semaphore %d", faultNum++);
    rc = P1_SemCreate(waitName, 0, &fault.wait); assert (rc == P1_SUCCESS);

    // ADD FAULT TO QUEUE
    if (faultQueueHead == NULL) {
        faultQueueHead = &fault;
    } else {
        Fault* current = faultQueueHead;
        while (current->nextFault != NULL) { current = current->nextFault; }
        current->nextFault = &fault;
    }

    rc = P1_V(faultMutex); assert(rc == P1_SUCCESS);

    // WAIT FOR FAULT TO BE HANDLED BEFORE RETURNING
    rc = P1_P(fault.wait); assert (rc == P1_SUCCESS);

    // fill in other fields in fault
    // add to queue of pending faults
    // let pagers know there is a pending fault
    // wait for fault to be handled
}

struct Pager {
    SID         startup;
    SID         completed;
    int         abort;
};

int pagersNum;
SID pagerWait;
struct Pager* pagerTable;

/*
 *----------------------------------------------------------------------
 *
 * P3PagerInit --
 *
 *  Initializes the pagers.
 *
 * Results:
 *   P3_ALREADY_INITIALIZED: this function has already been called
 *   P3_INVALID_NUM_PAGERS:  the number of pagers is invalid
 *   P1_SUCCESS:             success
 *
 *----------------------------------------------------------------------
 */
int
P3PagerInit(int pages, int frames, int pagers)
{
    if (pagerTable != NULL) { return P3_ALREADY_INITIALIZED; }
    if (pagers < 1) { return P3_INVALID_NUM_PAGERS; }

    int     result = P1_SUCCESS;
    int     rc;

    USLOSS_IntVec[USLOSS_MMU_INT] = FaultHandler;


    pagersNum = pagers;

    for (int pager = 0; pager < pagers; pager++) {

        // INITIALIZE THE SEMAPHORES
        char startupName[32];
        snprintf(startupName, 32, "Startup Semaphore %d", pager);
        rc = P1_SemCreate(startupName, 0, &(pagerTable[pager].startup));
        assert(rc == P1_SUCCESS);

        char completedSemName[32];
        snprintf(completedSemName, 32, "Completed Semaphore %d", pager);
        rc = P1_SemCreate(completedSemName, 0, &(pagerTable[pager].completed));
        assert(rc == P1_SUCCESS);

        // INITIALIZE ABORT FIELD
        pagerTable[pager].abort = FALSE;

        // FORK THE PROCESS
        int pid;
        char processesName[32];
        snprintf(processesName, 32, "Pager %d", pager);
        rc = P1_Fork(processesName, Pager, (void *)(uintptr_t) pager, 4*USLOSS_MIN_STACK, 2, 0, &pid);
        assert(rc == P1_SUCCESS);

        // WAIT FOR IT TO START RUNNING
        rc = P1_P(pagerTable[pager].startup);
        assert(rc == P1_SUCCESS);
    }
    return result;

    // initialize the pager data structures
    // fork off the pagers and wait for them to start running


}

/*
 *----------------------------------------------------------------------
 *
 * P3PagerShutdown --
 *
 *  Kills the pagers and cleans up.
 *
 * Results:
 *   P3_NOT_INITIALIZED:     P3PagerInit has not been called
 *   P1_SUCCESS:             success
 *
 *----------------------------------------------------------------------
 */
int
P3PagerShutdown(void)
{
    if (pagerTable == NULL) { return P3_NOT_INITIALIZED; }
    int rc;
    for (int pager = 0; pager < pagersNum; pager++) {
        pagerTable[pager].abort = TRUE;
    }


    rc = P1_SemFree(faultMutex); assert(rc == P1_SUCCESS);

    //V

    // cause the pagers to quit
    // clean up the pager data structures

    return P1_SUCCESS;
}

/*
 *----------------------------------------------------------------------
 *
 * Pager --
 *
 *  Handles page faults
 *
 *----------------------------------------------------------------------
 */

static int
Pager(void *arg)
{
    int pager = (int) arg;
    int rc;

    // LET STARTUP KNOW THE PROCESS IS RUNNING
    rc = P1_V(pagerTable[pager].startup); assert(rc == P1_SUCCESS);


    while (TRUE) {
        rc = P1_P(faultMutex); assert(rc == P1_SUCCESS);
        Fault* currentFault = faultQueueHead;
        faultQueueHead = currentFault->nextFault;
        rc = P1_V(faultMutex); assert(rc == P1_SUCCESS);

        rc = P1_P(pagerWait); assert(rc == P1_SUCCESS);


        //abort = 1
        //access = 2
        if (currentFault->cause == 1) {
            break;
        } else if (currentFault->cause == 2){
            P2_Terminate(0);
        } else {
            int frame  = - 1;
            for (int i = 0; i < P3_vmStats.frames; i++) {
                if (framesInUse[i].inUse == 0) { frame = i; }
            }

            if (frame == -1) {
                rc = P3SwapOut(&frame); assert(rc == P1_SUCCESS);
            }

            rc = P3SwapIn(P1_GetPid(), currentFault->page, frame);

            if (rc == P3_EMPTY_PAGE) {
                void* addr;
                int rc2;
                rc2 = P3FrameMap(frame, &addr);
            } else if (rc == P3_OUT_OF_SWAP) {
                P2_Terminate(0);
            }


        }
    }
    /********************************

    notify P3PagerInit that we are running
    loop until P3PagerShutdown is called
        wait for a fault
        if it's an access fault kill the faulting process
        if there are free frames
            frame = a free frame
        else
            P3SwapOut(&frame);
        rc = P3SwapIn(pid, page, frame)
        if rc == P3_EMPTY_PAGE
            P3FrameMap(frame, &addr)
            zero-out frame at addr
            P3FrameUnmap(frame);
        else if rc == P3_OUT_OF_SWAP
            kill the faulting process
        update PTE in faulting process's page table to map page to frame
        unblock faulting process

    **********************************/

    return 0;
}
