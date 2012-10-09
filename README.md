This collection started from my desire to do an adaptive thread/job queue
manager on linux in C++.  It has you can send a job to the queue and a 
thread is automatically dispatched to deal with it.  If the queue backs up,
more threads up to a configurable maximum are dispatched.

I wanted to test it and needed a task for it that you could give multiple
tasks to, and though that a skeletal http server would be an ideal thing,
since I could use freely available tools to bang on the http server and see
how the silly thing worked.

*DO NOT* use this http server as a real one.  I'm sure it has 10 billion security
holes in it.  It's not meant to be used for real sites.  It also doesn't 
support any server backend stuff like php or ruby except through the cgi
interface.

That said, feel free to modify as you desire.  If you want to push some changes,
I hope that you send me an email at patrick@dbp-consulting.com first.


