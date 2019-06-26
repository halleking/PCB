/* 
Halle King
hking

CIS415 Project 1

This is my own work, with the exception that I borrowed some code from lab code. 
I used the internet as a reference for learning about system calls and signals and how to use them.
Examples of these functions that I found online helped me with my code.


*/


#include "p1fxns.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <zconf.h>
#include <sys/types.h>
#include <sys/wait.h>

#define NOTRUN 0
#define RUNNING 1
#define PAUSED 2
#define EXITED 3

struct ProcessControlBlock{
	char * command;
	char ** args;
	pid_t PID;
	int status;
	struct ProcessControlBlock *next;
};

int MAXBUFF = 1000;
int MAXARG = 100;
int total_procs = 0;
int active_procs = 0;
struct ProcessControlBlock *Processes = NULL;
struct ProcessControlBlock *HeadNode =NULL;

//initialize PCBs in Process array
struct ProcessControlBlock *InitializePCB(char *command, char **args){

	struct ProcessControlBlock *Process = (struct ProcessControlBlock*)malloc(sizeof(struct ProcessControlBlock));
	Process->command = command;
	Process->args = args;
	Process->status = NOTRUN;
	Process->PID = -1;
	Process->next = NULL;
	return Process;
}

//append process to linked list of processes
void AppendProcess(struct ProcessControlBlock **head, struct ProcessControlBlock *New_Process){
	
	if (*head == NULL){
		*head = New_Process;
		HeadNode = New_Process;
		return;
	}
	
	while ((*head)->next != NULL){
		*head = (*head)->next;
	}
	(*head)->next = New_Process;	
}


//free allocated memory and reset variables 
void FreeAllPCBs(){

	struct ProcessControlBlock *current = HeadNode;
	struct ProcessControlBlock *next;
	while (current != NULL){
		free(current->command);
		int i; 
		for (i = 0; current->args[i] != NULL; i++){
			free(current->args[i]);
		}
		free(current->args);
		next = current->next;
		free(current);
		current = next;
	}		
}

void LaunchAllPCB(){
	pid_t PID;

	struct ProcessControlBlock *HeadNodeCopy = HeadNode;
	while (HeadNodeCopy != NULL){
		PID = fork();
		HeadNodeCopy->PID = PID;
		switch(PID){
			//error when forking
			case -1: 
				p1putstr(2,"Error forking new process.\n");
				FreeAllPCBs();
				break;
			//child program
			case 0: 		
				execvp(HeadNodeCopy->args[0], HeadNodeCopy->args);
				//if execvp() returns, it must have failed
				p1putstr(2, "Error: execvp() failed.\n");
				FreeAllPCBs();
				_exit(-1);
				break;		
		}
		//keep track of active processes
		active_procs++;
		HeadNodeCopy = HeadNodeCopy->next;
	}
	total_procs = active_procs;
}


//return the number of processes read from workload file
int CountProcesses(char *file){
	int count = 0;
	char buff[MAXBUFF];
	FILE *input = fopen(file, "r");
	while (fgets(buff, 100, input)){
		count++;
	}
	fclose(input);
	return count;

}

//return the number of words from a line
int CountWords(char *array){
	
	int i = 0; 
	int count = 0;
	char temp[1024];
	
	i = p1getword(array, i, temp);
	while(i != -1){
		count++;
		i = p1getword(array, i, temp);
	}
	return count;
}

int main(int argc, char *argv[]){

	//if no file given or too many arguments given, exit
	if (argc > 2){
		p1putstr(2, "Error: Wrong number or arguments. Enter 1 workload file name only.\n");
		exit(0);
	}

	//open file
	int fd = 0;	
	int i = 0;
	char buff[MAXBUFF];
	fd = open(argv[1],O_RDONLY);

	//check for bad file
	if (fd == -1){
		p1putstr(2, "Unable to open file.\n");
		close(fd);
		exit(0);
	}

	//Count the total number of processes 
	total_procs = CountProcesses(argv[1]);

	
	//loop through lines in file
	while (p1getline(fd, buff, MAXBUFF) != 0){
		
		int len = p1strlen(buff);
		buff[len-1] = '\0';

		//get number of words in line, add 1 for terminating character
		int num_args = CountWords(buff)+1;

		//allocate args array
		char **args = (char **)malloc(num_args * sizeof(char*));
		for (i = 0; i < num_args; i++){
			args[i] = (char *)malloc(MAXARG * sizeof(char));
		}
 		
		//get args in string 
		int j = 0;
		for (i = 0; i < num_args; i++){
			j = p1getword(buff, j, args[i]);
		}
		 
		//get length of command, allocate char array
		int cmd_len = p1strlen(args[0]);
		char *command = (char *)malloc(cmd_len * sizeof(char)+1);

		//set last element of args array to EOS character
		free(args[num_args-1]);	
		args[num_args-1] = '\0';
		
		//copy command
		p1strcpy(command,args[0]);
		

		//create new process
		struct ProcessControlBlock *New_Process = InitializePCB(command, args);


		//add new process to Processes array
		AppendProcess(&Processes,New_Process);

		
	}
	//run all processes
	LaunchAllPCB();
	
	//wait for all child processes to terminate
	while (total_procs > 0){
		wait(NULL);
		total_procs--;
	}

	//all programs successfully run, clean up processes
	FreeAllPCBs();
	close(fd);
	return 0;
}
