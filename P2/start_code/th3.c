#include "common.h"
#include "scheduler.h"
#include "util.h"

int time = 0;


void thread4(void)
{
    time = get_timer();
    do_yield();
    printstr("t4Exit");
    do_exit();
}

void thread5(void)
{
    time = get_timer() - time;
    printstr("tTimecst: ");
    printnum2(time);
    printstr("t5Exit");
    do_exit();
}
