#include "common.h"
#include "syslib.h"
#include "util.h"


bool_t YIELD = TRUE;
bool_t THREAD_FIRST = TRUE;

int time = 0;


void _start(void)
{
    /* need student add */
    if(THREAD_FIRST){
	THREAD_FIRST = FALSE;
	yield();
    }
    
    time = get_timer() - time;
    
    if(YIELD){
	YIELD = FALSE;
	yield();
    }
    else{
	time = get_timer() - time;
	printstr("pTimecst");
	printnum2(time);
	exit();
    }
}
