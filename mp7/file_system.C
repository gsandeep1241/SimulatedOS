/*
     File        : file_system.C

     Author      : Riccardo Bettati
     Modified    : 2017/05/01

     Description : Implementation of simple File System class.
                   Has support for numerical file identifiers.
 */

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

    /* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "assert.H"
#include "console.H"
#include "file_system.H"


/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR */
/*--------------------------------------------------------------------------*/

FileSystem::FileSystem() {
    Console::puts("In file system constructor.\n");
}

/*--------------------------------------------------------------------------*/
/* FILE SYSTEM FUNCTIONS */
/*--------------------------------------------------------------------------*/

bool FileSystem::Mount(SimpleDisk * _disk) {
    Console::puts("mounting file system form disk\n");
    disk = _disk;
    return true;
}

bool FileSystem::Format(SimpleDisk * _disk, unsigned int _size) {
    Console::puts("formatting disk\n");
    // when we format, we read the inode block and change num files to 0
    // and size of the file system to _size
    unsigned char buf[512];
    _disk->read(0, buf);
    memcpy(buf, &_size, 4);

    unsigned int num_created = 0;
    memcpy(buf+4, &num_created, 4);
    _disk->write(0, buf);
   // then we read the free_blocks_block and reset it such that all blocks (except 1 and 2) are 0
    
    _disk->read(1, buf);
    unsigned int first = 0xC000;
    memcpy(buf, &first, 4);

    // freeing up all disk blocks
    for (int i=4; i < 512; i+=4) {
      unsigned int val = 0;
      memcpy(buf+i, &val, 4);
    }
    _disk->write(1, buf);
    return true;
}

File * FileSystem::LookupFile(int _file_id) {
    Console::puts("looking up file\n");
    unsigned char inode[512];
    disk->read(0, inode);
    unsigned int num_created = 0;
    memcpy(&num_created, inode+4, 4);
    
    // iterating all created files
    // Console::puts("Nc: "); Console::puti(num_created); Console::puts("\n");
    for(int i=0; i < num_created*16; i+=16) {
        int id = 0;
        memcpy(&id, inode+8+i, 4);
    // Console::puts("id: "); Console::puti(id); Console::puts("\n");
        // id matched
        if (_file_id == id) {
            unsigned int is_deleted = 0;
            memcpy(&is_deleted, inode+8+i+12, 4);

            // ensure this is not deleted
            if (is_deleted == 1) {
               continue;
            }
            unsigned int block_number = 0;
            memcpy(&block_number, inode+8+i+8, 4);

            unsigned int file_size = 0;
            memcpy(&file_size, inode+8+i+4, 4);
            return new File(block_number, file_size, _file_id, 8+i);
        }
    }
    return NULL;
}

bool FileSystem::CreateFile(int _file_id) {
    Console::puts("creating file\n");
    unsigned char inode[512];
    disk->read(0, inode);
    unsigned int num_created = 0;
    memcpy(&num_created, inode+4, 4);

    unsigned int fs_size = 0;
    memcpy(&fs_size, inode, 4);
    
    unsigned char free_blocks[512];
    unsigned int block_num = 0;
    disk->read(1, free_blocks);


    int k=0;
    for (k=0; k < num_created*16; k+=16) {
        int is_deleted = 0;
        memcpy(&is_deleted, inode+8+k+12, 4);
        if (is_deleted == 1) {
           break;
        }
    }

    // finding a free disk block
    for(int i=2; i+8 < 512; i+=8) {
       assert(i*8 < fs_size);
       unsigned int val = 0;
       memcpy(&val, free_blocks+i, 4);
       if ((val & 0x8000) == 0) {
          val = 0x8000;
          memcpy(free_blocks+i, &val, 4);
          block_num = i*8;
         // writing to the inode block
         int file_size = 0;
         int is_deleted = 0;
         memcpy(inode+8+k, &_file_id, 4);
         memcpy(inode+8+k + 4, &file_size, 4);
         memcpy(inode+8+k + 8, &block_num, 4);
         memcpy(inode+8+k + 12, &is_deleted, 4);
         
         num_created++;
         memcpy(inode+4, &num_created, 4);
         // writing blocks back to disk
         disk->write(1, free_blocks);
         disk->write(0, inode);
         return true;
       }
    }
    return false;
}

bool FileSystem::DeleteFile(int _file_id) {
    Console::puts("deleting file\n");
    unsigned char inode[512];
    disk->read(0, inode);
    unsigned int num_created = 0;
    memcpy(&num_created, inode+4, 4);
    unsigned char free_blocks[512];
    
    // iterating all created files
    for(int i=0; i < num_created*16; i+=16) {
        int id = 0;
        memcpy(&id, inode+8+i, 4);

        // id matched
        if (_file_id == id) {
            unsigned int is_deleted = 0;
            memcpy(&is_deleted, inode+8+i+12, 4);

            // ensure this is not deleted
            if (is_deleted == 1) {
               continue;
            }

            // mark it deleted
            is_deleted = 1;
            memcpy(inode+8+12+i, &is_deleted, 4);

            // get the starting block num
            unsigned int block_num = 0;
            memcpy(&block_num, inode+8+i+8, 4);

            // update free blocks bitmap
            disk->read(1, free_blocks);
            unsigned int val = 0x0000;
            unsigned int j = block_num/8;
            memcpy(free_blocks+j, &val, 4);

            // write both free blocks and inode back to disk
            disk->write(1, free_blocks); 
            disk->write(0, inode);
            Console::puts("File deleted.\n");
            return true;           
        }
    }
    return false;
}
