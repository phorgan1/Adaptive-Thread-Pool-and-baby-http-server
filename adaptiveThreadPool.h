// copyright Patrick Horgan
// source is open, feel free to use it as you wish with no restrictions
// except that this copyright notice must be preserved intact
#ifndef adaptiveThreadPool_guard
#define adaptiveThreadPool_guard
#include "jobQueue.h"
#include <pthread.h>
#include <vector>

class adaptiveThreadPool
{
public:
    friend void* waitAndRun(void *); // method doesn't have right sig for thread
    adaptiveThreadPool(void*(task)(void*),const int maxsize=20); 
    void
    addjob(int fd){ jq.push(fd); /* std::cerr << "aj+\n";*/ };
    void
    killAll();
private:
    adaptiveThreadPool();
    adaptiveThreadPool(const adaptiveThreadPool&);
    void queueOne(void);
    const adaptiveThreadPool& operator=(const adaptiveThreadPool&);
    class jobQueue<int> jq;
    void *(*task)(void *);
    sem_t sem;			// count threads
    std::vector<pthread_t> tids;
    sem_t tids_sem;
    size_t maxsize;
};
#endif
