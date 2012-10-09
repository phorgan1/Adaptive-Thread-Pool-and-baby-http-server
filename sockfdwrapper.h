// copyright Patrick Horgan
// source is open, feel free to use it as you wish with no restrictions
// except that this copyright notice must be preserved intact
#ifndef sockfdwrapper_guard
#define sockfdwrapper_guard
#include <cstdlib>
#include <cstring>
#include <string>
#include <sstream>
#include <exception>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <iostream>
#include "http.h"

const int MAX_EVENTS=64;
const size_t RECV_BUF_SIZ=8*1024;
class
socket_insert_fail: public std::exception
{
public:
    socket_insert_fail(int err):err(err){
	std::stringstream ss;
	ss << "Couldn't insert into a socket, " << strerror_r(err,const_cast<char*>(the_buf),1023);
	the_str=ss.str();
    };
    virtual ~socket_insert_fail() throw() {};
    int
    error() const { return err; };
    virtual const char* what() const throw()
    {
	return the_str.c_str();
    };
private:
    int err;
    std::string the_str;
    char the_buf[1024];
};

class sockfdwrapper
{
public:
    // this is just so we have a class so we can make operator<<s that
    // will insert into a socket.  You can't insert into an int.  This
    // lets us pass the socket to an inserter so that it can send to it.
    sockfdwrapper(int i);
    void sendall(const char *msg, size_t len);
    char *getline(char *,size_t);
    bool is_closed(){ return open==false; };
    bool is_valid(){ return valid==true; };
    ~sockfdwrapper();
private:
    sockfdwrapper();
    sockfdwrapper(const sockfdwrapper&);
    ssize_t getbytes(void);
    const sockfdwrapper& operator=(const sockfdwrapper&);
    int fd;
    bool valid;
    bool open;
    int epoll_fd;
    struct epoll_event ev;
    struct epoll_event events[MAX_EVENTS];
    char rawbuffer[RECV_BUF_SIZ];
    char *begin,*cur,*end;
};

inline
sockfdwrapper&
operator<<(sockfdwrapper& sfw,const char *msg)
{
    sfw.sendall(msg,strlen(msg));
    return sfw;
}

inline
sockfdwrapper&
operator<<(sockfdwrapper& sfw,const fileblob& fb)
{
    sfw.sendall(reinterpret_cast<const char*>(fb.blob),fb.blob_size);
    return sfw;
}

inline
sockfdwrapper&
operator<<(sockfdwrapper& sfw,const std::string& s)
{
    sfw.sendall(s.c_str(),s.size());
    return sfw;
}

inline
sockfdwrapper&
operator<<(sockfdwrapper& sfw, const char c)
{
    sfw.sendall(&c,1);
    return sfw;
}

inline
sockfdwrapper&
operator<<(sockfdwrapper& sfd, const int i)
{
    std::stringstream ss;
    ss<<i;
    sfd << ss.str();
    return sfd;
}
#endif
