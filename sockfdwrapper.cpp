// copyright Patrick Horgan
// source is open, feel free to use it as you wish with no restrictions
// except that this copyright notice must be preserved intact
#include "sockfdwrapper.h"
#include <errno.h>
#include <iostream>
#include <strings.h>
#include <unistd.h>

sockfdwrapper::sockfdwrapper(int i):fd(i),valid(true),open(true),epoll_fd(0)
{
    // get our epoll_fd to monitor the socket
    if((epoll_fd=epoll_create1(0))==-1){
	valid=false;
	return;
    }
    bzero(&ev,sizeof(ev));
    bzero(rawbuffer,RECV_BUF_SIZ);
    ev.events=EPOLLIN;	// level trigger with non-blocking socket
    // tell it we're monitoring the socket
    ev.data.fd=fd;
    if(epoll_ctl(epoll_fd,EPOLL_CTL_ADD,fd,&ev)==-1){
	valid=false;
	return;
    }
    // initialize our markers
    begin=cur=end=rawbuffer;
}

sockfdwrapper::~sockfdwrapper()
{
    if(epoll_fd){
	// if we had an opened socket try to close it.  It can fail, and
	// if so would return -1 and set errno.  We don't check it because
	// we're destroying the socket wrapper and no longer care about the
	// state of the socket
	::close(epoll_fd);
    }
}

/**
 getline(char *buffer,size_t len)
 reads in at most one less that len characters.  Reading stops when we can't
 get any more characters, or when we find \n.  If a \n is found, then it is
 stored in the buffer, and in any case a \0 terminates the returned characters
 
 buffer - place to store the characters
 len    - size of the buffer

 return value - the buffer, or if the call fails, returns NULL (0).
 */
char*
sockfdwrapper::getline(char *buffer,size_t len)
{
    size_t cnt=0,retval;
    char *bufptr=buffer;

    if(cur==end){
	// if the buffer is empty we call this once.  That means that
	// if a user calls us they need to check and see if the last
	// character is '\n' of the returned buffer and if so, try to
	// call again.
	if(valid && open){
	    if((retval=getbytes())==0 or !valid or !open){
		std::cerr << "getbytes: " << retval << ", valid: " << valid << ", open: " << open << "\n";
		return NULL;
	    }
	}else{
	    std::cerr << "Not valid and open, valid: " << valid << ", open: " << open << "\n";
	    return NULL;
	}
    }
    while(*cur!='\n' && cur<end && cnt<len-1){
	if(cur!=end){
	    *bufptr++=*cur++;
	    cnt++;
	}else{
	    break;
	}
    }
    if(*cur=='\n' && cur<end && cnt<len-1){
	*bufptr++=*cur++;
    }
    *bufptr='\0';
    return buffer;
}

/** 
 * getbytes() - fill the buffer up if possible
 * There's a timeout waiting for input.  If there's none, then
 * returns - the number of available bytes
 */

ssize_t
sockfdwrapper::getbytes()
{
    ssize_t num_events;

    if(!valid || !open){
	return end-cur;
    }
    if(cur==end){
	// if you've consumed all the bytes, we just move the curpointer and
	// end to the beginning of the buffer to make room to read more.
	cur=end=begin;
    }else if(cur!=begin){
	// if you call getbytes() before consuming all the bytes in the buffer,
	// then the existing bytes get copied to the beginning of the buffer
	// before we try to get more.  It's more efficient to consume all of
	// the bytes to avoid this move
	size_t numbytes=end-cur;
	memmove(rawbuffer,cur,numbytes);
	cur=begin;
	end=begin+numbytes;
    }
    if(end==begin+RECV_BUF_SIZ){
	// We're already full so return
	return end-begin;
    }

    while(1){	// loop so that we can continue if a signal interrupts the epoll
	// the 15000 ms timeout means that the epoll will return in 15 seconds
	// whether anything is there or not.  We check for <= 0 for the 
	// return value.  0 would mean we timed out, -1 means an error.
	if((num_events=epoll_wait(epoll_fd,events,MAX_EVENTS,15000))<=0){
	    if(num_events==-1){
		if(errno==EINTR){
		    // got interrupted by signal, just restart
		    continue;
		}
		// any other error, like bad file descriptor can't write memory,
		// or the epoll_fd is bad, we set valid to false so they won't
		// get in again, but we still return number of available bytes
		std::cerr << "sockfdwrapper::getbytes() - problem from epoll_wait: " << strerror(errno) << '\n';
		valid=false;
	    }
	} else{
	    // got events, we only expect EPOLLIN on fd
	    for(size_t ctr=0;ctr<static_cast<size_t>(num_events);ctr++){
		if(!(events[ctr].events & EPOLLIN)){
		    continue;
		}else if(fd==events[ctr].data.fd){
		    // this is us!  Data is available to be read
		    while(1){
			ssize_t nbytes;
			if((nbytes=recv(fd,end,RECV_BUF_SIZ-(begin-end),0))<=0){
			    if(nbytes==0){
				//other side did orderly close of socket
				std::cerr << "sockfdwrapper::getbytes() - other end shutdown in an orderly fashion.\n";
				open=false;
			    }
			    break;
			}else if(nbytes==-1){
			    // handle error cases
			    continue;
			}else{
			    // got some bytes
			    end+=nbytes;
			    break;
			}
		    }
		}
	    }
	}
	break;	    // unless someone continued, we break out of the while(1);
    }
    return end-cur;
}

void
sockfdwrapper::sendall(const char *msg,size_t len)
{
    // we pass the len in rather than using strlen here, so that we can
    // send binary data that might include '\0'.
    size_t cnt=0;
    ssize_t retval;
    while(cnt<len){
	retval=send(fd,msg+cnt,len-cnt,MSG_NOSIGNAL);
	if(retval==-1){
	    // error
	    if(errno==EAGAIN or errno==EWOULDBLOCK or errno==EINTR){
		// EAGAIN or EWOULDBLOCK 'cause we filled buffers try again
		// EINTR 'cause someone invoked a signal handler
		continue;
	    }
	    // I could return something but this is called from
	    // inserters that have to keep returning the sockfdwrapper&
	    // so that you can chain.  There's no place to return an
	    // error code.
	    throw socket_insert_fail(errno);
	}else{
	    cnt+=retval;
	}
    }
}
