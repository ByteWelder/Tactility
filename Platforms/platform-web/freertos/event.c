// SPDX-License-Identifier: Apache-2.0
// See event.h for the contract and provenance.
#include "event.h"

#include <pthread.h>
#include <stdlib.h>

struct event
{
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    bool event_triggered;
};

struct event* event_create(void)
{
    struct event* ev = malloc(sizeof(struct event));

    if (ev != NULL)
    {
        ev->event_triggered = false;
        pthread_mutex_init(&ev->mutex, NULL);
        pthread_cond_init(&ev->cond, NULL);
    }

    return ev;
}

void event_delete(struct event* ev)
{
    pthread_mutex_destroy(&ev->mutex);
    pthread_cond_destroy(&ev->cond);
    free(ev);
}

bool event_wait(struct event* ev)
{
    pthread_mutex_lock(&ev->mutex);

    while (ev->event_triggered == false)
    {
        pthread_cond_wait(&ev->cond, &ev->mutex);
    }

    ev->event_triggered = false;
    pthread_mutex_unlock(&ev->mutex);
    return true;
}

void event_signal(struct event* ev)
{
    pthread_mutex_lock(&ev->mutex);
    ev->event_triggered = true;
    pthread_cond_signal(&ev->cond);
    pthread_mutex_unlock(&ev->mutex);
}
