ghc: ghc.o ghcontrol.o sensehat.o
#	g++ -g3 -o ghc ghc.o ghcontrol.o sensehat.o -lpython2.7
	g++ -g -o ghc ghc.o ghcontrol.o sensehat.o -lRTIMULib
ghc.o: ghc.c ghcontrol.h sensehat.h
	g++ -g -c ghc.c
ghcontrol.o: ghcontrol.c ghcontrol.h
	g++ -g -c ghcontrol.c
sensehat.o: sensehat.cpp sensehat.h
	g++ -g -c sensehat.cpp
clean:
	touch *
	rm *.o

