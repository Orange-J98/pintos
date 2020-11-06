/* This file is derived from source code for the Nachos
   instructional operating system.  The Nachos copyright notice
   is reproduced in full below. */

/* Copyright (c) 1992-1996 The Regents of the University of California.
   All rights reserved.

   Permission to use, copy, modify, and distribute this software
   and its documentation for any purpose, without fee, and
   without written agreement is hereby granted, provided that the
   above copyright notice and the following two paragraphs appear
   in all copies of this software.

   IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO
   ANY PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR
   CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OF THIS SOFTWARE
   AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF CALIFORNIA
   HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

   THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY
   WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
   PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS"
   BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
   PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR
   MODIFICATIONS.
*/

#include "threads/synch.h"
#include <stdio.h>
#include <string.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

/* Initializes semaphore SEMA to VALUE.  A semaphore is a
   nonnegative integer along with two atomic operators for
   manipulating it:

   - down or "P": wait for the value to become positive, then
     decrement it.

   - up or "V": increment the value (and wake up one waiting
     thread, if any). */
void sema_init(struct semaphore *sema, unsigned value)
{
  ASSERT(sema != NULL);

  sema->value = value;
  list_init(&sema->waiters);
}

/* Down or "P" operation on a semaphore.  Waits for SEMA's value
   to become positive and then atomically decrements it.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but if it sleeps then the next scheduled
   thread will probably turn interrupts back on. */
void sema_down(struct semaphore *sema)
{
  enum intr_level old_level;

  ASSERT(sema != NULL);
  ASSERT(!intr_context());

  old_level = intr_disable();
  while (sema->value == 0)
  {
    list_push_back(&sema->waiters, &thread_current()->elem);
    thread_block();
  }
  sema->value--;
  intr_set_level(old_level);
}

/* Down or "P" operation on a semaphore, but only if the
   semaphore is not already 0.  Returns true if the semaphore is
   decremented, false otherwise.

   This function may be called from an interrupt handler. */
bool sema_try_down(struct semaphore *sema)
{
  enum intr_level old_level;
  bool success;

  ASSERT(sema != NULL);

  old_level = intr_disable();
  if (sema->value > 0)
  {
    sema->value--;
    success = true;
  }
  else
    success = false;
  intr_set_level(old_level);

  return success;
}

/* Up or "V" operation on a semaphore.  Increments SEMA's value
   and wakes up one thread of those waiting for SEMA, if any.

   This function may be called from an interrupt handler. */
/*UP或“ V”操作信号灯。 增加SEMA的值，并唤醒等待SEMA的线程中的
   一个线程（如果有）。 可以从中断处理程序中调用此函数。*/
void sema_up(struct semaphore *sema)
{
  enum intr_level old_level;

  ASSERT(sema != NULL);

  old_level = intr_disable();

  //在waiters中取出优先级最高的thread，并yield()即可即可
  if (!list_empty(&sema->waiters))
  {
    struct list_elem *max_priority = list_max(&sema->waiters, thread_compare_priority, NULL);
    list_remove(max_priority);
    thread_unblock(list_entry(max_priority, struct thread, elem));
  }
  sema->value++;
  intr_set_level(old_level);
  thread_yield();
}

static void sema_test_helper(void *sema_);

/* Self-test for semaphores that makes control "ping-pong"
   between a pair of threads.  Insert calls to printf() to see
   what's going on. */
void sema_self_test(void)
{
  struct semaphore sema[2];
  int i;

  printf("Testing semaphores...");
  sema_init(&sema[0], 0);
  sema_init(&sema[1], 0);
  thread_create("sema-test", PRI_DEFAULT, sema_test_helper, &sema);
  for (i = 0; i < 10; i++)
  {
    sema_up(&sema[0]);
    sema_down(&sema[1]);
  }
  printf("done.\n");
}

/* Thread function used by sema_self_test(). */
static void
sema_test_helper(void *sema_)
{
  struct semaphore *sema = sema_;
  int i;

  for (i = 0; i < 10; i++)
  {
    sema_down(&sema[0]);
    sema_up(&sema[1]);
  }
}

