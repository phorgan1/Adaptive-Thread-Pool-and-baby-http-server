// copyright Patrick Horgan
// source is open, feel free to use it as you wish with no restrictions
// except that this copyright notice must be preserved intact
#include <map>
#include <netdb.h>
#include <fcntl.h>	    // only for O_NONBLOCK
#include "adaptiveThreadPool.h"
#include "http.h"
#include "sockfdwrapper.h"
#include <sys/stat.h>
#include <dirent.h>
#include <fstream>
#include <algorithm>
#include "time.h"
#include <strings.h>
#include <unistd.h>

void
error_exit(const char *msg, int status=1)
{
    std::cerr << msg << strerror(errno) << '\n';
    exit(status);
}

static int
createBindAndListenNonBlockingSocket(const char *service)
{
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int s, sfd;
    const int yes=1;
    const int yes_sz=sizeof(const int);

    // set up to call getaddrinfo which combines getservbyname and
    // getservbyport but works for both ipv6 and ipv4
    // getaddrinfo returns the information we need to open a socket
    // and bind it.  In this case, with the use of AI_PASSIVE we may get
    // more than one choice.  We'll use the first that actually lets us
    // open a socket and bind to it.  The first argument to getaddrinfo
    // is the name of a node, (internet host), 
    bzero (&hints, sizeof (struct addrinfo));
    hints.ai_family = AF_UNSPEC;     // Return IPv4 and IPv6 choices
    hints.ai_socktype = SOCK_STREAM; // TCP socket desired
    hints.ai_flags = AI_PASSIVE;     // All interfaces suitable for bind/accept
				     // since we use NULL for node

    if((s = getaddrinfo (NULL, service, &hints, &result))!=0){
	// sadly we couldn't get any interfaces
	std::cerr << "getaddrinfo: " << gai_strerror(s) << '\n';
	return -1;
    }

    // returned 1 or more interfaces that we can use
    for(rp=result;rp!=NULL;rp=rp->ai_next){
	// adding SOCK_NONBLOCK only works since Linux 2.6.27
	if((sfd=socket(rp->ai_family,rp->ai_socktype|SOCK_NONBLOCK,rp->ai_protocol))==-1){
	    // couldn't create a socket
	    continue;
	}else if(setsockopt(sfd,SOL_SOCKET,SO_REUSEADDR,&yes,yes_sz)==-1){
	    // SO_REUSEADDR lets us start the server right away if it dies
	    // we couldn't set the option so close the socket and try again
	    close(sfd);
	    continue;
	}else if((s=bind(sfd,rp->ai_addr,rp->ai_addrlen))!=0){
	    // so close!  oh well, close the socket and try another
	    close(sfd);
	    continue;
	}else if(listen(sfd, SOMAXCONN)==-1){
	    // durn!  couldn't mark the socket as a passive listener
	    // try another one
	    close(sfd);
	    continue;
	}else{
	    // We managed to: set opt, bind, and listen so get outahere!
	    break;
	}
    }

    freeaddrinfo (result);  // in any case we don't need this so free the list

    if (rp==NULL) {
	// weren't able to open a socket, set SO_REUSEADDR, bind, and listen
	std::cerr << "Could not bind\n";
	return -1;
    }
    return sfd;
}

