/*********************************************************************
 *        _       _         _
 *  _ __ | |_  _ | |  __ _ | |__   ___
 * | '__|| __|(_)| | / _` || '_ \ / __|
 * | |   | |_  _ | || (_| || |_) |\__ \
 * |_|    \__|(_)|_| \__,_||_.__/ |___/
 *
 * www.rt-labs.com
 * Copyright 2018 rt-labs AB, Sweden.
 *
 * This software is dual-licensed under GPLv3 and a commercial
 * license. See the file LICENSE.md distributed with this software for
 * full license information.
 ********************************************************************/

#include "osal.h"
#include "options.h"
#include "log.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

int c99_vsnprintf(char *outBuf, size_t size, const char *format, va_list ap)
{
    int count = -1;

    if (size != 0)
        count = _vsnprintf_s(outBuf, size, _TRUNCATE, format, ap);
    if (count == -1)
        count = _vscprintf(format, ap);

    return count;
}

int os_snprintf(char * str, size_t size, const char * fmt, ...)
{
   int ret;
   va_list list;

   va_start (list, fmt);
   ret = c99_vsnprintf (str, size, fmt, list);
   va_end (list);
   return ret;
}

void os_log (int type, const char * fmt, ...)
{
   va_list list;

   switch(LOG_LEVEL_GET (type))
   {
   case LOG_LEVEL_DEBUG:   printf ("[DEBUG] "); break;
   case LOG_LEVEL_INFO:    printf ("[INFO ] "); break;
   case LOG_LEVEL_WARNING: printf ("[WARN ] "); break;
   case LOG_LEVEL_ERROR:   printf ("[ERROR] "); break;
   default: break;
   }

   va_start (list, fmt);
   vprintf (fmt, list);
   va_end (list);
   fflush (stdout);
}

void * os_malloc (size_t size)
{
   return malloc (size);
}

os_mutex_t * os_mutex_create (void)
{
   return CreateMutex (NULL, FALSE, NULL);
}

void os_mutex_lock (os_mutex_t * mutex)
{
   WaitForSingleObject (mutex, INFINITE);
}

void os_mutex_unlock (os_mutex_t * mutex)
{
   ReleaseMutex (mutex);
}

void os_mutex_destroy (os_mutex_t * mutex)
{
   CloseHandle (mutex);
}

void os_usleep (uint32_t usec)
{
   Sleep (usec / 1000);
}

os_thread_t * os_thread_create (const char * name, int priority,
        int stacksize, void (*entry) (void * arg), void * arg)
{
   HANDLE handle;
   handle = CreateThread(NULL,
                         0,
                         (LPTHREAD_START_ROUTINE)entry,
                         (LPVOID)arg,
                         0,
                         NULL);

   SetThreadPriority (handle, THREAD_PRIORITY_TIME_CRITICAL);
   return handle;
}

uint32_t os_get_current_time_us (void)
{
   return GetTickCount() * 1000;
}

os_sem_t * os_sem_create (size_t count)
{
   os_sem_t * sem;

   sem = (os_sem_t *)malloc (sizeof(*sem));

   InitializeConditionVariable (&sem->condition);
   InitializeCriticalSection (&sem->lock);
   sem->count = count;

   return sem;
}

int os_sem_wait (os_sem_t * sem, uint32_t time)
{
   BOOL success = TRUE;

   EnterCriticalSection (&sem->lock);
   while (sem->count == 0)
   {
      /* FIXME - decrease timeout if woken early */
      success = SleepConditionVariableCS (&sem->condition, &sem->lock, time);
      if (!success && GetLastError() == ERROR_TIMEOUT)
      {
         goto timeout;
      }
      assert (success);
   }

   sem->count--;

 timeout:
   LeaveCriticalSection (&sem->lock);
   return (success) ? 0 : 1;
}

void os_sem_signal (os_sem_t * sem)
{
   EnterCriticalSection (&sem->lock);
   sem->count++;
   LeaveCriticalSection (&sem->lock);
   WakeAllConditionVariable (&sem->condition);
}

void os_sem_destroy (os_sem_t * sem)
{
   EnterCriticalSection (&sem->lock);
   free (sem);
}

os_event_t * os_event_create (void)
{
   os_event_t * event;

   event = (os_event_t *)malloc (sizeof(*event));

   InitializeConditionVariable (&event->condition);
   InitializeCriticalSection (&event->lock);
   event->flags = 0;

   return event;
}

int os_event_wait (os_event_t * event, uint32_t mask, uint32_t * value, uint32_t time)
{
   BOOL success = TRUE;

   EnterCriticalSection (&event->lock);
   while ((event->flags & mask) == 0)
   {
      /* FIXME - decrease timeout if woken early */
      success = SleepConditionVariableCS (&event->condition, &event->lock, time);
      if (!success && GetLastError() == ERROR_TIMEOUT)
      {
         break;
      }
   }
   *value = event->flags & mask;
   LeaveCriticalSection (&event->lock);
   return (success) ? 0 : 1;
}

void os_event_set (os_event_t * event, uint32_t value)
{
   EnterCriticalSection (&event->lock);
   event->flags |= value;
   LeaveCriticalSection (&event->lock);
   WakeAllConditionVariable (&event->condition);
}

