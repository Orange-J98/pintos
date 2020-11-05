/* Ensures that a high-priority thread really preempts.

   Based on a test originally submitted for Stanford's CS 140 in
   winter 1999 by by Matt Franklin
   <startled@leland.stanford.edu>, Greg Hutchins
   <gmh@leland.stanford.edu>, Yu Ping Hu <yph@cs.stanford.edu>.
   Modified by arens. */
/*这个就很简单了，创建一个新的高优先级线程抢占当前线程，因此在thread_create中，如果新线程的优先级高于当前线程优先级，调用thread_yield()函数即可。*/

#include <stdio.h>
#include "tests/threads/tests.h"
#include "threads/init.h"
#include "threads/synch.h"
#include "threads/thread.h"

static thread_func simple_thread_func;

void
test_priority_preempt (void) 
{
  /* This test does not work with the MLFQS. */
  ASSERT (!thread_mlfqs);

  /* Make sure our priority is the default.
   确保我们的优先级是default。即默认的优先级*/
  ASSERT (thread_get_priority () == PRI_DEFAULT);

  thread_create ("high-priority", PRI_DEFAULT + 1, simple_thread_func, NULL);
  msg ("The high-priority thread should have already completed.");
  /*新创建的更高优先级的线程需要先完成，所以当我们的线程创建一个更高级线程的时候，需要重新调度一下，这样才能保证新创建的更高级的线程优先完成！*/
}

static void 
simple_thread_func (void *aux UNUSED) 
{
  int i;
  
  for (i = 0; i < 5; i++) 
    {
      msg ("Thread %s iteration %d", thread_name (), i);
      thread_yield ();
    }
  msg ("Thread %s done!", thread_name ());
}
