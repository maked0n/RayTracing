CC=g++
CFLAGS=-c -std=c++11 -Wall

all: rt

rt: main.o csphere.o cscene.o
	$(CC) main.o csphere.o cscene.o -o rt
main.o: main.cpp iobject3d.h geometry.hpp
	$(CC) $(CFLAGS) main.cpp iobject3d.h geometry.hpp ccamera.hpp
csphere.o: csphere.h csphere.cpp
	$(CC) $(CFLAGS) csphere.h csphere.cpp
cscene.o: cscene.h cscene.cpp
	$(CC) $(CFLAGS) cscene.h cscene.cpp
clean:
	rm -rf *.o rt

