/*
     File        : file.C

     Author      : Riccardo Bettati
     Modified    : 2017/05/01

     Description : Implementation of simple File class, with support for
                   sequential read/write operations.
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
#include "file.H"
#include "file_system.H"

extern FileSystem* FILE_SYSTEM;
/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR */
/*--------------------------------------------------------------------------*/

File::File(unsigned int start, unsigned int size, unsigned int id) {
    /* We will need some arguments for the constructor, maybe pointer to disk
     block with file management and allocation data. */
    Console::puts("In file constructor.\n");
    start_block = start;
    size_in_bytes = size;
    current_pos = 0;
    file_id = id;
}

/*--------------------------------------------------------------------------*/
/* FILE FUNCTIONS */
/*--------------------------------------------------------------------------*/

int File::Read(unsigned int _n, char * _buf) {
    Console::puts("reading from file\n");
    assert(current_pos < size_in_bytes);

    unsigned int rem = _n;
    unsigned int idx = 0;
    while (rem > 0) {
        unsigned long block_no = start_block + current_pos/512;
        unsigned char content[512];
        FILE_SYSTEM->disk->read(block_no, content);
        unsigned int pos = current_pos%512;
        if (rem > 512-pos) {
          // read these; reduce rem; advance current_pos
          memcpy(_buf+idx, content+pos, 512-pos);
          current_pos += (512-pos); idx += (512-pos); rem -= (512-pos);
        } else {
          if ((block_no-start_block)*512 + pos + rem >= size_in_bytes) {
             // just read till end of file and return
             unsigned int val = size_in_bytes - (block_no-start_block)*512 - pos;
             memcpy(_buf+idx, content+pos, val);
             rem -= val; current_pos += val;
             return _n - rem;
          } else {
             memcpy(_buf+idx, content+pos, rem);
             rem = 0; current_pos += rem;
             return _n;
          }
        }

    }
}


void File::Write(unsigned int _n, const char * _buf) {
    Console::puts("writing to file\n");
    
}

void File::Reset() {
    Console::puts("reset current position in file\n");
    current_pos = 0;
}

void File::Rewrite() {
    Console::puts("erase content of file\n");
    current_pos = 0;
    size_in_bytes = 0;
}


bool File::EoF() {
    Console::puts("testing end-of-file condition\n");
    if (size_in_bytes == 0) return (current_pos == 0);
    return current_pos == size_in_bytes - 1;
}
