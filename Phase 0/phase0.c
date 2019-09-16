/*
 * Author: Kyle Snowden
 *
 * Practice switching contexts in USLOSS
 */

#include <stdio.h>
#include "usloss.h"

USLOSS_Context defaultContext;

USLOSS_Context contextHello;
char stackContextHello[USLOSS_MIN_STACK];

USLOSS_Context contextWorld;
char stackContextWorld[USLOSS_MIN_STACK];

void contextHelloFunction(void);
void contextWorldFunction(void);

void
startup(int argc, char **argv)
{
    USLOSS_ContextInit(&contextHello, stackContextHello, sizeof(stackContextHello), NULL, contextHelloFunction);
    USLOSS_ContextInit(&contextWorld, stackContextWorld, sizeof(stackContextWorld), NULL, contextWorldFunction);
    USLOSS_ContextSwitch(&defaultContext, &contextHello);
}

void
contextHelloFunction(void) {
    for (int iteration = 1; iteration <= 10; ++iteration) {
        USLOSS_Console("%d Hello ", iteration);
        USLOSS_ContextSwitch(&contextHello, &contextWorld);
    }
    USLOSS_Halt(0);
}

void
contextWorldFunction(void) {
    for (int iteration = 1; iteration <= 10; ++iteration) {
        USLOSS_Console("World");
        for (int i = 1; i <= iteration; ++i) { USLOSS_Console("!"); }
        USLOSS_Console("\n");
        USLOSS_ContextSwitch(&contextWorld, &contextHello);
    }
    USLOSS_Halt(0);
}

// Do not modify anything below this line.

void
finish(int argc, char **argv)
{
    USLOSS_Console("Goodbye.\n");
}

void
test_setup(int argc, char **argv)
{
    // Do nothing.
}

void
test_cleanup(int argc, char **argv)
{
    // Do nothing.
}

