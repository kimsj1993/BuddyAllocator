#include "SafeBuffer.h"
#include <string>
#include <queue>
#include <pthread.h>
#include <iostream>


using namespace std;

SafeBuffer::SafeBuffer() {
  pthread_mutex_init(&mutex, NULL);

}

SafeBuffer::~SafeBuffer() {
	
}

int SafeBuffer::size() {
  int s;
  
  pthread_mutex_lock(&mutex);
  s = q.size();
  pthread_mutex_unlock(&mutex);
  return s;
	/*
	Is this function thread-safe???
	Make necessary modifications to make it thread-safe
	*/
}

void SafeBuffer::push(string str) {
	/*
	Is this function thread-safe???
	Make necessary modifications to make it thread-safe
	*/
  
  
  pthread_mutex_lock(&mutex);
	q.push (str);
  pthread_mutex_unlock(&mutex);

}

string SafeBuffer::pop() {
	/*
	Is this function thread-safe???
	Make necessary modifications to make it thread-safe
	*/
  
  string s;
  pthread_mutex_lock(&mutex);
  if(q.empty()){
    s = "";
  }
  else {
    s = q.front();
    q.pop();
  }
  
  pthread_mutex_unlock(&mutex);
	return s;
}
