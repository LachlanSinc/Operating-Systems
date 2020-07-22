/*
 *Author: Lachlan Sinclair
 *Date: 7/10/2019
 * */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

struct room
{
	int id;
	int type;
	char name[256];
	int connections; //tracks the number of connections
	char connected[6][256];
};

//prototypes
void waitState();
void timeOut();

//global variables used by the threading
int state =1;
pthread_mutex_t myMutex = PTHREAD_MUTEX_INITIALIZER;

//this function gets the data from the files created by the room program
void getData(struct room arr[])
{
	//to find the most recent files used alot code from the lecture, since it does exactly what i need
	int newestDirTime = -1; // Modified timestamp of newest subdir examined
	char targetDirPrefix[32] = "sinclala.rooms."; // Prefix we're looking for
	char newestDirName[256]; // Holds the name of the newest dir that contains prefix
	memset(newestDirName, '\0', sizeof(newestDirName));

	DIR* dirToCheck; // Holds the directory we're starting in
	struct dirent *fileInDir; // Holds the current subdir of the starting dir
	struct stat dirAttributes; // Holds information we've gained about subdir

	dirToCheck = opendir("."); // Open up the directory this program was run in

	if (dirToCheck > 0) // Make sure the current directory could be opened
	{
		while ((fileInDir = readdir(dirToCheck)) != NULL) // Check each entry in dir
		{
			if (strstr(fileInDir->d_name, targetDirPrefix) != NULL && fileInDir->d_name[15] >=48 
					&& fileInDir->d_name[15] <=57) // If entry has prefix followed by a number
			{
				//printf("Found the prefex: %s\n", fileInDir->d_name);
				stat(fileInDir->d_name, &dirAttributes); // Get attributes of the entry

				if ((int)dirAttributes.st_mtime > newestDirTime) // If this time is bigger
				{
					newestDirTime = (int)dirAttributes.st_mtime;
					memset(newestDirName, '\0', sizeof(newestDirName));
					strcpy(newestDirName, fileInDir->d_name);
				}
			}
		}
	}

	closedir(dirToCheck); // Close the directory we opened

	//process the newest directory with the correct name
	DIR* corDir = opendir(newestDirName);
	char fileName[256];
	struct dirent *fileN;
	int count =0;

	//loop through all of the files in the latest dir
	//each loop obtains data from one file and puts it into one of the 
	//structs in the array
	while((fileN = readdir(corDir)) != NULL)
	{
		struct stat stbuf;

		//intialize the struct from the passed in arrays connections to 0
		arr[count].connections=0;

		//sprintf to make the correct file path
		sprintf(fileName, "%s/%s", newestDirName, fileN->d_name);

		//test for the . and .. folder that mess up the struct array
		char garbageFiles[256];
		sprintf(garbageFiles,"%s/.", newestDirName); 
		
		//if the current file is . or .. continue to the next run of the loop
		if(strstr(fileName, garbageFiles)!= NULL)
		{
			continue;
		}

		//open the file
		FILE *fr = fopen(fileName, "r");

		//strings used by sscanf
		char line[256];
		char roomType[256];
		char rName[256];
		char connectionName[256];
		char throwAway[256];

		//make sure line is instialized correctly
		memset(line, '\0', sizeof(line));

		//if the file is open
		if(fr)
		{
			//while fgets can get more lines from the file
			while(fgets(line, 256, fr))
			{
				//check to see if it is the connection line, start with this since 
				//it is the most common
				if(strstr(line, "CONNECTION") != NULL)
				{
					//use sscanf to get the useful information into connectionName
					sscanf(line, "%s %s %s", throwAway,throwAway, connectionName);
					//copy the string into the given struct in the array, in its
					//connections array of strings, and increment the connections count
					strcpy(arr[count].connected[arr[count].connections], connectionName);
					arr[count].connections++;
				}
				//else if it is the room name line
				else if(strstr(line, "ROOM NAME") != NULL)
				{
					//use sscanf to get the name into the rName string
					sscanf(line, "%s %s %s", throwAway,throwAway, rName);
					//assign the string to the name of the struct
					strcpy(arr[count].name, rName);
				}

				//look for the room type
				else if(strstr(line, "ROOM TYPE") != NULL)
				{
					//sscanf to get the room type in the roomType string
					sscanf(line, "%s %s %s", throwAway,throwAway, roomType);
					
					//check for the three possible cases and assign it to a int
					//that basically acts as a enum
					//start compare with mid since it is the most likely
					if(strcmp("MID_ROOM", roomType)==0)
					{
						arr[count].type=2;
					}
					else if(strcmp("END_ROOM", roomType)==0)
					{
						arr[count].type=3;
					}
					else if(strcmp("START_ROOM", roomType)==0)
					{
						arr[count].type=1;
					}		
				}		
			}
			//increment the count used by the array of structs
			count++;
		}
	}
}

