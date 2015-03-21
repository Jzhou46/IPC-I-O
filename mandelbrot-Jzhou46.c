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

pid_t child1;//child process #1
pid_t child2;//child process #2 
int fd[2];//pipe #1
int fd2[2];//pipe #2
int msqID1, msqID2, shmID;//Identifiers
int tmp, status, status2;
int options = 0;
struct rusage rusage;

//Signal Handler function to catch signals
void sigHandler(int signal){
	if(signal == SIGCHLD){
		wait4(child1, &status, options, &rusage);//Wait for the child1
		wait4(child2, &status2, options, &rusage);//Wait for the child2

		if (WIFEXITED(status)){
			//printf("Child1: status = %d, %s\n", status, "WIFEXITED");
			printf("\nThe number of mandelbrots created from mandelCalc: %d\n", WEXITSTATUS(status));
		}
		if (WIFEXITED(status)){
			//printf("Child2: status2 = %d, %s\n", status, "WIFEXITED");
			printf("The number of mandelbrots displayed from mandelDisplay: %d\n", WEXITSTATUS(status2));
		}
		printf("PROGRAM TERMINATED\n");
		exit(0);
	} 
}
struct message{
	long mtype;
	char text[256];
};

void removeAll(int msqID1, int msqID2, int shmID ){
	if(msgctl(msqID1, IPC_RMID, NULL) == -1){
		perror("msgctl failed\n");
		exit(0);
	}
	if(msgctl(msqID2, IPC_RMID, NULL) == -1){
		perror("msgctl failed\n");
		exit(0);
	}
	if(shmctl(shmID, IPC_RMID, NULL) == -1){
		perror("shmdt failed\n");
		exit(0);
	}
}

//Function to prompt the user if they are done
int Done(){
	printf("\nAre you done (enter yes or no) ?\n");
	char answer[5];
	int done = 0;
	do{
		scanf("%s", answer);
		if(strcmp(answer, "no") == 0)
			return 0;
		else if(strcmp(answer, "yes") == 0)
			return 1;
		else 
			done = 0;
	}
	while(!done);
	return 1;
}

//Function to receive the answer from Done()
int doneYet(int answer){
	return answer;
}

