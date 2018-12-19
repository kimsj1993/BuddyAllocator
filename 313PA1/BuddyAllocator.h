/*
    File: my_allocator.h

    Original Author: R.Bettati
            Department of Computer Science
            Texas A&M University
    Date  : 08/02/08

    Modified: Seungjin Kim

*/

#ifndef _BuddyAllocator_h_                   // include file only once
#define _BuddyAllocator_h_
#include <iostream>
#include <vector>
using namespace std;
typedef unsigned int uint;

/* declare types as you need */

class BlockHeader{
    // decide what goes here
    // hint: obviously block size will go here
    public:

    BlockHeader* next;

    BlockHeader(uint size);

    void setSize(uint size);

    uint getSize();

    bool isFree();

    void setFree(bool status);


    private:
            uint size;
            bool free;  //reserved 1 : free 0




};

class LinkedList{
    // this is a special linked list that is made out of BlockHeader s.
    private:
        //for next;
        BlockHeader* head;		// you need a head of the list
        int size;
    public:
        void insert (BlockHeader* b); // adds a block to the list


        void remove (BlockHeader* b);  // removes a block from the list

        LinkedList();

        BlockHeader* findBlock(BlockHeader* b);

        bool isEmpty();

};


class BuddyAllocator{
private:
    uint basicBlockSize;
    uint totalMemoryLength;
    char* headerPtr;
    vector<LinkedList> freeList;
    int freeListSize;


    /* declare member variables as necessary */

private:
    /* private function you are required to implement
     this will allow you and us to do unit test */

    char* getbuddy (char * addr);
    // given a block address, this function returns the address of its buddy

    bool isvalid (char *addr);
    // Is the memory starting at addr is a valid block
    // This is used to verify whether the a computed block address is actually correct

    bool arebuddies (char* block1, char* block2);
    // checks whether the two blocks are buddies are not

    char* merge (char* block1, char* block2);
    // this function merges the two blocks returns the beginning address of the merged block
    // note that either block1 can be to the left of block2, or the other way around

    char* split (char* block);
    // splits the given block by putting a new header halfway through the block
    // also, the original header needs to be corrected


public:
    BuddyAllocator (uint _basic_block_size, uint _total_memory_length);
    /* This initializes the memory allocator and makes a portion of
       â€™_total_memory_lengthâ€™ bytes available. The allocator uses a â€™_basic_block_sizeâ€™ as
       its minimal unit of allocation. The function returns the amount of
       memory made available to the allocator. If an error occurred,
       it returns 0.
    */

    ~BuddyAllocator();
    /* Destructor that returns any allocated memory back to the operating system.
       There should not be any memory leakage (i.e., memory staying allocated).
    */

    char* alloc(uint _length);
    /* Allocate _length number of bytes of free memory and returns the
        address of the allocated portion. Returns 0 when out of memory. */
    /* determine block size needed
     * do we have the block that size?
     * split(block)
     * return a ptr to data section of block
     * return (char*)block+

    int free(char* _a);
    /* Frees the section of physical memory previously allocated
       using â€™my_mallocâ€™. Returns 0 if everything ok. */

    /* get actual block ptr
     * get block's buddy
     * check if buddy is also fre
     * if yes then merge()
     * or just insert block into free_list

    void debug ();
    /* Mainly used for debugging purposes and running short test cases */
    /* This function should print how many free blocks of each size belong to the allocator
    at that point. The output format should be the following (assuming basic block size = 128 bytes):

    128: 5
    256: 0
    512: 3
    1024: 0
    ....
    ....
     which means that at point, the allocator has 5 128 byte blocks, 3 512 byte blocks and so on.*/
};



#endif