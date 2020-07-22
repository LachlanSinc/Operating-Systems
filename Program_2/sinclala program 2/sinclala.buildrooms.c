/*Author: Lachlan Sinclair
 *Date: 7/10/2019
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>


//simple function to check if an array contains a number
int contains(int arr[], int size, int var) {

	int i =0;
	for (i; i < size; i++)
	{
		//if the array contains var return true
		if (arr[i] == var) {
			return 1;
		}
	}
	//does not contain var return fales
	return 0;

}
//function used to make sure all of the selected rooms
//are fully connected
int fullyConnected(int arr[])
{
	int i=0;
	for (i; i < 7; i++)
	{
		//return false if a room has less than three connections
		if (arr[i] < 3)
		{
			return 0;
		}
	}
	//all rooms have atleast 3 connections therefor the map is fully connected
	return 1;
}


int main() {

	//srand for proper use of random numbers
	srand(time(0));

	//define the room names
	char roomNames[][20] = {"hills", "waterfall", "temple", "cave", "stairs",
	       	"jungle", "lake", "narrow", "wide", "steep"}; 

	//array used to hold the random indexes generated
	int randomNums[7];

	//set the array to numbers that are out of the random generators range
	//this needs to be done since we need to have unique random numbers
	int i=0;
	for (i; i < 7; i++)
	{
		randomNums[i] = 20;
	}

	
	//pick the 7 random file names
	i=0;
	for (i; i < 7; i++)
	{
		int randomNum = rand() % 10;
		//while the array already has this index, generate a new random index
		while (contains(randomNums, 7, randomNum))
		{
			randomNum = rand() % 10;
		}
		//insert the unique random index to the array of random indexes
		randomNums[i] = randomNum;
	}

	//generate the connections
	//matrix representing all connections, intialize everything to 0
	int connected[7][7] = { 0 };
	int numOfCon[7] = { 0 };

	//while the selected rooms are not fully connected
	while (!fullyConnected(numOfCon))
	{
		//select two rooms at random
		int roomA = rand() % 7;
		int roomB = rand() % 7;
		//make sure they dont equal each other
		while (roomA == roomB)
		{
			roomA = rand() % 7;
			roomB = rand() % 7;
		}

		//check to see if the rooms can be connected
		//there total connections must be less than 6 and they cant already be connected
		//i choose to track connections row and column wise to keep it simple
		if (numOfCon[roomA] < 6 && numOfCon[roomB] < 6 && connected[roomA][roomB] == 0 && connected[roomB][roomA] == 0)
		{
			//increment both rooms connections number
			numOfCon[roomA]= numOfCon[roomA]+1;
			numOfCon[roomB] = numOfCon[roomB] + 1;
			//document the connection row and column wise in the matrix
			connected[roomA][roomB] = 1;
			connected[roomB][roomA] = 1;
		}
	}

	//rooms now have names and all there connections listed in the matrix
	//we will now set up the files and directory
	
	//get the process id
	int PID = getpid();

	//make the dir used to hold the rooms
	char dirName[40] = "sinclala.rooms.";
	sprintf(dirName, "%s%d", dirName, PID);
	mkdir(dirName, 0755);
	
	char fileName[50];

	//randomly assign the start and end rooms
	int startInd = rand() % 7;
	int endInd = rand() % 7;
	while(startInd==endInd)
	{
		startInd = rand() % 7;
		endInd = rand() % 7;	
	}


	//loopthrough the file names creating the files
	i=0;
	for (i; i < 7; i++)
	{
		//clear the filename string	
		memset(fileName, 0, sizeof(fileName));
		//sprinf filename to represent the directory path and txt file name
		sprintf(fileName, "%s/%s.txt", dirName, roomNames[randomNums[i]]);

		//create the sring used to store the room name
		char nameString[50] = "ROOM NAME: ";
		
		//sprintf the name of the room to the previously made string
		sprintf(nameString, "%s%s\n", nameString, roomNames[randomNums[i]]);

		//create the txt file
		FILE *fp = fopen(fileName, "w");

		//output the name of the file
		fputs(nameString, fp);

		//add the connections to the txt file
		int connectionNum = 0;
		int j=0;
		for (j; j < 7; j++)
		{
				//use the connections matrix to write out all connections
				if (connected[i][j] == 1)
				{
					//increment the number used to dsplay the connection number
					connectionNum++;

					//add the connection string
					char connectionString[50] = "CONNECTION ";
					//sprint f the connection string, connection number and connected file name
					sprintf(connectionString, "%s%d: %s\n", connectionString, connectionNum, roomNames[randomNums[j]]);
					//write the string to the file
					fputs(connectionString, fp);
				}
		}

		//add the room type to the end of the file
		char roomString[50] = "ROOM TYPE: ";
		//if the index matches the start room index assign it as starting room
		if (i == startInd)
		{
			sprintf(roomString, "%sSTART_ROOM", roomString);
		}
		//if it matches the end index assign it as the end room
		else if (i ==endInd)
		{
			sprintf(roomString, "%sEND_ROOM", roomString);
		}
		//all other rooms are middle rooms
		else 
		{
			sprintf(roomString, "%sMID_ROOM", roomString);
		}
		//write the roomstring to the txt file
		fputs(roomString, fp);

		//close the file
		fclose(fp);
	}
	//all files have be created end the program
	//no data was malloc'd so no need to free any memory
	return 0;
}





