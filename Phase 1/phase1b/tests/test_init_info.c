#include <phase1.h>
#include <phase1Int.h>
#include <assert.h>

static int
Output(void *arg)
{
    char *msg = (char *) arg;

    USLOSS_Console("%s", msg);
    P1_Quit(11);
    // should not return
    return 0;
}

void startup(int argc, char **argv)
{
    int pid = 0;
    int rc = 0;
    P1ProcInit();
    USLOSS_Console("startup\n");
    P1_ProcInfo test;
    rc = P1_GetProcInfo(pid, &test);
    assert(rc == P1_SUCCESS);
    printf("TEST PASSED\n");
}

void test_setup(int argc, char **argv) {}

void test_cleanup(int argc, char **argv) {}

void finish(int argc, char **argv) {}