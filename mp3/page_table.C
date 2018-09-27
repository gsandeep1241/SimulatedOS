#include "assert.H"
#include "exceptions.H"
#include "console.H"
#include "paging_low.H"
#include "page_table.H"

PageTable * PageTable::current_page_table = NULL;
unsigned int PageTable::paging_enabled = 0;
ContFramePool * PageTable::kernel_mem_pool = NULL;
ContFramePool * PageTable::process_mem_pool = NULL;
unsigned long PageTable::shared_size = 0;



void PageTable::init_paging(ContFramePool * _kernel_mem_pool,
                            ContFramePool * _process_mem_pool,
                            const unsigned long _shared_size)
{
    Console::puts("Initializing Paging system...");
    kernel_mem_pool = _kernel_mem_pool;
    process_mem_pool = _process_mem_pool;
    shared_size = _shared_size;
    Console::puts("Initialized Paging System\n");
}

PageTable::PageTable()
{
    Console::puts("Constructing Page Table object\n");
    unsigned long dir = kernel_mem_pool->get_frames(1);
    page_directory = (unsigned long *) (dir * PAGE_SIZE);
    Console::puts("dir: "); Console::puti(dir); Console::puts("\n");

    unsigned long page_table_frame_no = kernel_mem_pool->get_frames(1);
    unsigned long *page_table;
    page_table = (unsigned long *) (page_table_frame_no * PAGE_SIZE);

    unsigned long address = 0;
    unsigned long i;

    for(i=0; i < 1024; i++) {
       page_table[i] = (address | 3);
       address = address + 4096;
    }
    page_directory[0] = page_table_frame_no * PAGE_SIZE;
    page_directory[0] = (page_directory[0] | 3);

    for(i=1; i < 1024; i++) {
        page_directory[i] = (0 | 2);
    }
    
    Console::puts("Constructed Page Table object\n");
}


void PageTable::load()
{
    unsigned long temp = (unsigned long) page_directory;
    write_cr3(temp);
    Console::puts("Loaded page table\n");
}

void PageTable::enable_paging()
{
    unsigned long temp = read_cr0();
    write_cr0(temp | 0x80000000);
    paging_enabled = 1;
    Console::puts("Enabled paging\n");
}

void PageTable::handle_fault(REGS * _r)
{
  assert(false);
  Console::puts("handled page fault\n");
}

