// copyright Patrick Horgan
// source is open, feel free to use it as you wish with no restrictions
// except that this copyright notice must be preserved intact
#ifndef jobQueue_guard
#define jobQueue_guard
#include <pthread.h>
#include <semaphore.h>
#include <queue>
#include <errno.h>
#include <iostream>

class jobQueue
{
public:
    jobQueue(){
	pthread_mutex_init(&lock,NULL);
	sem_init(&sem,0,0);
    };

    void
    push(int sd){
	pthread_mutex_lock(&lock);  // got the lock
	jobs.push(sd);		    // now add the job
	sem_post(&sem);		    // post so that the threads know
	pthread_mutex_unlock(&lock);// unleash the horses
    }
    int num_jobs(){ return jobs.size(); }

    int
    pop(){
	// returns -1 if there's nothing to pop
	int sd,retval;
	pthread_mutex_lock(&lock);
	if(jobs.empty()){
	    sd=-1;
	}else{
	    // we think there's a job, but wait_and_pop() could have 
	    // grabbed the semaphore and be waiting for us to release the
	    // mutex.  So check sem_trywait and if it fails return -1
	    retval=sem_trywait(&sem);
	    if(retval==-1){
		// we aren't getting it, someone else already has it
		// one of
		// EINTR - this could have happened on purpose.  This is how
		//         we tell a thread to die
		// EAGAIN - only for sem_trywait - this could be it
		// return -1 so we harm none.
		sd=-1;
	    }else{
		sd=jobs.front();
		jobs.pop();
	    }
	}
	pthread_mutex_unlock(&lock);
	return sd;
    }

    int
    wait_and_pop()
    {
	// will not return until it can return to us a socket descriptor
	int sd,retval;
	retval=sem_wait(&sem);
	if(retval==-1){
	    // one of
	    // EINTR - this could have happened on purpose.  This is how
	    //         we tell a thread to die
	    // if it does, return -1 so we harm none.
	    return -1;
	}
	pthread_mutex_lock(&lock);
	sd=jobs.front();
	jobs.pop();
	pthread_mutex_unlock(&lock);
	return sd;
    }
private:
    pthread_mutex_t lock;
    sem_t sem;
    std::queue<int> jobs;
};
#endif