void
send301(sockfdwrapper& sfd,const std::string to,const std::string host, const std::string port)
{
    std::stringstream ss;
    char timebuffer[30];
    struct tm thetm;
    time_t thetime_t=time(NULL);
    gmtime_r(&thetime_t,&thetm);
    strftime(timebuffer,30,"%a, %d %b %Y %T GMT",&thetm);

    std::string data=
    "<!DOCTYPE html>\n"
    "<html>\n"
    "  <head>\n"
    "    <title>301 Moved Permanently</title>\n"
    "  </head>\n"
    "  <body>\n"
    "    <h1>Moved Permanently</h1>\n"
    "    <p>The document has move <a href=\"";
    data+=to;
    data+=
    "\">here</a>.</p>\n"
    "    <hr />\n"
    "    <address>Patrick's fine server at "+host+" Port "+port+"</address>\n"
    "  </body>\n"
    "</html>\n";
    ss << data.size();
    try{
	sfd << "HTTP/1.1 301 Moved Permanently\r\n"
	    << "Date: " << timebuffer << "\r\n"
	    << "Location: " << to << "\r\n"
	    << "Content-Length: " << ss.str() << "\r\n"
	    << "Content-Type: text/html; charset=iso-8859-1\r\n\r\n"
	    << data;
    }catch(const socket_insert_fail& sif){
	std::cerr << sif.what() << '\n';
    }
}

void
send404(sockfdwrapper& sfd)
{
    try{
    sfd<<
    "HTTP/1.1 404 Mysteriously missing file.\n\n"
    "<!DOCTYPE html >\n"
    "<!-- Copyright 2011 Patrick Horgan patrick at dbp-consulting dot com\n"
    "     all rights reserved.\n"
    "-->\n"
    "<html>\n"
    "    <head>\n"
    "	<meta http-equiv=\"content-type\" content=\"text/html; charset=UTF-8\">\n"
    "	<title>404 - Missing Page</title>\n"
    "	<style>\n"
    "	    .giant { font-size: 404%; }\n"
    "	    .teenytiny { font-size: 70%; }\n"
    "	    p, body {\n"
    "		font-family: \"LMRomanDunh10\", Cambria,'Times New Roman','Nimbus Roman No9 L','Freeserif',Times,serif;\n"
    "		font-weight: bold;\n"
    "	    }\n"
    "\n"
    "	    div#notice {\n"
    "		width: 60%;\n"
    "		margin-left: 20%;\n"
    "		text-align: center;\n"
    "		border: 1px rgb(80,255,0);\n"
    "		border-color: rgb(80,190,0);\n"
    "		border-style: groove;\n"
    "		border-width: 5px;\n"
    "		padding: 5px;\n"
    "	    }\n"
    "	</style>\n"
    "    </head>\n"
    "    <body>\n"
    "	<div id=\"notice\">\n"
    "	    <h1 class='center'>Sorry, that page doesn't exist.</h1>\n"
    "	    <p>\n"
    "	    That's so embarrassing.  I may or may not have failed in some\n"
    "	    way that caused this to happen.\n"
    "	    Maybe it used to be a good page and moved, and I (I'm the owner\n"
    "	    of the website) forgot to redirect it.  Maybe it's a typo.\n"
    "	    Maybe, you actually clicked on a link to this page.  <em>That</em>\n"
    "	    would be weird, wouldn't it, deliberately going to an error page?\n"
    "	    In that case I take back everything.  It could be that I moved a\n"
    "	    page, but left a link to it and you clicked on that link.  That\n"
    "	    one would be my fault, huh?  Right?\n"
    "	    </p>\n"
    "	    <p class='teenytiny'>\n"
    "	    In case you wanted to know, this is also known as a\n"
    "	    </p>\n"
    "	    <div class='giant center'>404</div>\n"
    "	    <p class='teenytiny'>\n"
    "	    Well--actually, it's always known as a 404 which just means the\n"
    "	    silly file didn't exist, so even if you didn't want to know, I'm\n"
    "	    sorry.  The name of it is not dependent on whether you wanted to\n"
    "	    know or not.  The universe does not revolve around you, but rather\n"
    "	    around a particular person, (her name is Maria), in Chile.  I\n"
    "	    don't think that she knows though, but even if she did, <em>she</em>\n"
    "	    wouldn't make such a big deal out of it, because that's how she \n"
    "	    rolls.\n"
    "	    </p>\n"
    "	</div>\n"
    "    </body>\n"
    "</html>\n";
    }catch(const socket_insert_fail& sif){
	std::cerr << sif.what() << '\n';
    }
    return;
}

