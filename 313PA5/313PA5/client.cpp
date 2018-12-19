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
#include "BoundedBuffer.h"
#include "Histogram.h"
#include <math.h>

#include <ctime>


using namespace std;

struct Thread_arg
{
  int counter;
  string data;
  BoundedBuffer* request_buffer;
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
  
  Thread_arg *args = (Thread_arg *) arg;
  
  for(int i = 0; i < args->counter; i++) {
    args->request_buffer->push(args->data);
    
  }
  
}

struct Stat_thread_arg
{
  string data;
  Histogram* hist;
  BoundedBuffer* res_buffer;
  int counter;
};


Histogram hist;

void sigalarm_handler(int)
{
  hist.print();
  alarm(2);
}



struct Event_handler_thread_arg
{
  RequestChannel* chan;
  BoundedBuffer *request_buffer, *john_res_buffer, *jane_res_buffer, *joe_res_buffer;
  int w;
  int n;
};





void* event_handler_thread_function(void* arg) {
  
  Event_handler_thread_arg *args = (Event_handler_thread_arg *) arg;
  int num_chan = args->w;
  int n = args->n;
  
  string requested[num_chan];
  int s_counter = 0;
  int r_counter = 0;
  RequestChannel* channels[num_chan];
  
  
  //wrint channel
  for (int i = 0; i < num_chan; i++)
  {
    args->chan->cwrite("newchannel");
    string name = args->chan->cread();
    channels[i] = new RequestChannel (name, RequestChannel::CLIENT_SIDE);
    requested[i] = args->request_buffer->pop();
    channels[i]->cwrite(requested[i]);
    s_counter++;
  }
  
  fd_set readset, backup;
  int max_fd = 0;
  for (int i=0; i<num_chan; i++)
  {
    if(max_fd < channels[i]->read_fd()) max_fd = channels[i]->read_fd();
    FD_SET (channels[i] -> read_fd(), &readset);
  }
  
  backup = readset;
  
  while(r_counter < 3 * n) {
    readset = backup;
    int k = select(max_fd+1, &readset, NULL, NULL, NULL);
    
    for (int i = 0; i < num_chan; i++)
    {
      if (FD_ISSET(channels[i]->read_fd(), &readset))
      {
        string res = channels[i]->cread();
        string req = requested[i];
        r_counter ++;
        
        if(req == "data John Smith")
        {
          args->john_res_buffer->push(res);
        }
        else if (req == "data Jane Smith")
        {
          args->jane_res_buffer->push(res);
          
        }
        else if (req == "data Joe Smith")
        {
          args->joe_res_buffer->push(res);
        }
        
        if (s_counter < 3 * n)
        {
          string more_req = args->request_buffer->pop();
          requested[i] = more_req;
          channels[i]->cwrite(more_req);
          s_counter ++;
          
        }
        
      }
    }
  }
  for (int  i = 0; i < num_chan; i++)
  {
    channels[i]->cwrite("quit");
    delete channels[i];
  }
}
void* stat_thread_function(void* arg) {
    /*
		Fill in this function. 

		There should 1 such thread for each person. Each stat thread 
        must consume from the respective statistics buffer and update
        the histogram. Since a thread only works on its own part of 
        histogram, does the Histogram class need to be thread-safe????

     */
  Stat_thread_arg *args = (Stat_thread_arg *) arg;
  string req = args->data;
  
  
  for(int i =0; i< args->counter; i++) {
    string res = args->res_buffer->pop();
    if(res == "quit")
      break;
    else
      args->hist->update(req, res);
    }
}


/*--------------------------------------------------------------------------*/
/* MAIN FUNCTION */
/*--------------------------------------------------------------------------*/

