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
    inode_block_num = 0;
    free_block_num = 1;
    size = 0;
    num_files = 0;
}

/*--------------------------------------------------------------------------*/
/* FILE SYSTEM FUNCTIONS */
/*--------------------------------------------------------------------------*/

bool FileSystem::Mount(SimpleDisk * _disk) {
    Console::puts("mounting file system form disk\n");
    disk = _disk;
}

bool FileSystem::Format(SimpleDisk * _disk, unsigned int _size) {
    Console::puts("formatting disk\n");
    // when we format, we read the inode block and change num files to 0
    // and size of the file system to _size
    unsigned char buf[512];
    _disk->read(0, buf);
    memcpy(buf, &_size, 4);

    unsigned int number_of_files = 0;
    memcpy(buf+4, &number_of_files, 4);
    _disk->write(0, buf);
   // then we read the free_blocks_block and reset it such that all blocks (except 1 and 2) are 0
    
    _disk->read(1, buf);
    unsigned int first = 0xC000;
    memcpy(buf, &first, 4);

    // freeing up all disk blocks
    for (int i=4; i < 512; i+=4) {
      unsigned int val = 0x0000;
      memcpy(buf+i, &val, 4);
    }
    _disk->write(1, buf);
}

File * FileSystem::LookupFile(int _file_id) {
    Console::puts("looking up file\n");
    unsigned char inode[512];
    disk->read(inode_block_num, inode);
    unsigned int num_files = 0;
    memcpy(&num_files, inode+4, 4);
    
    for(int i=0; i < num_files; i+=24) {
        int id = 0;
        memcpy(&id, inode+8+i, 4);
        if (_file_id == id) {
            return new File();
        }
    }
    return NULL;
}

bool FileSystem::CreateFile(int _file_id) {
    Console::puts("creating file\n");
    assert(false);
}

bool FileSystem::DeleteFile(int _file_id) {
    Console::puts("deleting file\n");
    assert(false);
}
