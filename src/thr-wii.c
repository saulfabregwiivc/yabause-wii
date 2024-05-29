/*  Copyright 2010 Lawrence Sebald

    This file is part of Yabause.

    Yabause is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Yabause is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Yabause; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

#include "core.h"
#include "threads.h"

//#include <pthread.h>
#include <sched.h>
#include <ogcsys.h>
#include <gccore.h>
#include <ogc/cond.h>
#define STACKSIZE 8*1024

//for pthread wrapper to LWP
typedef lwp_t pthread_t;
typedef mutex_t pthread_mutex_t;
typedef cond_t pthread_cond_t;
typedef void* pthread_mutexattr_t;
typedef int pthread_attr_t;

/* Thread handle structure. */
struct thd_s {
    int running;
    pthread_t thd;
    void (*func)(void);
    pthread_mutex_t mutex;
    pthread_cond_t cond;
};

static struct thd_s thread_handle[YAB_NUM_THREADS];
//static pthread_key_t hnd_key;
//static pthread_once_t hnd_key_once;

//static void make_key() {
//    pthread_key_create(&hnd_key, NULL);
//}

static void *wrapper(void *hnd) {
    struct thd_s *hnds = (struct thd_s *)hnd;

    //pthread_mutex_lock(&hnds->mutex);
    LWP_MutexLock(hnds->mutex);

    /* Set the handle for the thread, and call the actual thread function. */
    //pthread_setspecific(hnd_key, hnd);
    hnds->func();

    //pthread_mutex_unlock(&hnds->mutex);
    LWP_MutexUnlock(hnds->mutex);

    return NULL;
}

int YabThreadStart(unsigned int id, void (*func)(void)) {
    /* Create the key to access the thread handle if we haven't made it yet. */
    //pthread_once(&hnd_key_once, make_key);

    /* Make sure we aren't trying to start a thread twice. */
    if(thread_handle[id].running) {
        fprintf(stderr, "YabThreadStart: Thread %u is already started!\n", id);
        return -1;
    }

    /* Create the mutex and condvar for the thread. */
    //if(pthread_mutex_init(&thread_handle[id].mutex, NULL)) {
    if(!LWP_MutexInit(&thread_handle[id].mutex, false)) {
        fprintf(stderr, "YabThreadStart: Error creating mutex\n");
        return -1;
    }

    //if(pthread_cond_init(&thread_handle[id].cond, NULL)) {
    if(!LWP_CondInit(&thread_handle[id].cond)) {
        fprintf(stderr, "YabThreadStart: Error creating condvar\n");
        //pthread_mutex_destroy(&thread_handle[id].mutex);
        LWP_MutexDestroy(thread_handle[id].mutex);
        return -1;
    }

    thread_handle[id].func = func;

    /* Create the thread. */
    //if(pthread_create(&thread_handle[id].thd, NULL, wrapper,
    //                  &thread_handle[id])) {
    if(LWP_CreateThread(&thread_handle[id].thd, wrapper, &thread_handle[id],
			0, STACKSIZE, 64)) {
        fprintf(stderr, "YabThreadStart: Couldn't start thread\n");
        //pthread_cond_destroy(&thread_handle[id].cond);
        LWP_CondDestroy(thread_handle[id].cond);
        //pthread_mutex_destroy(&thread_handle[id].mutex);
        LWP_MutexDestroy(thread_handle[id].mutex);
        return -1;
    }

    thread_handle[id].running = 1;

    return 0;
}

void YabThreadWait(unsigned int id) {
    /* Make sure the thread is running. */
    if(!thread_handle[id].running)
        return; 

    /* Join the thread to wait for it to finish. */
    //pthread_join(thread_handle[id].thd, NULL);
    LWP_JoinThread(thread_handle[id].thd, NULL);

    /* Cleanup... */
    //pthread_cond_destroy(&thread_handle[id].cond);
    LWP_CondDestroy(thread_handle[id].cond);
    //pthread_mutex_destroy(&thread_handle[id].mutex);
    LWP_MutexDestroy(thread_handle[id].mutex);
    thread_handle[id].thd = LWP_THREAD_NULL;
    thread_handle[id].func = NULL;

    thread_handle[id].running = 0;
}

void YabThreadYield(void) {
    //sched_yield();
    LWP_YieldThread(); 
}

//void YabThreadSleep(void) {
void YabThreadSleep(unsigned int id) {
    //struct thd_s *thd = (struct thd_s *)pthread_getspecific(hnd_key);

    /* Wait on the condvar... */
    //pthread_cond_wait(&thd->cond, &thd->mutex);
    LWP_CondWait(thread_handle[id].cond, thread_handle[id].mutex);
}

void YabThreadWake(unsigned int id) {
    if(!thread_handle[id].running)
        return;

    //pthread_cond_signal(&thread_handle[id].cond);
    LWP_CondSignal(thread_handle[id].cond);
}
