// copyright Patrick Horgan
// source is open, feel free to use it as you wish with no restrictions
// except that this copyright notice must be preserved intact
#ifndef http_guard
#define http_guard
#include <string>
#include <algorithm>
#include <map>

/*
         foo://example.com:8042/over/there?name=ferret#nose
         \_/   \______________/\_________/ \_________/ \__/
          |           |            |            |        |
       scheme     authority       path        query   fragment
          |   _____________________|__
         / \ /                        \
         urn:example:animal:ferret:nose
*/

std::string file2string(std::string filename);

// check ranges from rfc 2616 (http)
inline bool isalpha(const char& c)
{
    return (c>=0x41 && c<=0x5a) || (c>=0x61 && c<=0x7a);
}
inline bool isdigit(const char& c)
{
    return c>=0x30 && c<=0x39;
}
inline bool isalphanum(const char& c)
{
    return isalpha(c) || isdigit(c);
}
inline bool ishexdigit(const char& c)
{
    return (c>=0x41 && c<=0x46) || (c>=0x61 && c<=0x66);
}
inline bool isgendelim(const char& c)
{
    return c==':' || c=='/' || c=='?' || c=='#' || c=='[' || c==']' || c=='@';
}
inline bool issubdelim(const char&c)
{
    return c=='!' || c=='$' || c=='&' || c=='\'' || c=='(' || c==')'
	|| c=='*' || c=='+' || c==',' || c==';' || c=='=';
}

inline bool isreserved(const char& c)
{
    return isgendelim(c) || issubdelim(c);
}

class fileblob
{
public:
    std::string path;
    uint8_t* blob;
    size_t blob_size;
    uint8_t* begin(){return blob;}
    uint8_t* end(){return blob+blob_size;}
    fileblob(std::string);  // name of file to open and read
    fileblob(size_t);
    typedef uint8_t* iterator;
    bool stringvalid;
    std::string&
    getstring() const;
    std::string asstring;
    std::string curdir();
    std::string ext();
    ~fileblob();
};

class authority
{
public:
    authority(){};
    authority(std::string);
    authority(std::string h,std::string p);
    authority(std::string u,std::string h,std::string p);
    authority(const authority&);
    authority& operator=(const authority&);
    std::string to_string() const;
    bool is_valid() const { return true; };
    const std::string& get_host(){ return host; };
    const std::string& get_port(){ return port; };
private:
    std::string userinfo;
    std::string host;
    std::string port;
};

class uri
{
public:
    uri(std::string request_uri,std::string host);
    uri(){};
    uri& operator=(const uri&);
    uri(const uri&){};

    std::string to_string() const;
    std::string get_ext();
    std::string& get_path(){return path;}
    const std::string& get_host(){ return auth.get_host(); };
    const std::string& get_port(){ return auth.get_port(); };
private:
    std::string scheme;
    authority auth;
    std::string path;
    std::string query;
    std::string fragment;
    bool isvalidscheme();
    bool valid;
};

class http_request_line
{
public:
    http_request_line(const char *inrequest,std::map<std::string,std::string>&);
    std::string to_string() const;
    bool is_valid(){ return valid; };
    std::string get_ext(){ return theuri.get_ext(); };
    const std::string& get_path(){ return theuri.get_path(); };
    std::string get_uri() const{ return theuri.to_string(); };
    const std::string& get_method(){ return method; }
    std::string get_major_release();
    std::string get_minor_release();
    const std::string& get_host(){ return theuri.get_host(); };
    const std::string& get_port(){ return theuri.get_port(); };
private:
    bool isvalidmethod();
    http_request_line();
    http_request_line(const http_request_line&);
    http_request_line& operator=(const http_request_line&);
    std::string method;
    uri theuri;
    int major_release;
    int minor_release;
    bool valid;
};

inline
std::ostream& operator<<(std::ostream& os, const fileblob& b)
{
    // warning!  This will dump any kind of file to os.
    // I use it for debugging, only when I know it's an html file.
    // You could open a file for writing and dump a png with this as well.
    os << std::string(b.blob,b.blob+b.blob_size);
    return os;
}

inline
std::ostream& operator<<(std::ostream& os,const http_request_line& hrl)
{
    os << hrl.get_uri();
    return os;
}

#endif

