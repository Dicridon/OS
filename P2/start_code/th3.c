#include "common.h"
#include "scheduler.h"
#include "util.h"

int TTtime = 0;
int TPtime = 0;

void thread4(void)
{
    TTtime = get_timer();
    do_yield();
    TPtime = (get_timer()-TPtime)/2;
    printstr("tPTtimecst: ");
    printnum2(TPtime);
    printstr("t4Exit");
    do_exit();
}

void thread5(void)
{
    TTtime = get_timer() - TTtime;
    printstr("tTTtimecst: ");
    printnum2(TTtime);
    printstr("t5Exit");
    TPtime = get_timer();
    do_yield();
    do_exit();
}
