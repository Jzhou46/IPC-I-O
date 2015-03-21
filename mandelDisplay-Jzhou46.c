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
int nRows, nCols;
char* filename;
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

//Function to display the mandelbrot
void displayMandelbrot(int *sharedMemory){
	FILE *fp;
	fp = fopen(filename, "w");
	if(fp == NULL){
		perror("File can not be opened\n");
		exit(0);
	}
	int nColors = 15;
	char colors[ 15 ] = ".-~:+*%O8&?$@#X";
	int r, c, n;
	char space = ' ';
	for (r = nRows - 1; r > 0; r--){
		for(c = 0; c < nCols; c++){
			n = *( sharedMemory + r * nCols + c ); /* = data[r][c] */
			if( n < 0 ){
				printf(" "); // Typically black with real pixels.
				fprintf(fp, "%c", space);
			}
			else{
				printf("%c", colors[ n % nColors ]);
				fprintf(fp, "%c", colors[n % nColors]);
			}
		}
		printf("\n");
		fprintf(fp, "\n");
	}
	fclose(fp);
}


int main(int argc, char *argv[]){
	int i, shmID, msqID1, msqID2;
	struct message msg, msg2;
	msg.mtype = 1;
	msg2.mtype = 1;
	char xmin[255], xmax[255], ymin[255], ymax[255], nrows[255], ncols[255];
	int *sharedMemory;

	//Parse shmid and 2 msgids from command line arguments.
	shmID = atoi(argv[1]);
	msqID1 = atoi(argv[2]);
	msqID2 = atoi(argv[3]);
	while(1){
		//read xMin, xMax, yMin, yMax, nRows, nCols, and maxIters from stdin
		if (signal(SIGUSR1, sigHandler) == SIG_ERR){
			 perror( "ncan't catch SIGUSR1\n");
		}
		fgets(xmin, 255, stdin);
		fgets(xmax, 255, stdin);
		fgets(ymin, 255, stdin);
		fgets(ymax, 255, stdin);
		fgets(nrows, 255, stdin);
		fgets(ncols, 255, stdin);

		//Converting characters to floats
		xMin = atof(xmin);
		xMax = atof(xmax);
		yMin = atof(ymin);
		yMax = atof(ymax);
		nRows = atoi(nrows);
		nCols = atoi(ncols);

		//Read filename from message queue 2 and open file
		if(msgrcv(msqID2, &msg, 512, 1, 0) == -1){
			perror("ERROR: msgrcv failed from mandelDisplay.c\n");
			exit(0);
		}
		filename = msg.text;

		//Read data from shared memory and display image on screen 
		sharedMemory = (int *)shmat(shmID, NULL, 0);
		displayMandelbrot(sharedMemory);

		//Write done message to message queue 1
		strcpy(msg2.text, "DONE");
		if(msgsnd(msqID1, &msg2, strlen(msg2.text) + 1, 0) == -1){
			perror("ERROR: msgsnd failed mandelDisplay.c\n");
			exit(0);
		}
	
	mandelbrotCount++;
	}//End of while loop
	return 0;
}

