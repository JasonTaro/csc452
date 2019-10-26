#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include <usloss.h>
#include <phase1.h>

#include "phase2Int.h"


static int      DiskDriver(void *);
static void     DiskReadStub(USLOSS_Sysargs *sysargs);
static void     DiskWriteStub(USLOSS_Sysargs *sysargs);
static void     DiskSizeStub(USLOSS_Sysargs *sysargs);

#define     ABORT   -1

struct DiskInfo {
    int disk;
    int mutex_SemID; // initialized to 1
    int waitingRequest_SemID; // initialized to 0
    int requestCompleted_SemID; // initialized to 0;
//    int tracks;

    int operation;
    int startingTrack;
    int startingSector;
    int sectors;
    void** buffer;
    int* tracks;
    int status;
} typedef DiskInfo;

static DiskInfo DiskInfoArray[USLOSS_DISK_UNITS];

/*
 * P2DiskInit
 *
 * Initialize the disk data structures and fork the disk drivers.
 */
void 
P2DiskInit(void) 
{
    int rc;

    // initialize data structures here

    // install system call stubs here
    rc = P2_SetSyscallHandler(SYS_DISKREAD, DiskReadStub);
    assert(rc == P1_SUCCESS);

    rc = P2_SetSyscallHandler(SYS_DISKWRITE, DiskWriteStub);
    assert(rc == P1_SUCCESS);

    rc = P2_SetSyscallHandler(SYS_DISKSIZE, DiskSizeStub);
    assert(rc == P1_SUCCESS);


    // fork the disk drivers here
    for (int disk = 0; disk < USLOSS_DISK_UNITS; disk++) {
        int pid;
        char processesName[20];
        snprintf(processesName, 16, "Disk Driver %d", disk);
        rc = P1_Fork(processesName, DiskDriver, (void*) disk, 4*USLOSS_MIN_STACK, 2, 0, &pid);
        assert(rc == P1_SUCCESS);

        char mutexName[20];
        snprintf(mutexName, 16, "Mutex Semaphore %d", disk);
        rc = P1_SemCreate(mutexName, 1, &DiskInfoArray[disk].mutex_SemID);
        assert(rc == P1_SUCCESS);

        char requestSemName[24];
        snprintf(requestSemName, 16, "Request Semaphore %d", disk);
        rc = P1_SemCreate(requestSemName, 0, &DiskInfoArray[disk].waitingRequest_SemID);
        assert(rc == P1_SUCCESS);

        char completedSemName[24];
        snprintf(completedSemName, 16, "Completed Semaphore %d", disk);
        rc = P1_SemCreate(completedSemName, 0, &DiskInfoArray[disk].requestCompleted_SemID);
        assert(rc == P1_SUCCESS);
    }
}

/*
 * P2DiskShutdown
 *
 * Clean up the disk data structures and stop the disk drivers.
 */

void 
P2DiskShutdown(void) 
{

}

/*
 * DiskDriver
 *
 * Kernel process that manages a disk device and services disk I/O requests from other processes.
 * Note that it may require several disk operations to service a single I/O request.
 */
static int 
DiskDriver(void *arg) 
{
    int disk = (int) arg;
    int rc;
    int status;
    USLOSS_DeviceRequest request;
    // Jack! I made our data structures an array so that the parameter is the disk number

    while (TRUE) {
        P1_P(DiskInfoArray[disk].waitingRequest_SemID);
        if (DiskInfoArray[disk].operation == ABORT) { break; }

        if (DiskInfoArray[disk].operation == USLOSS_DISK_TRACKS) {
            request.opr = USLOSS_DISK_TRACKS;
            request.reg1 = DiskInfoArray[disk].tracks;
            DiskInfoArray[disk].status = USLOSS_DeviceOutput(USLOSS_DISK_DEV, disk, &request);
            rc = P1_WaitDevice(USLOSS_DISK_DEV, disk, &status); assert(rc == P1_SUCCESS);
        }

        if


        switch (DiskInfoArray[disk].operation) {

            case USLOSS_DISK_TRACKS:


            default:
                continue;

        }

        P1_P(DiskInfoArray[disk].waitingRequest_SemID);



//    typedef struct USLOSS_DeviceRequest
//    {
//        int opr; USLOSS_DISK_READ, USLOSS_DISK_WRITE, USLOSS_DISK_SEEK, USLOSS_DISK_TRACKS
//        void *reg1;
//        void *reg2;
//    } USLOSS_DeviceRequest;
    }




    // repeat
    //   wait for next request
    //   while request isn't complete
    //          send appropriate operation to disk (USLOSS_DeviceOutput)
    //          wait for operation to finish (P1_WaitDevice)
    //          handle errors
    //   update the request status and wake the waiting process
    // until P2DiskShutdown has been called
    return P1_SUCCESS;
}

