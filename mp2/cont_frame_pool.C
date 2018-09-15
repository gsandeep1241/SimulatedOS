/*
 File: ContFramePool.C
 
 Author:
 Date  : 
 
 */

/*--------------------------------------------------------------------------*/
/* 
 POSSIBLE IMPLEMENTATION
 -----------------------

 The class SimpleFramePool in file "simple_frame_pool.H/C" describes an
 incomplete vanilla implementation of a frame pool that allocates 
 *single* frames at a time. Because it does allocate one frame at a time, 
 it does not guarantee that a sequence of frames is allocated contiguously.
 This can cause problems.
 
 The class ContFramePool has the ability to allocate either single frames,
 or sequences of contiguous frames. This affects how we manage the
 free frames. In SimpleFramePool it is sufficient to maintain the free 
 frames.
 In ContFramePool we need to maintain free *sequences* of frames.
 
 This can be done in many ways, ranging from extensions to bitmaps to 
 free-lists of frames etc.
 
 IMPLEMENTATION:
 
 One simple way to manage sequences of free frames is to add a minor
 extension to the bitmap idea of SimpleFramePool: Instead of maintaining
 whether a frame is FREE or ALLOCATED, which requires one bit per frame, 
 we maintain whether the frame is FREE, or ALLOCATED, or HEAD-OF-SEQUENCE.
 The meaning of FREE is the same as in SimpleFramePool. 
 If a frame is marked as HEAD-OF-SEQUENCE, this means that it is allocated
 and that it is the first such frame in a sequence of frames. Allocated
 frames that are not first in a sequence are marked as ALLOCATED.
 
 NOTE: If we use this scheme to allocate only single frames, then all 
 frames are marked as either FREE or HEAD-OF-SEQUENCE.
 
 NOTE: In SimpleFramePool we needed only one bit to store the state of 
 each frame. Now we need two bits. In a first implementation you can choose
 to use one char per frame. This will allow you to check for a given status
 without having to do bit manipulations. Once you get this to work, 
 revisit the implementation and change it to using two bits. You will get 
 an efficiency penalty if you use one char (i.e., 8 bits) per frame when
 two bits do the trick.
 
 DETAILED IMPLEMENTATION:
 
 How can we use the HEAD-OF-SEQUENCE state to implement a contiguous
 allocator? Let's look a the individual functions:
 
 Constructor: Initialize all frames to FREE, except for any frames that you 
 need for the management of the frame pool, if any.
 
 get_frames(_n_frames): Traverse the "bitmap" of states and look for a 
 sequence of at least _n_frames entries that are FREE. If you find one, 
 mark the first one as HEAD-OF-SEQUENCE and the remaining _n_frames-1 as
 ALLOCATED.

 release_frames(_first_frame_no): Check whether the first frame is marked as
 HEAD-OF-SEQUENCE. If not, something went wrong. If it is, mark it as FREE.
 Traverse the subsequent frames until you reach one that is FREE or 
 HEAD-OF-SEQUENCE. Until then, mark the frames that you traverse as FREE.
 
 mark_inaccessible(_base_frame_no, _n_frames): This is no different than
 get_frames, without having to search for the free sequence. You tell the
 allocator exactly which frame to mark as HEAD-OF-SEQUENCE and how many
 frames after that to mark as ALLOCATED.
 
 needed_info_frames(_n_frames): This depends on how many bits you need 
 to store the state of each frame. If you use a char to represent the state
 of a frame, then you need one info frame for each FRAME_SIZE frames.
 
 A WORD ABOUT RELEASE_FRAMES():
 
 When we releae a frame, we only know its frame number. At the time
 of a frame's release, we don't know necessarily which pool it came
 from. Therefore, the function "release_frame" is static, i.e., 
 not associated with a particular frame pool.
 
 This problem is related to the lack of a so-called "placement delete" in
 C++. For a discussion of this see Stroustrup's FAQ:
 http://www.stroustrup.com/bs_faq2.html#placement-delete
 
 */
/*--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "cont_frame_pool.H"
#include "console.H"
#include "utils.H"
#include "assert.H"

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
/* METHODS FOR CLASS   C o n t F r a m e P o o l */
/*--------------------------------------------------------------------------*/

static ContFramePool* pool = NULL;

ContFramePool::ContFramePool(unsigned long _base_frame_no,
                             unsigned long _n_frames,
                             unsigned long _info_frame_no,
                             unsigned long _n_info_frames)
{
    // Number of frames must be "fill" the bitmap!
    assert ((_n_frames % 8 ) == 0);
    assert(_n_frames <= FRAME_SIZE * 4);
    
    base_frame_no = _base_frame_no;
    n_frames = _n_frames;
    info_frame_no = _info_frame_no;
    n_info_frames = _n_info_frames;
    n_free_frames = n_frames;

    if (info_frame_no == 0) {
        bitmap = (unsigned char *) (base_frame_no * FRAME_SIZE);
        headmap = (unsigned char *) (base_frame_no * FRAME_SIZE + (n_frames/8));
    } else {
        bitmap = (unsigned char *) (info_frame_no * FRAME_SIZE);
        headmap = (unsigned char *) (info_frame_no * FRAME_SIZE + (n_frames/8));
    }
    
    
    // Everything ok. Proceed to mark all bits in the bitmap and headmap
    for(int i=0; i*8 < n_frames; i++) {
        bitmap[i] = 0xFF;
        headmap[i] = 0xFF;
    }

    if (info_frame_no == 0) {
        unsigned long remaining_info_frames = n_info_frames;
        int counter = 0;
        while (remaining_info_frames > 0) {
            for (int i=0; i < 8; i++) {
                if (remaining_info_frames == 0) {break;}
                bitmap[counter] = (0x7F >> i);
                if (remaining_info_frames == n_info_frames) {
                    headmap[counter] = (0x7F >> i);
                }
                remaining_info_frames -= 1;
            }
            counter++;
        }
    }
    n_free_frames -= n_info_frames;

    if (pool == NULL) {
        pool = this;
    } else {
       ContFramePool* prev = NULL; ContFramePool* curr = pool;
       while(curr != NULL && curr->base_frame_no < base_frame_no) {
         prev = curr;
         curr = curr->next;
      }
      if (prev == NULL) {
         this->next = curr;
         pool = this;
      } else {
        prev->next = this;
        this->next = curr;
      }
    }
    Console::puts("Cont Frame Pool initialized\n");
}

