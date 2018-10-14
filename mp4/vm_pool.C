/*
 File: vm_pool.C
 
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

#include "vm_pool.H"
#include "console.H"
#include "utils.H"
#include "assert.H"
#include "simple_keyboard.H"
#include "page_table.H"

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
/* METHODS FOR CLASS   V M P o o l */
/*--------------------------------------------------------------------------*/

VMPool::VMPool(unsigned long  _base_address,
               unsigned long  _size,
               ContFramePool *_frame_pool,
               PageTable     *_page_table) {
    base_address = _base_address;
    size = _size;
    frame_pool = _frame_pool;
    page_table = _page_table;
    unsigned long addr = _frame_pool->get_frames(1);
    region = (Region*) (_base_address);
    num_regions = 1;
    page_table->register_pool(this);
    Console::puts("Constructed VMPool object.\n");
}

unsigned long VMPool::allocate(unsigned long _size) {
    // We handle the easy case where size is always a multiple of page size
    unsigned round = _size/PAGE_SIZE;
    if (_size%PAGE_SIZE != 0) {
        round++;
        _size = round * PAGE_SIZE;
    }
    
    assert(_size%PAGE_SIZE == 0);

    // We handle only a few regions (few === number of regions that fit in one page)
    assert(num_regions < PAGE_SIZE/sizeof(Region));
    
    unsigned long start_addr = base_address + PAGE_SIZE;
    if (num_regions != 1) {
        start_addr = region[num_regions-1].start_address + region[num_regions-1].size;
    }
    region[num_regions].start_address = start_addr;
    region[num_regions].size = _size;
    num_regions++;

    return start_addr;
}

void VMPool::release(unsigned long _start_address) {
    long i;
    long region_to_rel = -1;
    for (i=0; i < num_regions; i++) {
        if (region[i].start_address == _start_address) {
            region_to_rel = i; break;
        } 
    }
    assert(region_to_rel != -1);
    // swap with last region
    unsigned long temp = region[num_regions-1].start_address;
    region[num_regions-1].start_address = region[region_to_rel].start_address;
    region[region_to_rel].start_address = temp;

    temp = region[num_regions-1].size;
    region[num_regions-1].size = region[region_to_rel].size;
    region[region_to_rel].size = temp;

    // now act on last region
    num_regions--;
    unsigned long num_pages_to_free = region[num_regions].size/PAGE_SIZE;
    unsigned long addr = region[num_regions].start_address;

    for (int k=0; k < num_pages_to_free; k++) {
        page_table->free_page(addr);
        addr += PAGE_SIZE;
    }
    
    Console::puts("Released region of memory.\n");
}

bool VMPool::is_legitimate(unsigned long _address) {
    // The first page used by the region data
    if (_address >= base_address && _address < base_address + PAGE_SIZE) {
        return true;
    }
    for (int i=1; i < num_regions; i++) {
        if (_address >= region[i].start_address && _address < region[i].start_address + region[i].size) {
           return true;
        }
    }
    /*if (_address >= base_address && _address < base_address + size) {
        return true;
    }*/
    return false;
}