void
send400(sockfdwrapper& sfd)
{
    try{
    sfd<<
	"HTTP/1.1 400 Bad Request\n\n"
	"<!DOCTYPE html >"
	"<html><head>"
	"<title>400 Bad Request</title>"
	"</head><body>"
	"<h1>Bad Request</h1>"
	"<p>Your browser sent a request that this server could not understand.<br />"
	"</p>"
	"<hr>"
	"</body></html>";
    }catch(const socket_insert_fail& sif){
	std::cerr << sif.what() << '\n';
    }
    return;
}

void
send500(sockfdwrapper& sfd)
{
    try{
    sfd<<
	"HTTP/1.1 500 Bad Request\n\n"
	"<!DOCTYPE html >"
	"<html><head>"
	"<title>500 Internal Server Error</title>"
	"</head><body>"
	"<h1>Internal Server Error</h1>"
	"<p>Your browser sent a request and the server had something strange happen.  Sorry.<br />"
	"</p>"
	"<hr>"
	"</body></html>";
    }catch(const socket_insert_fail& sif){
	std::cerr << sif.what() << '\n';
    }
    return;
}
void
send_directory(sockfdwrapper& sfd,std::string& directory,std::string uri)
{
    struct dirent entry;
    struct dirent *retval;
    DIR *dir=opendir(directory.c_str());;


    try{
	sfd <<
	    "HTTP/1.1 200 OK\n"
	    "Set-Cookie: server=patrick0.7\n\n"
	    "<html>\n"
	    "  <head>\n"
	    "    <style>\n"
	    "      pre {\n"
	    "        border: solid 1px;\n"
	    "        background-color: rgb(270,260,200);\n"
	    "        width: auto;\n"
	    "      }\n"
	    "    </style>\n"
	    "  </head>\n"
	    "  <body>\n"
	    "    <h1>" << uri << "</h1>\n"
	    "    <pre>\n";
	while(!readdir_r(dir,&entry,&retval)&&retval!=NULL){
	    sfd << entry.d_name << '\n';
	}
	sfd << 
	    "    </pre>\n"
	    "  </body>\n"
	    "</html>\n";
    }catch(const socket_insert_fail& sif){
	std::cerr << sif.what() << '\n';
    }

    closedir(dir);

}

