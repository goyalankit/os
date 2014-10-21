all: clean loader.out test1.out demand_loader.out test1.o

demand_loader.out:
	gcc demand_loader.c -o demand_loader.out -static -m64

loader.out:
	gcc loader.c -o loader.out -DDBG -static -m64

test1.out:
	gcc test1.c -o test1.out -Wl,-Ttext-segment=0x2000000 -static

test1.o:
	gcc test1.c -c -o test1.o

clean:
	rm -rf *.o *.out
