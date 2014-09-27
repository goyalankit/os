all: clean netfs

netfs.o:
	gcc -Wall netfs.c `pkg-config fuse --cflags --libs` -c

#log.o:
#	gcc -Wall log.c `pkg-config fuse --cflags --libs` -c

ssh_connect.o:
	gcc -Wall ssh_connect.c -c -lssh

netfs: netfs.o ssh_connect.o
	gcc -Wall netfs.o ssh_connect.o `pkg-config fuse --cflags --libs` -o netfs -lssh
	rm netfs.o ssh_connect.o

clean:
	rm -rf log.o netfs.o netfs

