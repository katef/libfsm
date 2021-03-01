#ifndef LX_H
#define LX_H

#define USE_LOG 0

#if USE_LOG
#include <unistd.h>
#define LOG() getenv("LOG")
#else
#define LOG() (0)
#endif

void
lx_mutex_lock(void);

void
lx_mutex_unlock(void);

#endif
