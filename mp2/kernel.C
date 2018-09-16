/*
    File: kernel.C

    Author: R. Bettati
            Department of Computer Science
            Texas A&M University
    Date  : 02/02/17


    This file has the main entry point to the operating system.

*/


/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

#define MB * (0x1 << 20)
#define KB * (0x1 << 10)
/* Makes things easy to read */

#define KERNEL_POOL_START_FRAME ((2 MB) / (4 KB))
#define KERNEL_POOL_SIZE ((2 MB) / (4 KB))
#define PROCESS_POOL_START_FRAME ((4 MB) / (4 KB))
#define PROCESS_POOL_SIZE ((28 MB) / (4 KB))
/* Definition of the kernel and process memory pools */

#define MEM_HOLE_START_FRAME ((15 MB) / (4 KB))
#define MEM_HOLE_SIZE ((1 MB) / (4 KB))
/* We have a 1 MB hole in physical memory starting at address 15 MB */

#define TEST_START_ADDR_PROC (4 MB)
#define TEST_START_ADDR_KERNEL (2 MB)
/* Used in the memory test below to generate sequences of memory references. */
/* One is for a sequence of memory references in the kernel space, and the   */
/* other for memory references in the process space. */

#define N_TEST_ALLOCATIONS 
/* Number of recursive allocations that we use to test.  */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "machine.H"     /* LOW-LEVEL STUFF   */
#include "console.H"

#include "assert.H"
#include "cont_frame_pool.H"  /* The physical memory manager */

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

void test_memory(ContFramePool * _pool, unsigned int _allocs_to_go);

/*--------------------------------------------------------------------------*/
/* MAIN ENTRY INTO THE OS */
/*--------------------------------------------------------------------------*/

int main() {

    Console::init();


    /* ---- KERNEL POOL -- */


// /* BLOCK-1    
   ContFramePool kernel_mem_pool(KERNEL_POOL_START_FRAME,
                                  KERNEL_POOL_SIZE,
                                  0,
                                  0);
// */   

   /* -- TEST MEMORY ALLOCATOR */

/* BLOCK-2
    test_memory(&kernel_mem_pool, 32);
*/    


/* BLOCK-3   
    unsigned long n_info_frames = ContFramePool::needed_info_frames(PROCESS_POOL_SIZE);
    unsigned long process_mem_pool_info_frame = kernel_mem_pool.get_frames(n_info_frames);
    Console::puts("Process mem pool info frame: "); Console::puti(process_mem_pool_info_frame); Console::puts("\n");  // Expected 513
    ContFramePool process_mem_pool(PROCESS_POOL_START_FRAME,
                                   PROCESS_POOL_SIZE,
                                   process_mem_pool_info_frame,
                                   n_info_frames);
    process_mem_pool.mark_inaccessible(MEM_HOLE_START_FRAME, MEM_HOLE_SIZE);
    ContFramePool::release_frames(MEM_HOLE_START_FRAME);                  // Should release MEM_HOLE_SIZE number of frames
*/    


/* BLOCK-4
    unsigned long frame1 = kernel_mem_pool.get_frames(2);
    Console::puts("Frame1: "); Console::puti(frame1); Console::puts("\n");  // Expect 513
    unsigned long frame2 = kernel_mem_pool.get_frames(3);
    Console::puts("Frame2: "); Console::puti(frame2); Console::puts("\n");  // Expect 515
    unsigned long frame3 = kernel_mem_pool.get_frames(7);
    Console::puts("Frame3: "); Console::puti(frame3); Console::puts("\n");  // Expect 518
    unsigned long frame4 = kernel_mem_pool.get_frames(9);
    Console::puts("Frame4: "); Console::puti(frame4); Console::puts("\n");  // Expect 525
    
    ContFramePool::release_frames(frame3);                                  // Expect 7 to be released
    unsigned long frame5 = kernel_mem_pool.get_frames(15);
    Console::puts("Frame5: "); Console::puti(frame5); Console::puts("\n");  // Expect 534

    ContFramePool::release_frames(frame4);                                  // Expect 9 to be released
    unsigned long frame6 = kernel_mem_pool.get_frames(15);
    Console::puts("Frame6: "); Console::puti(frame6); Console::puts("\n");  // Expect 518 beacuse it is freed now
*/


    /* ---- Add code here to test the frame pool implementation. */
   

     /* -- NOW LOOP FOREVER */
    Console::puts("Testing is DONE. We will do nothing forever\n");
    Console::puts("Feel free to turn off the machine now.\n");

    for(;;);

    /* -- WE DO THE FOLLOWING TO KEEP THE COMPILER HAPPY. */
    return 1;
}

void test_memory(ContFramePool * _pool, unsigned int _allocs_to_go) {
    Console::puts("alloc_to_go = "); Console::puti(_allocs_to_go); Console::puts("\n");
    if (_allocs_to_go > 0) {
        int n_frames = _allocs_to_go % 4 + 1;
        unsigned long frame = _pool->get_frames(n_frames);
        int * value_array = (int*)(frame * (4 KB));        
        for (int i = 0; i < (1 KB) * n_frames; i++) {
            value_array[i] = _allocs_to_go;
        }
        test_memory(_pool, _allocs_to_go - 1);
        for (int i = 0; i < (1 KB) * n_frames; i++) {
            if(value_array[i] != _allocs_to_go){
                Console::puts("MEMORY TEST FAILED. ERROR IN FRAME POOL\n");
                Console::puts("i ="); Console::puti(i);
                Console::puts("   v = "); Console::puti(value_array[i]); 
                Console::puts("   n ="); Console::puti(_allocs_to_go);
                Console::puts("\n");
                for(;;); 
            }
        }
        ContFramePool::release_frames(frame);
    }
}

