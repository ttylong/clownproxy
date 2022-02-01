.PHONY: all clean

all:	clownproxy

clownproxy: clownproxy.cpp
	g++ -O2 -Wall clownproxy.cpp -o clown

clean:
	-/bin/rm -f clownproxy *.o *~