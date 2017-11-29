#include "common.h"
#include "syslib.h"
#include "util.h"
#include "printf.h"

void SunQuan(void)
{
    pid_t pid = getpid();
    init_a_lock(1);
    printf(1, 1, "(%d)I am SunQuan, the lock seeker", pid);
    printf(2, 1, "(%d) Lock1 should be mine!!!!!!!!", pid);
    acquire_a_lock(1);
    printf(3, 1, "(%d) Lock1 is mine, hahahaha!!!!!", pid);
    sleep(3000);
    printf(4, 1, "(%d) Lock2 should be mine!!!!!!!!", pid);
    acquire_a_lock(2);
    printf(5, 1, "(%d) Lock2 is mine, hahahaha!!!!!", pid);
    release_a_lock(2);
    release_a_lock(1);
    exit();
}
