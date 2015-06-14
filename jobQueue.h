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

class
job_queue_empty: public std::exception
{
public:
    job_queue_empty(){};
    virtual ~job_queue_empty() throw() {};
};
class
jq_semaphore_unavailable: public std::exception
{
public:
    jq_semaphore_unavailable(){};
    virtual ~jq_semaphore_unavailable() throw() {};
};

template<typename T>
class jobQueue
{
public:
    jobQueue(){
	pthread_mutex_init(&lock,NULL);
	sem_init(&sem,0,0);
    };

    void
    push(T job){
	pthread_mutex_lock(&lock);  // got the lock
	jobs.push(job);		    // now add the job
	sem_post(&sem);		    // post so that the threads know
	pthread_mutex_unlock(&lock);// unleash the horses
    }

    size_t size() { return jobs.size(); }
    int num_jobs(){ return jobs.size(); }

    T
    pop(){
	// throws job_queue_empty if there's nothing to pop
	T job;
	int retval;
	pthread_mutex_lock(&lock);
	if(jobs.empty()){
	    // we need to throw here as well
	    //sd=-1;
	    pthread_mutex_unlock(&lock);
	    throw job_queue_empty();
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
		pthread_mutex_unlock(&lock);
		throw jq_semaphore_unavailable();
	    }else{
		// semantics of std::queue require front to get a copy, and
		// pop to get the original off.  If you do either one without
		// have anything in the queue you've entered undefined behavior
		// luckily, there's something, or we couldn't have got the 
		// semaphore.  push() locks the lock, pushes a job on the 
		// queue (really a file descriptor from a socket), posts
		// the semaphore and then unlocks the lock.  That means we
		// could not have gotten in here in the middle since we need
		// the lock to get into pop()
		job=jobs.front();
		jobs.pop();
	    }
	}
	pthread_mutex_unlock(&lock);
	return job;
    }

    T
    wait_and_pop()
    {
	// will not return until it can return to us a socket descriptor

	T job;
	int retval;
	retval=sem_wait(&sem);
	if(retval==-1){
	    // one of
	    // EINTR - this could have happened on purpose.  This is how
	    //         we tell a thread to die
	    // if it does, return -1 so we harm none.
	    // we'll have to throw here.
	    throw job_queue_empty();
	}
	pthread_mutex_lock(&lock);
	job=jobs.front();
	jobs.pop();
	pthread_mutex_unlock(&lock);
	return job;
    }
private:
    pthread_mutex_t lock;
    sem_t sem;
    std::queue<T> jobs;
};
#endif
