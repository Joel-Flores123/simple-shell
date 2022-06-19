//libraries
//to compile do
//gcc sish.c -o sish -Wall -Werror -std=c99
//
#define  _GNU_SOURCE //to be able to use getline
#include <unistd.h> //for fork
#include <sys/types.h> //for fork
#include <sys/wait.h> //for wait
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h> //to check if file descriptors work

//prototype for the isDigit function 

int isDigit(char* string);

//prototype for print history

void printHistory(char* history[]);

//prototype for adding a record to our history

void addRecord(char* newCmd, char* history[]);

//prototype for clearing history

void clearHistory(char* history[]);

//prototype for count pipes

int countPipes(char* buffer);

//prototype for pipe commands

void pipeCommands(int oldfd[], char* buffer, int pipes, char* save);

//main function will have a while loop that continously asks for user input

int main(int argc, char* argv[])
{

	size_t buffSize = 40;
	char* buffer = (char*)(malloc(sizeof(char) * 40)); //buffer to get user input
	size_t running = 1; // our flag for running our shell
	char** myargs;
	char** his;//history
	int hFlag = 0; //if this is 1 that means that the history command happened and there is no need to execute
	int rc;
	int fd[2];
	size_t ARGNUM = 15;
	size_t hSize = 100;
	char* token = NULL; //stores our tokens
	char* save; //this will be used to save our spot during piping
	
	his = (char**)(malloc(sizeof(char*) * hSize));
	clearHistory(his); //makes everything null in the history array
	
	while(running == 1) //while running
	{
	
		printf("sish> "); //prints shell line 
		getline(&buffer, &buffSize, stdin); //get user input
		buffer[strlen(buffer) - 1] = ' '; //get rid of the newline and make a delimeter	
		addRecord(buffer, his); //add a record
		int pipes = countPipes(buffer);
		token = strtok_r(buffer, " ", &save);// will get tokens 
		
		if(token != NULL && strcmp(token, "exit") == 0) //if we see exit we stop running
                {

                        running = 0; //we stop running

                }
		else
		if(token != NULL && strcmp(token,"cd") == 0) //if we see cd
		{
		
			char* dir = strtok_r(NULL, " ", &save);
			
			if(dir != NULL) //make sure we got input	
			{
			
				if(chdir(dir) != 0)
				{
				
					printf("%s not valid\n", dir);
				
				}
			
			}
			else
			{
				
				puts("correct input is cd [path]");
			
			}

		}
		else //we drop into a loop of tokens
		{

			if(token != NULL && strcmp(token, "history") == 0) //if this is a history command
			{
			
				token = strtok_r(NULL, " ", &save); //get next token
				
				if(token != NULL && strcmp(token, "-c") == 0) //if it is a clear command we clear
				{
				
					clearHistory(his);
					hFlag = 1;

				}
				else
				if(token != NULL && isDigit(token) != 0)
				{
				
					buffer = his[atoi(token)]; //set buffer to what is in the history
					pipes = countPipes(buffer); //count pipes
					token = strtok_r(buffer, " ", &save); //make this the new place we get tokens from

				}
				else
				if(token == NULL) //print history
				{
					
					printHistory(his);
					hFlag = 1;
				
				}
			
			}

			if(hFlag != 1) //if we did not run into a built in history command unrelated to executing
			{
				if(pipes == 0) //if no pipes go as normal
				{
		
					int counter = 0;
					myargs = (char**)(malloc(sizeof(char*) * ARGNUM)); // allocate memory 

					while(token != NULL) //while we still have tokens
					{
			
						myargs [counter] = token;
						token = strtok_r(NULL, " ", &save);// get the next token
						counter++;

					}
			
					myargs[counter] = NULL; //make final token null	
			
					rc = fork(); //fork
			
					if(rc == 0) //if child
					{
			
						execvp(myargs[0], myargs); // execute
						puts("Token unrecognized");
						running = 0; //if we get here there that means that we didnt have anything executed
			
					}
					else
					{
			
						wait(NULL);
			
					}
				
					free(myargs); //free my args
			
				}
				else //if we have pipes
				{
		
					int counter = 0;
                                	myargs = (char**)( malloc(sizeof(char*) * ARGNUM)); // allocate memory 

                                	while(token != NULL && strcmp(token, "|") != 0) //while we still have tokens or we reach a pipe
                                	{

                                        	myargs [counter] = token;
                                        	token = strtok_r(NULL, " ", &save);// get the next token
                                        	counter++;

                                	}

                               		myargs[counter] = NULL; //make final token null 
					pipes--; //we are now going to pipe

					if(pipe(fd) != -1) //make sure pipe works
					{
				
						rc = fork();

						if(rc == 0)
						{
						
							close(fd[0]);//we arent reading
						
							if(dup2(fd[1], STDOUT_FILENO) != -1) //check if dup2 works
							{
							
								close(fd[1]);//no longer necessary
								execvp(myargs[0], myargs);
								puts("Unrecognized token"); //error message
								running = 0;

							}
							else
							{
							
								running = 0; //exit if error
						
							}

						}
						else
						{
						
							pipeCommands(fd, buffer, pipes, save); //call pipe commands	
							wait(NULL); //wait for the end

						}

				
					}

					free(myargs); //free my args
			
				}
			
			}
			else
			{
				
				hFlag = 0; //reset hFlag

			}

		}

	}

	free(his); //free history

	return 0;

}	

//given a string will check if the given string is a digit or not

