#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>


void findCom(char *);
void simpleCom(char* name[], int num);
void builtIns(char* name[], int nums);
void forGRedirect(char* fullInput[], int stdOutInd, int stdInInd, int nums);
int removeTwo(char* fullInput[], int index, int num);
void checkBack();
void backGround(char *fullInput[], int stdOutInd, int stdInInd, int numOfArgs);


//two globabl ints used to track the exits status
int forGroundExitStat = 0;
int forGroundSigStat= 0;
int exitBool =0; //0 is normal exit status, 1 means signal exit status was used


int ctrlZ = 0;

//method provided in the  lectures
void catchSIGINT(int signo)
{
	//char* message = "SIGINT. Use CTRL-Z to Stop.\n";
	//write(STDOUT_FILENO, message, 28);
	
}

void catchSIGTSTP(int signo)
{
	//enter foreground only mode
	if(ctrlZ==0)
	{
		char* notAllowed = "\nEntering foreground-only mode (& is now ignored)\n";
		fflush(stdout);
		write(STDOUT_FILENO, notAllowed, 51);
		ctrlZ=1;
	}
	//exit fg only mode
	else
	{
		char* allowed = "\nExiting foreground-only mode\n";
		fflush(stdout);
		write(STDOUT_FILENO, allowed, 31);	
		ctrlZ=0;
	}	
}



void main()
{

	//create the sigaction struct for SIGTSTP
  	struct sigaction SIGTSTP_action={0};


	SIGTSTP_action.sa_handler = catchSIGTSTP;
      	sigfillset(&SIGTSTP_action.sa_mask);

      	sigaction(SIGTSTP, &SIGTSTP_action, NULL);

	signal(SIGINT, SIG_IGN);


        int numCharsEntered = -5; // How many chars we entered
	int currChar = -5; // Tracks where we are when we print out every char
	size_t bufferSize = 0; // Holds how large the allocated buffer is
	char* lineEntered = NULL; // Points to a buffer allocated by getline() that holds our entered string + \n + \0
	char* command, *remainder;	
	char delimiter[]=" ";

	//loop until exited
  	while(1)
	{
		//loop showing the prompt	      
		while(1)
		{
			//print the prompt and get input from the user
			printf(":");
			numCharsEntered = getline(&lineEntered, &bufferSize, stdin);
			if (numCharsEntered == -1)
				clearerr(stdin);
			else
				break; // Exit the loop - we've got input
		}

		//replace the newline with a end of string		 
		lineEntered[strcspn(lineEntered, "\n")] = '\0';

		
		//check for comment
		if(lineEntered[0]!='#'&&lineEntered[0]!='\0')
		{
			//call the function to figure out what command was passed
			findCom(lineEntered);
			
		}
		fflush(stdout);

		//check for background processes that have finished
		checkBack();
		
		//clean up then loop again
		free(lineEntered);
		lineEntered = NULL;
		
	}

}
//check to see if any background processes have terminated
void checkBack()
{
	//check for any process that has finished
	int childPid, childES;
	childPid = waitpid(-1, &childES, WNOHANG);

	//loop to check for multiple finished background processes
	while(childPid>0)
	{
		//figure out how the child process ternimnated
		if (WIFEXITED(childES))
		{
			//the process was exited normally
			int backExitStat = WEXITSTATUS(childES);
			printf("Background process %d has terminated with a exit status of %d\n",childPid, backExitStat);
			fflush(stdout);
		}
		//else it was terinmated by a signal
		else
		{
			//get the signal term. value
			int backSigStat= WTERMSIG(childES);
			printf("Background process %d has terminated with a exit signal status of %d\n",childPid, backSigStat);	
			fflush(stdout);
		}
		//check for another finished process
		childPid = waitpid(-1, &childES, WNOHANG);

	}


}
//function to replacee the process id in the commands
void processID(char *fullInput[], int numOfArgs, int arr[])
{
	//i doubt process ids will exceed 40 digits
	char pid[40];
	int size = sprintf(pid, "%ld\0", (long)getpid());
	
	
	//loop there every word
	int i=0;
	for(i;i<numOfArgs;i++)
	{
		int index=-1;
		//if the wor contains $$
		if(strstr(fullInput[i], "$$")!=NULL)	
		{
			//set arr to allow for the memory to be cleaned
			arr[i]=1;

			char *newString = malloc(strlen(fullInput[i])-2+size);
			
			//loop through the chars of the old word
			int x=0;
		   	 while (*fullInput[i]) 
        		{ 
				//once $$ is reach input it, increment both indexes
        			if (strstr(fullInput[i], "$$") == fullInput[i]) 
        			{ 
            				strcpy(&newString[x], pid); 
            				x += size; 
            				fullInput[i] += 2; 
        			} 
				//just copy the char
        			else
	            			newString[x++] = *fullInput[i]++; 
    			} 
	      		//append the endd string char and reassign 
	    		newString[x] = '\0'; 	    
			fullInput[i]=newString;
		}
	}	



}

