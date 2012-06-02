
# Makefile
# Irene Huang, 2012/05/12

CXX=g++
CFLAGS=-Wall -g
LD=g++
LDFLAGS=


S_OBJS=server.o
C_OBJS=client.o


all: s.out c.out

s.out: $(S_OBJS) 
	$(CXX) $(CFLAGS) $(LDFLAGS) -o s.out $(S_OBJS) 

server.o: server.cpp local_sock.h
	$(CXX) $(CFLAGS) -c server.cpp

c.out: $(C_OBJS) 
	$(CXX) $(CFLAGS) $(LDFLAGS) -o c.out $(C_OBJS) 

client.o: client.cpp local_sock.h
	$(CXX) $(CFLAGS) -c client.cpp

.PHONY: clean
clean:
	rm -f *.o *.out 
