all: sph_out

### Compile single process hash calculation files.
single: sph_out

sph_out: city.o single_process_hash.o
	g++ city.o single_process_hash.o -o sph_out

city.o:
	g++ -c city.cc

single_process_hash.o:
	g++ -c single_process_hash.cc

clean:
	rm -rf city.o single_process_hash.o sph_out
