# Makefile for Proxy Lab 
#
# You may modify this file any way you like (except for the handin
# rule). You instructor will type "make" on your specific Makefile to
# build your proxy from sources.

CC = gcc
CFLAGS = -g -Wall
LDFLAGS = -lpthread

all: proxy

cache.o: ../lib/cache.c ../include/cache.h
	$(CC) $(CFLAGS) -c ../lib/cache.c

read_line.o: ../lib/read_line.c ../include/read_line.h
	$(CC) $(CFLAGS) -c ../lib/read_line.c

inet_sockets.o: ../lib/inet_sockets.c ../include/inet_sockets.h
	$(CC) $(CFLAGS) -c ../lib/inet_sockets.c

csapp.o: csapp.c csapp.h
	$(CC) $(CFLAGS) -c csapp.c

proxy.o: proxy.c csapp.h ../include/read_line.h ../include/inet_sockets.h ../include/cache.h
	$(CC) $(CFLAGS) -c proxy.c

proxy: proxy.o csapp.o read_line.o inet_sockets.o cache.o
	$(CC) $(CFLAGS) proxy.o csapp.o read_line.o inet_sockets.o cache.o -o proxy $(LDFLAGS)

# Creates a tarball in ../proxylab-handin.tar that you can then
# hand in. DO NOT MODIFY THIS!
handin:
	(make clean; cd ..; tar cvf $(USER)-proxylab-handin.tar proxylab-handout --exclude tiny --exclude nop-server.py --exclude proxy --exclude driver.sh --exclude port-for-user.pl --exclude free-port.sh --exclude ".*")

clean:
	rm -f *~ *.o proxy core *.tar *.zip *.gzip *.bzip *.gz