std::string&
expand_includes(fileblob& b,std::string& data)
{
    // We have to check for included files
    // format is something like:
    // <!--#include virtual="/cgi-bin/counter.pl" -->
    // There's also file= but we don't support it, nor do we support
    // any other SSI.
    //std::cerr << "expand_includes for " << b.path << "\n";
    //std::cerr << "on input, data is\n";
    //std::cerr << data << "\n";

    const char* searchSSIinclude="<!--#include";
    const char* searchVirtual="virtual";
    const char* searchSSIEnd="-->";
    fileblob::iterator start,foundit,last=b.begin();

    while(last!=b.end() && (foundit=std::search(last,b.end(),searchSSIinclude,searchSSIinclude+12))!=b.end()){
	start=foundit; // remember beginning in case we can't use it
	// data gets from beginning to the include
	data+=std::string(last,foundit);
	// search for virtual
	if((foundit=std::search(foundit+13,b.end(),searchVirtual,searchVirtual+7))==b.end()){
	    // didn't find virtual, so look for end of SSI tag
	    foundit=std::search(start,b.end(),searchSSIEnd,searchSSIEnd+3);
	    if(foundit!=b.end()){
		// if we found the ending -->, add to output and skip
		data+=std::string(start,foundit+3);
		last=foundit+3;
		continue;
	    }else{
		// no end of tag in the whole thing!!! skip to end
		data=std::string(start,b.end());
		last=b.end();
		continue;
	    }
	}else{
	    // Found the word virtual and foundit points at it
	    foundit+=7;	// get past "virtual" then find the = sign
	    while(*foundit!='=') foundit++;
	    foundit++;	// skip the equal sign
	    // now skip ws
	    while(*foundit==' ' || *foundit=='\t' || *foundit=='\r' || *foundit=='\n') foundit++;
	    // after white space expect 'filename' or "filename"
	    if(*foundit=='\'' || *foundit=='"'){
		fileblob::iterator idx;
		char sep=*foundit++;	// remember ' or "
		idx=foundit;
		while(*idx!=sep && idx!=b.end()) idx++;
		if(idx==b.end()){
		    // malformed file with 'filename.... and never a '
		    data+=std::string(start,b.end());
		    last=b.end();
		    continue;
		}
		// found a name between quotes, look for such a file
		std::string thefile=b.curdir()+"/"+std::string(foundit,idx);
		fileblob tfb(thefile);
		if(tfb.blob_size!=0){
		    data+=expand_includes(tfb,data);
		}else{
		    // what do we do if the file doesn't open
		}
		// now skip to end of the SSI tag and skip it.
		foundit=std::search(++idx,b.end(),searchSSIEnd,searchSSIEnd+3);
		if(foundit!=b.end()){
		    // if we found the ending --> just skip past it.
		    last=foundit+3;
		    continue;
		}else{
		    // otherwise, add the data past the idx to the end to the
		    // string  The would let something like
		    // <--#include virtual="some.file"
		    // work incorrectly but what else do we do?
		    last=b.end();
		    data+=std::string(++idx,b.end());
		    continue;
		}
	    }else{
		// found virtual= but then no ' or "
		// skip to end of ssi element if found
		foundit=std::search(start,b.end(),searchSSIEnd,searchSSIEnd+3);
		if(foundit!=b.end()){
		    data+=std::string(start,foundit+3);
		    last=foundit+3;
		    continue;
		}else{
		    data+=std::string(start,b.end());
		    last=b.end();
		    continue;
		}
	    }
	}
    }
    // If we never found an include last is still pointing at the start.  
    // Otherwise, it's pointing past the -->, or if not found at end()
    data+=std::string(last,b.end());   // Add rest to data
    return data;
}