int isDigit(char* string)
{
	int valid = 1;
	int counter = 0;
	
	while(counter < strlen(string) && valid == 1) //while string is valid or not at the end of the string
	{
		
		if(string[counter] < '0' || string[counter] > '9') //check if there is anything wrong
		{
			
			valid = 0;
		
		}
	
		counter++;
	}

	return valid;
	
}

//given a string will add it to the next available area in the array
//and then if the array is filled will shift all the commands forward by one

void addRecord(char* newCmd, char** history)
{

	int counter = 0;

	while(history[counter] != NULL && counter < 100) //look for the next empty spot but make sure it is in bounds
	{
		
		counter++;

	}

	if(history[counter] == NULL) //in the empty spot add the new cmd
	{
		
		history[counter] = (char*) (malloc(sizeof(char) * 40));	
		strcpy(history[counter], newCmd);
	
	}
	else
	if(counter >= 100) //if there was no empty spot start shifting 
	{
		
		counter = 0;

		while(counter < 99) //while not at the end shift everything forward
		{
			
			strcpy(history[counter], history[counter + 1]);
			counter++;

		}

		history[counter] = (char*) (malloc(sizeof(char) * 40));	
		strcpy(history[counter], newCmd); //now at the end we add the new command
	
	}

}

//prints history given history array

void printHistory(char** history)
{

	int counter = 0;
	
	while(history[counter] != NULL && counter < 100) //while not at the end or null
	{
		
		printf("%d %s\n", counter, history[counter]); //print
		counter++; //increment
	
	}

}


//clears history we know history is size 100 so we can hard code this 

void clearHistory(char** history)
{
	
	int size = 100;
	int counter = 0;

	while(counter < size) //while not at the last element
	{
		
		if(history[counter] != NULL)
		{
		
			free(history[counter]); //to prevent memory leak
		
		}

		history[counter] = NULL; // make it null
		counter++;
	
	}

}

//takes a file descriptor, buffer to pipe commands, save pointer ,
//and the amount of pipes remaining if something went wrong return 0 

void pipeCommands(int oldfd[], char* buffer, int pipes, char* save)  
{
	
	int rc; //to tell our parent from child
	char** myargs; //holds arguments
	char* token; //holds tokens
	int ARGNUM = 15;

	if(fcntl(oldfd[0], F_GETFD) != -1 && fcntl(oldfd[1],F_GETFD) != -1) //check if file descriptor works
	{
	
		token = strtok_r(NULL, " ", &save); //start collecting tokens
		myargs = malloc(sizeof(char*) * ARGNUM); // allocate memory 
		int counter = 0;

		while(token != NULL && strcmp(token, "|") != 0) //while token is not a pipe or null
		{
			myargs [counter] = token; //put arguments in
			token = strtok_r(NULL, " ", &save); //start collecting tokens
			counter++;

		}

		myargs[counter] = NULL; //make end null to make ending more easy

		if(pipes == 0) //base case 
		{
			

			rc = fork();
			
			if(rc == 0)
			{

				close(oldfd[1]);//we are not writing to another pipe
			
				if(dup2(oldfd[0], STDIN_FILENO) != -1) //make sure our dup2 works
				{

					close(oldfd[0]);//no longer needed because dup2
					execvp(myargs[0], myargs);//execute command
					puts("Unrecognized token"); //print error if nothing happened
				
				}
				
				puts("Error in dup2");
				exit(EXIT_FAILURE); //exit failure
			
			}
			else
			{	
			
				wait(NULL);
				close(oldfd[0]); //close old fds
				close(oldfd[1]);
			
			}

			
		}
		else //if we have more pipes 
		{
			
			pipes--; //decrement pipes
			int newfd[2]; //make a new fd
			
			if(pipe(newfd) != -1) //make sure the pipe works
			{

				rc = fork();

                        	if(rc == 0)
                        	{

                                	close(oldfd[1]);//we are not writing to the old pipe

                                	if(dup2(oldfd[0], STDIN_FILENO) != -1) //make sure our dup2 works
                                	{
						
                                        	close(oldfd[0]);//no longer needed because dup2
						close(newfd[0]); //we are not reading

						if(dup2(newfd[1], STDOUT_FILENO) != -1)
						{
						
							close(newfd[1]); //no longer needed because dup2
                                        		execvp(myargs[0], myargs);//execute command
                                			puts("Unrecognized token"); //print error if nothing happened
						
						}
						else
						{
						
							puts("failure in dup2");
							exit(EXIT_FAILURE); //exit failure
						
						}

                                	}
					
					puts("failure in dup2");
					exit(EXIT_FAILURE); //exit failure

                        	}
                       		else
                        	{
					
					close(oldfd[0]);//no longer needed
					close(oldfd[1]);	
					pipeCommands(newfd, buffer, pipes, save); //recursive call
                                	wait(NULL); //wait for end

                        	}
			
			}
			else
			{
			
				puts("Error in pipe");
			
			}

		
		}
	
		free(myargs);//free myargs so we dont have a memory leak

	}
	
}
//returns the amount of pipes in a line

int countPipes(char* buffer)
{

	int pipes = 0; //counts pipes
	int counter = 0; //will count where we are
	int length = (int)(strlen(buffer)); //get length of string
	
	while(counter < length) //while still reading the string
	{
		
		if(buffer[counter] == '|') //check if token is a pipe
		{
		
			pipes++; //we found a pipe so we count it
		
		}
		
		counter++;
		
	}

	return pipes; //return pipes

}
