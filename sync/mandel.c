 /*program to draw the Mandelbrot Set on a 256-color xterm.
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include <signal.h>

#include "mandel-lib.h"

#define MANDEL_MAX_ITERATION 100000

/***************************
 * Compile-time parameters *
 ***************************/

#define perror_pthread(ret, msg) \
        do { errno = ret; perror(msg); } while (0)

void ResetAndExit(int sign)		/*if Ctrl-C stops the excecution reset the color of the terminal*/
{
	signal(sign, SIG_IGN);		//ignor signal
	reset_xterm_color(1);		//reset color
	exit(-1);			//exit
}

/*
 * Output at the terminal is is x_chars wide by y_chars long
*/
int y_chars = 50;
int x_chars = 90;
int n;										//Global metavlhth arithmou threads

typedef struct
{
	pthread_t tid;								//id gia kathe thread pou kanw create, oste na ginoun argotera join
	int l;									//to antistoixo line		
	sem_t mutex;								//to antistoixo semaphore metaksi line kai line+1
}mystruct;
mystruct *saved;								//malloc gia n sth main


/*
 * The part of the complex plane to be drawn:
 * upper left corner is (xmin, ymax), lower right corner is (xmax, ymin)
*/
double xmin = -1.8, xmax = 1.0;
double ymin = -1.0, ymax = 1.0;
	
/*
 * Every character in the final output is
 * xstep x ystep units wide on the complex plane.
 */
double xstep;
double ystep;

/*
 * This function computes a line of output
 * as an array of x_char color values.
 */
void compute_mandel_line(int line, int color_val[])
{
	/*
	 * x and y traverse the complex plane.
	 */
	double x, y;

	int n;
	int val;

	/* Find out the y value corresponding to this line */
	y = ymax - ystep * line;

	/* and iterate for all points on this line */
	for (x = xmin, n = 0; n < x_chars; x+= xstep, n++) {

		/* Compute the point's color value */
		val = mandel_iterations_at_point(x, y, MANDEL_MAX_ITERATION);
		if (val > 255)
			val = 255;

		/* And store it in the color_val[] array */
		val = xterm_color(val);
		color_val[n] = val;
	}
}

/*
 * This function outputs an array of x_char color values
 * to a 256-color xterm.
 */
void output_mandel_line(int fd, int color_val[])
{
	int i;
	
	char point ='@';
	char newline='\n';

	for (i = 0; i < x_chars; i++) {
		/* Set the current color, then output the point */
		set_xterm_color(fd, color_val[i]);
		if (write(fd, &point, 1) != 1) {
			perror("compute_and_output_mandel_line: write point");
			exit(1);
		}
	}

	/* Now that the line is done, output a newline character */
	if (write(fd, &newline, 1) != 1) {
		perror("compute_and_output_mandel_line: write newline");
		exit(1);
	}
}

void *compute_and_output_mandel_line(void *arg)
{
	int k;							//Trexousa grammh
	int line=*(int*)arg;					//Proth grammh tou Thread(0..n-1)
	for (k = line; k < y_chars; k += n){			//kai gia tis epomenes me vima n
		int color_val[x_chars];
		compute_mandel_line(k, color_val);              //Ypologismos k grammhs *parallhla
                                                                //Sygxronismos:
                sem_wait(&saved[(k % n)].mutex);                //perimenoun to semaphore tou Thread tous.
                output_mandel_line(1, color_val);               //Print *sigxronismena.
                sem_post(&saved[((k % n)+1)%n].mutex);          //Auksanoun to semaphore tou epomenou Thread.
	}
	return 0;
}


int main(int argc, char *argv[])
{	
	signal(SIGINT, ResetAndExit);						//An erthei Ctrl-C signal phgaine sthn ResetAndExit

	int line, ret;
	xstep = (xmax - xmin) / x_chars;
	ystep = (ymax - ymin) / y_chars;
	/*
	 * draw the Mandelbrot Set, one line at a time.
	 * Output is sent to file descriptor '1', i.e., standard output.
	 */

	//printf("Enter Number of Threads: ");					//Diavazw ton arithmo twn Threads
	//scanf("%d", &n);
  //      printf( "argc = %d\n", argc );
//	printf ("%s\n", argv[1]);
	
        n= atoi(argv[1]); 
	if ((n < 1) || (n > y_chars-1)){
		printf("invalid input \n");
		return -1;
	}
	printf("%d\n", n);
	saved = (mystruct*)malloc(n*sizeof(mystruct));				//malloc
	sem_init(&saved[0].mutex, 0, 1);					//To semaphore 0 ksekinaei prwto me value 1
	if (n > 1){								//an uparxoun polla Threads	
		for (line = 1; line < n; line++){				//tha exo alla tosa semaphores
			sem_init(&saved[line].mutex, 0, 0);			//me value 0 (waiting...)
		}
	}
	
	for (line = 0; line < n; line++) {					//Crate n threads kai kathe ena pernaei kai to 
		saved[line].l=line;						//antistoixo line gia na ksekinisei apo ekei
		ret = pthread_create(&saved[line].tid, NULL, compute_and_output_mandel_line, &saved[line].l);
		if (ret){
			perror_pthread(ret, "pthread_create");			//error check
                	exit(1);
		}
	}

	for (line = 0; line < n; line++) {					//Join ta threads sto telos
		ret = pthread_join(saved[line].tid, NULL);
		if (ret)
                	perror_pthread(ret, "pthread_join");			//error check
	}
	for (line = 0; line < n; line++) {					//Kai destroy ta semaphores
		sem_destroy(&saved[line].mutex);
	}

	reset_xterm_color(1);
	return 0;
}