void
send_file(sockfdwrapper& sfd,http_request_line& hrl, std::map<std::string,std::string>&hdrs)
{
    struct stat sb;
    int statrtn;
    std::string refdir;
    std::string ext=hrl.get_ext();

    // technically should check for existence:
    // if(hdrs.find("DOCUMENT_ROOT")!=hdrs.end()) but I always put this one in.
    std::string filename=hdrs["DOCUMENT_ROOT"];
    // Now we point to the document root, add the string from the request
    filename+=hrl.get_path();
    // Now we've got the filename, check to see if it exists
    if((statrtn=stat(filename.c_str(),&sb))==-1){
	// the stat failed, see why
	if(errno==ENOENT){
	    std::cerr << filename.c_str() << " does not exist\n";
	    // The file does not exist with that name, try building it again
	    // as a relative reference using the referer
	    std::string refval=hdrs["Referer"];
	    if(refval!=""){
		int offset=refval.find('/',7);
		std::string referer=std::string(hdrs["Referer"],offset);
		std::string dirname=hdrs["DOCUMENT_ROOT"]+referer;
		// Now we might have the directory the file is in, check
		// to see if it exists
		if(stat(dirname.c_str(),&sb)==-1){
		    // Nope, doesn't exist, out of luck
		    send404(sfd);
		    return;
		}else{
		    // exists, check if it's a directory
		    if((sb.st_mode&S_IFMT)==S_IFDIR){
			// yep, try adding our filename to it
			std::cerr << "~~~~~building from DOCUMENT_ROOT: " 
			    << hdrs["DOCUMENT_ROOT"]
			    << ", Referer: " << referer
			    << " and the request: "
			    << hrl.get_path()
			    << '\n';
			filename=dirname+hrl.get_path();
			if(stat(filename.c_str(),&sb)==-1){
			    // doesn't exist
			    send404(sfd);
			    return;
			}
		    }else{
			// Not a directory so won't find our file in there
			send404(sfd);
			return;
		    }
		}
	    }
	}else{ // stat failed on file and wasn't enoent so log an error
	    std::cerr << "Unknown error: " << strerror(errno) << '\n';
	    // and send a 500 unexpected server error ??? What else?
	    send500(sfd);
	    return;
	}
    }
    // Here we have found something in the filesystem, either a directory
    // or a file
    if((sb.st_mode&S_IFMT)==S_IFDIR){
	// If it's a directory, check to see if the request ended with a slash.
	// If not, send a redirect to the one with the slash.  Most servers
	// do this, because otherwise things get too hard with relative
	// requests
	if(filename[filename.size()-1]!='/'){
	    send301(sfd,
		static_cast<std::string>(hrl.get_uri())+="/",hrl.get_host(),hrl.get_port());
	    return;
	}
	// Well it's a directory, see if it has an index.html in it.
	// I've arbitrarily decided not to check for index.htm or index.php etc
	// for now.
	// If not, send the directory
	// todo: needs checking that still under Document-Root
	std::string index=filename+"index.html";
	if(stat(index.c_str(),&sb)!=-1){
	    filename=index;
	}else{
	    // Send the directory contents
	    send_directory(sfd,filename,hrl.get_uri());
	    return;
	}
    }
    // Directories dealt with, now we know that it's a file
    // get the name of the directory the file is in
    int lastslash=filename.rfind('/');
    std::string curdir=filename.substr(0,lastslash);

    // this is where we get the blob
    fileblob b(filename);
    if(b.blob_size==0){
	send404(sfd);
	return;
    }
    try{
	ext=b.ext();
	sfd <<
	    "HTTP/1.1 200 OK\r\n"
	    "Set-Cookie: server=patrick0.7\r\n";
	if(ext=="ico" or ext=="jpeg" or ext=="jpg"
		or ext=="png" or ext=="gif" or ext=="bmp"){
	    sfd << "Content-Type: image/" << ext << "\r\n";
	    sfd << "Content-Length: " << static_cast<int>(b.blob_size) << "\r\n";
	    sfd << "\r\n";
	    sfd << b;
	}else if(ext=="js"){
	    sfd << "Content-Type: application/javascript\r\n";
	    sfd << "Content-Length: " << static_cast<int>(b.blob_size) << "\r\n";
	    sfd << "\r\n";
	    sfd << b;
	}else if(ext=="bz2"){
	    sfd << "Content-Type: application/x-bzip2\r\n";
	    sfd << "Content-Length: " << static_cast<int>(b.blob_size) << "\r\n";
	    sfd << "\r\n";
	    sfd << b;
	}else if(ext=="ogg"){
	    sfd << "Content-Type: audio/ogg\r\n"
		<< "Content-Length: " << static_cast<int>(b.blob_size) << "\r\n";
	    sfd << "\r\n";
	    sfd << b;
	}else if(ext=="css"){
	    sfd << "Content-Type: text/css\r\n"
		<< "Content-Length: " << static_cast<int>(b.blob_size) << "\r\n";
	    sfd << "\r\n";
	    sfd << b;
	}else if(ext=="html" || ext=="htm"){
	    std::string data;
	    expand_includes(b,data);
	    sfd << "Content-Type: text/html\r\n"
		<< "Content-Length: " << static_cast<int>(data.size()) << "\r\n";
	    sfd << "\r\n";
	    sfd << data;
	}
	return;
    }catch(const socket_insert_fail& sif){
	std::cerr << sif.what() << '\n';
    }catch(const std::bad_alloc& ba){
	std::cerr << "send_file caught a bad_alloc() - " << ba.what() << '\n';
    }
}

