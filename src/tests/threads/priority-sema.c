/* Tests that the highest-priority thread waiting on a semaphore
   is the first to wake up. 
   测试：等待的线程中优先级最高的那个是第一个被唤醒的线程。

   这个测试创建了10个优先级不等的线程，并且每个线程调用sema_down函数，
   其他得不到信号量的线程都得阻塞，而每次运行的线程释放信号量时必须确
   保优先级最高的线程继续执行，因此修改sema_up。查到semaphore结构体
   如下，waiters为阻塞队列
   */

#include <stdio.h>
#include "tests/threads/tests.h"
#include "threads/init.h"
#include "threads/malloc.h"
#include "threads/synch.h"
#include "threads/thread.h"
#include "devices/timer.h"

static thread_func priority_sema_thread;
static struct semaphore sema;

void
test_priority_sema (void) 
{
  int i;
  
  /* This test does not work with the MLFQS. */
  ASSERT (!thread_mlfqs);

  sema_init (&sema, 0);
  thread_set_priority (PRI_MIN);
  for (i = 0; i < 10; i++) 
    {
      int priority = PRI_DEFAULT - (i + 3) % 10 - 1;
      char name[16];
      snprintf (name, sizeof name, "priority %d", priority);
      thread_create (name, priority, priority_sema_thread, NULL);
    }

  for (i = 0; i < 10; i++) 
    {
      sema_up (&sema);
      msg ("Back in main thread."); 
    }
}

static void
priority_sema_thread (void *aux UNUSED) 
{
  sema_down (&sema);
  msg ("Thread %s woke up.", thread_name ());
}
