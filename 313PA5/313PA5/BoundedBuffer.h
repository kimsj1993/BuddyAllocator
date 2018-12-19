#ifndef BoundedBuffer_h
#define BoundedBuffer_h

#include <stdio.h>
#include <queue>
#include <string>
#include <pthread.h>
#include <iostream>



using namespace std;

class BoundedBuffer {
private:
	queue<string> q;
	pthread_mutex_t mutex;
	pthread_cond_t until_not_full, until_not_empty;
  int capacity;
  int count;
public:
    BoundedBuffer(int);
	~BoundedBuffer();
	int size();
    void push (string);
    string pop();
};

#endif /* BoundedBuffer_ */
