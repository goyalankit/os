all: apager test1.out test2.out test3.out dpager hpager

apager: loader.c loader.h
	gcc loader.c -o apager -m64 -g

dpager: demand_loader.c demand_loader.h loader.h
	gcc demand_loader.c -o dpager -m64 -g

hpager: hybrid_loader.c loader.h
	gcc hybrid_loader.c -o hpager -m64 -g

test1.out:
	gcc test1.c -o test1.out -Wl,-Ttext-segment=0x2000000 -static -O0 -g

test2.out:
	gcc test2.c -o test2.out -Wl,-Ttext-segment=0x2000000 -static -O0 -g

test3.out:
	gcc test3.c -o test3.out -Wl,-Ttext-segment=0x2000000 -static -O0 -g

clean:
	rm -rf *.o *.out