//find the starting room in the array of structs, returns its index
int findStart(struct room arr[])
{
	int i=0;
	for(i;i<7;i++)
	{
		//return the index of the starting room when it is found
		if(arr[i].type==1)
		{
			return i;
		}
	}
	//error with the data if the loop exits
	return -1;

}

//display the rooms information to the console
void displayRoom(struct room arr[], int roomNum)
{
	//write the rooms location to the console
	printf("CURRENT LOCATION: %s\n", arr[roomNum].name);
	
	//display all of the possible connections
	char connections[256]="POSSIBLE CONNECTIONS: ";
	int i=0;
	for(i;i<arr[roomNum].connections-1;i++)
	{
		//concatinate all but the last connections to the string with commas
		strcat(connections, arr[roomNum].connected[i]);
		strcat(connections, ", ");
	}
	//concatinate the last connection and a period to the string the display it the console
	strcat(connections, arr[roomNum].connected[i]);
	strcat(connections, ".\n");
	printf(connections);
}

//this function handles moving rooms, this is also involves threading
int moveRooms(struct room arr[], int roomNum)
{
	//variables used by the getline function
	char *b = malloc(256);
	size_t size = 256;
	size_t characters;
	
	//infinity loop untile valid input is recieved
	while(1)
	{
		//prompt the user to enter the next room or time
		printf("WHERE TO? >");

		//get the users input
		characters=getline(&b, &size, stdin);

		//get rid of the new line to allow strcmp to work	
		b[characters-1]='\0';

		//if they type time let the threading begin
		if(strcmp(b, "time")==0)
		{
			//call wait state
			waitState();

			//output the time
			timeOut();
			
			//reprompt the user for input by continuing back to the start of loop
			continue;
		}

		//loop through all possible connections
		int i=0;
		for(i;i<arr[roomNum].connections;i++)
		{
			//see if the user input matches a connected room
			if(strcmp(b, arr[roomNum].connected[i])==0)
			{
				//formating
				printf("\n");

				//find the index of the connected room in the array of strcuts
				int j=0;
				for(j;j<7;j++)
				{
					//if found free the malloced memory and return the index
					if(strcmp(b, arr[j].name)==0)
					{
						free(b);
						return j;
					}
				}
			}		
		}
		//display the error message
		printf("\nHUH? I DONT UNDERSTAND THAT ROOM. TRY AGAIN.\n\n");
		
		//redisplay the rooms information 
		displayRoom(arr, roomNum);
	}
	//free the memory, note this should never be reached
	free(b);
}
//the game has finised, display the results
void results(struct room arr[], int path[], int steps)
{
	//nothing fancy here
	printf("YOU HAVE FOUND THE END ROOM. CONGRATULATIONS!\n");
	printf("YOU TOOK %d STEPS. YOUR PATH TO VICTORY WAS:\n", steps);
	
	//loop through the array that tracked rooms visted, display their names in order
	int i=0;
	for(i;i<steps;i++)
	{
		printf("%s\n", arr[path[i]].name);
	}

	//set the state to three so neither thread will relock it then release the mutix
	state=3;
	pthread_mutex_unlock(&myMutex);
}