void
log_request(sockfdwrapper& sfd,http_request_line&hrl, std::map<std::string,std::string>&hdrs)
{
    std::cerr << "~logging request~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n"
	<< "Method      : " << hrl.get_method() << '\n'
	<< "URI         : " << hrl.get_uri() << '\n'
	<< "HTTP release: " << hrl.get_major_release() << '.' << hrl.get_minor_release() << '\n';
    for(std::map<std::string,std::string>::iterator i=hdrs.begin();i!=hdrs.end();i++){
	std::cerr << i->first << ": " << i->second << '\n';
    }
    std::cerr << "~end request~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";
}

/**
 one_request is the entry point for a thread handling one request
 browserFDPointer is a pointer to our wrapper for an fd
 */

void *
one_request(void *browserFDPointer)
{
    int browser_fd=*static_cast<int *>(browserFDPointer);
    sockfdwrapper sfd(browser_fd);
    if(!sfd.is_valid()){
	return browserFDPointer;
    }
    std::vector<std::string> headers;
    std::map<std::string,std::string> mapheaders;
    mapheaders["DOCUMENT_ROOT"]="/home/patrick/public_html";

    const int RECV_BUF_SIZ=1024;
    char buffer[RECV_BUF_SIZ+1];    // leave room for a trailing '\0'
    char *bufptr;
    std::string request;


    try{
	if((bufptr=sfd.getline(buffer,RECV_BUF_SIZ+1))==NULL){
	    if(sfd.is_valid() && sfd.is_closed()){
		close(browser_fd);
		return browserFDPointer;
	    }
	    std::cerr << "bad getline\n";
	    send400(sfd);
	    close(browser_fd);
	    return browserFDPointer;
	}
	request=bufptr;
	while((bufptr=sfd.getline(buffer,RECV_BUF_SIZ+1))!=NULL){
	    if(bufptr[0]=='\n' || (bufptr[0]=='\r'&&bufptr[1]=='\n')){
		break;
	    }
	    while(*bufptr!='\n') bufptr++;
	    while(*bufptr=='\n' || *bufptr=='\r' || *bufptr==' ' || *bufptr=='\t'){
		*bufptr--='\0';
	    }
	    if(*buffer==' ' || *buffer=='\t'){
		if(headers.size()>0){
		    headers[headers.size()-1]+=buffer;
		}
	    }
	    headers.push_back(std::string(buffer));
	}	
	for(std::vector<std::string>::iterator i=headers.begin();i!=headers.end();i++){
	    std::string key;
	    std::string value;
	    size_t idx;
	    if((idx=(*i).find(":"))!=std::string::npos){
		key=std::string(*i,0,idx);
		value=std::string(*i,idx+2);
		mapheaders[key]=value;
	    }
	}
	http_request_line hrl(request.c_str(),mapheaders);
	if(hrl.is_valid()==false){
	    send400(sfd);
	    return browserFDPointer;
	} else if(hrl.get_method()=="GET"){
	    log_request(sfd,hrl,mapheaders);
	    send_file(sfd,hrl,mapheaders);
	}
    }catch(const std::bad_alloc& ba){
	std::cerr << "one_request caught a bad_alloc() - " << ba.what() << '\n';
    }
    return browserFDPointer;
}

template <class T>
bool from_string(T& t,const std::string& s,std::ios_base& (*f)(std::ios_base&))
{
    std::istringstream iss(s);
    return !(iss >> f >> t).fail();
}