//this function creates an array out of the input, then determines which 
//function needs to be called
void findCom (char* lineEntered)
{
	//this section of code processes the user input, parsing the words into
	//an array of strings.
	
	//bool to track whether or not it will be a foreground process
	int backG=0;
	//these two ints track the index of and redirection chars(-1=none
	int stdOutInd=-1;
	int stdInInd=-1;

	//array used to track what memory was created in the heap
	int *arr;

	char* command, *remainder;	
	char delimiter[]=" ";

	//array to hold all words(max 512)
	char* fullInput[513];

	//get the first word from the input
	fullInput[0] = strtok_r(lineEntered, delimiter, &remainder);
	int numOfArgs=1, orginalNum;

	//while there are more words
	while(strlen(remainder)!=0)	
	{
		//get the next word
		fullInput[numOfArgs] = strtok_r(NULL, delimiter, &remainder);
		
		//if statements used to track redirection
		if(strcmp(fullInput[numOfArgs],"<")==0)
		{
			stdInInd=numOfArgs;	
		}
		else if(strcmp(fullInput[numOfArgs],">")==0)
		{
			stdOutInd=numOfArgs;
		}

		numOfArgs++;
	}
	//set the last value to null to prevent errors
	fullInput[numOfArgs]=NULL;

	//used in other functions, since redirection can stip out args
	orginalNum=numOfArgs;

	//make an array to track malloced memory
	arr = malloc(sizeof(int)*numOfArgs);

	int i =0;
	for(i;i<orginalNum;i++)
	{
		arr[i]=0;
	}

	//expand $$
	processID(fullInput, numOfArgs, arr);

	//set the background bool if the last character is a &
	if(strcmp(fullInput[numOfArgs-1],"&")==0)
		backG=1;

	//check for built in functions
	if(strcmp(fullInput[0], "cd")==0 || strcmp(fullInput[0], "exit")==0 || strcmp(fullInput[0], "status")==0)
	{
		builtIns(fullInput, numOfArgs);	
	}
	//check for background process
	else if(backG==1)
	{
		//remove the &
		fullInput[numOfArgs-1]=NULL;
		numOfArgs--;
		//check if it is in fg only mode, call the corresponding function
		if(ctrlZ==0)
		{
			backGround(fullInput, stdOutInd, stdInInd, numOfArgs);
		}
		else
		{
			forGRedirect(fullInput, stdOutInd, stdInInd, numOfArgs);
		}
	}

	//section for fg commands
	else
	{	
		forGRedirect(fullInput, stdOutInd, stdInInd, numOfArgs);
	}
	//clean up the mallocd memory1
	i =0;
	for(i;i<orginalNum;i++)
	{
		if(arr[i]==1)
		{
			free(fullInput[i]);
		}
	}
	free(arr);
	
}

