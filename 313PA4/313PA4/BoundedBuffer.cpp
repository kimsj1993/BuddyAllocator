#include "BoundedBuffer.h"
#include <string>
#include <queue>
#include <pthread.h>
#include <iostream>


using namespace std;

BoundedBuffer::BoundedBuffer(int _cap) {
	pthread_mutex_init(&mutex, NULL);
	pthread_cond_init(&until_not_empty, NULL);
	pthread_cond_init(&until_not_full, NULL);
  capacity = _cap;
  count = 0;

}

BoundedBuffer::~BoundedBuffer() {
	
}

int BoundedBuffer::size() {
  int s;
  
  pthread_mutex_lock(&mutex);
  s = q.size();
  pthread_mutex_unlock(&mutex);
  
  return s;
}

void BoundedBuffer::push(string str) {
	/*
	Is this function thread-safe??? Does this automatically wait for the pop() to make room 
	when the buffer if full to capacity???
	*/
  pthread_mutex_lock(&mutex);
  while( count >= capacity){
    pthread_cond_wait(&until_not_full, &mutex);
  }

	q.push (str);
  count ++;
  pthread_cond_signal(&until_not_empty);
  pthread_mutex_unlock(&mutex);

}

string BoundedBuffer::pop() {
	/*
	Is this function thread-safe??? Does this automatically wait for the push() to make data available???
	*/
  pthread_mutex_lock(&mutex);
  while( count <= 0){
    pthread_cond_wait(&until_not_empty, &mutex);
  }
  
	string s = q.front();
	q.pop();
  count --;
  
  pthread_cond_signal(&until_not_full);
  pthread_mutex_unlock(&mutex);
  
	return s;
}
