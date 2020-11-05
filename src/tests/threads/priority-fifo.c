/* Creates several threads all at the same priority and ensures
   that they consistently run in the same round-robin order.

   Based on a test originally submitted for Stanford's CS 140 in
   winter 1999 by by Matt Franklin
   <startled@leland.stanford.edu>, Greg Hutchins
   <gmh@leland.stanford.edu>, Yu Ping Hu <yph@cs.stanford.edu>.
   Modified by arens. */

/*这个测试创建了一个优先级PRI_DEFAULT+2的主线程，并用这个线程创建了16个优先级PRI_DEFAULT+1的子线程，然后把主线程的优先级设置为优先级PRI_DEFAULT，所以现在pintos内有16个优先级PRI_DEFAULT+1的线程和1个优先级PRI_DEFAULT的线程在跑，测试需要把16个线程跑完再结束那一个线程，看起来没什么问题，但注意OS中线程是并行执行的，有可能最开始的一个线程在设置完优先级之后立刻结束了，而此时其他线程并未结束，即37行的注释，因此在线程设置完优先级之后应该立刻重新调度，因此只需要在thread_set_priority()函数里添加thread_yield()函数即可。*/

#include <stdio.h>
#include "tests/threads/tests.h"
#include "threads/init.h"
#include "devices/timer.h"
#include "threads/malloc.h"
#include "threads/synch.h"
#include "threads/thread.h"

struct simple_thread_data 
  {
    int id;                     /* Sleeper ID. */
    int iterations;             /* Iterations so far. */
    struct lock *lock;          /* Lock on output. */
    int **op;                   /* Output buffer position. */
  };

#define THREAD_CNT 16
#define ITER_CNT 16

static thread_func simple_thread_func;

void
test_priority_fifo (void) 
{
  struct simple_thread_data data[THREAD_CNT];
  struct lock lock;
  int *output, *op;
  int i, cnt;

  /* This test does not work with the MLFQS. */
  ASSERT (!thread_mlfqs);

  /* Make sure our priority is the default. */
  ASSERT (thread_get_priority () == PRI_DEFAULT);

  msg ("%d threads will iterate %d times in the same order each time.",
       THREAD_CNT, ITER_CNT);
  msg ("If the order varies then there is a bug.");

  output = op = malloc (sizeof *output * THREAD_CNT * ITER_CNT * 2);
  ASSERT (output != NULL);
  lock_init (&lock);

  thread_set_priority (PRI_DEFAULT + 2);
  for (i = 0; i < THREAD_CNT; i++) 
    {
      char name[16];
      struct simple_thread_data *d = data + i;
      snprintf (name, sizeof name, "%d", i);
      d->id = i;
      d->iterations = 0;
      d->lock = &lock;
      d->op = &op;
      thread_create (name, PRI_DEFAULT + 1, simple_thread_func, d);
    }

  thread_set_priority (PRI_DEFAULT);
  /* All the other threads now run to termination here. 
  现在，所有其他线程在此处终止。*/
  ASSERT (lock.holder == NULL);

  cnt = 0;
  for (; output < op; output++) 
    {
      struct simple_thread_data *d;

      ASSERT (*output >= 0 && *output < THREAD_CNT);
      d = data + *output;
      if (cnt % THREAD_CNT == 0)
        printf ("(priority-fifo) iteration:");
      printf (" %d", d->id);
      if (++cnt % THREAD_CNT == 0)
        printf ("\n");
      d->iterations++;
    }
}

static void 
simple_thread_func (void *data_) 
{
  struct simple_thread_data *data = data_;
  int i;
  
  for (i = 0; i < ITER_CNT; i++) 
    {
      lock_acquire (data->lock);
      *(*data->op)++ = data->id;
      lock_release (data->lock);
      thread_yield ();
    }
}
