#include <usloss.h>
#include <stdio.h>
#include <stdlib.h>


void
startup(int argc, char **argv)
{
    USLOSS_Halt(0);
}

void
finish(int argc, char **argv) {}
void test_setup(int argc, char **argv) {}
void test_cleanup(int argc, char **argv) {}