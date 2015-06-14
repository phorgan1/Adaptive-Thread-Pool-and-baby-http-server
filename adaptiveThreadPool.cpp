// copyright Patrick Horgan
// source is open, feel free to use it as you wish with no restrictions
// except that this copyright notice must be preserved intact
#include "adaptiveThreadPool.h"
#include <pthread.h>
#include <sys/socket.h>
#include <unistd.h>

// when we start a pool we start one job
adaptiveThreadPool::adaptiveThreadPool(void*(task)(void*),const int maxsize):
    task(task),maxsize(maxsize)
{
    sem_init(&sem,0,0);
    sem_init(&tids_sem,0,1);
    queueOne();
} 

void *
waitAndRun(void *voidatp)
{
    adaptiveThreadPool *atp=(adaptiveThreadPool*)voidatp;
    int sd;
    sem_post(&atp->sem);	    // we have one more thread
    while(true){
	try{
	    sd=atp->jq.wait_and_pop();  // get a socket descriptor
	    if(sd==-1){		    // didn't get one
		if(errno==EINTR){
		    // this is how we tell a thread to die
		    pthread_exit(0);
		}
	    }
	    atp->task(&sd);
	    shutdown(sd,SHUT_RDWR);
	    close(sd);
	}catch(std::bad_alloc ba){
	    std::cerr << "waitAndRun caught a bad_alloc() running - " << ba.what() << '\n';
	}
	// here's where we can check if more threads are needed
	try{
	    int numthreads;
	    sem_getvalue(&atp->sem,&numthreads);
	    if((atp->jq.num_jobs() > numthreads) &&
		    (static_cast<size_t>(numthreads) < atp->maxsize)){
		atp->queueOne();
	    }
	}catch(std::bad_alloc ba){
	    std::cerr << "waitAndRun caught a bad_alloc() starting a new thread - " << ba.what() << '\n';
	}
    }
}

void
adaptiveThreadPool::queueOne()
{
    pthread_t tid;
    pthread_attr_t theattr;
    sem_wait(&tids_sem);
    pthread_attr_init(&theattr);
    pthread_attr_setdetachstate(&theattr,PTHREAD_CREATE_DETACHED);

    pthread_create(&tid,&theattr,waitAndRun,this);
    tids.push_back(tid);
    sem_post(&tids_sem);
    return;
}

void
adaptiveThreadPool::killAll()
{
    // we'll get it, but not post it because no one else gets to run after
    // this.  The thread system is dead
    sem_wait(&tids_sem);
    for(size_t ctr=0;ctr<tids.size();ctr++){
	pthread_cancel(tids[ctr]);
    }
}