int DiskReadWriteHelper(int unit, int track, int first, int sectors, void *buffer, int operation)  {

    if (unit < 0 || unit >= USLOSS_DISK_UNITS) {
        return P1_INVALID_UNIT;
    } else if () {

    }




//    } else if (sector == NULL || track == NULL || disk == NULL) {
//        return P2_NULL_ADDRESS;
//    } else {
//        *sector = 512;
//        *track = 16;
//
//        int rc;
//        rc = P1_P(DiskInfoArray[unit].mutex_SemID); assert(rc == P1_SUCCESS);
//
//        DiskInfoArray[unit].operation = USLOSS_DISK_TRACKS;
//        DiskInfoArray[unit].tracks = disk;
//
//        rc = P1_V(DiskInfoArray[unit].waitingRequest_SemID); assert(rc == P1_SUCCESS);
//        rc = P1_P(DiskInfoArray[unit].requestCompleted_SemID); assert(rc == P1_SUCCESS);
//
//
//        rc = P1_V(DiskInfoArray[unit].mutex_SemID); assert(rc == P1_SUCCESS);
//
//        return P1_SUCCESS;
//    }
}

/*
 * P2_DiskRead
 *
 * Reads the specified number of sectors from the disk starting at the specified track and sector.
 */
int 
P2_DiskRead(int unit, int track, int first, int sectors, void *buffer) 
{
    // give request to the proper device driver
    // wait until device driver completes the request
    return DiskReadWriteHelper(unit, track, first, sectors, buffer, USLOSS_DISK_READ);
}

/*
 * DiskReadStub
 *
 * Stub for the Sys_DiskRead system call.
 */
static void 
DiskReadStub(USLOSS_Sysargs *sysargs) 
{
    // unpack sysargs
    void* buffer = sysargs->arg1;
    int sectors = (int) sysargs->arg2;
    int firstTrack = (int) sysargs->arg3;
    int firstSector = (int) sysargs->arg4;
    int unit = (int) sysargs->arg5;

    // call P2_DiskRead
    int rc;
    rc = P2_DiskRead(unit, firstTrack, firstSector, sectors, buffer);

    // put result in sysargs
    sysargs->arg4 = (void *) rc;
}


int
P2_DiskWrite(int unit, int track, int first, int sectors, void *buffer)
{
    return DiskReadWriteHelper(unit, track, first, sectors, buffer, USLOSS_DISK_WRITE);
}

static void
DiskWriteStub(USLOSS_Sysargs *sysargs)
{
    void* buffer = sysargs->arg1;
    int sectors = (int) sysargs->arg2;
    int firstTrack = (int) sysargs->arg3;
    int firstSector = (int) sysargs->arg4;
    int unit = (int) sysargs->arg5;

    int rc;
    rc = P2_DiskWrite(unit, firstTrack, firstSector, sectors, buffer);

    sysargs->arg4 = (void *) rc;
}


int
P2_DiskSize(int unit, int *sector, int *track, int *disk)
{
    if (unit < 0 || unit >= USLOSS_DISK_UNITS) {
        return P1_INVALID_UNIT;
    } else if (sector == NULL || track == NULL || disk == NULL) {
        return P2_NULL_ADDRESS;
    } else {
        *sector = 512;
        *track = 16;

        int rc;
        rc = P1_P(DiskInfoArray[unit].mutex_SemID); assert(rc == P1_SUCCESS);

        DiskInfoArray[unit].operation = USLOSS_DISK_TRACKS;
        DiskInfoArray[unit].tracks = disk;

        rc = P1_V(DiskInfoArray[unit].waitingRequest_SemID); assert(rc == P1_SUCCESS);
        rc = P1_P(DiskInfoArray[unit].requestCompleted_SemID); assert(rc == P1_SUCCESS);


        rc = P1_V(DiskInfoArray[unit].mutex_SemID); assert(rc == P1_SUCCESS);

        return P1_SUCCESS;
    }
}

static void
DiskSizeStub(USLOSS_Sysargs *sysargs)
{
    int unit = (int) sysargs->arg1;
    int bytesInSector, sectorsInTrack, tracksInDisk;

    int rc;
    rc = P2_DiskSize(unit, &bytesInSector, &sectorsInTrack, &tracksInDisk);

    sysargs->arg1 = (void *) bytesInSector;
    sysargs->arg2 = (void *) sectorsInTrack;
    sysargs->arg3 = (void *) tracksInDisk;
    sysargs->arg4 = (void *) rc;
}
