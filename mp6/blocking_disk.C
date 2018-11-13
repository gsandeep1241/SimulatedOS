/*
     File        : blocking_disk.c

     Author      : 
     Modified    : 

     Description : 

*/

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

    /* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "assert.H"
#include "utils.H"
#include "console.H"
#include "blocking_disk.H"

extern Scheduler* SYSTEM_SCHEDULER;
/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR */
/*--------------------------------------------------------------------------*/

BlockingDisk::BlockingDisk(DISK_ID _disk_id, unsigned int _size) 
  : SimpleDisk(_disk_id, _size) {
  head = new Node();
  tail = new Node();
  head->prev = NULL;
  head->next = tail;
  tail->prev = head;
  tail->next = NULL; 
  Console::puts("Constructed BlockingDisk Object.\n");
}

BlockingDisk::~BlockingDisk() {
  delete head;
  delete tail;
}


void BlockingDisk::wait_until_ready() {
  // adding the current thread to blocking queue
  // and yielding
  // will be picked back up by the scheduler
  Console::puts("Goes into waiting....\n");
  Node* node = new Node();
  node->thread = Thread::CurrentThread();
  
  Node* next = head->next;
  head->next = node;
  node->prev = head;
  next->prev = node;
  node->next = next;
  
  SYSTEM_SCHEDULER->yield(); 
}

/*--------------------------------------------------------------------------*/
/* SIMPLE_DISK FUNCTIONS */
/*--------------------------------------------------------------------------*/

void BlockingDisk::read(unsigned long _block_no, unsigned char * _buf) {
  // -- REPLACE THIS!!!
  SimpleDisk::read(_block_no, _buf);

}


void BlockingDisk::write(unsigned long _block_no, unsigned char * _buf) {
  // -- REPLACE THIS!!!
  SimpleDisk::read(_block_no, _buf);
}

bool BlockingDisk::ready() {
  return ((head->next != tail) &&  (is_ready()));
}

void BlockingDisk::resume() {
  Console::puts("Resuming..\n");
  Node* node = tail->prev;
  Thread* thread = node->thread;
  Node* prev = node->prev;

  prev->next = tail;
  tail->prev = prev;

  delete node;
  SYSTEM_SCHEDULER->add(thread); 
}

