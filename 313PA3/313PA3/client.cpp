/*
    Based on original assignment by: Dr. R. Bettati, PhD
    Department of Computer Science
    Texas A&M University
    Date  : 2013/01/31
 */


#include <iostream>
#include <fstream>
#include <cstring>
#include <string>
#include <sstream>
#include <iomanip>

#include <sys/time.h>
#include <cassert>
#include <assert.h>

#include <cmath>
#include <numeric>
#include <algorithm>

#include <list>
#include <vector>

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>

#include "reqchannel.h"
#include "SafeBuffer.h"
#include "Histogram.h"

#include <cstdio>
#include <ctime>

using namespace std;

struct Thread_data
{
  int n;
  string data;
  SafeBuffer* buf;
};

struct Working_thread_data
{
  int n;
  int w;
  int id;
  SafeBuffer* buf;
  RequestChannel* chan;
  Histogram* hist;
  
};


void* request_thread_function(void* arg) {
	/*
		Fill in this function.

		The loop body should require only a single line of code.
		The loop conditions should be somewhat intuitive.

		In both thread functions, the arg parameter
		will be used to pass parameters to the function.
		One of the parameters for the request thread
		function MUST be the name of the "patient" for whom
		the data requests are being pushed: you MAY NOT
		create 3 copies of this function, one for each "patient".
	 */
  
  Thread_data *data = (Thread_data *) arg;
  
  for(int i = 0; i < data->n; i++) {
    data->buf->push(data->data);

  }
  data->buf->push("quit");
  
}

void* worker_thread_function(void* arg) {
    /*
		Fill in this function. 

		Make sure it terminates only when, and not before,
		all the requests have been processed.

		Each thread must have its own dedicated
		RequestChannel. Make sure that if you
		construct a RequestChannel (or any object)
		using "new" that you "delete" it properly,
		and that you send a "quit" request for every
		RequestChannel you construct regardless of
		whether you used "new" for it.
     */
  
  Working_thread_data *data = (Working_thread_data *) arg;

  
  int index_for_quit = 0;
  int count_for_quit;
  if(data->w >= 3)
    count_for_quit = 1;
  else
    count_for_quit = 3;

    while(true) {
      string request = data->buf->pop();
      
      if(request.length()  < 1) {
        request = "quit";
      }
      
      if(request == "quit") {
        index_for_quit++;
        
        if(index_for_quit >= count_for_quit) {
          data->chan->cwrite(request);
          delete data->chan;
          break;
        }
      }
      
      else {
        data->chan->cwrite(request);
        string response = data->chan->cread();


        data->hist->update(request, response);
      }
    }
  
}

/*--------------------------------------------------------------------------*/
/* MAIN FUNCTION */
/*--------------------------------------------------------------------------*/

int main(int argc, char * argv[]) {
    int n = 100; //default number of requests per "patient"
    int w = 1; //default number of worker threads
    int opt = 0;
    while ((opt = getopt(argc, argv, "n:w:")) != -1) {
        switch (opt) {
            case 'n':
                n = atoi(optarg);
                break;
            case 'w':
                w = atoi(optarg); //This won't do a whole lot until you fill in the worker thread function
                break;
			}
    }

    int pid = fork();
	if (pid == 0){
		execl("dataserver", (char*) NULL);
	}
	else {

        cout << "n == " << n << endl;
        cout << "w == " << w << endl;

        cout << "CLIENT STARTED:" << endl;
        cout << "Establishing control channel... " << flush;
        RequestChannel *chan = new RequestChannel("control", RequestChannel::CLIENT_SIDE);
        cout << "done." << endl<< flush;

		SafeBuffer request_buffer;
		Histogram hist;
    

    
    pthread_t threads[3];
    Thread_data args[3];
    
    for(int i = 0; i < 3; i++)
    {
      args[i].n = n;
      args[i].buf = &request_buffer;
    }
    
    args[0].data = "data John Smith";
    args[1].data = "data Jane Smith";
    args[2].data = "data Joe Smith";
    
    for(int i=0; i < 3; i++)
    {
      pthread_create(&threads[i], NULL, request_thread_function, (void *) &args[i]);
    }
    auto begin = std::chrono::steady_clock::now();

    
    pthread_t working_threads[w];
    Working_thread_data working_thread_args[w];
    
    for (int i = 0; i < w; i++) {
      chan->cwrite("newchannel");
      
      string value = chan->cread();
      
      working_thread_args[i].n = n;
      working_thread_args[i].w = w;
      working_thread_args[i].id = i;
      working_thread_args[i].buf = &request_buffer;
      working_thread_args[i].chan = new RequestChannel(value, RequestChannel::CLIENT_SIDE);
      working_thread_args[i].hist = &hist;
      
      pthread_create(&working_threads[i], NULL, worker_thread_function, &working_thread_args[i]);
      
    }
    
    for (int i=0 ; i < 3; i++)
    {
      pthread_join(threads[i], NULL);
    }
    
    for (int i=0; i < w; i++)
    {
      pthread_join(working_threads[i], NULL);
    }

        chan->cwrite ("quit");
        delete chan;
        cout << "All Done!!!" << endl; 

		hist.print();
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - begin);
    std::cout << "time taken: " << elapsed .count() << " milliseconds" << std::endl;
    
    
    }
}