int main(int argc, char * argv[]) {
    int n = 100; //default number of requests per "patient"
    int w = 1; //default number of worker threads
    int b = 3 * n; // default capacity of the request buffer, you should change this default
    int opt = 0;
    while ((opt = getopt(argc, argv, "n:b:w:")) != -1) {
        switch (opt) {
            case 'n':
                n = atoi(optarg);
                break;
            case 'b':
                b = atoi(optarg); //This won't do a whole lot until you fill in the worker thread function
                break;
            case 'w':
                w = atoi (optarg);
                break;
		}
    }

    int pid = fork();
	if (pid == 0){
		execl("dataserver", (char*) NULL);
	}
	else {

        cout << "n == " << n << endl;
        cout << "b == " << b << endl;
        cout << "w == " << w << endl;

        RequestChannel *chan = new RequestChannel("control", RequestChannel::CLIENT_SIDE);
        BoundedBuffer request_buffer(b);
        //Histogram hist;
    

    
    
    
      //creating request threads
        pthread_t request_threads[3];
        Thread_arg request_thread_args[3];
    
        for(int i = 0; i < 3; i++)
        {
          request_thread_args[i].counter = n;
          request_thread_args[i].request_buffer = &request_buffer;
        }
    
        request_thread_args[0].data = "data John Smith";
        request_thread_args[1].data = "data Jane Smith";
        request_thread_args[2].data = "data Joe Smith";
    
    auto begin = std::chrono::steady_clock::now();

    
        for(int i=0; i < 3; i++)
        {
          pthread_create(&request_threads[i], NULL, request_thread_function, (void *) &request_thread_args[i]);
        }
        cout << "Done populating request buffer" << endl;

    

    //res_bufers for each person
        BoundedBuffer john_res_buffer(b);
        BoundedBuffer jane_res_buffer(b);
        BoundedBuffer joe_res_buffer(b);
    
        pthread_t stat_threads[3];
        Stat_thread_arg stat_thread_args[3];
    
        for(int i = 0; i < 3; i++)
        {
          stat_thread_args[i].counter = n;
          stat_thread_args[i].hist = &hist;
        }
        stat_thread_args[0].data = "data John Smith";
        stat_thread_args[1].data = "data Jane Smith";
        stat_thread_args[2].data = "data Joe Smith";
        stat_thread_args[0].res_buffer = &john_res_buffer;
        stat_thread_args[1].res_buffer = &jane_res_buffer;
        stat_thread_args[2].res_buffer = &joe_res_buffer;
    
        for(int i=0; i < 3; i++)
        {
          pthread_create(&stat_threads[i], NULL, stat_thread_function, (void *) &stat_thread_args[i]);
        }
    
    cout<< "Done populating stat treahds" <<endl;

    
    pthread_t EH_thread;
    Event_handler_thread_arg EH_thread_args;
    
    EH_thread_args.w = w;
    EH_thread_args.n = n;
    EH_thread_args.chan = chan;
    EH_thread_args.request_buffer= &request_buffer;
    EH_thread_args.john_res_buffer = &john_res_buffer;
    EH_thread_args.jane_res_buffer = &jane_res_buffer;
    EH_thread_args.joe_res_buffer = &joe_res_buffer;

    
    pthread_create(&EH_thread, NULL, event_handler_thread_function,(void *) &EH_thread_args);
    
    signal(SIGALRM, sigalarm_handler);
    alarm(2);
    
    cout << "Done populating EH_thread" << endl;
    
    
    pthread_join(request_threads[0], NULL);
    pthread_join(request_threads[1], NULL);
    pthread_join(request_threads[2], NULL);
    
    cout << "Pushing quit request to the event handler thread... ";
    request_buffer.push("quit");
    cout << "requeest done";
    
    pthread_join (EH_thread, NULL);
    
    cout << "eh EH DONE " << endl;
    
    john_res_buffer.push("quit");
    jane_res_buffer.push("quit");
    joe_res_buffer.push("quit");
    
    
    cout << "Pushing quit requests to stat threads... ";
    pthread_join(stat_threads[0], NULL);
    pthread_join(stat_threads[1], NULL);
    pthread_join(stat_threads[2], NULL);
    
    
        chan->cwrite ("quit");
        delete chan;
        cout << "All Done!!!" << endl; 

		hist.print ();
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - begin);
    std::cout << "time taken: " << elapsed .count() << " milliseconds" << std::endl;
    }
}
