all : specterjs

specterjs : specterjs.o
	gcc -o specterjs specterjs.o `pkg-config --libs webkit-1.0`

specterjs.o : specterjs.c
	gcc -Wall -c -o specterjs.o specterjs.c `pkg-config --cflags webkit-1.0`

clean :
	-rm specterjs specterjs.o
