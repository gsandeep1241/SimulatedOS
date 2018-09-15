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

struct Node
{
  public:
    Node* next;
    unsigned long base;
    unsigned long n_frames;
    unsigned long info_frame_no;
    unsigned long n_info_frames;
};

static Node* node = NULL;

ContFramePool::ContFramePool(unsigned long _base_frame_no,
                             unsigned long _n_frames,
                             unsigned long _info_frame_no,
                             unsigned long _n_info_frames)
{
    base_frame_no = _base_frame_no;
    n_frames = _n_frames;
    info_frame_no = _info_frame_no;
    n_info_frames = _n_info_frames;
    n_free_frames = n_frames;

    if (info_frame_no == 0) {
        bitmap = (unsigned char *) (base_frame_no * FRAME_SIZE);
    } else {
        bitmap = (unsigned char *) (info_frame_no * FRAME_SIZE);
    }
    // Number of frames must be "fill" the bitmap!
    assert ((n_frames % 8 ) == 0);
    
    
    // Everything ok. Proceed to mark all bits in the bitmap
    for(int i=0; i*8 < n_frames; i++) {
        bitmap[i] = 0xFF;
    }

    if (info_frame_no == 0) {
        unsigned long remaining_info_frames = n_info_frames;
        int counter = 0;
        while (remaining_info_frames > 0) {
            for (int i=0; i < 8; i++) {
                if (remaining_info_frames == 0) {break;}
                bitmap[counter] = (0x7F >> i);
            }
            remaining_info_frames = remaining_info_frames - 8;
            counter++;
        }
    }
    n_free_frames -= n_info_frames;

    Node* new_node;
    new_node->base = base_frame_no;
    new_node->n_frames = n_frames;
    new_node->info_frame_no = info_frame_no;
    new_node->n_info_frames = n_info_frames;
    new_node->next = NULL;    

    if (node == NULL) {
        node = new_node;
    } else {
      Node* prev = NULL; Node* curr = node;
      while(curr != NULL && curr->base < new_node->base) {
         prev = curr;
         curr = curr->next;
      }
      prev->next = new_node;
      new_node->next = curr;
    }
    Console::puts("Cont Frame Pool initialized\n");
}

unsigned long ContFramePool::get_frames(unsigned int _n_frames)
{
    assert(n_free_frames >= _n_frames);
   
    unsigned int req = _n_frames;
    for (int i=0; i < n_frames; i++) {
        for (int j=0; j < 8; j++) {
            int num = (0x80 >> j);
            if ((bitmap[i] & (num)) != 0) {
               int big = i; int small = j;
               while (req > 0) {
                   if (small == 8) {
                       big++; small = 0; continue;
                   }
                   int temp = (0x80 >> small);
                   if ((bitmap[big] & (temp) != 0)) {
                       req--; small++;
                   } else {
                      break;
                   }       
               }
               if (req == 0) {
                   big = i; small = j;
                   while (req > 0) {
                       if (small == 8) {
                           big++; small = 0; continue;
                       }
                       int temp = (0x80 >> small);
                       bitmap[big] ^= temp; req--; small++;
                   }
                   n_free_frames -= _n_frames;
                   unsigned long ans_frame_no = base_frame_no + i*8 + j;
                   return ans_frame_no;
               }
            }
        }
    }
    return 0;
}

void ContFramePool::mark_inaccessible(unsigned long _base_frame_no,
                                      unsigned long _n_frames)
{
    int big = (_base_frame_no-base_frame_no)/8; int small = (_base_frame_no-base_frame_no)%8;
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

void ContFramePool::release_frames(unsigned long _first_frame_no)
{
    Node* temp = node;
    while(temp != NULL && temp->base+temp->n_frames < _first_frame_no) {
      temp = temp->next;
    }

    ContFramePool pool(temp->base, temp->n_frames, temp->info_frame_no, temp->n_info_frames); 
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
