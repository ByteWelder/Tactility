// SPDX-License-Identifier: Apache-2.0
#include <pthread/module.h>

#include <pthread.h>
#include <semaphore.h>

extern "C" {

static const ModuleSymbol pthread_module_symbols[] = {
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
    DEFINE_MODULE_SYMBOL(pthread_cond_clockwait),
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
    DEFINE_MODULE_SYMBOL(pthread_mutexattr_getpshared),
    DEFINE_MODULE_SYMBOL(pthread_mutexattr_gettype),
    DEFINE_MODULE_SYMBOL(pthread_mutexattr_init),
    DEFINE_MODULE_SYMBOL(pthread_mutexattr_setpshared),
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
    DEFINE_MODULE_SYMBOL(pthread_rwlock_clockrdlock),
    DEFINE_MODULE_SYMBOL(pthread_rwlock_clockwrlock),
    DEFINE_MODULE_SYMBOL(pthread_rwlock_destroy),
    DEFINE_MODULE_SYMBOL(pthread_rwlock_init),
    DEFINE_MODULE_SYMBOL(pthread_rwlock_rdlock),
    DEFINE_MODULE_SYMBOL(pthread_rwlock_timedrdlock),
    DEFINE_MODULE_SYMBOL(pthread_rwlock_timedwrlock),
    DEFINE_MODULE_SYMBOL(pthread_rwlock_tryrdlock),
    DEFINE_MODULE_SYMBOL(pthread_rwlock_trywrlock),
    DEFINE_MODULE_SYMBOL(pthread_rwlock_unlock),
    DEFINE_MODULE_SYMBOL(pthread_rwlock_wrlock),
    DEFINE_MODULE_SYMBOL(pthread_rwlockattr_destroy),
    DEFINE_MODULE_SYMBOL(pthread_rwlockattr_getpshared),
    DEFINE_MODULE_SYMBOL(pthread_rwlockattr_init),
    DEFINE_MODULE_SYMBOL(pthread_rwlockattr_setpshared),
    MODULE_SYMBOL_TERMINATOR
};

Module pthread_module = {
    .name = "pthread",
    .symbols = pthread_module_symbols
};

}
