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
    // Here paging is disabled.
    Console::puts("Constructing Page Table object\n");
    unsigned long dir = process_mem_pool->get_frames(1);
    page_directory = (unsigned long *) (dir * PAGE_SIZE);
    
    unsigned long page_table_frame_no = process_mem_pool->get_frames(1);
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
    
    i=0;
    for(i=1; i < 1023; i++) {
        page_directory[i] = (0 | 2);
    }
    page_directory[i] = dir * PAGE_SIZE;
    page_directory[i] = (page_directory[i] | 3);

    current_page_table = this;
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
    Console::puts("Enabling page table\n");
    unsigned long temp = read_cr0();
    write_cr0(temp | 0x80000000);
    paging_enabled = 1;
    Console::puts("Enabled paging\n");
}

void PageTable::handle_fault(REGS * _r)
{
    unsigned long cr3 = read_cr3();
    unsigned long temp = read_cr2();
  
    VMPool* pool = current_page_table->head;
    bool leg = false;

    while (pool != NULL) {
        if(pool->is_legitimate(temp)) {
           leg = true; break;
        }
        pool = pool->next;
    }

    if (!leg) {
        Console::puts("Aborting page fault handling.\n");
        assert(false);
        return;
    }
    unsigned long err = _r->err_code;
    
    unsigned long a = (temp & 0xFFC00000) >> 22;
    unsigned long b = (temp & 0x3FF000) >> 12;
    unsigned long c = (temp & 0xFFF);

    unsigned long* pde = (unsigned long *) ((0xFFFFF << 12) | (a << 2));
    unsigned long* pte = (unsigned long *) (((b << 2) | (a << 12)) | (0x3FF << 22));

    if ((*pde & 1) == 0) {
        // need to create page table page
        unsigned long ptp = process_mem_pool->get_frames(1);
        *pde = (ptp * PAGE_SIZE);
        *pde = ((*pde) | 3);

        unsigned long val = 0;
        for (int i=0; i < 1024; i++) {
            unsigned long* addr = (unsigned long *) (((val << 2) | (a << 12)) | (0x3FF << 22));
	    *addr = (0 | 2);
	    val++;
        }
        unsigned long pt = process_mem_pool->get_frames(1);
        *pte = (pt * PAGE_SIZE);
        *pte = (*pte | 3);

    }else {
        // page directory entry is valid. page table entry is not valid
        if ((*pte & 1) == 0) {
            unsigned long pt = process_mem_pool->get_frames(1);
            *pte = (pt * PAGE_SIZE);
            *pte = ((*pte) | 3);
        } else {
            Console::puts("Unknown error.");
            assert(false);
        }
    }

/*
    // MP3
    if ((page_directory[a] & 1) == 0) {
        unsigned long page_table_frame_no = kernel_mem_pool->get_frames(1);
        unsigned long *page_table;
        page_table = (unsigned long *) (page_table_frame_no * PAGE_SIZE);
        page_directory[a] = page_table_frame_no * PAGE_SIZE;
        page_directory[a] = (page_directory[a] | 3);
       
        unsigned long frame_no = process_mem_pool->get_frames(1);
        page_table[b] = frame_no * PAGE_SIZE;
        page_table[b] = (page_table[b] | 3);
        for(int i=0; i < 1024; i++) {
           if (i == b) {continue;}
           page_table[i] = (0 | 2);
        }
    }else {
        unsigned long d = (page_directory[a] & 0xFFFFF000);
        unsigned long * page_table = (unsigned long *) d;

        if ((page_table[b] & 1) == 0) {
            unsigned long frame_no = process_mem_pool->get_frames(1);
            page_table[b] = frame_no * PAGE_SIZE;
            page_table[b] = (page_table[b] | 3);
        } else {
            Console::puts("This error scenario is not handled yet.");
            assert(false);
	}
    }
*/
//     Console::puts("Handled page fault\n");
}

void PageTable::register_pool(VMPool * _vm_pool)
{
    if (head == NULL) {
        head = _vm_pool;
    }else {
        VMPool* pool = head;
        while(pool->next != NULL) {
           pool = pool->next;
        }
        pool->next = _vm_pool;
    }
    Console::puts("registered VM pool\n");
}

void PageTable::free_page(unsigned long _page_no) {
    unsigned long addr_num = (((_page_no >> 12) << 2) | (0x3FF << 22));
    unsigned long num = addr_num / PAGE_SIZE;
    unsigned long* pte_pointer = (unsigned long *) addr_num;
    *pte_pointer = (0 | 2);
    // process_mem_pool->release_frames(num);

    unsigned long temp = read_cr3();
    write_cr3(temp);

    // Console::puts("freed page\n");
}