//function to handle creating back ground processes
void backGround(char *fullInput[], int stdOutInd, int stdInInd, int numOfArgs)
{
	pid_t spawnPid = -5;
	int childES =-5, result;

	//spawn the backgroundd process
	spawnPid = fork();

	switch(spawnPid)
	{
		case -1: 
			perror("error encountered!\n");
			exit(1);
			break;

		case 0:
			//make the backround process ignore sigstp other wise it will ccause the process to end
			signal(SIGTSTP, SIG_IGN);

			//check for redirections
			if(stdOutInd >=0)
			{
				//redirect the std out
				int targetFD = open(fullInput[stdOutInd+1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
				if (targetFD == -1) { perror("open()"); exit(1); }
				result = dup2(targetFD, 1);
				if (result == -1) { perror("target dup2()"); exit(2); }
				//fix the array
				numOfArgs = removeTwo(fullInput, stdOutInd, numOfArgs);

			}
			//default redirect stdout to dev/null
			else
			{
				int devNull = open("/dev/null", O_WRONLY);				
				if (devNull == -1) { perror("open()"); exit(1); }
				result = dup2(devNull, 1);
				if (result == -1) { perror("target dup2()"); exit(2); }

			}
			//check for redirection of stdin		
			if(stdInInd >=0)
			{

				//check to see if the index needs to be adjusted from the std out if statment
				if(stdOutInd>=0 && stdOutInd<stdInInd)
				{
					stdInInd = stdInInd-2;
				}

				//use dup2 to redirect stdin
				int sourceFD = open(fullInput[stdInInd+1], O_RDONLY);
				if (sourceFD == -1) { perror("source open()"); exit(1); }
				result = dup2(sourceFD, 0);
				if (result == -1) { perror("source dup2()"); exit(2); }
				numOfArgs = removeTwo(fullInput, stdInInd, numOfArgs);

			}
			//default stdin to devnull
			else
			{
				int devNull2 = open("/dev/null", O_RDONLY);				
				if (devNull2 == -1) { perror("open()"); exit(1); }
				result = dup2(devNull2, 0);
				if (result == -1) { perror("target dup2()"); exit(2); }
			}


			fflush(stdout);

			

			execvp(fullInput[0], fullInput);
			//code shouldnt reach this point
			perror("");
			fflush(stdout);
			exit(1);
			break;
		default:
			//parent prints the bg pid
			printf("background pid is %d\n", spawnPid);
			fflush(stdout);

			break;
	}
}

//function that handles foreground processes
void forGRedirect(char* fullInput[], int stdOutInd, int stdInInd, int numOfArgs)
{

	pid_t spawnPid = -5;
	int childES =-5, result;

	
	//block ctrlz using a sig set we want to keep track if ctrl z was pressed
	
	sigset_t x;
	sigemptyset(&x);
	sigaddset(&x, SIGTSTP);
	sigprocmask(SIG_BLOCK, &x, NULL);

	//create the child process
	spawnPid = fork();

	switch(spawnPid)
	{
		case -1: 
			perror("error encountered!\n");
			fflush(stdout);
			exit(1);
			break;

		case 0:
			//unblock crtl c andd allow default behaviour	
			signal(SIGINT, SIG_DFL);

			//check to see if stdout needs to be redirected
			if(stdOutInd >=0)
			{
				//redirect the std out using dup2
				int targetFD = open(fullInput[stdOutInd+1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
				if (targetFD == -1) { 
					perror("cannot open file for output"); exit(1);
			       		fflush(stdout);
				}
				result = dup2(targetFD, 1);
				if (result == -1) { 
					perror("target dup2()"); exit(2); 
					fflush(stdout);
				}
				//fix the array(remove the file and the arrow)
				numOfArgs = removeTwo(fullInput, stdOutInd, numOfArgs);

			}
			//check to see if stdin needs to be redirected		
			if(stdInInd >=0)
			{
				//check to see if the index needs to be adjusted from the std out if statment
				if(stdOutInd>=0 && stdOutInd<stdInInd)
				{
					//shift the index to acount for th etwo stdout variables
					stdInInd = stdInInd-2;
				}

				//open the input file
				int sourceFD = open(fullInput[stdInInd+1], O_RDONLY);
				if (sourceFD == -1) { 
					perror("Cannot open file for input"); 
					fflush(stdout);
					exit(1); 
				}
				//use dup2 to redirect stdin
				result = dup2(sourceFD, 0);
				if (result == -1) { 
					perror("source dup2()"); exit(2); 
					fflush(stdout);
				}
				//clean the command
				numOfArgs = removeTwo(fullInput, stdInInd, numOfArgs);	
			}


			fflush(stdout);

			//execut the command using the path variable
			execvp(fullInput[0], fullInput);
			//code shouldnt reach this point
			//print the error messsage and exit
			perror("");
			fflush(stdout);
			exit(1);
			break;
		default:
			//wait for the foreground process to finish
			fflush(stdout);	
			waitpid(spawnPid, &childES, 0);
			
			//release the block
			sigprocmask(SIG_UNBLOCK, &x, NULL);

			fflush(stdout);
			break;
	}
	//this will only be reached by the parent
	//figure out how the child process ternimnated
	if (WIFEXITED(childES))
	{
		forGroundExitStat = WEXITSTATUS(childES);
		//set the bool to track reg exit vs signal
		exitBool =0;
	}
	//else it was terinmated by a signal
	else
	{
		//get the signal term. value
		forGroundSigStat= WTERMSIG(childES);
		//display the canceling signal
		printf("terminated by signal %d\n", forGroundSigStat);
		fflush(stdout);
		//set the bool
		exitBool=1;

	}
}
//function to remove ridirects and file names from the array
int removeTwo(char* fullInput[], int index, int numOfArgs)
{
	//shift elements starting at 2+index back two spots
	int i=index;
       	for(i; i<numOfArgs-2; i++)
	{
		fullInput[i]=fullInput[i+2];	
	}
	//set the next value to null to allow execp to work correctly
	fullInput[index]=NULL;

	//return thr new size
	return numOfArgs-2;

}

//this function handles the three built in commands
void builtIns(char *fullInput[], int numOfArgs)
{
	//if the command was cd
	if(strcmp(fullInput[0], "cd")==0)
	{
		//if it was just cd go to the home dir
		if(numOfArgs==1)
		{
			chdir(getenv("HOME"));
		}
		//else go to the dir passed
		else if(numOfArgs==2)
		{
			chdir(fullInput[1]);
		}
	}
	//else exit the program	
	else if(strcmp(fullInput[0], "exit")==0)
	{

		//block the parent from sigquit	
		signal(SIGQUIT, SIG_IGN);

		//call sigquit on all children
		kill(0, SIGQUIT);
		//then exit
		exit(0);
	}
	//could just use else but ya never know
	else if(strcmp(fullInput[0], "status")==0)
	{
		//if the last foreground process terminated with a exit status
		if(exitBool==0)
		{
			//print the status
			printf("exit value %d\n", forGroundExitStat);
			fflush(stdout);	
		}
		//else it was terminated by a signal
		else
		{
			//print the signal
			printf("terminated by signal %d\n", forGroundSigStat);
			fflush(stdout);
		}
	}

}


