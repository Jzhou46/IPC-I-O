//
// Jeffrey Zhou
// JZhou46
// CS 361
// Inter-Process Communications & I/O
// 11/16/14
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <setjmp.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>

double deltaX, deltaY, xMax, xMin, yMax, yMin;
int nRows, nCols, maxIters;
int mandelbrotCount = 0;

//Signal Handler function to catch signals
void sigHandler(int signal){
        if(signal == SIGUSR1){
                exit(mandelbrotCount);
        }
}

struct message{
        long mtype;
        char text[256];
};

//Function to create the mandelbrot in shared memory
void createMandelbrot(int *sharedMemory){
	deltaX = (xMax - xMin)/(nCols - 1);
	deltaY = (yMax - yMin)/(nRows - 1);
	int r, c, n;
	double Cy, Cx, Zx, Zy, Zx_next, Zy_next; 
	for( r = 0; r <  nRows; r++ ) { 
		Cy = yMin + r * deltaY; 
		for( c = 0; c < nCols; c++ ) { 
			Cx = xMin + c * deltaX; 
			Zx = Zy = 0.0 ;
			for( n = 0; n < maxIters; n++ ) { 
				if( Zx * Zx + Zy * Zy >= 4.0 ) 
					break;
				Zx_next = Zx * Zx - Zy * Zy + Cx;
				Zy_next = 2.0 * Zx * Zy + Cy; 
				Zx = Zx_next; 
				Zy = Zy_next; 
			} 
			if( n >= maxIters )
				* ( sharedMemory + r * nCols + c ) = -1;
			else  
				* ( sharedMemory + r * nCols + c ) = n;	
		} 
	}
}

int main(int argc, char *argv[]){
	int i, shmID, msgqID;
	char xmin[255], xmax[255], ymin[255], ymax[255], nrows[255], ncols[255], maxiters[255];
	struct message msg;
	msg.mtype = 1;
	int *sharedMemory;

	//Parse shmid and msgid from command line arguments.
	shmID = atoi(argv[1]);
	msgqID = atoi(argv[2]);
	while(1){
		if (signal(SIGUSR1, sigHandler) == SIG_ERR){
			char error[] = "ncan't catch SIGUSR1\n";
            		write(1, error, sizeof(strlen(error) + 1));
		}
		
		//read xMin, xMax, yMin, yMax, nRows, nCols, and maxIters from stdin
		fgets(xmin, 255, stdin);
		fgets(xmax, 255, stdin);
		fgets(ymin, 255, stdin);
		fgets(ymax, 255, stdin);
		fgets(nrows, 255, stdin);
		fgets(ncols, 255, stdin);
		fgets(maxiters, 255, stdin);
		xMin = atof(xmin);
		xMax = atof(xmax);
		yMin = atof(ymin);
		yMax = atof(ymax);
		nRows = atoi(nrows);
		nCols = atoi(ncols);
		maxIters = atoi(maxiters);

		//Implement Mandelbrot algorithm to fill shared memory
		sharedMemory = (int *)shmat(shmID, NULL, 0);
		createMandelbrot(sharedMemory);

		//Write xMin, xMax, yMin, yMax, nRows, nCols, and maxIters to pipe
		write(1, xmin, strlen(xmin));
		write(1, xmax, strlen(xmax));
		write(1, ymin, strlen(ymin));
		write(1, ymax, strlen(xmax));
		write(1, nrows, strlen(nrows));
		write(1, ncols, strlen(ncols));
		
		//Send message from Child1 to parent
		strcpy(msg.text, "DONE");
		if(msgsnd(msgqID, &msg, strlen(msg.text) + 1, 0) == -1){
			perror("ERROR: msgsnd failed mandelCalc.c\n");
			exit(0);
		}

	mandelbrotCount++;
	}//End of while loop
	return 0;
}