void* game()
{

	//lock the mutex at the start, the time thread will be blocked by the state at this point
	pthread_mutex_lock(&myMutex);
	
	//create the sarray of rooms
	struct room arr[7];

	int currentRoom;

	//array to keep tack of the path take, limited to 256 rooms
	int *path;
	path = malloc(sizeof(int)*256);
	int steps=0;

	//get the data from the files and create save all of the information into an array of structs
	getData(arr);
	
	//formatting
	printf("\n");

	//find the starting room, should be arr[0] better safe than sorry i guess
	int start = findStart(arr);

	//display the first room
	displayRoom(arr, start);

	//use moveRooms to assign the new index
	currentRoom = moveRooms(arr, start);

 	//increment the steps taken and save the struct the user moved to
	path[steps]=currentRoom;
	steps++;
	

	//loop until the game is won
	while(arr[currentRoom].type!=3)
	{
		//display the room
		displayRoom(arr, currentRoom);

		//use move move rooms to assign the new index
		currentRoom = moveRooms(arr, currentRoom);
		
		//record the path taken	
		path[steps]=currentRoom;
		steps++;
	}	
	
	//display the resutls since the game has been won
	results(arr, path, steps);
	
	//free the path array
	free(path);
}

//this function places the main thread into a waitstate until the 
//time thread locks the mutex, runs, flips the state and unlocks the mutex
void waitState()
{
	//flip the state so the main thread doesnt relock the mutex
	state=2;

	//unlock the mutex
	pthread_mutex_unlock(&myMutex);

	//loop to wait for time to grab the mutix and release it
	while(1)
	{
		//lock the mutex
		pthread_mutex_lock(&myMutex);
		//if state is good, return to the calling function	
		if(state==1)
		{
			return;
		}
		//time thread hasnt run, unlock the mutex
		else if(state==2)
		{
			pthread_mutex_unlock(&myMutex);
		}
	}
}

//display the content of the currentTime txt file
void timeOut()
{
	char timesUp[256];
	
	//open the txt file
	FILE *timeFile;
	timeFile = fopen("currentTime.txt", "r");

	//copy the contents of the file into the string
	fgets(timesUp, sizeof(timesUp), timeFile);

	//write out the string
	printf("\n%s\n\n",timesUp);	
}

//main function used by the time thread
void* time_routine()
{

	
	//endlessly loop
	while(1)
	{
		//lock the mutix
		pthread_mutex_lock(&myMutex);
		//time was not requested release the mutex
		if(state==1)
		{
			pthread_mutex_unlock(&myMutex);
		}
		
		//time has been requested hold onto the mutex
		else if(state == 2)
		{
			//execute the code required for the time function.	
			
			//create the time file
			FILE* timeFile;
			timeFile = fopen("currentTime.txt", "w+");
			

			char timeS[256];

			//create a time_t variable
			time_t stamp=time(0);

			//convert time_t to tm as local time
			struct tm *ti = localtime(&stamp);

			//create a string out of the tm struct
			strftime (timeS, sizeof(timeS), "%_I:%M%#p, %A, %B %d, %Y", ti);

			//output the string to the txt file
			fputs(timeS, timeFile);

			//close the file
			fclose(timeFile);

			//swap states and unlock the mutex to allow the game to retake control
			state=1;
			pthread_mutex_unlock(&myMutex);

		}
		//game has finised
		else if(state==3)
		{
			//release mutex and break out of infinite loop
			pthread_mutex_unlock(&myMutex);
			break;
		}	
	}
}



int main()
{
	//threading variables
	int resultInt, resultInt2;
	pthread_t mainThreadID, timeThreadID;

	//create the main thread
	resultInt = pthread_create( &mainThreadID,
			NULL,
			game,
			NULL );
	
	//create the time thread
	resultInt2 = pthread_create( &timeThreadID,
			NULL,
			time_routine,
			NULL );

	//join them to prevent a core dump
	pthread_join(mainThreadID, NULL);
	pthread_join(timeThreadID, NULL);

	//destory the mutex
	pthread_mutex_destroy(&myMutex);
	return 0;
}








