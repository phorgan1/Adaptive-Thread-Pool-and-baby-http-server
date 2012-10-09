// copyright Patrick Horgan
// source is open, feel free to use it as you wish with no restrictions
// except that this copyright notice must be preserved intact
#include "http.h"
#include <cstring>
#include <iostream>
#include <sstream>
#include <vector>
#include <fstream>

/*
    All this stuff is syntax for HTTP
    reserved    = gen-delims / sub-delims
    gen-delims  = ":" / "/" / "?" / "#" / "[" / "]" / "@"
    sub-delims  = "!" / "$" / "&" / "'" / "(" / ")"
		    / "*" / "+" / "," / ";" / "="
    unreserved  = ALPHA / DIGIT / "-" / "." / "_" / "-"
    ALPHA       = *( %41-5A / %61-7A )
    DIGIT       = *( %30-39 )
    pct-encoded = "%" HEXDIG HEXDIG
    HEXDIG      = "A" / "B" / "C" / "D" / "E" / "F"
		    / "a" / "b" / "c" / "d" / "e" / "f" /
		    ; %41-46 / %61-66
    URI         = scheme ":" hier-part [ "?" query ] [ "#" fragment ]
    scheme      = ALPHA *( ALPHA / DIGIT / "+" / "-" / "." )
    hier-part   = "//" authority path-abempty
		    / path-absolute
		    / path-rootless
		    / path-empty
    path          = path-abempty    ; begins with "/" or is empty
		    / path-absolute   ; begins with "/" but not "//"
		    / path-noscheme   ; begins with a non-colon segment
		    / path-rootless   ; begins with a segment
		    / path-empty      ; zero characters
    path-abempty  = *( "/" segment )
    path-absolute = "/" [ segment-nz *( "/" segment ) ]
    path-noscheme = segment-nz-nc *( "/" segment )
    path-rootless = segment-nz *( "/" segment )
    path-empty    = 0<pchar>
    segment       = *pchar
    segment-nz    = 1*pchar
    segment-nz-nc = 1*( unreserved / pct-encoded / sub-delims / "@" )
		    ; non-zero-length segment without any colon ":"
    pchar         = unreserved / pct-encoded / sub-delims / ":" / "@"
    query       = *( pchar / "/" / "?" )
    fragment    = *( pchar / "/" / "?" )
    authority   = [ userinfo "@" ] host [ ":" port ]
    userinfo    = *( unreserved / pct-encoded / sub-delims / ":" )
    host        = IP-literal / IPv4address / reg-name
    IP-literal = "[" ( IPv6address / IPvFuture  ) "]"
    IPvFuture  = "v" 1*HEXDIG "." 1*( unreserved / sub-delims / ":" )
    IPv6address =                            6( h16 ":" ) ls32
		/                       "::" 5( h16 ":" ) ls32
		/ [               h16 ] "::" 4( h16 ":" ) ls32
		/ [ *1( h16 ":" ) h16 ] "::" 3( h16 ":" ) ls32
		/ [ *2( h16 ":" ) h16 ] "::" 2( h16 ":" ) ls32
		/ [ *3( h16 ":" ) h16 ] "::"    h16 ":"   ls32
		/ [ *4( h16 ":" ) h16 ] "::"              ls32
		/ [ *5( h16 ":" ) h16 ] "::"              h16
		/ [ *6( h16 ":" ) h16 ] "::"

    ls32        = ( h16 ":" h16 ) / IPv4address
		; least-significant 32 bits of address

    h16         = 1*4HEXDIG
		; 16 bits of address represented in hexadecimal
    IPv4address = dec-octet "." dec-octet "." dec-octet "." dec-octet

    dec-octet   = DIGIT                 ; 0-9
		/ %x31-39 DIGIT         ; 10-99
		/ "1" 2DIGIT            ; 100-199
		/ "2" %x30-34 DIGIT     ; 200-249
		/ "25" %x30-35          ; 250-255
    reg-name    = *( unreserved / pct-encoded / sub-delims )
    port        = *DIGIT


*/

/*
    std::stringstream ss;
    char timebuffer[30];
    struct tm thetm;
    time_t thetime_t=time(NULL);
    gmtime_r(&thetime_t,&thetm);
    strftime(timebuffer,30,"%a, %d %b %Y %T GMT",&thetm);
    */

