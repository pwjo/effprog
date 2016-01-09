
PERF_OPTS= -e cycles:u -e instructions:u -e branch-misses:u -e L1-dcache-load-misses -e L1-dcache-loads
C_OPTS= -g -O3 -Wall 

all: ep15 ep15-hash ep15-gperf-hash ep15-gperf-hash-alloc
	perf stat $(PERF_OPTS) ./ep15 cross.input
	perf stat $(PERF_OPTS) ./ep15-hash cross.input
	perf stat $(PERF_OPTS) ./ep15-gperf-hash cross.input
	perf stat $(PERF_OPTS) ./ep15-gperf-hash-alloc cross.input

gperf: input.gperf
	gperf -c input.gperf > gperf-hash.c

ep15: ep15.c
	gcc $(C_OPTS) -o ep15 ep15.c

ep15-hash: ep15.c
	gcc $(C_OPTS) -DHASHTABLE -o ep15-hash ep15.c

ep15-gperf-hash: ep15.c
	gcc $(C_OPTS) -DHASHTABLE -DUSE_GPERF -o ep15-gperf-hash ep15.c

ep15-gperf-hash-alloc: ep15.c
	gcc $(C_OPTS) -DHASHTABLE -DUSE_GPERF -DUSE_CUSTOM_ALLOC -o ep15-gperf-hash-alloc ep15.c

dist:
	mkdir ep15
	ln ep15.c Makefile cross.input ep15
	tar cvfz ep15.tar.gz ep15
	rm -rf ep15

clean: 
	rm -f ep15 ep15-hash ep15-gperf-hash