/* Initializes LOCK.  A lock can be held by at most a single
   thread at any given time.  Our locks are not "recursive", that
   is, it is an error for the thread currently holding a lock to
   try to acquire that lock.
   初始化LOCK。 在任何给定时间，一个锁最多只能由一个线程持有。 我们的锁
   不是“递归的”，也就是说，当前持有锁的线程尝试获取该锁是错误的。
   
   A lock is a specialization of a semaphore with an initial
   value of 1.  The difference between a lock and such a
   semaphore is twofold.  First, a semaphore can have a value
   greater than 1, but a lock can only be owned by a single
   thread at a time.  Second, a semaphore does not have an owner,
   meaning that one thread can "down" the semaphore and then
   another one "up" it, but with a lock the same thread must both
   acquire and release it.  When these restrictions prove
   onerous, it's a good sign that a semaphore should be used,
   instead of a lock.
   锁是初始值为1的信号量的一种特殊形式。锁与此类信号量之间的差异是双重
   的。 首先，信号量的值可以大于1，但锁一次只能由单个线程拥有。 其次，
   信号量没有所有者，这意味着一个线程可以“降低”该信号量，然后另一个线程
   可以“升高”它，但是具有锁的同一线程必须同时获取和释放它。 当这些限制
   变得繁重时，使用信号量而不是锁是一个很好地习惯。
    */
void lock_init(struct lock *lock)
{
  ASSERT(lock != NULL);
  lock->holder = NULL;
  sema_init(&lock->semaphore, 1);
}

/* Acquires LOCK, sleeping until it becomes available if
   necessary.  The lock must not already be held by the current
   thread.
   获取LOCK，并在必要时保持睡眠直到可用。 该锁必须尚未被当前线程持有。

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but interrupts will be turned back on if
   we need to sleep. 
   该函数可能处于休眠状态，因此不得在中断处理程序中调用它。 可以在禁用
   中断的情况下调用此函数，但是如果我们需要睡眠，则会重新打开中断。*/
void lock_acquire(struct lock *lock)
{
  ASSERT(lock != NULL);
  ASSERT(!intr_context());
  ASSERT(!lock_held_by_current_thread(lock));

  /*这里用于优先级馈赠接下来处理lock_acquire()，为了区别与第三部分的mlfqs，
  所有添加部分都进行了thread_mlfqs判断。在获取锁之前，根据前面的分析，循环
  更新所有参与嵌套的线程的优先级。*/
  if (lock->holder != NULL && !thread_mlfqs)
  {
    thread_current()->waiting_lock = lock;
    struct lock *wlock = lock;
    //当当前线程优先级高于锁的拥有者中最高的优先级时，进行循环
    while (wlock != NULL && thread_current()->priority > wlock->max_priority)
    {
      //首先将锁的最高优先级更新
      wlock->max_priority = thread_current()->priority;
      //比较锁的拥有者中拥有的所有锁里面最高的优先级；
      struct list_elem *max_priority_in_locks = list_max(&wlock->holder->locks, lock_compare_max_priority, NULL);
      int maximal = list_entry(max_priority_in_locks, struct lock, elem)->max_priority;
      //如果锁的拥有者的优先级比最高的优先级小，馈赠优先级；
      if (wlock->holder->priority < maximal)
        wlock->holder->priority = maximal;
      //这里进行一个嵌套循环，现在wlock是锁的拥有者正在等待的锁，进行进一步的馈赠；
      wlock = wlock->holder->waiting_lock;
    }
  }

  sema_down(&lock->semaphore);
  lock->holder = thread_current();
  //优先级馈赠
   if(!thread_mlfqs){
     thread_current()->waiting_lock = NULL;
     lock->max_priority = thread_current()->priority;
     list_push_back(&thread_current()->locks,&lock->elem);
     if(lock->max_priority > thread_current()->priority){
       thread_current()->priority = lock->max_priority;
       thread_yield();
     }
   }
}

/* Tries to acquires LOCK and returns true if successful or false
   on failure.  The lock must not already be held by the current
   thread.
   尝试获取LOCK，如果成功则返回true，否则返回false。 该锁必须尚未被当前线程持有。

   This function will not sleep, so it may be called within an
   interrupt handler.
   该函数不会休眠，因此可以在中断处理程序中调用。 */
bool lock_try_acquire(struct lock *lock)
{
  bool success;

  ASSERT(lock != NULL);
  ASSERT(!lock_held_by_current_thread(lock));

  success = sema_try_down(&lock->semaphore);
  if (success)
    lock->holder = thread_current();
  return success;
}

/* Releases LOCK, which must be owned by the current thread.
  释放LOCK，该锁必须由当前线程拥有。

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to release a lock within an interrupt
   handler. 
   中断处理程序无法获取锁，因此尝试在中断处理程序中释放锁没有任何意义。*/