unsigned long ContFramePool::get_frames(unsigned int _n_frames)
{
    assert(n_free_frames >= _n_frames);
  
    // Variable "big" represents the current byte
    // Variable "small" represents the bit within the byte
    // Outer two while loops check for the first occurance of '1'
    // The first while loop continues from whereever the '1' was found
    // It looks for _n_frames number of ones.
    // If not found, we continue our check at the outer loop at the
    // position where we left off.
    unsigned int req = _n_frames;
    int i=0;
    while(i < n_frames) {
        int j=0;
        while (j < 8) {
            int num = (0x80 >> j);
            if ((bitmap[i] & (num)) != 0) {
               int big = i; int small = j;
               while (req > 0) {
                   if (big >= n_frames) {
                       return 0;
                   }
                   if (small == 8) {
                       big++; small = 0; continue;
                   }
                   int temp = (0x80 >> small);
                   if ((bitmap[big] & (temp)) != 0) {
                       req--; small++;
                       if (req == 0) {break;}
                   } else {
                      j = small; i = big; req = _n_frames;
                      break;
                   }       
               }
               if (req == 0) {
                   big = i; small = j; req = _n_frames;
                   while (req > 0) {
                       if (small == 8) {
                           big++; small = 0; continue;
                       }
                       int temp = (0x80 >> small);
                       bitmap[big] ^= temp; req--; small++;
                   }
                   n_free_frames -= _n_frames;
                   unsigned long ans_frame_no = base_frame_no + i*8 + j;
                   headmap[i] ^= num;
                   return ans_frame_no;
               }
            } else {
                j++;
            }
        }
        i++;
    }
    return 0;
}

void ContFramePool::mark_inaccessible(unsigned long _base_frame_no,
                                      unsigned long _n_frames)
{
    assert(_base_frame_no >= base_frame_no);
    assert(_base_frame_no + _n_frames <= base_frame_no + n_frames);

    // Please do not try to mark inaccessible a region that is already allocated.
    // This code will break for such cases.
    // I have handled it this way because the return type is void and there is no way for
    // letting the caller know if he was successful or not.
    // (of course, I am avoiding assertions and exceptions here)
    int big = (_base_frame_no-base_frame_no)/8; int small = (_base_frame_no-base_frame_no)%8;
    headmap[big] ^= (0x80 >> small);
    long req = _n_frames;
    while (req > 0) {
        if (small == 8) {
               big++; small = 0; continue;
         }
         int temp = (0x80 >> small);
         bitmap[big] ^= temp; req--; small++;
    }
    n_free_frames -= _n_frames;
}

// static
void ContFramePool::release_frames(unsigned long _first_frame_no)
{
    ContFramePool* temp = pool;
    while(temp != NULL && temp->base_frame_no+temp->n_frames <= _first_frame_no) {
      temp = temp->next;
    }
    
    if (temp == NULL) { assert(false); }
    
    if (temp ->base_frame_no > _first_frame_no) {
       // the given frame is not part of any framepools
       assert(false);
       return;
      }

    temp->rf(_first_frame_no);
}


// private
void ContFramePool::rf(unsigned long _base_frame_no) {
    // Variable "big" represents the byte number
    // Variable "small" represents the bit within the byte
    int big = (_base_frame_no-base_frame_no)/8; int small = (_base_frame_no-base_frame_no)%8;
    if ((headmap[big] & (0x80 >> small)) != 0) {
       // The given frame is not allocated. 
       assert(false); return;
    }

    // Unallocate the head
    headmap[big] ^= (0x80 >> small);
    
   unsigned long num_rel_frames = 0;
    while (true) {
        if (small == 8) {
               big++; small = 0; continue;
        }
        if (big >= n_frames) {
           // That's it. We have reached the last frame.
           break;
        }
        int temp = (0x80 >> small);
        int curr_val1 = (headmap[big] & (temp));
        int curr_val2 = (bitmap[big] & temp);
        if (curr_val1 == 0 || curr_val2 != 0) {
            // That's it. A new head or another free frame has been found.
            break;
        }
        bitmap[big] ^= temp; small++;
        num_rel_frames++;
    }
    n_free_frames += num_rel_frames;
}

unsigned long ContFramePool::needed_info_frames(unsigned long _n_frames)
{
    // In this implementation, we need 2 bits per frame.
    // Thus, number of bits = 2*_n_frames;
    // A frame contains FRAME_SIZE number of bytes..
    // .. FRAME_SIZE*8 number of bits.
    // So, number of frames needed = Ceiling((2*_n_frames)/FRAME_SIZE*8)
    long a = (_n_frames << 1);
    long b = (FRAME_SIZE << 3);

    return (a%b == 0) ? a/b : (a/b)+1;
}
