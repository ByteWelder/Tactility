// SPDX-License-Identifier: Apache-2.0
#include <pthread/module.h>

#include <pthread.h>
#include <semaphore.h>

#if !defined(ESP_PLATFORM) && defined(__GLIBC_PREREQ)
#if __GLIBC_PREREQ(2, 30)
#define TT_PTHREAD_HAS_CLOCKWAIT 1
#else
#define TT_PTHREAD_HAS_CLOCKWAIT 0
#endif
#else
#define TT_PTHREAD_HAS_CLOCKWAIT 0
#endif

extern "C" {

static const ModuleSymbol SYMBOLS[] = {
    // pthread
    DEFINE_MODULE_SYMBOL(pthread_attr_init),
    DEFINE_MODULE_SYMBOL(pthread_attr_setstacksize),
    DEFINE_MODULE_SYMBOL(pthread_create),
    DEFINE_MODULE_SYMBOL(pthread_detach),
    DEFINE_MODULE_SYMBOL(pthread_exit),
    DEFINE_MODULE_SYMBOL(pthread_join),
    // pthread_cond
    DEFINE_MODULE_SYMBOL(pthread_cond_init),
    DEFINE_MODULE_SYMBOL(pthread_cond_broadcast),
#if TT_PTHREAD_HAS_CLOCKWAIT
    DEFINE_MODULE_SYMBOL(pthread_cond_clockwait),
#endif
    DEFINE_MODULE_SYMBOL(pthread_cond_destroy),
    DEFINE_MODULE_SYMBOL(pthread_cond_signal),
    DEFINE_MODULE_SYMBOL(pthread_cond_timedwait),
    // pthread_mutex
    DEFINE_MODULE_SYMBOL(pthread_mutex_destroy),
    DEFINE_MODULE_SYMBOL(pthread_mutex_init),
    DEFINE_MODULE_SYMBOL(pthread_mutex_lock),
    DEFINE_MODULE_SYMBOL(pthread_mutex_timedlock),
    DEFINE_MODULE_SYMBOL(pthread_mutex_trylock),
    DEFINE_MODULE_SYMBOL(pthread_mutex_unlock),
    // pthread_mutexattr
    DEFINE_MODULE_SYMBOL(pthread_mutexattr_destroy),
#ifndef ESP_PLATFORM
    DEFINE_MODULE_SYMBOL(pthread_mutexattr_getpshared),
    DEFINE_MODULE_SYMBOL(pthread_mutexattr_setpshared),
#endif
    DEFINE_MODULE_SYMBOL(pthread_mutexattr_gettype),
    DEFINE_MODULE_SYMBOL(pthread_mutexattr_init),
    DEFINE_MODULE_SYMBOL(pthread_mutexattr_settype),
    // sem
    DEFINE_MODULE_SYMBOL(sem_destroy),
    DEFINE_MODULE_SYMBOL(sem_getvalue),
    DEFINE_MODULE_SYMBOL(sem_init),
    DEFINE_MODULE_SYMBOL(sem_post),
    DEFINE_MODULE_SYMBOL(sem_timedwait),
    DEFINE_MODULE_SYMBOL(sem_trywait),
    DEFINE_MODULE_SYMBOL(sem_wait),
    // pthread_rwlock
#if TT_PTHREAD_HAS_CLOCKWAIT
    DEFINE_MODULE_SYMBOL(pthread_rwlock_clockrdlock),
    DEFINE_MODULE_SYMBOL(pthread_rwlock_clockwrlock),
#endif
#ifndef ESP_PLATFORM
    DEFINE_MODULE_SYMBOL(pthread_rwlock_timedrdlock),
    DEFINE_MODULE_SYMBOL(pthread_rwlock_timedwrlock),
    DEFINE_MODULE_SYMBOL(pthread_rwlockattr_destroy),
    DEFINE_MODULE_SYMBOL(pthread_rwlockattr_getpshared),
    DEFINE_MODULE_SYMBOL(pthread_rwlockattr_init),
    DEFINE_MODULE_SYMBOL(pthread_rwlockattr_setpshared),
#endif
    DEFINE_MODULE_SYMBOL(pthread_rwlock_destroy),
    DEFINE_MODULE_SYMBOL(pthread_rwlock_init),
    DEFINE_MODULE_SYMBOL(pthread_rwlock_rdlock),
    DEFINE_MODULE_SYMBOL(pthread_rwlock_tryrdlock),
    DEFINE_MODULE_SYMBOL(pthread_rwlock_trywrlock),
    DEFINE_MODULE_SYMBOL(pthread_rwlock_unlock),
    DEFINE_MODULE_SYMBOL(pthread_rwlock_wrlock),
    MODULE_SYMBOL_TERMINATOR,
};

Module pthread_module = {
    .name = "pthread",
    .start = nullptr,
    .stop = nullptr,
    .drivers = nullptr,
    .symbols = SYMBOLS,
    .internal = nullptr,
};

}
