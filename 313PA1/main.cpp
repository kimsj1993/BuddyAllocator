#include <iostream>
#include "BuddyAllocator.h"

int main() {
    std::cout << "Hello, World!" << std::endl;
    BuddyAllocator buddyAllocator(128, 1024);
    buddyAllocator.alloc(128);
    return 0;
}