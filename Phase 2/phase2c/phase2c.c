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

#define     ABORT       -1
#define     NUM_SECTORS 16
#define     NUM_BYTES   512

struct DiskInfo {
    int disk;
    int mutex_SemID;                // initialized to 1
    int waitingRequest_SemID;       // initialized to 0
    int requestCompleted_SemID;     // initialized to 0

    // used to keep track of disk's size
    int numTracks;

    // used to pass values between helper and disk driver
    int operation;
    int startingTrack;
    int startingSector;
    int sectors;
    void* buffer;
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
    int status;

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
        char processesName[32];
        snprintf(processesName, 32, "Disk Driver %d", disk);
        rc = P1_Fork(processesName, DiskDriver, (void *)(uintptr_t) disk, 4*USLOSS_MIN_STACK, 2, 0, &pid);
        assert(rc == P1_SUCCESS);

        char mutexName[32];
        snprintf(mutexName, 32, "Mutex Semaphore %d", disk);
        rc = P1_SemCreate(mutexName, 1, &(DiskInfoArray[disk].mutex_SemID));
        assert(rc == P1_SUCCESS);

        char requestSemName[32];
        snprintf(requestSemName, 32, "Request Semaphore %d", disk);
        rc = P1_SemCreate(requestSemName, 0, &(DiskInfoArray[disk].waitingRequest_SemID));
        assert(rc == P1_SUCCESS);

        char completedSemName[32];
        snprintf(completedSemName, 32, "Completed Semaphore %d", disk);
        rc = P1_SemCreate(completedSemName, 0, &(DiskInfoArray[disk].requestCompleted_SemID));
        assert(rc == P1_SUCCESS);

        USLOSS_DeviceRequest request;
        request.opr = USLOSS_DISK_TRACKS;
        request.reg1 = &(DiskInfoArray[disk].numTracks);
        request.reg2 = NULL;
        rc = USLOSS_DeviceOutput(USLOSS_DISK_DEV, disk, &request); assert(rc == USLOSS_DEV_OK);
        rc = P1_WaitDevice(USLOSS_DISK_DEV, disk, &status); assert(rc == P1_SUCCESS);
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
    int rc;
    for (int disk = 0; disk < USLOSS_DISK_UNITS; disk++) {
        rc = P1_P(DiskInfoArray[disk].mutex_SemID); assert(rc == P1_SUCCESS);

        DiskInfoArray[disk].operation = ABORT;

        rc = P1_V(DiskInfoArray[disk].waitingRequest_SemID); assert(rc == P1_SUCCESS);

        rc = P1_SemFree(DiskInfoArray[disk].waitingRequest_SemID); assert(rc == P1_SUCCESS);
        rc = P1_SemFree(DiskInfoArray[disk].requestCompleted_SemID); assert(rc == P1_SUCCESS);
        rc = P1_SemFree(DiskInfoArray[disk].mutex_SemID); assert(rc == P1_SUCCESS);
    }
}

/*
 * DiskDriver
 *
 * Kernel process that manages a disk device and services disk I/O requests from other processes.
 * Note that it may require several disk operations to service a single I/O request.
 */