std::string
file2string(std::string filename)
{
    std::ifstream file(filename.c_str());
    if(!file.is_open()){
	// If they passed a bad file name, or one we have no read access to,
	// we pass back an empty string.
	return "";
    }
    // find out how much data there is
    file.seekg(0,std::ios::end);
    std::streampos length = file.tellg();
    file.seekg(0,std::ios::beg);
    // Get a vector that size and
    std::string s(length,0);
    file.read(&s[0],length);
    file.close();
    return s;
}


fileblob::~fileblob()
{
    if(blob){
	delete []  blob;
	blob=0;
    }
}

std::string
fileblob::curdir()
{
    int lastslash=path.rfind('/');
    return std::string(path.substr(0,lastslash));
}

std::string
fileblob::ext()
{
    int thedot=path.rfind('.');
    return std::string(path.substr(thedot+1));
}

fileblob::fileblob(std::string filename):stringvalid(false)
{
    std::ifstream file(filename.c_str());
    if(!file.is_open()){
	// If they passed a bad file name, or one we have no read access to,
	// we pass back an empty string.
	blob_size=0;
	blob=0;
	return;
    }
    path=filename;
    // find out how much data there is
    file.seekg(0,std::ios::end);
    std::streampos fsize=0, length = file.tellg()-fsize;
    file.seekg(0,std::ios::beg);
    blob_size=length;
    try{
	blob=new uint8_t[blob_size];
    }catch(...){
	blob=0;
	blob_size=0;
	return;
    }

    try{
	file.read(reinterpret_cast<char *>(blob),length);
	file.close();
    }catch(...){
	blob=0;
	blob_size=0;
    }
}

// copy constructor for authority
authority::authority(const authority&a):
    userinfo(a.userinfo),host(a.host),port(a.port){};

// usual use is to get a string and parse it
authority::authority(std::string s)
{
    size_t p,n;
    if((p=s.find('@'))!=std::string::npos){
	userinfo=std::string(s,0,p);
	p+=1;
    }else{
	p=0;
    }
    if((n=s.find(':',p))!=std::string::npos){
	port=std::string(s,n+1);
	host=std::string(s,p,n-p);
    }else{
	host=std::string(s,p);
    }
}

// assignment operator
authority&
authority::operator=(const authority& a)
{
    userinfo=a.userinfo;
    host=a.host;
    port=a.port;
    return *this;
}

// simplified version gets a host and port explicitly
authority::authority(std::string h,std::string p):host(h),port(p){};

// explicit user host and port
authority::authority(std::string u,std::string h,std::string p):
    userinfo(u),host(h),port(p){};

// Convert the authority back to uri text
std::string 
authority::to_string() const
{
    std::string retval;
    if(userinfo!=""){
	retval+=userinfo+'@';
    }
    retval+=host;
    if(port!=""){
	retval+=":"+port;
    }
    return retval;
}

// main constructor for http_request_line
http_request_line::http_request_line(const char *inrequest,
	std::map<std::string,std::string>& headers)
{
    // we expect the line to have any trailing \r or \n removed
    const char *ctr=inrequest,*savectr;
    int util;
    valid=false;

    while(*ctr!=' '&&*ctr) ctr++;
    if(*ctr=='\0'){
	// no request method or anything else
	return;
    }
    method=std::string(inrequest,ctr);
    if(!isvalidmethod()){
	return;
    }
    while(*ctr==' '&&*ctr) ctr++;
    if(*ctr=='\0'){
	// no uri
	return;
    }
    // pointing at beginning of uri
    savectr=ctr;
    while(*ctr!=' '&&*ctr) ctr++;
    if(*ctr=='\0'){
	// nothing past the request so it looks like a 0.9 request
	major_release=0;
	minor_release=9;
	// here's the problem
	theuri=uri(savectr,headers["Host"]);
	valid=true;
	return;
    }
    theuri=uri(std::string(savectr,ctr),headers["Host"]);
    // either pointing at HTTP/1.x or space
    while(*ctr==' '&&*ctr) ctr++;
    if(*ctr=='\0'){
	// once again it was a 0.9 but with trailing space
	major_release=0;
	minor_release=9;
	valid=true;
	return;
    }
    // now we should be pointing at HTTP/1.x
    savectr=ctr;
    if(strncmp("HTTP/",ctr,5)!=0){
	return;
    }
    ctr+=5;
    // now pointing at the major part of the release
    if(*ctr>='0' && *ctr<='9'){
	util=0;
	while(*ctr>='0' && *ctr<='9'){
	    util=10*util+*ctr-'0';
	    ctr++;
	}
    }else{
	// no major version so return invalid
	return;
    }
    if(*ctr!='.'){
	return;
    }
    major_release=util;
    ctr++;
    if(*ctr>='0' && *ctr<='9'){
	util=0;
	while(*ctr>='0' && *ctr<='9'){
	    util=10*util+*ctr-'0';
	    ctr++;
	}
    }else{
	// no minor version so return invalid
	return;
    }
    minor_release=util;
    valid=true;
    return;
}