int main(int argc, char *argv[])
{
    const int MAX_EVENTS=64;
    int numthreads=25;
    argc=argc;
    std::cout << "argv[0]: " << argv[0] << " argc: " << argc << '\n';
    if(argc>1){
	// get the max thread count
	int cnt;
	if(from_string<int>(cnt,argv[1],std::dec)){
	    numthreads=cnt;
	}
    }
    int epollfd;
    struct epoll_event ev, events[MAX_EVENTS];
    int listen_sock;		    /* listening socket descriptor */

    // up to numthreads all calling one_request()
    adaptiveThreadPool atp(one_request,numthreads);

    // set up the attributes for our master thread
    pthread_attr_t theattr;
    pthread_attr_init(&theattr);
    pthread_attr_setdetachstate(&theattr,PTHREAD_CREATE_DETACHED);

    //numCPU not currently used but can be used to make decisions about
    //number of thread or whether to let the master thread do any jobs
    size_t numCPU;
    if(((numCPU = sysconf( _SC_NPROCESSORS_ONLN ))==-1)&&errno==EINVAL){
	numCPU=1;
    }
    //std::cout << "There are " << numCPU << " cpus online.\n";

    //daemon(1,1);
     
    /* get the master listen_sock */
    if((listen_sock=createBindAndListenNonBlockingSocket("8080")) ==-1){
	error_exit("We couldn't create and bind and listen on a socket",1);
    }
     
    // If you aren't used to epoll there are three steps
    // 1) epoll_create to get an epoll instance
    // 2) one or more epoll_ctl to register file descriptors to be tracked
    // 3) epoll_wait to sleep until a connection happens
    if((epollfd=epoll_create1(0))==-1){	// step 1
	error_exit("epoll_create1 failed",1);
    }

    // Now we'll set up for the epoll_ctl
    // EPOLLIN - available for reads
    bzero(&ev,sizeof(ev));
    ev.events = EPOLLIN;
    ev.data.fd=listen_sock;	// this is the socket we'll listen to
    if(epoll_ctl(epollfd,EPOLL_CTL_ADD,listen_sock,&ev)==-1){ // step 2
	close(epollfd);
	error_exit("epoll_ctl failed",1);
    }
     
    // now enter our main loop
    while(1){
	int num_events,retval;
	// the -1 means no timeout
	if((num_events=epoll_wait(epollfd,events,MAX_EVENTS,-1))==-1){ //step3
	    if(errno==EINTR){
		// on interrupt just go around again
		continue;
	    }else{
		// non recoverable error
		close(epollfd);
		close(listen_sock);
		error_exit("epoll_wait failed");
	    }
	}
	// got events, loop through them
	for(int ctr=0;ctr<num_events;ctr++){
	    if(!(events[ctr].events & EPOLLIN)){
		std::cerr << "epoll_error" << strerror(errno) << '\n';
		continue;
	    } else if(listen_sock==events[ctr].data.fd){
		struct sockaddr in_addr;
		socklen_t in_len=sizeof in_addr;
		int infd;
		char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
		// accept4 let's us pass the O_NONBLOCK to save a couple
		// of fcntl calls on the new socket.
		if((infd=accept4(listen_sock,&in_addr,&in_len,O_NONBLOCK))==-1){
		    if(errno==EAGAIN||errno==EWOULDBLOCK){
			// no more incoming connections
			// Of course we shouldn't get this, but let's be
			// defensive, ok?
			break;
		    }else {
			std::cerr << "accept" << strerror(errno) << '\n';
			break;
		    }
		}
		// make sure they can be reverse looked up or we don't want
		// to talk to them.
		switch( retval=getnameinfo(&in_addr,in_len,hbuf,sizeof hbuf,
			sbuf,sizeof sbuf,NI_NUMERICHOST|NI_NUMERICSERV)){
		    case 0:
			//std::cout << "Accepted connection on descriptor "
			    //<< infd << "(host="
			    //<< hbuf << ", port=" << sbuf << ")\n";
			break;
		    default:
			std::cerr << gai_strerror(retval) << '\n';
		}
		// push the socket onto the job queue
		atp.addjob(infd);
	    }
	} // for(int ctr=0;ctr<num_events;ctr++)
    } // while(1)
    return 0;
} // main
