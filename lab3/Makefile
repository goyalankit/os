all: clean loader.out test1.out test2.out test3.out test4.out demand_loader.out hybrid_loader.out test1.o os_test.out



loader.out:
	gcc loader.c -o loader.out -m64 -g

demand_loader.out:
	gcc demand_loader.c -o demand_loader.out -m64 -g

hybrid_loader.out:
	gcc hybrid_loader.c -o hybrid_loader.out -m64 -g

test1.out:
	gcc test1.c -o test1.out -Wl,-Ttext-segment=0x2000000 -static -O0 -g

test2.out:
	gcc test2.c -o test2.out -Wl,-Ttext-segment=0x2000000 -static -O0 -g

test3.out:
	gcc test3.c -o test3.out -Wl,-Ttext-segment=0x2000000 -static -O0 -g

test4.out:
	gcc test4.c -o test4.out -Wl,-Ttext-segment=0x2000000 -static -O0 -g

test1.o:
	gcc test1.c -c -o test1.o -g

os_test.out:
	gcc os_test.c -o os_test.out -Wl,-Ttext-segment=0x2000000 -static -g

clean:
	rm -rf *.o *.out