// returns false if request isn't valid
inline bool
http_request_line::isvalidmethod()
{
    return method=="GET" || method=="OPTIONS" || method=="HEAD"
	|| method=="POST" || method=="PUT" || method=="DELETE"
	|| method=="TRACE" || method=="CONNECT";
}

// getter routines
std::string http_request_line::get_minor_release()
{
    std::stringstream ss;
    ss << minor_release;
    return ss.str();
}

std::string http_request_line::get_major_release()
{
    std::stringstream ss;
    ss << major_release;
    return ss.str();
}

// turn a http_request_line object back into a http request
std::string
http_request_line::to_string() const
{
    std::stringstream ssmaj;
    std::stringstream ssmin;
    ssmaj << major_release;
    ssmin << minor_release;

    return std::string(method+" "+theuri.to_string()+" HTTP/"+ssmaj.str()+"."+ssmin.str());
}

// turn a uri object into a uri string
std::string
uri::to_string() const
{
    //URI         = scheme ":" hier-part [ "?" query ] [ "#" fragment ]
    std::string rets="";
    if(scheme!=""){
	rets+=scheme+"://";
    }
    if(auth.is_valid()){
	rets+=auth.to_string();
    }
    rets+=path;
    if(query!=""){
	rets+="?"+query;
    }
    if(fragment!=""){
	rets+="#"+fragment;
    }
    return rets;
}

// if the uri had a file and the file had an extension, this will return it,
// otherwise an empty string
std::string uri::get_ext()
{
    std::string ext="";
    size_t idx;
    if((idx=path.rfind("."))!=std::string::npos){
	ext=path.substr(idx+1);
	std::transform(ext.begin(),ext.end(),ext.begin(),::tolower);
    }
    return ext;
}

// assignment operator for uris
uri& uri::operator=(const uri& u)
{
    scheme=u.scheme;
    path=u.path;
    auth=u.auth;
    query=u.query;
    fragment=u.fragment;
    valid=u.valid;
    return *this;
}

// main constructor for uri, expects the uri from the request and the host
// from the headers
uri::uri(std::string r,std::string h)
{
    // URI         = scheme ":" hier-part [ "?" query ] [ "#" fragment ]
    // http://dobby.com/fooburger/doohicky.html?a=sturgess#wacky
    // schm   auth            path               query      fragment
    size_t pos,pos2,begin=0;
    valid=false;
    scheme="http";
    //std::cerr << "uri(" << r << ',' << h << ")\n";
    auth=authority(h);

    if((pos=r.find('?',begin))!=std::string::npos){
	if((pos2=r.find('#',begin))!=std::string::npos){
	    path=r.substr(begin,pos);
	    query=r.substr(pos+1,pos2);
	    fragment=r.substr(pos2+1);
	}else{
	    path=r.substr(begin,pos);
	    query=r.substr(pos+1);
	}
    }else{
	path=r.substr(begin);
    }
}

// Will make sure the scheme matches the syntax of scheme
bool
uri::isvalidscheme()
{
    // scheme      = ALPHA *( ALPHA / DIGIT / "+" / "-" / "." )
    std::string::iterator i=scheme.begin();
    if(!isalpha(*i++)){
	return false;
    }
    for(;i!=scheme.end();i++){
	if(!(isalpha(*i)||isdigit(*i)||*i=='+'||*i=='-'||*i=='.')){
	    return false;
	}
    }
    return true;
}
