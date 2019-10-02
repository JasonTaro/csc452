#include <phase1.h>
#include <phase1Int.h>
#include <assert.h>
#include <stdio.h>

int current_car = 0;
static int
Passenger_1(void *arg)
{
    int rc;
    while(current_car <= 0){
        P1Dispatch(0);
    } //do nothing
    rc = P1_V(1);
    assert(rc == P1_SUCCESS);
    P1_Quit(current_car);
    // will never get here as Blocks will run and call USLOSS_Halt.
    return 12;
}

static int
Passenger_2(void *arg)
{
    int rc;
    while(current_car <= 0){
        P1Dispatch(0);
    } //do nothing
    rc = P1_V(1);
    assert(rc == P1_SUCCESS);
    P1_Quit(current_car);
    // will never get here as Blocks will run and call USLOSS_Halt.
    return 12;
}

static int
Passenger_3(void *arg)
{
    int rc;
    while(current_car <= 0){
        P1Dispatch(0);
    } //do nothing
    rc = P1_V(1);
    assert(rc == P1_SUCCESS);
    P1_Quit(current_car);
    // will never get here as Blocks will run and call USLOSS_Halt.
    return 12;
}

static int
Car(void *arg)
{

    int rc;
    int pid;
    int pid1;
    int pid2;
    rc = P1_Fork("p1", Passenger_1, (void *) 0, USLOSS_MIN_STACK, 2, 0, &pid);
    rc = P1_Fork("p2", Passenger_2, (void *) 0, USLOSS_MIN_STACK, 2, 0, &pid1);
    rc = P1_Fork("p3", Passenger_3, (void *) 0, USLOSS_MIN_STACK, 2, 0, &pid2);
    assert(rc == P1_SUCCESS);

    rc = P1_P(0);
    assert(rc == P1_SUCCESS);

    current_car = (int) arg;

    rc = P1_P(1);
    assert(rc == P1_SUCCESS);

    rc = P1_P(1);
    assert(rc == P1_SUCCESS);

    rc = P1_P(1);
    assert(rc == P1_SUCCESS);


    current_car = 0;
    rc = P1_V(0);
    assert(rc == P1_SUCCESS);

    USLOSS_Console("Test passed.\n");
    USLOSS_Halt(0);
    // should not return
    assert(0);
    return 0;
}

void
startup(int argc, char **argv)
{
    int pid;
    int rc;
    int sem;
    int sem1;
    P1SemInit();
    rc = P1_SemCreate("isOkToLoad", 1, &sem);
    assert(rc == P1_SUCCESS);

    rc = P1_SemCreate("passCount", 3, &sem1);
    assert(rc == P1_SUCCESS);

    printf("1");
    // Blocks blocks then Unblocks unblocks it
    int i = 0;
    while(i < 9){
        rc = P1_Fork("Car", Car, (void *) i, USLOSS_MIN_STACK, 1, 0, &pid);
    }
    assert(rc == P1_SUCCESS);
    assert(0);
}

void dummy(int type, void *arg) {};

void test_setup(int argc, char **argv) {}

void test_cleanup(int argc, char **argv) {}

void finish(int argc, char **argv) {}