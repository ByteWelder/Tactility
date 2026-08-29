#include <private/elf_symbol.h>
#include <cstddef>

#include <symbols/pthread.h>

#include <pthread.h>
#include <semaphore.h>

const esp_elfsym pthread_symbols[] = {
    // pthread
    ESP_ELFSYM_EXPORT(pthread_attr_init),
    ESP_ELFSYM_EXPORT(pthread_attr_setstacksize),
    ESP_ELFSYM_EXPORT(pthread_create),
    ESP_ELFSYM_EXPORT(pthread_detach),
    ESP_ELFSYM_EXPORT(pthread_exit),
    ESP_ELFSYM_EXPORT(pthread_join),
    // pthread_cond
    ESP_ELFSYM_EXPORT(pthread_cond_init),
    ESP_ELFSYM_EXPORT(pthread_cond_broadcast),
    ESP_ELFSYM_EXPORT(pthread_cond_clockwait),
    ESP_ELFSYM_EXPORT(pthread_cond_destroy),
    ESP_ELFSYM_EXPORT(pthread_cond_signal),
    ESP_ELFSYM_EXPORT(pthread_cond_timedwait),
    // pthread_mutex
    ESP_ELFSYM_EXPORT(pthread_mutex_destroy),
    ESP_ELFSYM_EXPORT(pthread_mutex_init),
    ESP_ELFSYM_EXPORT(pthread_mutex_lock),
    ESP_ELFSYM_EXPORT(pthread_mutex_timedlock),
    ESP_ELFSYM_EXPORT(pthread_mutex_trylock),
    ESP_ELFSYM_EXPORT(pthread_mutex_unlock),
    // pthread_mutexattr
    ESP_ELFSYM_EXPORT(pthread_mutexattr_destroy),
    ESP_ELFSYM_EXPORT(pthread_mutexattr_getpshared),
    ESP_ELFSYM_EXPORT(pthread_mutexattr_gettype),
    ESP_ELFSYM_EXPORT(pthread_mutexattr_init),
    ESP_ELFSYM_EXPORT(pthread_mutexattr_setpshared),
    ESP_ELFSYM_EXPORT(pthread_mutexattr_settype),
    // sem
    ESP_ELFSYM_EXPORT(sem_destroy),
    ESP_ELFSYM_EXPORT(sem_getvalue),
    ESP_ELFSYM_EXPORT(sem_init),
    ESP_ELFSYM_EXPORT(sem_post),
    ESP_ELFSYM_EXPORT(sem_timedwait),
    ESP_ELFSYM_EXPORT(sem_trywait),
    ESP_ELFSYM_EXPORT(sem_wait),
    // pthread_rwlock
    ESP_ELFSYM_EXPORT(pthread_rwlock_clockrdlock),
    ESP_ELFSYM_EXPORT(pthread_rwlock_clockwrlock),
    ESP_ELFSYM_EXPORT(pthread_rwlock_destroy),
    ESP_ELFSYM_EXPORT(pthread_rwlock_init),
    ESP_ELFSYM_EXPORT(pthread_rwlock_rdlock),
    ESP_ELFSYM_EXPORT(pthread_rwlock_timedrdlock),
    ESP_ELFSYM_EXPORT(pthread_rwlock_timedwrlock),
    ESP_ELFSYM_EXPORT(pthread_rwlock_tryrdlock),
    ESP_ELFSYM_EXPORT(pthread_rwlock_trywrlock),
    ESP_ELFSYM_EXPORT(pthread_rwlock_unlock),
    ESP_ELFSYM_EXPORT(pthread_rwlock_wrlock),
    ESP_ELFSYM_EXPORT(pthread_rwlockattr_destroy),
    ESP_ELFSYM_EXPORT(pthread_rwlockattr_getpshared),
    ESP_ELFSYM_EXPORT(pthread_rwlockattr_init),
    ESP_ELFSYM_EXPORT(pthread_rwlockattr_setpshared),
    // delimiter
    ESP_ELFSYM_END
};
