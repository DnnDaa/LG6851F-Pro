
#include <assert.h>
#include <stdio.h>
static unsigned long calls,elapsed;
static unsigned long msecs_to_jiffies(unsigned int ms){return ((unsigned long)ms+9)/10;}
static long schedule_timeout_idle(long timeout){unsigned long slept=(timeout+1)/2;calls++;elapsed+=slept;return timeout-slept;}
static void wed_ser_idle_sleep(unsigned int msecs)
{
	unsigned long timeout = msecs_to_jiffies(msecs);

	while (timeout)
		timeout = schedule_timeout_idle(timeout);
}

int main(void){
 wed_ser_idle_sleep(15000);assert(elapsed==1500 && calls>1);
 calls=elapsed=0;wed_ser_idle_sleep(100);assert(elapsed==10);
 calls=elapsed=0;wed_ser_idle_sleep(0);assert(!calls && !elapsed);
 calls=elapsed=0;wed_ser_idle_sleep(1);assert(calls==1 && elapsed==1);
 puts("IDLE_SLEEP_DURATION_AND_EARLY_WAKE_PASS");return 0;
}