static int 
DiskDriver(void *arg) {
    // repeat
    //   wait for next request
    //   while request isn't complete
    //          send appropriate operation to disk (USLOSS_DeviceOutput)
    //          wait for operation to finish (P1_WaitDevice)
    //          handle errors
    //   update the request status and wake the waiting process
    // until P2DiskShutdown has been called

    int disk = (int) arg;
    int rc;
    int status;
    int currentTrack;
    int currentSector;
    void *buffer;
    USLOSS_DeviceRequest request;

    //p semaphore that is being read / written to
    //v it when it is done
    while (TRUE) {
        rc = P1_P(DiskInfoArray[disk].waitingRequest_SemID);
        assert(rc == P1_SUCCESS);

        if (DiskInfoArray[disk].operation == ABORT) {
            break;
        } else {
//            DiskInfoArray[disk];
            currentTrack = DiskInfoArray[disk].startingTrack;
            currentSector = DiskInfoArray[disk].startingSector;
            buffer = DiskInfoArray[disk].buffer;

            // Move head to correct track;
            request.opr = USLOSS_DISK_SEEK;
            request.reg1 = (void *)(uintptr_t) DiskInfoArray[disk].startingTrack;
            rc = USLOSS_DeviceOutput(USLOSS_DISK_DEV, disk, &request); assert(rc == P1_SUCCESS);
            rc = P1_WaitDevice(USLOSS_DISK_DEV, disk, &status); assert(rc == P1_SUCCESS);
            assert(rc == USLOSS_DEV_OK);

            for (int i = 0; i < DiskInfoArray[disk].sectors; i++) {
                if (currentSector % NUM_SECTORS == 0 && i != 0) { //if the current_sector is a multiple of 16 - move to the next track
                    currentTrack++;
//                    if(currentTrack > DiskInfoArray[disk].numTracks){
//                        DiskInfoArray[disk].status = P2_INVALID_TRACK;
//                        goto end_of_operation;
//                    } I think I successfully moved this error check to the helper function

                    request.opr = USLOSS_DISK_SEEK;
                    request.reg1 = (void *)(uintptr_t) currentTrack;
                    rc = USLOSS_DeviceOutput(USLOSS_DISK_DEV, disk, &request);
                    assert(rc == USLOSS_DEV_OK);
                }

                // Fill out request struct for read or write
                request.opr = DiskInfoArray[disk].operation;
                request.reg1 = (void *)(uintptr_t) currentSector;
                request.reg2 = buffer + (NUM_BYTES * i);

                rc = USLOSS_DeviceOutput(USLOSS_DISK_DEV, disk, &request); assert(rc == USLOSS_DEV_OK);
                rc = P1_WaitDevice(USLOSS_DISK_DEV, disk, &status); assert(rc == P1_SUCCESS);

//                if(rc != USLOSS_DEV_OK){
//                    free(request);
//                    assert(rc == P1_SUCCESS);
//                    return P1_INVALID_UNIT;
//                } I think I successfully moved this error check to the helper function

                currentSector++;
//                request.reg1 = (void *) currentSector;
//                request.reg2 = &(buffer) + 512;
            }
        }
        rc = P1_V(DiskInfoArray[disk].requestCompleted_SemID);
        assert(rc == P1_SUCCESS);
    }
    return P1_SUCCESS;
}

int
DiskReadWriteHelper(int unit, int track, int first, int sectors, void *buffer, int operation)
{
    int rc;

    if (unit < 0 || unit >= USLOSS_DISK_UNITS) {
        return P1_INVALID_UNIT;
    } else if (sectors <= 0 || ((track * NUM_SECTORS) + first + sectors) > (NUM_SECTORS * DiskInfoArray[unit].numTracks)) {
        return P2_INVALID_SECTORS;
    } else if(!buffer){
        return P2_NULL_ADDRESS;
//    } else if(first < 1 || first > 16){
    } else if(first < 0 || first >= NUM_SECTORS){
        return P2_INVALID_FIRST;
    } else if(track < 0 || track >= DiskInfoArray[unit].numTracks){
        return P2_INVALID_TRACK;
    } else {
        rc = P1_P(DiskInfoArray[unit].mutex_SemID); assert(rc == P1_SUCCESS);

        DiskInfoArray[unit].operation = operation;
        DiskInfoArray[unit].startingSector = first;
        DiskInfoArray[unit].startingTrack = track;
        DiskInfoArray[unit].sectors = sectors;
        DiskInfoArray[unit].buffer = buffer;
        DiskInfoArray[unit].operation = operation;

        rc = P1_V(DiskInfoArray[unit].waitingRequest_SemID); assert(rc == P1_SUCCESS);
        rc = P1_P(DiskInfoArray[unit].requestCompleted_SemID); assert(rc == P1_SUCCESS);

        rc = P1_V(DiskInfoArray[unit].mutex_SemID); assert(rc == P1_SUCCESS);

        return P1_SUCCESS;
    }
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
    sysargs->arg4 = (void *)(uintptr_t) rc;
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

    sysargs->arg4 = (void *)(uintptr_t) rc;
}


int
P2_DiskSize(int unit, int *sector, int *track, int *disk)
{
    if (unit < 0 || unit >= USLOSS_DISK_UNITS) {
        return P1_INVALID_UNIT;
    } else if (sector == NULL || track == NULL || disk == NULL) {
        return P2_NULL_ADDRESS;
    } else {
        *sector = NUM_BYTES;
        *track = NUM_SECTORS;
        *disk = DiskInfoArray[unit].numTracks;
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

    sysargs->arg1 = (void *)(uintptr_t) bytesInSector;
    sysargs->arg2 = (void *)(uintptr_t) sectorsInTrack;
    sysargs->arg3 = (void *)(uintptr_t) tracksInDisk;
    sysargs->arg4 = (void *)(uintptr_t) rc;
}
