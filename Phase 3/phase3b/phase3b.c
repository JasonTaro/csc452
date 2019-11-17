/*
 * phase3b.c
 *
 */

#include <assert.h>
#include <phase1.h>
#include <phase2.h>
#include <usloss.h>
#include <string.h>
#include <libuser.h>

#include "phase3Int.h"

void
P3PageFaultHandler(int type, void *arg)
{
    if ((USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) != USLOSS_PSR_CURRENT_MODE) {
        USLOSS_Console("ERROR: Call to P3PageFaultHandler from user mode.\n");
        USLOSS_IllegalInstruction();
    }

    /*******************

    if the cause is USLOSS_MMU_FAULT (USLOSS_MmuGetCause)
        if the process does not have a page table  (P3PageTableGet)
            print error message
            USLOSS_Halt(1)
        else
            determine which page suffered the fault (USLOSS_MmuPageSize)
            update the page's PTE to map page x to frame x
            set the PTE to be read-write and incore
            update the page table in the MMU (USLOSS_MmuSetPageTable)
    else
        print error message
        USLOSS_Halt(1)
    *********************/

    int rc;
    if (USLOSS_MmuGetCause() == USLOSS_MMU_FAULT) {
        USLOSS_PTE* currentPageTable;
        if (P3PageTableGet(P1_GetPid(), &currentPageTable) != P1_SUCCESS) {
            USLOSS_Console("ERROR: Current process does not have page table\n");
            USLOSS_Halt(1);
        } else {
            int offset = (int) arg;
            int page = offset / USLOSS_MmuPageSize();
            USLOSS_Console("%d\n", page);

            currentPageTable[page].incore = 1;
            currentPageTable[page].frame = page;
            currentPageTable[page].read = 1;
            currentPageTable[page].write = 1;

            rc = USLOSS_MmuSetPageTable(currentPageTable); assert(rc == USLOSS_MMU_OK);
        }
    } else {
        USLOSS_Console("ERROR: MMU Interrupt caused by invalid protections\n");
        USLOSS_Halt(1);
    }
}

USLOSS_PTE *
P3PageTableAllocateEmpty(int pages)
{
    if ((USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) != USLOSS_PSR_CURRENT_MODE) {
        USLOSS_Console("ERROR: Call to P3PageTableAllocateEmpty from user mode.\n");
        USLOSS_IllegalInstruction();
    }

    if (pages < 0) { return NULL; }

    USLOSS_PTE  *table = malloc(sizeof(USLOSS_PTE) * pages);
    if(table == NULL){ return NULL; }

    for(int i = 0; i < pages; i++){
        table[i].incore = 0;
    }
    return table;

}

