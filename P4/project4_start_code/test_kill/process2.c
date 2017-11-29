#include "common.h"
#include "syslib.h"
#include "util.h"
#include "printf.h"

void LiuBei(void)
{
    pid_t pid = getpid();
    init_a_lock(2);
    printf(7, 1, "(%d) I am LuiBei, deadlock seeker", pid);
    printf(8, 1, "(%d) Lock2 should be mine!!!!!!!!", pid);
    acquire_a_lock(2);
    printf(9, 1, "(%d) Lock2 is mine, hahahaha!!!!!", pid);
    sleep(3000);
    printf(10, 1, "(%d) Lock1 should be mine!!!!!!!!", pid);
    acquire_a_lock(1);
    printf(11, 1, "(%d) Lock1 is mine, hahahaha!!!!!", pid);
    release_a_lock(2);
    release_a_lock(1);
    exit();
}

