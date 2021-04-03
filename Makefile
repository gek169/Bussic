CC= g++
#CC= clang
CFLAGS= -O3 -lm -fopenmp -Wno-unused-function -Wno-absolute-value -std=c++11 -finline-limit=64000 -fno-math-errno

all: main

main:
	$(CC) $(CFLAGS) *.cpp -o bi.out 

clean:
	rm -f *.exe *.out *.o
