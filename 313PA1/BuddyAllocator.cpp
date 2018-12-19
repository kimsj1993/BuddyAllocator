#include "BuddyAllocator.h"
#include <iostream>
#include <math.h>
using namespace std;

BlockHeader::BlockHeader(uint size)
{
    this->size = size;
    this->free = false;
    next = NULL;
}

uint BlockHeader::getSize()
{
    return size;
}

void BlockHeader::setSize(uint size)
{
    this->size = size;
}

bool BlockHeader::isFree()
{
    return free;
}

void BlockHeader::setFree(bool free)
{
    this->free = free;
}


LinkedList::LinkedList()
{
    head = NULL;
    size = 0;
}

void LinkedList::insert(BlockHeader* b)
{
    if(head == NULL)
    {
        head = b;
    }
    else
    {
        b->next  = head;
        head = b;
    }
    this->size++;
}

void LinkedList::remove(BlockHeader* b)
{
    BlockHeader* current = head;
    BlockHeader* previous = NULL;
    while (current->next != NULL)
    {
        if(current == b)
        {
            previous->next = current->next;
            this->size--;

            return;
        }
        previous = current;
        current = current->next;
    }
}

BlockHeader* LinkedList::findBlock(BlockHeader *b)
{

}


bool LinkedList::isEmpty()
{
    return size == 0 ? true : false;
}


char* BuddyAllocator::getbuddy(char *addr) {

    char* p;
    return p;
}

bool BuddyAllocator::isvalid(char *addr) {
    return false;
}

bool BuddyAllocator::arebuddies(char *block1, char *block2) {
    return false;
}

char* BuddyAllocator::merge(char *block1, char *block2) {

    char* p;
    return p;
}

char* BuddyAllocator::split(char *block) {


    BlockHeader* header = (BlockHeader*) block;
    header->setSize(header->getSize()/2);


    // remove header
    char* p;
    return p;
}


BuddyAllocator::BuddyAllocator (uint _basic_block_size, uint _total_memory_length){

    try{
        if(_total_memory_length % basicBlockSize != 0)
        {
            throw 10;
        }
    }
    catch (int e)
    {
        cout << "you just requested which is not a multiple of the basic block size";
    }

    this->totalMemoryLength = _total_memory_length;
    this->basicBlockSize = _basic_block_size;
    this->freeListSize = _total_memory_length;


    this->headerPtr = (char*) malloc(_total_memory_length);

    //initialize the header
    BlockHeader* head = (BlockHeader*)headerPtr;
    head->setSize(totalMemoryLength);
    head->setFree(false);



    LinkedList linkedList;
    freeList.push_back(linkedList);
    freeList[0].insert((BlockHeader*) headerPtr);


}

BuddyAllocator::~BuddyAllocator ()
{
}


int nextPower_2 (uint x) {

    int counter = 0;

    while (x > 0) {

        counter++;

        x = x >> 1;

    }
    return (1 << counter);
}

char* BuddyAllocator::alloc(uint _length)
{
    /* This preliminary implementation simply hands the call over the
       the C standard library!
       Of course this needs to be replaced by your implementation.
    */
    BlockHeader* head = (BlockHeader*)headerPtr;

    _length = nextPower_2(_length);

    if(freeListSize < _length) {
        return 0;
    }

    int index = 0;
    while((_length + freeListSize)/ (basicBlockSize * pow(2, index))>= 1)
    {
        index ++;
    }


        return new char [_length];
}


/*

int BuddyAllocator::free(char* _a) {
    delete _a;
    return 0;
}

void BuddyAllocator::debug (){

}
*/