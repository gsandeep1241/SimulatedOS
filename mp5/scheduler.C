/*
 File: scheduler.C
 
 Author:
 Date  :
 
 */

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "scheduler.H"
#include "thread.H"
#include "console.H"
#include "utils.H"
#include "assert.H"
#include "simple_keyboard.H"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* CONSTANTS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* METHODS FOR CLASS   S c h e d u l e r  */
/*--------------------------------------------------------------------------*/

Scheduler::Scheduler() {
  size = 0;
  head->thread = NULL;
  head->prev = NULL;
  head->next = tail;
  tail->prev = head;
  tail->next = NULL; 
  Console::puts("Constructed Scheduler.\n");
}

void Scheduler::yield() {
  if (size == 0) {
    return;
  }
  Node* node = tail->prev;
  Thread* thread = node->thread;
  Node* prev = node->prev;

  prev->next = tail;
  tail->prev = prev;

  Thread::dispatch_to(thread);  
}

void Scheduler::resume(Thread * _thread) {
  Node* node;
  node->thread = _thread;
  
  Node* next = head->next;
  head->next = node;
  node->prev = head;
  next->prev = node;
  node->next = next;
}

void Scheduler::add(Thread * _thread) {
  resume(_thread);
}

void Scheduler::terminate(Thread * _thread) {
  assert(false);
}
