all: zing zing1
zing : zing.o main1.o
	gcc -Wall  main1.o zing.o -o zing

zing1: zing2.o main1.o
	gcc -Wall zing2.o main1.o -o zing1 
 
main1.o : main1.c
	gcc -Wall -c main1.c

zing2.o: zing2.c 
	 gcc -Wall -c zing2.c
