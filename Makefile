CXX=g++
CFLAGS=-ggdb -Wall -Wextra -pedantic -Wconversion -Wfloat-equal -Wshadow -Wmissing-declarations -std=c99
CPPFLAGS=-std=c++0x -ggdb -Wall  -I/usr/local/ootbc/include -L/usr/lib/i386-linux-gnu 
allbins=httpserver
all: $(allbins)

adaptiveThreadPool.o: adaptiveThreadPool.cpp adaptiveThreadPool.h
http.o: http.cpp http.h
jobQueue.o: jobQueue.h
sockfdwrapper.o: sockfdwrapper.h
httpserver: httpserver.cpp adaptiveThreadPool.h jobQueue.h adaptiveThreadPool.o http.o sockfdwrapper.o
	$(CXX) $(CPPFLAGS) -o httpserver httpserver.cpp adaptiveThreadPool.o http.o sockfdwrapper.o -lpthread
clean:
	rm -rf $(allbins) core *~ *.o
