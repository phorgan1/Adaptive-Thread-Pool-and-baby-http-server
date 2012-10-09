CXX=g++
CFLAGS=-ggdb -Wall -Wextra -pedantic -Wconversion -Wfloat-equal -Wshadow -Wmissing-declarations -std=c99
CPPFLAGS=-ggdb -Wall  -std=c++0x -I/usr/local/ootbc/include
allbins=testauthority testhttp_request_line
all: $(allbins)

testhttp_request_line: testhttp_request_line.cpp ../http.cpp ../http.h
	$(CXX) $(CPPFLAGS) testhttp_request_line.cpp ../http.cpp -o testhttp_request_line
testauthority: testauthority.cpp ../http.cpp ../http.h
	$(CXX) $(CPPFLAGS) testauthority.cpp ../http.cpp -o testauthority
clean:
	rm -rf $(allbins) core *~ *.o