void lock_release(struct lock *lock)
{
  ASSERT(lock != NULL);
  ASSERT(lock_held_by_current_thread(lock));

/*处理lock_release()函数，在释放锁之前，对该线程的优先级进行更新，如果他
没有拥有的锁，就直接更新为original_priority，否则从所有锁的max_priority
中找到最大值进行更新。*/
  if(!thread_mlfqs){
     list_remove(&lock->elem);
     int maximal = thread_current()->original_priority;
     if(!list_empty(&thread_current()->locks)){
       struct list_elem *max_priority_in_locks = list_max(&thread_current()->locks,lock_compare_max_priority,NULL);
       int p = list_entry(max_priority_in_locks,struct lock,elem)->max_priority;
       if(p > maximal){
         maximal = p;
       }
     }
     thread_current()->priority = maximal;
   }

  lock->holder = NULL;
  sema_up(&lock->semaphore);
}

/* Returns true if the current thread holds LOCK, false
   otherwise.  (Note that testing whether some other thread holds
   a lock would be racy.) 
   如果当前线程持有LOCK，则返回true，否则返回false。 （请注意，测试其
   他线程是否持有锁会很合理。）*/
bool lock_held_by_current_thread(const struct lock *lock)
{
  ASSERT(lock != NULL);

  return lock->holder == thread_current();
}

/* One semaphore in a list. 
列表中的一个信号量。*/
struct semaphore_elem
{
  struct list_elem elem;      /* List element. */
  struct semaphore semaphore; /* This semaphore. */
};

/* Initializes condition variable COND.  A condition variable
   allows one piece of code to signal a condition and cooperating
   code to receive the signal and act upon it. 
   初始化条件变量COND。 条件变量允许一条代码发出条件信号，而协作代码则
   接收信号并对其执行操作。*/
void cond_init(struct condition *cond)
{
  ASSERT(cond != NULL);

  list_init(&cond->waiters);
}

/* Atomically releases LOCK and waits for COND to be signaled by
   some other piece of code.  After COND is signaled, LOCK is
   reacquired before returning.  LOCK must be held before calling
   this function.

   The monitor implemented by this function is "Mesa" style, not
   "Hoare" style, that is, sending and receiving a signal are not
   an atomic operation.  Thus, typically the caller must recheck
   the condition after the wait completes and, if necessary, wait
   again.

   A given condition variable is associated with only a single
   lock, but one lock may be associated with any number of
   condition variables.  That is, there is a one-to-many mapping
   from locks to condition variables.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but interrupts will be turned back on if
   we need to sleep. */
void cond_wait(struct condition *cond, struct lock *lock)
{
  struct semaphore_elem waiter;

  ASSERT(cond != NULL);
  ASSERT(lock != NULL);
  ASSERT(!intr_context());
  ASSERT(lock_held_by_current_thread(lock));

  sema_init(&waiter.semaphore, 0);
  list_push_back(&cond->waiters, &waiter.elem);
  lock_release(lock);
  sema_down(&waiter.semaphore);
  lock_acquire(lock);
}

/* If any threads are waiting on COND (protected by LOCK), then
   this function signals one of them to wake up from its wait.
   LOCK must be held before calling this function.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to signal a condition variable within an
   interrupt handler. */
void cond_signal(struct condition *cond, struct lock *lock UNUSED)
{
  ASSERT(cond != NULL);
  ASSERT(lock != NULL);
  ASSERT(!intr_context());
  ASSERT(lock_held_by_current_thread(lock));

  if (!list_empty(&cond->waiters))
  {
    struct list_elem *max_priority = list_max(&cond->waiters, cond_compare_priority, NULL);
    list_remove(max_priority);
    sema_up(&list_entry(max_priority, struct semaphore_elem, elem)->semaphore);
  }
}
bool cond_compare_priority(const struct list_elem *a, const struct list_elem *b, void *aux UNUSED)
{
  struct semaphore_elem *sa = list_entry(a, struct semaphore_elem, elem);
  struct semaphore_elem *sb = list_entry(b, struct semaphore_elem, elem);
  return list_entry(list_front(&sa->semaphore.waiters), struct thread, elem)->priority <
         list_entry(list_front(&sb->semaphore.waiters), struct thread, elem)->priority;
}

/* Compare function in list of a lock 锁列表的比较函数*/
bool lock_compare_max_priority(const struct list_elem *a, const struct list_elem *b, void *aux UNUSED)
{
  return list_entry(a, struct lock, elem)->max_priority < list_entry(b, struct lock, elem)->max_priority;
}

/* Wakes up all threads, if any, waiting on COND (protected by
   LOCK).  LOCK must be held before calling this function.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to signal a condition variable within an
   interrupt handler. */
void cond_broadcast(struct condition *cond, struct lock *lock)
{
  ASSERT(cond != NULL);
  ASSERT(lock != NULL);

  while (!list_empty(&cond->waiters))
    cond_signal(cond, lock);
}