void os_event_clr (os_event_t * event, uint32_t value)
{
   EnterCriticalSection (&event->lock);
   event->flags &= ~value;
   LeaveCriticalSection (&event->lock);
}

void os_event_destroy (os_event_t * event)
{
   EnterCriticalSection (&event->lock);
   free (event);
}

os_mbox_t * os_mbox_create (size_t size)
{
   os_mbox_t * mbox;

   mbox = (os_mbox_t *)malloc (sizeof(*mbox) + size * sizeof(void *));

   InitializeConditionVariable (&mbox->condition);
   InitializeCriticalSection (&mbox->lock);

   mbox->r     = 0;
   mbox->w     = 0;
   mbox->count = 0;
   mbox->size  = size;

   return mbox;
}

int os_mbox_fetch (os_mbox_t * mbox, void ** msg, uint32_t time)
{
   BOOL success = TRUE;

   EnterCriticalSection (&mbox->lock);
   while (mbox->count == 0)
   {
      /* FIXME - decrease timeout if woken early */
      success = SleepConditionVariableCS (&mbox->condition, &mbox->lock, time);
      if (!success && GetLastError() == ERROR_TIMEOUT)
      {
         goto timeout;
      }
      assert (success);
   }

   *msg = mbox->msg[mbox->r++];
   if (mbox->r == mbox->size)
      mbox->r = 0;

   mbox->count--;

 timeout:
   LeaveCriticalSection (&mbox->lock);
   WakeAllConditionVariable (&mbox->condition);

   return (success) ? 0 : 1;
}

int os_mbox_post (os_mbox_t * mbox, void * msg, uint32_t time)
{
   BOOL success = TRUE;

   EnterCriticalSection (&mbox->lock);
   while (mbox->count == mbox->size)
   {
      /* FIXME - decrease timeout if woken early */
      success = SleepConditionVariableCS (&mbox->condition, &mbox->lock, time);
      if (!success && GetLastError() == ERROR_TIMEOUT)
      {
         goto timeout;
      }
      assert (success);
   }

   mbox->msg[mbox->w++] = msg;
   if (mbox->w == mbox->size)
      mbox->w = 0;

   mbox->count++;

 timeout:
   LeaveCriticalSection (&mbox->lock);
   WakeAllConditionVariable (&mbox->condition);

   return (success) ? 0 : 1;
}

void os_mbox_destroy (os_mbox_t * mbox)
{
   EnterCriticalSection (&mbox->lock);
   free (mbox);
}

static VOID CALLBACK os_timer_callback (os_timer_t * timer, BOOLEAN timer_or_waitfired)
{
   if (timer->fn)
      timer->fn (timer, timer->arg);
}

os_timer_t * os_timer_create (uint32_t us, void (*fn) (os_timer_t *, void * arg), void * arg,
                              bool oneshot)
{
   DWORD period = us / 1000;
   os_timer_t * timer;

   timer = (os_timer_t *)malloc (sizeof(*timer));

   timer->fn = fn;
   timer->arg = arg;
   timer->us = us;
   timer->oneshot = oneshot;

   return timer;
}

void os_timer_set (os_timer_t * timer, uint32_t us)
{
   timer->us = us;
}

void os_timer_start (os_timer_t * timer)
{
   DWORD period = timer->us / 1000;
   DWORD dueTime = period;
   ULONG flags;

   flags = WT_EXECUTEINTIMERTHREAD;
   if (timer->oneshot)
   {
      flags |= WT_EXECUTEONLYONCE;
      period = 0;
   }

   CreateTimerQueueTimer (&timer->handle, NULL, os_timer_callback,
                          timer, dueTime, period, flags);
}

void os_timer_stop (os_timer_t * timer)
{
   DeleteTimerQueueTimer (NULL, timer->handle, INVALID_HANDLE_VALUE);
}

void os_timer_destroy (os_timer_t * timer)
{
   free (timer);
}

uint32_t    osal_buf_alloc_cnt = 0; /* Count outstanding buffers */

os_buf_t * os_buf_alloc(uint16_t length)
{
   os_buf_t *p = malloc(sizeof(os_buf_t) + length);

   if (p)
   {
      p->payload = &p[1];  /* Payload follows header struct */
      p->len = length;

      osal_buf_alloc_cnt++;
   }
   else
   {
      assert("malloc() failed\n");
   }

   return p;
}
void os_buf_free(os_buf_t *p)
{
   free(p);
   osal_buf_alloc_cnt--;
   return;
}

uint8_t os_buf_header(os_buf_t *p, int16_t header_size_increment)
{
   return 255;
}

int os_set_ip_suite(
   os_ipaddr_t             *p_ipaddr,
   os_ipaddr_t             *p_netmask,
   os_ipaddr_t             *p_gw,
   const char              *hostname)
{
   return 0;
}

void os_cpy_mac_addr(uint8_t * mac_addr)
{

}

void os_get_button(uint16_t id, bool *p_pressed)
{

}

void os_set_led(uint16_t id, bool on)
{

}
