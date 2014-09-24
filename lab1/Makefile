all: clean sph_out multi


multi: mph_out

multi_process_hash.o:
	g++ -c multi_process_hash.cpp -L/lib #-DDEBUG #-DSCHED_INFO #-DDEBUG # remove -DDEBUG to disable debug information

backgroundTask.o:
	g++ -c backgroundTask.c

mph_out: city.o multi_process_hash.o backgroundTask.o
	g++ city.o backgroundTask.o multi_process_hash.o -o mph_out -lcgroup

### Compile single process hash calculation files.
single: sph_out

sph_out: city.o single_process_hash.o
	g++ city.o single_process_hash.o -o sph_out

city.o:
	g++ -c city.cc

single_process_hash.o:
	g++ -c single_process_hash.cc

clean:
	rm -rf city.o single_process_hash.o multi_process_hash.o sph_out mph_out