int main(int argc, char *argv[]){
	int i = 0;
	char msqIDstr[20], msqIDstr2[20], shmIDstr[20];//Variables to store string values
	char xMin[256], xMax[256], yMin[256], yMax[256], nRows[256], nCols[256], maxIters[256];//Variables to store string values
	double xmin, xmax, ymin, ymax;
	int nrows, ncols, maxiters;
	struct message msg, msg2, msg3;//Struct variables
	struct timeval time;
	char buf;
	int *sharedMemory;
	msg.mtype = 1;
	msg2.mtype = 1;
	msg3.mtype = 1;
	char *filename = argv[1];
	int answer = 0;
	printf("-------------------------\n");
	printf("  Jeffrey Zhou\n  Jzhou46\n");
	printf("-------------------------\n");
	if(argc < 2){//If argument count is less than 2
		printf("Not enought arguments\n");
		return 0; 
	}
	//Create pipe(s) 
	if(pipe(fd) == -1){//create pipe #1
		perror("ERROR: pipe failed\n");
		exit(0);
	}
	if(pipe(fd2) == -1){//create pipe #2
		perror("ERROR: pipe failed\n");
		exit(0);
	}	

	//Create message queue #1
	if((msqID1 = msgget(IPC_PRIVATE, 0600 | IPC_CREAT)) == -1){
		perror("ERROR: msgget failed\n");
		exit(0);
	}
	sprintf(msqIDstr, "%d\n", msqID1);//Convert int to string

	//Create message queue #2
	if((msqID2 = msgget(IPC_PRIVATE, 0600 | IPC_CREAT)) == -1){
		perror("ERROR: msgget failed\n");
		exit(0);
	}
	sprintf(msqIDstr2, "%d\n", msqID2);//Convert int to string

	//Create shared memory
	if((shmID = shmget(IPC_PRIVATE, sizeof(msg), 0600 | IPC_CREAT)) == -1){
		perror("ERROR: shmget failed\n");
		exit(0);
	}
	sprintf(shmIDstr, "%d\n", shmID);//Convert int to string
	char *array[] = {"./mandelCalc", shmIDstr, msqIDstr, NULL};//String array to be passed to Child1 to exec
	char *array2[] = {"./mandelDisplay", shmIDstr, msqIDstr, msqIDstr2, NULL};//String array to be passed to Child2 to exec
	child1 = fork();//Fork first child
	if(child1 < 0) {
		perror("ERROR: fork failed\n");
		exit(-1);
	}
	else if (child1 == 0){
		//Child1 work happens here
		close(fd[1]);
		dup2(fd[0],0);
		close(fd2[0]);
		dup2(fd2[1],1);
		execvp(array[0], array);//Execute command
		exit(0);
	}
	else{
		child2 = fork();//Fork second child
		if(child2 < 0) {
			perror("ERROR: fork failed\n");
			exit(-1);
		}
		else if (child2 == 0){
			//Child2 work happens here
			close(fd2[1]);
			dup2(fd2[0], 0);
			execvp(array2[0], array2);//Execute command
			exit(0);
		}
		else{	
			//Parent work continues here
			close(fd[0]);//Close the read end of pipe #1

			while(1){
				if(doneYet(answer) == 0){
					if (signal(SIGCHLD, sigHandler) == SIG_ERR)
						printf("\ncan't catch SIGCHLD\n");
					printf("\n------------------------------------\n");
					printf(" Inter-Process Communications & I/O\n");
					printf(" 	 Mandelbrot Program	   \n");
					printf("------------------------------------\n");
					
					//Prompt the user to enter values for each parameters
					printf("\nPlease enter a value for xMin:\n");
					scanf("%lf", &xmin);
					printf("Please enter a value for xMax:\n");
					scanf("%lf", &xmax);
					if(xmin > xmax){
						printf("ERROR: xMin can't be greater than xMax\n");
						return 0;
					}
					printf("Please enter a value for yMin:\n");
					scanf("%lf", &ymin);
					printf("Please enter a value for yMax:\n");
					scanf("%lf", &ymax);
					if(ymin > ymax){
						printf("ERROR: yMin can't be greater than yMax\n");
						return 0;
					}
					printf("Please enter a value for nRows:\n");
					scanf("%d", &nrows);
					if(nrows < 0){
						printf("ERROR: nRows can not be a negative value\n");
						return 0;
					}
					printf("Please enter a value for nCols:\n");
					scanf("%d", &ncols);
					if(ncols < 0){
						printf("ERROR: nCols can not be a negative value\n");
						return 0;
					}	
					printf("Please enter a value for maxIters:\n");
					scanf("%d", &maxiters);
					if(maxiters < 0){
						printf("ERROR: maxIters can not be a negative value\n");
						return 0;
					}

					sprintf(xMin, "%lf\n", xmin);//Convert int to string
					sprintf(xMax, "%lf\n", xmax);//Convert int to string
					sprintf(yMin, "%lf\n", ymin);//Convert int to string
					sprintf(yMax, "%lf\n", ymax);//Convert int to string
					sprintf(nRows, "%d\n", nrows);//Convert int to string
					sprintf(nCols, "%d\n", ncols);//Convert int to string
					sprintf(maxIters, "%d\n", maxiters);//Convert int to string

					//Write filename to message queue 2 
					strcpy(msg.text, filename);//Copy the name of the file to the text field of the struct
					if(msgsnd(msqID2, &msg, strlen(msg.text) + 1, 0) == -1){
						perror("ERROR: msgsnd failed from mandelbrot.c\n");
						removeAll(msqID1, msqID2, shmID);
						exit(0);
					}
					//Write xMin, xMax, yMin, yMax, nRows, nCols, and maxIters to pipe 
					write(fd[1], xMin, strlen(xMin));
					write(fd[1], xMax, strlen(xMax));
					write(fd[1], yMin, strlen(yMin));
					write(fd[1], yMax, strlen(xMax));
					write(fd[1], nRows, strlen(nRows));
					write(fd[1], nCols, strlen(nCols));
					write(fd[1], maxIters, strlen(maxIters));
					int done = 0;
					while(done != 2){
						done = 0;
						//Receive message from Child1
						if(msgrcv(msqID1, &msg2, 512, 1, 0) == -1){
							perror("ERROR: msgrcv failed from mandelbrot.c\n");
							removeAll(msqID1, msqID2, shmID);
							exit(0);
						}
						else if(strcmp(msg2.text, "DONE") == 0){
							done++;
						}
						//Receive message from Child2
						if(msgrcv(msqID1, &msg3, 512, 1, 0) == -1){
							perror("ERROR: msgrcv failed from mandelbrot.c\n");
							removeAll(msqID1, msqID2, shmID);
							exit(0);
						}
						else if(strcmp(msg3.text, "DONE") == 0){
							done++;
						}
						printf("Message received from Child1: %s\n", msg2.text);
						printf("Message received from Child2: %s\n", msg3.text);
					}//End of while loop
				}//End of if statement	
				else if(doneYet(answer) == 1){
					//Send SIGUSR1 signals to both children 
					kill( child1, SIGUSR1 );
					kill( child2, SIGUSR1 );
					//signal(SIGCHLD, SIG_IGN);
					//Wait for both children, and report exit codes
					wait4(child1, &status, options, &rusage);//Wait for the child1
					wait4(child2, &status2, options, &rusage);//Wait for the child2
					//signal(SIGCHLD, SIG_IGN);
					if (WIFEXITED(status)){
						//printf("Child1: status = %d, %s\n", status, "WIFEXITED");
						printf("\nThe number of mandelbrots created from mandelCalc: %d\n", WEXITSTATUS(status));
					}
					if (WIFEXITED(status2)){
						//printf("Child2: status2 = %d, %s\n", status2, "WIFEXITED");
						printf("The number of mandelbrots displayed from mandelDisplay: %d\n", WEXITSTATUS(status2));
					}
					//Remove message queues and shared memory
					sharedMemory = (int*)shmat(shmID, NULL, 0);
					shmdt(sharedMemory);
					removeAll(msqID1, msqID2, shmID);
					printf("PROGRAM TERMINATED\n");
					break;
				}//End of else
				answer = Done();
			}//End of while loop
		}
	}
	return 0;
}
