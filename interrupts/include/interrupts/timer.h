#ifndef TIMER_H
#define TIMER_H

#include <drivers/types.h>

void init_timer(u32 freq);
u32 get_tick(void);

#endif
