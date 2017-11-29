#include "common.h"
#include "syslib.h"
#include "util.h"
#include "printf.h"

void CaoCao(void)
{
    printf(15, 1, "I am CaoCao, the deadlock killer");
    printf(16, 1, "deadlock can be solved by killing");
    sleep(6000);
    kill(4);
    sleep(3000);
    printf(17, 1, "SunQuan acquired the lock");
}




