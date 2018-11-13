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
#include "blocking_disk.H"
#include "simple_keyboard.H"

extern BlockingDisk* SYSTEM_DISK;
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
  head = new Node();
  tail = new Node();
  head->prev = NULL;
  head->next = tail;
  tail->prev = head;
  tail->next = NULL; 
  Console::puts("Constructed Scheduler.\n");
}

Scheduler::~Scheduler() {
  delete head;
  delete tail;
}

void Scheduler::yield() {
  Node* node = tail->prev;
  Thread* thread = node->thread;
  Node* prev = node->prev;

  prev->next = tail;
  tail->prev = prev;

  delete node;

  // check the blocking queue's first node. If it is ready,
  // remove it from there and add it by using scheduler add.
  if (SYSTEM_DISK->ready()) {
    SYSTEM_DISK->resume();
  }
  Thread::dispatch_to(thread);  
}

void Scheduler::resume(Thread * _thread) {
  Node* node = new Node();
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
  Console::puts("Termination\n");
  delete _thread;
  yield(); 
}
