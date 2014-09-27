all: clean netfs

netfs.o:
	gcc -Wall netfs.c `pkg-config fuse --cflags --libs` -c

log.o:
	gcc -Wall log.c `pkg-config fuse --cflags --libs` -c

netfs: netfs.o log.o
	gcc -Wall netfs.o log.o `pkg-config fuse --cflags --libs` -o netfs
	rm netfs.o log.o

clean:
	rm -rf log.o netfs.o netfs

