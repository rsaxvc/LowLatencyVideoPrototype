#ifndef SIGNAL_CONTROL_H
#define SIGNAL_CONTROL_H

#include <signal.h>

#ifdef __cplusplus
extern "C"{
#endif

extern sigset_t ALL_SIGNALS;

#define DISABLE_SIGNALS() sigprocmask( SIG_BLOCK, &ALL_SIGNALS, NULL )
#define ENABLE_SIGNALS() sigprocmask( SIG_UNBLOCK, &ALL_SIGNALS, NULL )

#ifdef __cplusplus
}
#endif

#endif
