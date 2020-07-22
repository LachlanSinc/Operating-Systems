#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>


//Note: I should have created a header file and had all of the repeated functions included there.
//I had issues with getting 5 processes working correctly (dang zombies)
//and completely ran out of time. Got everything in working order now its time to cram from my other finals, 
//sorry to leave this as somewhat of a mess. 
//Thanks for all the help in slack. 

// decryption server

void error(const char *msg) { perror(msg); exit(1); } // Error function used for reporting issues

//decrypt the plain text, length is the length of the plain text.
void decrypt(char *message, int length)
{
  int  plain, key, combined;
  int i = 0;
  for (i; i < length - 1; i++)
    {
      //convert the plain text and key chars to their ascii values
      //space is a special case
      if (message[i + length - 1] == ' ')
        {
          key = 91;
        }
      else
        {
          key = (int) message[i + length - 1];
        }
    
      if (message[i] == ' ')
        {
          plain = 91;
        }
      else
        {
          plain = (int) message[i];
        }
      //subtract the key ascii value from the plain text
      //add 27 if less than 0 
      combined = plain - key;
      if (combined < 0) {
        combined = combined + 27;
      }
    
      //take take the modules
      combined = combined % 27;

      //check for the special case of a 0
      if (combined == 26)
        {
          combined = 32;
        }
      else
        {
          combined = combined + 65;
        }
    
      //set the orginal char value
      message[i] = combined;
    }
}

int main(int argc, char *argv[])
{
  int listenSocketFD, establishedConnectionFD, portNumber, charsRead;
  socklen_t sizeOfClientInfo;
  char buffer[280];
  struct sockaddr_in serverAddress, clientAddress;

  const int maxConnections = 5;
  int activeConnections = 0;
  int childStatus;
  int activePids[maxConnections];

  int i;

  if (argc < 2)
    {
      fprintf(stderr, "USAGE: %s port\n", argv[0]); exit(1);
    } // Check usage & args

  // No children, zero out since 0 is not a valid pid
  for (i = 0; i < maxConnections; i++)
    {
      activePids[i] = 0;
    }
  
  // Set up the address struct for this process (the server)
  memset((char *) &serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
  portNumber = atoi(argv[1]); // Get the port number, convert to an integer from a string
  serverAddress.sin_family = AF_INET; // Create a network-capable socket
  serverAddress.sin_port = htons(portNumber); // Store the port number
  serverAddress.sin_addr.s_addr = INADDR_ANY; // Any address is allowed for connection to this process

  // Set up the socket
  listenSocketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
  if (listenSocketFD < 0) error("ERROR opening socket");

  // Enable the socket to begin listening
  if (bind(listenSocketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to port
    error("ERROR on binding");
  listen(listenSocketFD, 5); // Flip the socket on - it can now receive up to 5 connections

  //inifinte loop
  while (1)
    {
      // Accept a connection, blocking if one is not available until one connects
      sizeOfClientInfo = sizeof(clientAddress); // Get the size of the address for the client that will connect
      establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *)&clientAddress, &sizeOfClientInfo); // Accept
      if (establishedConnectionFD < 0) error("ERROR on accept");

      // Check to see if any children have terminated and
      // release resources and free up count
      if (activeConnections > 0) {
        for(i = 0; i < maxConnections; i++) {
          if (activePids[i] != 0) {
            int childPid = waitpid(activePids[i], NULL, WNOHANG);
            if (childPid == activePids[i]) {
              activeConnections -= 1;
              activePids[i] = 0;
            }
          }
        }
      }

      // if maximum connections have been reached wait for one
      // to terminate
      while (activeConnections == maxConnections) {
        int childPid = waitpid(0, NULL, 0);
        for (i = 0; i < maxConnections; i++) {
          if (activePids[i] == childPid) {
            activeConnections -= 1;
            activePids[i] = 0;
            break;
          }
        }
      }
    
      // There is an available connection
      pid_t pid = fork();
      if (pid == -1)
        {
          // error creating the child, terminate
          error("error creating child process.\n");
        }
      // Execute decryption in child process
      if (pid == 0)
        {
          //size of the key and text and the strings that will hold them
          int keySize, plainSize, totalSize;
          char *keyString, *plainString, *fullString;
          //buffer used to validate enc accessed the port
          char bufferValidate[256];
          char intBuffer[256]; //used to recieve the char count for the 2 strings
          charsRead = 0;

          //strings used to pass back information
          char confirmed[]    = "confirmed";
          char keySuccess[]   = "key sucess";
          char plainSuccess[] = "plain sucess";

          //read the first message
          memset(bufferValidate, '\0', 256);
          charsRead = recv(establishedConnectionFD, bufferValidate, 255, 0);
          if (charsRead < 0) error("ERROR reading from socket");
          else if(charsRead==0)
            {
              exit(1);
            }

          //make sure enc was the contacting daemon
          if (strcmp(bufferValidate, "dec") != 0)
            {
              fprintf(stderr, "calling program did not validate properly.\n");
              exit(1);
            }
          else
            {
              charsRead = send(establishedConnectionFD, confirmed, 10, 0);
            }

          //get the key length
          memset(intBuffer, '\0', 256);
          charsRead = recv(establishedConnectionFD, intBuffer, 255, 0);
          if (charsRead < 0) error("ERROR reading from socket");

          //convert the key length string to a int
          keySize = atoi(intBuffer);
          //sendd the sucess message
          charsRead = send(establishedConnectionFD, keySuccess, 255, 0);

          //clear the buffer for plain text length
          memset(intBuffer, '\0', 256);
          //get the plain text length
          charsRead = recv(establishedConnectionFD, intBuffer, 255, 0);
          if (charsRead < 0) error("ERROR reading from socket");

          //convert the size to a int and send the confirmation message3
          plainSize = atoi(intBuffer);
          charsRead = send(establishedConnectionFD, plainSuccess, 255, 0);

          //make sure the key is long enoguh
          if (keySize < plainSize)
            {
              error("The key is to small.\n");
              exit(1);
            }

          //malloc a string to the size of the incoming large string
          totalSize = keySize + plainSize - 1; //get the length of the combined string -1 for the dropped \0
          fullString = malloc(totalSize * sizeof(char));


          memset(fullString, '\0', totalSize);
          //loop until the entire string has been read in
          charsRead = 0;
          while (charsRead < totalSize - 1)
            {
              charsRead = charsRead + recv(establishedConnectionFD, fullString+charsRead, totalSize, 0);
              if (charsRead < 0) error("ERROR reading from socket");

            }

          // encrypt the plain text
          decrypt(fullString, plainSize);

          //send the encrypted plain text back
          charsRead = send(establishedConnectionFD, fullString, plainSize-1, 0);
          return 0;
        }
      //parent
      else
        {
          // Parent does not need the connection
          close(establishedConnectionFD);

          // Find an empty slot to store the PID
          for (i = 0; i < maxConnections; i++) {
            if (activePids[i] == 0) {
              activePids[i] = pid;
              activeConnections += 1;
              break;
            }
          }
        }
    }
  close(listenSocketFD); // Close the listening socket
  return 0;
}
